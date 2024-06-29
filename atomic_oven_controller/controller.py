import argparse
from artiq.protocols.pc_rpc import simple_server_loop
from artiq.tools import *
import asyncio
import logging
import time

from atomic_oven_controller import interface, integrator

logger = logging.getLogger(__name__)


class InterlockTripped(Exception):
    """Raised if a safety interlock has tripped on the on oven controller - this
    indicates something pretty out-of-whack, or that the oven reached the
    maximum burn time"""
    pass


class OvenController:
    """Atomic oven controller Artiq interface

    High level interface to the oven controllers. This is designed to be pretty
    safe - the maximum temperature is flashed into the PIC (via the
    write_settings script), hence if one goes crazy with the setpoint here
    nothing will likely explode. The maximum oven burn time is set by a value
    flashed into the PIC (again, write_settings) - the oven
    interlock will trip if the oven is on for longer than this.
    """

    def __init__(self, names, temperatures, integrator_interface):
        """Initialise the oven rpc controller.
        names and temperatures are two-element lists containing the names
        and temperature setpoints for channels 0 and 1"""

        # Initialise the HAL
        self.pic = interface.Interface()

        # Names of the channels
        self.names = [name.lower() for name in names]

        # Temperature setpoints to use when the oven channels are turned on
        self.temperature_setpoints = temperatures

        # Callbacks to the loading time integrator.
        self.integrator_interface = integrator_interface

    def _channel_sanitiser(self, channel):
        if channel.lower() == self.names[0]:
            channel_id = 0
        elif channel.lower() == self.names[1]:
            channel_id = 1
        else:
            raise ValueError("Bad oven channel '{}'...".format(channel) +
                             " options are: '{}' and '{}".format(
                                 self.names[0], self.names[1]))
        return channel_id

    def check_interlocks(self, channel):
        """Raise an InterlockTripped error if any interlocks tripped on the given channel."""
        channel_id = self._channel_sanitiser(channel)
        self._check_interlocks(channel_id)

    def _check_interlocks(self, channel_id):
        status = self.pic.safety_status()
        any_tripped = any(v for k, v in status[channel_id].items())
        if any_tripped:
            self.integrator_interface.off_now()
            raise InterlockTripped(status)

    def turn_on(self, channel):
        """Turn the oven channel on to the set temperature"""
        channel_id = self._channel_sanitiser(channel)

        # Zero the current setpoint
        self.pic.fb_set_setpoint("current_{}".format(channel_id), 0)

        # Set the temperature setpoint
        self.pic.fb_set_setpoint("temperature_{}".format(channel_id),
                                 self.temperature_setpoints[channel_id])

        # Start current feedback
        self.pic.fb_start("current_{}".format(channel_id))

        # Start temperature feedback
        self.pic.fb_start("temperature_{}".format(channel_id))

        # Inform integrator.
        self.integrator_interface.on_now()

    def turn_off(self, channel):
        """Turn off the oven"""
        channel_id = self._channel_sanitiser(channel)

        # Check no interlocks have already tripped, so the user knows if
        # something funny happened on this oven burn
        # NB: it seems that the overtemperature interlock can be tripped
        # if the setpoint is close (~20 degrees) to the safety upper limit.
        # If this occurs, verify that it is a glitch before ignoring
        # in user code
        self._check_interlocks(channel_id)

        # Set the temperature setpoint to 20 C
        self.pic.fb_set_setpoint("temperature_{}".format(channel_id),
                                 20,
                                 immediate=True)

        # Stop temperature feedback
        self.pic.fb_stop("temperature_{}".format(channel_id))

        # Set current setpoint to zero
        self.pic.fb_set_setpoint("current_{}".format(channel_id),
                                 0,
                                 immediate=True)

        # Stop current feedback
        self.pic.fb_stop("current_{}".format(channel_id))

        # Turn pwm off
        self.pic.pwm_set_duty(channel_id, 0)

        # Inform integrator.
        self.integrator_interface.off_now()

    def read_status(self, channel):
        """Read the temperature and current of a given channel
        in C and A"""
        channel_id = self._channel_sanitiser(channel)

        values = self.pic.adc_read_calibrated_sample()
        temperature = values[channel_id]["temperature"]
        current = values[channel_id]["current"]

        return temperature, current

    def reset_interlocks(self):
        self.pic.reset()

    def close(self):
        self.turn_off("ca")
        self.turn_off("sr")

    def ping(self):
        return True


class SoftwareLimiter:
    """Controller-based loading time & attempts tracker
    """

    def __init__(self, integrator):
        self._integrator = integrator
        self.configure()

    def configure(self,
                  hourly_allowance=15 * 60,
                  max_attempts: int = 10,
                  nodelay_attempts: int = 3,
                  timeout: float = 60 * 60):
        """Configure the loading limiter

        :param hourly_allowance: maximum integrated time per hour allowed to load
        :param max_attempts: total maximum number of consecutive loading attempts
        :param nodelay_attempts: number of attempts allowed back-to-back without timeout
        :param timeout: delay between attempts after `nodelay_attempts`
        """
        self.hourly_allowance = hourly_allowance
        self.max_attempts = max_attempts
        self.nodelay_attempts = nodelay_attempts
        self.timeout = timeout

    def request(self) -> bool:
        """Request to load

        Grants the request if neither the maximum number of attempts nor the hourly
        allowance is exceeded, and if the `timeout` has passed in the case where the
        number of unsuccessful loading attempts exceeds `nodelay_attempts`.

        :returns: whether the request to load is granted
        """
        load_time_integral = self.get_integrated_load_time()
        attempts = self.get_num_attempts()

        hourly_allowance_exceeded = load_time_integral >= self.hourly_allowance
        load_attempts_max_exceeded = attempts >= self.max_attempts
        load_attempts_exceeded = attempts >= self.nodelay_attempts

        if load_attempts_max_exceeded or hourly_allowance_exceeded:
            return False
        elif load_attempts_exceeded:
            past_timeout = time.monotonic() >= (self._integrator.t_last_input +
                                                self.timeout)
            return past_timeout
        else:
            return True

    def get_num_attempts(self) -> int:
        """Return the attempt count"""
        return self._integrator.get_counter()

    def reset_num_attempts(self, count: int = 0):
        """Reset the attempt count"""
        self._integrator.reset_counter(count)

    def get_integrated_load_time(self) -> float:
        """Return the integrated load time"""
        return self._integrator.get_integral()

    def reset_integrated_load_time(self):
        """Reset the integrated load time"""
        self._integrator.reset_integral()

    def ping(self) -> bool:
        return True


def get_argparser():
    parser = argparse.ArgumentParser()
    simple_network_args(parser, 4000)
    verbosity_args(parser)
    for ch in ["0", "1"]:
        parser.add_argument("--ch" + ch,
                            default="ch" + ch + ",0",
                            help="<name>,<temperature> for channel " + ch)
    return parser


def main():
    args = get_argparser().parse_args()
    init_logger(args)

    def parse_channel(ch, s):
        parts = s.split(",")
        assert len(parts) == 2
        name = parts[0]
        temp = float(parts[1])
        logger.info("Channel {} : name = {}, temperature = {}".format(
            ch, name, temp))
        return name, temp

    moving_integrator = integrator.MovingIntegrator()
    integrator_interface = integrator.InputInterface(moving_integrator)
    software_limiter = SoftwareLimiter(moving_integrator)

    ch0_config = parse_channel("0", args.ch0)
    ch1_config = parse_channel("1", args.ch1)

    chs = [ch0_config, ch1_config]

    dev = OvenController([ch[0] for ch in chs], [ch[1] for ch in chs],
                         integrator_interface)
    try:
        simple_server_loop(
            {
                "atomic_oven_controller": dev,
                "loading_limiter": software_limiter
            }, bind_address_from_args(args), args.port)
    finally:
        dev.close()


if __name__ == "__main__":
    main()
