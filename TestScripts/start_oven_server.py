
from oven_pic_interface import OvenPICInterface
import argparse
import sys
from artiq.protocols.pc_rpc import simple_server_loop
from artiq.tools import *


def simple_network_args(parser, default_port):
    group = parser.add_argument_group("network server")
    group.add_argument(
        "--bind", default=['*'], action="append",
        help="additional hostname or IP addresse to bind to; "
        "use '*' to bind to all interfaces (default: %(default)s)")
    group.add_argument(
        "--no-localhost-bind", default=False, action="store_true",
        help="do not implicitly also bind to localhost addresses")
    if isinstance(default_port, int):
        group.add_argument("-p", "--port", default=default_port, type=int,
                           help="TCP port to listen on (default: %(default)d)")
    else:
        for name, purpose, default in default_port:
            h = ("TCP port to listen on for {} connections (default: {})"
                 .format(purpose, default))
            group.add_argument("--port-" + name, default=default, type=int,
                               help=h)

def get_argparser():
    parser = argparse.ArgumentParser()
    simple_network_args(parser, 5000)
    verbosity_args(parser)
    return parser

def main():
    args = get_argparser().parse_args()
    init_logger(args)
    product = 'atomic_oven_controller'
    dev = OvenPICInterface()
    try:
        simple_server_loop({product: dev},
            bind_address_from_args(args), args.port)
    finally:
        dev.close()

if __name__ == "__main__":
    main()
