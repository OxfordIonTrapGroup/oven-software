

CMD_ECHO = "echo"
CMD_VERSION = "version"

CMD_SET_PWM_DUTY = "set_pwm_duty"

CMD_ADC_STREAM = "adc_stream_channels"
CMD_ADC_DECIMATE = "adc_set_decimation"
CMD_ADC_READ_LAST_CONVERSION = "adc_read_sample"
CMD_ADC_READ_LAST_CALIBRATED_DATA = "adc_read_calibrated_sample"

CMD_FEEDBACK_SET_CONFIG = "fb_set_config"
CMD_FEEDBACK_GET_CONFIG = "fb_get_config"
CMD_FEEDBACK_START = "fb_start"
CMD_FEEDBACK_STOP = "fb_stop"
CMD_FEEDBACK_SETPOINT = "fb_set_setpoint"
CMD_FEEDBACK_READ_STATUS = "fb_read_status"
CMD_FEEDBACK_SET_LIMITS = "fb_set_limits"
CMD_FEEDBACK_GET_LIMITS = "fb_get_limits"

CMD_SETTINGS_LOAD = "settings_load"
CMD_SETTINGS_SET_TO_FACTORY = "settings_set_to_factory"
CMD_SETTINGS_SAVE = "settings_save"
CMD_SETTINGS_PRINT = "settings_print"

CMD_SAFETY_STATUS = "safety_status"
CMD_SAFETY_READ_CHANNEL = "safety_read_channel"
CMD_SAFETY_SET_CHANNEL = "safety_set_channel"

CMD_CALIBRATION_READ_CHANNEL = "calibration_read_channel"
CMD_CALIBRATION_SET_CHANNEL = "calibration_set_channel"


# Lookup table for adc channel meanings
ADC_CHANNELS = {
    "T": [3, 6], # Thermocouple on oven
    "I": [0, 5], # Current
    "V_out": [1, 4], # Voltage at output
    "V": [2, 7], # Voltage at oven
    }

CALIBRATION_KEYS = {
    "T": ["temperature_scale", "temperature_offset"],
    "I": ["current_scale", "current_offset"],
    "V_out": ["output_voltage_scale", "output_voltage_offset"],
    "V": ["oven_voltage_scale", "oven_voltage_offset"],
}

