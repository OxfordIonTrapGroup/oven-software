import time
from collections import deque


class MovingIntegrator:
    """Integrates inputs for one hour by binning them into 30-second intervals"""

    def __init__(self):
        self.reset_integral()
        self.reset_counter(0)

    def input(self, sample: float):
        """Update the integrator with a new sample

        :param: sample: the sample to add to the integrator
        """
        self._advance_deq()
        self._deq[-1] += sample
        self._input_counter += 1
        self.t_last_input = time.monotonic()

    def _advance_deq(self):
        t1 = time.monotonic()
        dt = t1 - self._t0
        di = int(dt // 30)

        # feed-forward number of time bins (30 seconds steps) since last update
        # while capping at the deque length to avoid unnecessary `append`s.
        nstep = min(di, len(self._deq))
        for _ in range(nstep):
            self._deq.append(0.)

        # Save time at start of current bin.
        self._t0 += di * 30

    def reset_integral(self):
        """Reset the integrator

        Samples are binned in 30-s intervals and kept for 1 hour.
        """
        self._deq = deque([0.0] * 120, maxlen=120)
        self._t0 = time.monotonic()

    def get_integral(self) -> float:
        """Return the integral of samples over the last hour"""
        self._advance_deq()
        return sum(self._deq)

    def reset_counter(self, count):
        self._input_counter = count
        self.t_last_input = time.monotonic()

    def get_counter(self) -> int:
        """Return the input count"""
        return self._input_counter


class InputInterface:
    """Input interface for MovingIntegrator"""

    def __init__(self, integrator):
        self._integrator = integrator
        self.t_on = None

    def on_now(self):
        # When calling on multiple times without off, assume it stayed on.
        if self.t_on is None:
            self.t_on = time.monotonic()

    def off_now(self):
        # When calling off without preceding on, assume it stayed off.
        if self.t_on is not None:
            t_off = time.monotonic()
            self._integrator.input(t_off - self.t_on)
            self.t_on = None
