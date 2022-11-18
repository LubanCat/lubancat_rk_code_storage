#!/usr/bin/env python3
"""Pymodbus asynchronous Server Example.

An example of a multi threaded asynchronous server.

usage: server_async.py [-h] [--comm {serial}]
                       [--framer {rtu}]
                       [--log {critical,error,warning,info,debug}]
                       [--port PORT] [--store {sequential}]
                       [--slaves SLAVES]

Command line options for examples

options:
  -h, --help            show this help message and exit
  --comm {serial}
            "serial"
  --framer {rtu}
            "rtu"
  --log {critical,error,warning,info,debug}
                        "critical", "error", "warning", "info" or "debug"
  --port PORT           the port to use
  --store {sequential}
                        "sequential"
  --slaves SLAVES       number of slaves to respond to

The corresponding client can be started as:
    python3 client_sync.py
"""
import asyncio
import logging
import argparse
import os

from pymodbus import pymodbus_apply_logging_config
from pymodbus.transaction import (
    ModbusRtuFramer,
)
from pymodbus.datastore import (
    ModbusSequentialDataBlock,
    ModbusServerContext,
    ModbusSlaveContext,
    ModbusSparseDataBlock,
)
from pymodbus.device import ModbusDeviceIdentification

from pymodbus.server import (
    StartAsyncSerialServer,
)
from pymodbus.version import version

_logger = logging.getLogger()

def get_commandline(server=False, description=None, extras=None):
    """Read and validate command line arguments"""
    parser = argparse.ArgumentParser(description=description)
    parser.add_argument(
        "--comm",
        choices=["serial"],
        help="set communication",
        default="serial",
        type=str,
    )
    parser.add_argument(
        "--framer",
        choices=["rtu"],
        help="set framer, default depends on --comm",
        type=str,
    )
    parser.add_argument(
        "--log",
        choices=["critical", "error", "warning", "info", "debug"],
        help="set log level, default is info",
        default="info",
        type=str,
    )
    parser.add_argument(
        "--port",
        help="set port",
        type=str,
    )
    if server:
        parser.add_argument(
            "--store",
            choices=["sequential"],
            help="set type of datastore",
            default="sequential",
            type=str,
        )
        parser.add_argument(
            "--slaves",
            help="set number of slaves, default is 0 (any)",
            default=0,
            type=int,
            nargs="+",
        )
    if extras:
        for extra in extras:
            parser.add_argument(extra[0], **extra[1])
    args = parser.parse_args()

    # set defaults
    comm_defaults = {
        "serial": ["rtu", "/dev/ptyp0"],
    }
    framers = {
        "rtu": ModbusRtuFramer,
    }
    pymodbus_apply_logging_config()
    _logger.setLevel(args.log.upper())
    args.framer = framers[args.framer or comm_defaults[args.comm][0]]
    args.port = args.port or comm_defaults[args.comm][1]
    if args.comm != "serial" and args.port:
        args.port = int(args.port)
    return args

def setup_server(args):
    """Run server setup."""
    # The datastores only respond to the addresses that are initialized
    # If you initialize a DataBlock to addresses of 0x00 to 0xFF, a request to
    # 0x100 will respond with an invalid address exception.
    # This is because many devices exhibit this kind of behavior (but not all)
    _logger.info("### Create datastore")
    # 定义co线圈寄存器，存储起始地址为0，长度为10，内容为5个True及5个False
    co_block = ModbusSequentialDataBlock(0, [True]*5 + [False]*5)
    # 定义di离散输入寄存器，存储起始地址为0，长度为10，内容为5个True及5个False
    di_block = ModbusSequentialDataBlock(0, [True]*5 + [False]*5)
    # 定义ir输入寄存器，存储起始地址为0，长度为10，内容为0~10递增数值列表
    ir_block = ModbusSequentialDataBlock(0, [0, 1, 2, 3, 4, 5, 6, 7, 8, 9])
    # 定义hr保持寄存器，存储起始地址为0，长度为10，内容为0~10递减数值列表
    hr_block = ModbusSequentialDataBlock(0, [9, 8, 7, 6, 5, 4, 3, 2, 1, 0])

    if args.slaves:
        # The server then makes use of a server context that allows the server
        # to respond with different slave contexts for different unit ids.
        # By default it will return the same context for every unit id supplied
        # (broadcast mode).
        # However, this can be overloaded by setting the single flag to False and
        # then supplying a dictionary of unit id to context mapping::
        #
        # The slave context can also be initialized in zero_mode which means
        # that a request to address(0-7) will map to the address (0-7).
        # The default is False which is based on section 4.4 of the
        # specification, so address(0-7) will map to (1-8)::
        context = {
            0x01: ModbusSlaveContext(
                di=di_block,
                co=co_block,
                hr=hr_block,
                ir=ir_block,
                zero_mode=True
            ),
            0x02: ModbusSlaveContext(
                di=di_block,
                co=co_block,
                hr=hr_block,
                ir=ir_block,
            ),
            0x03: ModbusSlaveContext(
                di=di_block,
                co=co_block,
                hr=hr_block,
                ir=ir_block,
            ),
        }
        single = False
    else:
        context = ModbusSlaveContext(
            di=di_block, co=co_block, hr=hr_block, ir=ir_block, unit=1
        )
        single = True

    # Build data storage
    args.context = ModbusServerContext(slaves=context, single=single)
    return args


async def run_async_server(args):
    """Run server."""
    txt = f"### start ASYNC server, listening on {args.port} - {args.comm}"
    _logger.info(txt)
    # socat -d -d PTY,link=/tmp/ptyp0,raw,echo=0,ispeed=9600
    #             PTY,link=/tmp/ttyp0,raw,echo=0,ospeed=9600
    server = await StartAsyncSerialServer(
        context=args.context,  # Data storage
        #identity=args.identity,  # server identify
        timeout=1,  # waiting time for request to complete
        port=args.port,  # serial port
        # custom_functions=[],  # allow custom handling
        framer=args.framer,  # The framer strategy to use
        # handler=None,  # handler for each session
        stopbits=1,  # The number of stop bits to use
        bytesize=8,  # The bytesize of the serial messages
        # parity="N",  # Which kind of parity to use
        baudrate=115200,  # The baud rate to use for the serial device
        # handle_local_echo=False,  # Handle local echo of the USB-to-RS485 adaptor
        # ignore_missing_slaves=True,  # ignore request to a missing slave
        # broadcast_enable=False,  # treat unit_id 0 as broadcast address,
        # strict=True,  # use strict timing, t1.5 for Modbus RTU
        # defer_start=False,  # Only define server do not activate
    )
    return server

if __name__ == "__main__":
    cmd_args = get_commandline(
        server=True,
        description="Run asynchronous server.",
    )
    run_args = setup_server(cmd_args)
    asyncio.run(run_async_server(run_args), debug=True)
