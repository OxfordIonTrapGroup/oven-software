
import numpy as np
import serial
import time
import struct
import threading
import Adafruit_BBIO.GPIO as GPIO

from . import constants as c

GPIO.setup("P9_23", GPIO.OUT)

def reset_pic():
    GPIO.output("P9_23", GPIO.LOW)
    time.sleep(0.01)
    GPIO.output("P9_23", GPIO.HIGH)
    time.sleep(0.01)





class PICError(Exception):
    pass


class Interface:
    # Baud rate for the command and data ports
    _command_baudrate = 900000
    _data_baudrate = 900000

    # How long to wait between polling the data port
    _streaming_poll_time = 0.1

    _DEBUG = False

    # Should we automatically read calibration data?
    _READ_CALIBRATION = True

    def __init__(self, command_port="/dev/ttyO1", data_port="/dev/ttyO4",
        timeout=1):

        self._quit = False

        # Open the command port
        self.sc = serial.Serial(
            port=command_port,
            baudrate=self._command_baudrate,
            timeout=timeout
            )

        # Open the data port
        self.sd = serial.Serial(
            port=data_port,
            baudrate=self._data_baudrate,
            timeout=timeout
            )

        # Reset the PIC
        self.reset()

        self._streaming_thread.start()

    def _read_calibrations(self):
        self._calibrations[0] = self.calibration_read_channel(0)
        self._calibrations[1] = self.calibration_read_channel(1)

    def _send_command(self, line):
        """Send a command line and read the response"""

        if line[-1] != "\n":
            line += "\n"

        # If there is any data waiting, complain
        if self.sc.in_waiting > 0:
            raise PICError("Response sent by PIC without prompting: " + \
                str(self.sc.read(self.sc.in_waiting)))

        # Send the command
        self.sc.write(line.encode())

        if self._DEBUG:
            print("SEND: " + str(line.encode()))

        # Read a response
        response = self.sc.readline()

        if len(response) == 0:
            raise PICError("PIC did not respond within timeout")

        if response[0] != b">"[0]:
            raise PICError("PIC responded with error: " + \
                str(response))

        # Trim the ">" character and the newline
        response = response[1:]

        if self._DEBUG:
            print("RECV: " + str(response))

        return response

    def _streaming_thread_exec(self):
        """This is executed in a separate thread to read out the data port"""

        while not self._quit:

            if self._streaming_mode is not None:
                if self.sd.in_waiting > 0:
                    # Read a buffer in and append it to the list
                    self._data_buffers.append(
                        self.sd.read(self.sd.in_waiting)
                        )

            time.sleep(self._streaming_poll_time)

    def _process_channel_data(self, channel_array):
        values = np.bitwise_and(channel_array, 0x7FFFFF).astype('int32')
        # Generate logical array of signs (+ = 0, - = 1)
        signs  = np.bitwise_and(channel_array, 1 << 23) != 0 
        values = np.where(signs, values - 0x800000, values)
        float_values = values*2.5/(0x800000*1.0)

        return float_values

    def close(self):
        self._quit = True

    def echo(self, data):
        line = c.CMD_ECHO + " {}".format(data)

        response = self._send_command(line)
        print("ECHO: " + str(response))

    def reset(self):
        # Reset the PIC
        reset_pic()

        # Clear the input buffers
        self.sd.reset_input_buffer()

        self.sc.readline()

        # When streaming, this is non-zero
        self._streaming_mode = None
        self._data_buffers = []

        self._streaming_thread = threading.Thread(
            target=self._streaming_thread_exec,
            daemon=True,
            )

        version = self.get_version()
        print("Connected to PIC, firmware version: " + version)

        self._calibrations = [None, None]
        if self._READ_CALIBRATION:
            self._read_calibrations()

    def get_version(self):
        response = self._send_command(c.CMD_VERSION)

        version_str = response.decode().strip().split(" ")[1].strip()
        return version_str

    def set_pwm_duty(self, channel, duty):
        """Set the pwm duty cycle on the given channel.
        channel = 0 or 1
        duty = 0.0 -> 1.0
        """

        if channel not in [0,1]:
            raise PICError("Bad channel: {}".format(channel))

        if duty > 1 or duty < 0:
            raise PICError("Bad duty cycle: {}".format(duty))

        line = c.CMD_SET_PWM_DUTY + " {:d} {:f}".format(channel, duty)
        response = self._send_command(line)

    def adc_read_sample(self):
        """Read the last set of adc samples"""

        response = self._send_command(c.CMD_ADC_READ_LAST_CONVERSION)

        values_str = response.decode().strip().split(" ")
        values = [float(s.strip()) for s in values_str]
        return values

    def adc_read_calibrated_sample(self):
        """Read the last set of adc samples after calibration"""

        response = self._send_command(c.CMD_ADC_READ_LAST_CALIBRATED_DATA)

        channels = response.decode().strip().split(";")[:2]
        values = []
        for channel in channels:
            index, temperature, current, output_voltage, oven_voltage = \
                channel.strip().split()
            channel_values = {
                "temperature": float(temperature.strip()),
                "current": float(current.strip()),
                "output_voltage": float(output_voltage.strip()),
                "oven_voltage": float(oven_voltage.strip()),
                }
            values.append(channel_values)

        return values

    def adc_decimate(self, decimation):

        line = c.CMD_ADC_DECIMATE + " {:d}".format(decimation)
        self._send_command(line)

    def adc_start_streaming(self, channels):
        if self._streaming_mode != None:
            raise PICError(
                'start_streaming called when streaming already active')

        channel_byte = 0
        safe_channels = []
        for i, channel in enumerate(channels):
            if not 0 <= channel <= 7:
                raise Exception(
                    'Requested channel out of range: {0}'.format(channel))
            if channel not in safe_channels:
                safe_channels.append(channel)
                channel_byte |= 1 << channel

        # Clear the rx buffer
        if self.sd.in_waiting > 0:
            self.sd.reset_input_buffer()
        self._data_buffers = []

        self._streaming_start_time = time.time()
        self._streaming_channels = safe_channels
        self._streaming_channel_byte = channel_byte
        self._streaming_buffers = []
        self._streaming_mode = "adc"
        # Start streaming
        line = c.CMD_ADC_STREAM + " {}".format(channel_byte)
        self._send_command(line)

    def adc_stop_streaming(self, read_delay=0.1, return_calibrated_data=True):

        if self._streaming_mode != "adc":
            raise PICError(
                'adc_stop_streaming called when adc_streaming not active')

        # Send the stop command (stream 0 channels)
        line = c.CMD_ADC_STREAM + " {}".format(0)
        self._send_command(line)

        self._streaming_mode = None
        self._streaming_stop_time = time.time()

        # Sleep to allow the data to finish being sent
        time.sleep(read_delay)

        # Poll the stream one final time
        if self.sd.in_waiting > 0:
            self._data_buffers.append(self.sd.read(self.sd.in_waiting))

        # Now cat the data buffers together
        if len(self._data_buffers) > 1:
            array_data = np.concatenate([
                np.frombuffer(b, dtype='uint8') 
                for b in self._data_buffers])
        elif len(self._data_buffers) == 1:
            array_data = np.frombuffer(self._data_buffers[0], dtype='uint8')
        else:
            raise Exception('No data received in adc stream')

        # If we stopped receiving half-way through a packet, clip it off
        unit_length = 1 + 4*len(self._streaming_channels)
        array_data = array_data[:unit_length*int(len(array_data)/unit_length)]

        # Reshape the array so that the first index references the 
        # sample number
        array_data = np.reshape(
            array_data,
            (-1, 1 + 4*len(self._streaming_channels)),
            )
        channel_bytes = array_data[:,0]

        # Check that the channel_bytes are all the same
        if np.sum(channel_bytes != self._streaming_channel_byte) != 0:
            # If there are any samples which have the wrong byte
            print(channel_bytes)
            raise Exception(
                'Bad channel_bytes in adc_streamed data, sync error')

        channel_arrays = []
        channel_values = []
        for i in range(len(self._streaming_channels)):
            channel_array = array_data[:,1+4*i:1+4*(i+1)]
            channel_array = channel_array.flatten()
            channel_array = np.frombuffer(channel_array, dtype='uint32')

            channel_values.append(self._process_channel_data(channel_array))

        # Now sort the data into a list indexed by the channel number
        all_channel_values = []
        channel_values_index = 0

        for i in range(8):
            if i in self._streaming_channels:
                all_channel_values.append(channel_values[channel_values_index])
                channel_values_index += 1
            else:
                all_channel_values.append([])

        if not return_calibrated_data:
            return all_channel_values

        results = [{},{}]
        for channel in [0,1]:
            for name in c.ADC_CHANNELS:
                data = all_channel_values[c.ADC_CHANNELS[name][channel]]
                if len(data) == 0:
                    continue

                data *= self._calibrations[channel][c.CALIBRATION_KEYS[name][0]]/2.5
                data += self._calibrations[channel][c.CALIBRATION_KEYS[name][1]]

                results[channel][name] = data

            # Add current/temperature correction
            if "T" in results[channel] and \
                "I" in results[channel]:
                results[channel]["T"] = results[channel]["T"] \
                    + results[channel]["I"] * self._calibrations[channel][
                        "temperature_current_coefficient"]

        return results

    def fb_set_config(self, name, config):
        p = config['p']
        i = config['i']
        d = config['d']
        sample_decimation = int(config['sample_decimation'])

        line = c.CMD_FEEDBACK_SET_CONFIG \
            + " {:s} {:f} {:f} {:f} {:d}".format(
                name, p, i, d, sample_decimation)
        self._send_command(line)

    def fb_get_config(self, name):
        line = c.CMD_FEEDBACK_GET_CONFIG \
            + " {:s}".format(name)
        response = self._send_command(line)
        response_split = iter(response.decode().split(" "))

        if name != next(response_split):
            raise Exception("Bad response during get_config: "+str(response))

        config = {}
        config['p'] = float(next(response_split))
        config['i'] = float(next(response_split))
        config['d'] = float(next(response_split))
        config['sample_decimation'] = float(next(response_split))
        return config


    def fb_start(self, name):
        self._send_command(c.CMD_FEEDBACK_START + " " + name)

    def fb_stop(self, name):
        self._send_command(c.CMD_FEEDBACK_STOP + " " + name)

    def fb_set_setpoint(self, name, new_setpoint, immediate=False):
        if immediate:
            command = c.CMD_FEEDBACK_SETPOINT_IMMEDIATE
        else:
            command = c.CMD_FEEDBACK_SETPOINT
        line = command + " {:s} {:f}".format(name, new_setpoint)
        self._send_command(line)

    def fb_read_status(self, name):
        response = self._send_command(c.CMD_FEEDBACK_READ_STATUS + " " + name)

        return response.decode()

    def fb_set_limits(self, name, limits):
        
        cv_min = limits['cv_min']
        cv_max = limits['cv_max']
        value_max = limits['value_max']
        setpoint_slewrate = limits['setpoint_slewrate']

        line = c.CMD_FEEDBACK_SET_LIMITS + " {:s}".format(name)
        line += " {:f} {:f} {:f} {:f}".format(cv_min, cv_max, value_max, setpoint_slewrate)
        self._send_command(line)

    def fb_get_limits(self, name):
        line = c.CMD_FEEDBACK_GET_LIMITS + " {:s}".format(name)
        response = self._send_command(line)
        response_split = iter(response.decode().split(" "))

        if name != next(response_split):
            raise Exception("Bad response during get_limits: "+str(response))

        limits = {}
        limits['cv_min'] = float(next(response_split))
        limits['cv_max'] = float(next(response_split))
        limits['value_max'] = float(next(response_split))
        limits['setpoint_slewrate'] = float(next(response_split))
        return limits

    def settings_reload(self):
        """Reload settings from flash"""
        self._send_command(c.CMD_SETTINGS_LOAD)

    def settings_set_to_factory(self):
        """Reset settings to factory defaults"""
        self._send_command(c.CMD_SETTINGS_SET_TO_FACTORY)

    def settings_save(self):
        """Save current settings to flash"""
        self._send_command(c.CMD_SETTINGS_SAVE)

    def settings_print(self):
        """Dump out all current settings"""
        response = self._send_command(c.CMD_SETTINGS_PRINT)
        return response

    def safety_status(self, print_output=False):
        """Get list of status flags for channel 0 and 1. Any True flags indicate
        errors that have tripped the interlock"""
        response = self._send_command(c.CMD_SAFETY_STATUS)
        if print_output:
            print(response)
        responses = response.decode().split(",")
        assert len(responses) == 3
        statuses = []
        for ch, s in zip(range(2), responses[0:2]):
            status = {}
            for err in ["over-current",
                        "over-temperature",
                        "over-time",
                        "under-temperature",
                        "adc-crc",
                        "adc-sampling"]:
                status[err.replace("-","")] = err in s
            statuses.append(status)
        return statuses

    def safety_read_channel(self, channel):
        line = c.CMD_SAFETY_READ_CHANNEL + " {:d}".format(channel)
        response = self._send_command(line)

        response_split = iter(response.decode().split(" "))

        received_channel = int(next(response_split))
        if received_channel != channel:
            raise Exception("Wrong channel response! {} vs {}".format(
                channel, received_channel))

        safety_settings = {}
        safety_settings["oven_temperature_max"] = float(next(response_split))
        safety_settings["oven_temperature_min"] = float(next(response_split))
        safety_settings["oven_temperature_check_disabled"] = float(next(response_split))
        safety_settings["oven_current_max"] = float(next(response_split))
        safety_settings["oven_current_check_disabled"] = float(next(response_split))
        safety_settings["on_time_max"] = float(next(response_split))
        safety_settings["on_time_check_disabled"] = float(next(response_split))
        safety_settings["duty_max"] = float(next(response_split))

        return safety_settings

    def safety_set_channel(self, channel, key_name, key_value):
        line = c.CMD_SAFETY_SET_CHANNEL + " {:d} ".format(channel)
        line += key_name + " {:g}".format(key_value)
        response = self._send_command(line)

    def calibration_read_channel(self, channel):
        line = c.CMD_CALIBRATION_READ_CHANNEL + " {:d}".format(channel)
        response = self._send_command(line)

        response_split = iter(response.decode().split(" "))

        received_channel = int(next(response_split))
        if received_channel != channel:
            raise Exception("Wrong channel response! {} vs {}".format(
                channel, received_channel))

        calibration = {}
        calibration["current_scale"] = float(next(response_split))
        calibration["current_offset"] = float(next(response_split))
        calibration["temperature_scale"] = float(next(response_split))
        calibration["temperature_offset"] = float(next(response_split))
        calibration["output_voltage_scale"] = float(next(response_split))
        calibration["output_voltage_offset"] = float(next(response_split))
        calibration["oven_voltage_scale"] = float(next(response_split))
        calibration["oven_voltage_offset"] = float(next(response_split))
        calibration["temperature_current_coefficient"] = float(next(response_split))

        return calibration

    def calibration_set_channel(self, channel, calibration):
        line = c.CMD_CALIBRATION_SET_CHANNEL + " {:d}".format(channel)

        line += " {:g}".format(calibration["current_scale"])
        line += " {:g}".format(calibration["current_offset"])
        line += " {:g}".format(calibration["temperature_scale"])
        line += " {:g}".format(calibration["temperature_offset"])
        line += " {:g}".format(calibration["output_voltage_scale"])
        line += " {:g}".format(calibration["output_voltage_offset"])
        line += " {:g}".format(calibration["oven_voltage_scale"])
        line += " {:g}".format(calibration["oven_voltage_offset"])
        line += " {:g}".format(calibration["temperature_current_coefficient"])

        response = self._send_command(line)

    def settings_read(self):
        """Read all user configurable settings to a dictionary"""
        settings = {}
        settings["cals"] = {}
        settings["safetys"] = {}
        settings["configs"] = {}
        settings["limits"] = {}
        for ch in range(2):
            settings["cals"][ch] = self.calibration_read_channel(ch)
            settings["safetys"][ch] = self.safety_read_channel(ch)
        for name in ["current_0", "current_1",
                     "temperature_0", "temperature_1"]:
            settings["configs"][name] = self.fb_get_config(name)
            settings["limits"][name] = self.fb_get_limits(name)
        return settings

    def settings_write(self, settings):
        """Set all user configurable settings from a dictionary. These are not
        programmed into flash hence are volatile. To save these settings to
        flash call settings_save()"""
        for ch in range(2):
            self.calibration_set_channel(ch, settings["cals"][ch])
            for k,v in settings["safetys"][ch].items():
                self.safety_set_channel(ch, k, v)
        for name in ["current_0", "current_1",
                     "temperature_0", "temperature_1"]:
            self.fb_set_config(name, settings["configs"][name])
            self.fb_set_limits(name, settings["limits"][name])
