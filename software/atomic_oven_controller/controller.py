import argparse
from artiq.protocols.pc_rpc import simple_server_loop
from artiq.tools import *

from atomic_oven_controller import oven_pic_interface


class OvenController:
    def __init__(self, names, temperatures):
        """Intialise the oven rpc controller.
        names and temperatures are two-element lists containing the names 
        and temperature setpoints for channels 0 and 1"""

        # Initialise the HAL
        self.pic = oven_pic_interface.OvenPICInterface()

        # Names of the channels
        self.names = [name.lower() for name in names]

        # Temperature setpoints to use when the oven channels are turned on
        self.temperature_setpoints = temperatures

    def _channel_sanitiser(self, channel):
        if channel.lower() == self.names[0]:
            channel_id = 0
        elif channel.lower() == self.names[1]:
            channel_id = 1
        else:
            raise ValueError(
                "Bad oven channel '{}'...".format(channel) + 
                " options are: '{}' and '{}".format(
                    self.names[0],self.names[1]))
        return channel_id

    def turn_on(self, channel):
        """Turn the oven channel on to the set temperature"""
        channel_id = self._channel_sanitiser(channel)

        # Zero the current setpoint
        self.pic.fb_set_setpoint(
            "current_{}".format(channel_id),
            0)

        # Set the temperature setpoint
        self.pic.fb_set_setpoint(
            "temperature_{}".format(channel_id),
            self.temperature_setpoints[channel_id])

        # Start current feedback
        self.pic.fb_start("current_{}".format(channel_id))

        # Start temperature feedback
        self.pic.fb_start("temperature_{}".format(channel_id))

    def turn_off(self, channel):
        """Turn off the oven"""
        channel_id = self._channel_sanitiser(channel)

        # Set the temperature setpoint to 20 C
        self.pic.fb_set_setpoint(
            "temperature_{}".format(channel_id),
            20, immediate=True)

        # Stop temperature feedback
        self.pic.fb_stop("temperature_{}".format(channel_id))

        # Set current setpoint to zero
        self.pic.fb_set_setpoint(
            "current_{}".format(channel_id),
            0, immediate=True)

        # Stop current feedback
        self.pic.fb_stop("current_{}".format(channel_id))

        # Turn pwm off
        self.pic.set_pwm_duty(channel_id, 0)

    def read_status(self, channel):
        """Read the current and temperature of a given channel
        in C and A"""

        channel_id = self._channel_sanitiser(channel)

        values = self.pic.adc_read_calibrated_sample()
        temperature = values[channel_id]["temperature"]
        current = values[channel_id]["current"]

        return temperature, current

    def close(self):
        self.turn_off("ca")
        self.turn_off("sr")




def get_argparser():
    parser = argparse.ArgumentParser()
    simple_network_args(parser, 4000)
    verbosity_args(parser)
    for ch in ["0","1"]:
        parser.add_argument("--ch"+ch,
                            default="ch"+ch+",0",
                            help="<name>,<temperature> for channel "+ch)
    return parser

def main():
    args = get_argparser().parse_args()
    init_logger(args)

    def parse_channel(ch, s):
        parts = s.split(s)
        assert len(parts)==2
        name = parts[0]
        temp = float(parts[1])
        print("Channel {} : name = {}, temperature = {}".format(ch, name, temp))
        return name, temp

    ch0_config = parse_channel("0", args.ch0)
    ch1_config = parse_channel("1", args.ch1)

    chs = [ch0_config, ch1_config]

    dev = OvenController([ch[0] for ch in chs], [ch[1] for ch in chs])
    try:
        simple_server_loop({"atomic_oven_controller": dev},
            bind_address_from_args(args), args.port)
    finally:
        dev.close()

if __name__ == "__main__":
    main()