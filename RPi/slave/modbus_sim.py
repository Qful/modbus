#!/usr/bin/env python
"""
A Modbus slave device used to simulate various RTU/PLC serial devices for testing.

Sets up a simmple device construct with 10,000 of each type of register.
Increments values on the input registers every 5 seconds.

.. todo::
   Create several device CSV templates as examples.

.. csvtable::

   +------------+--------------+------+--------------+------------+----------+---------+---------------+-------------+----------+
   | DaviceName | Manufacturer | Port | SerialConfig | ModbusMode | Function | Address | RegisterCount | Description | Encoding |
   +------------+--------------+------+--------------+------------+----------+---------+---------------+-------------+----------+

* **Port** is TCP, UDP or Serial
* **SerialConfig** example 9600-8N1
* **ModbusMode** TCP, RTU, ASCII
* **Function** code e.g. 0x04 Read Input Registers
* **Address** is the zero-mode address (e.g. PLC - 1)
* **RegisterCount** is the number of contiguous registers representing a value
* **Description** what the value represents
* **DataType**
* **Scale**

"""

__version__ = "0.1.0"

import sys
import argparse
import serial
import glob

import headless
from headless import RepeatingTimer

from pymodbus import __version__ as pymodbus_version
from pymodbus.server.async import StartTcpServer
from pymodbus.server.async import StartUdpServer
from pymodbus.server.async import StartSerialServer

from pymodbus.device import ModbusDeviceIdentification
from pymodbus.datastore import ModbusSequentialDataBlock, ModbusSparseDataBlock
from pymodbus.datastore import ModbusSlaveContext, ModbusServerContext
from pymodbus.transaction import ModbusRtuFramer, ModbusAsciiFramer, ModbusSocketFramer

from pymodbus.constants import Endian
from pymodbus.payload import BinaryPayloadBuilder
from pymodbus.payload import BinaryPayloadDecoder

PORT_DEFAULT = 'tcp'

# --------------------------------------------------------------------------- #
# Service logging configuration
# --------------------------------------------------------------------------- #
log = headless.get_wrapping_log(debug=False)

# --------------------------------------------------------------------------- #
# Modbus function code reference
# --------------------------------------------------------------------------- #
READ_CO = 0x01
READ_DI = 0x02
READ_HR = 0x03
READ_IR = 0x04
WRITE_SINGLE_CO = 0x05
WRITE_SINGLE_HR = 0x06
WRITE_MULTI_CO = 0x0f
WRITE_MULTI_HR = 0x10
READ_EXCEPTION_STATUS = 0x07
READ_DIAGNOSTICS = 0x08

# --------------------------------------------------------------------------- #
# File parser configuration
# --------------------------------------------------------------------------- #
TEMPLATE_PARSER_DESC = "/*DEVICE"
TEMPLATE_PARSER_PORT = "port"
TEMPLATE_PARSER_NETWORK = "deviceId"
TEMPLATE_PARSER_REG_DESC = "/*REGISTER"
TEMPLATE_PARSER_REG = "paramId"
TEMPLATE_PARSER_SEPARATOR = ";"

DEFAULT_TEMPLATE = "/*DEVICE;VendorName=PyModbus;ProductCode=PM;VendorUrl=http://github.com/bashwork/pymodbus;" \
                   "ProducName=PyModbus;ModelName=AsyncServer;MajorMinorRevision=1.0.0;sparse=1\n" \
                   "port=tcp;mode=tcp\n" \
                   "deviceId=1;networkId=1;plcBaseAddress=0\n" \
                   "/*REGISTER;paramId=1;Name=Pressure;Default=100\n" \
                   "paramId=1;deviceId=1;registerType=analog;address=0;encoding=int16\n" \
                   "/*REGISTER;paramId=2;Name=HighPressure;Default=150\n" \
                   "paramId=2;deviceId=1;registerType=holding;address=0;encoding=int16\n" \
                   "/*REGISTER;paramId=3;Name=Indicator;Default=0\n" \
                   "paramId=3;deviceId=1;registerType=input;address=0;encoding=boolean\n" \
                   "/*REGISTER;paramId=4;Name=DoorOpen;Default=0\n" \
                   "paramId=4;deviceId=1;registerType=coil;address=0;encoding=boolean\n" \
                   "/*REGISTER;paramId=5;Name=Temperature;Default=22.5\n" \
                   "paramId=5;deviceId=1;registerType=analog;address=1;encoding=float32\n"


class DeviceEndpoint(object):
    """
    Data model for an endpoint device such as sensor or actuator
    """
    def __init__(self, name, device_type, register_type, register_address, value_range=None, units=None,
                 value=None, refresh_interval=None):
        self.name = name
        self.device_types = ['sensor', 'actuator']
        if device_type in self.device_types:
            self.type = device_type
        else:
            raise EnvironmentError("Invalid device type {type}".format(type=type))
        self.register_type = register_type
        self.register_address = register_address
        self.value_range = value_range
        self.units = units
        self.value = value
        self.refresh_interval = refresh_interval

    def something(self):
        pass


class Slave(object):
    """
    Data model for a Modbus Slave (e.g. RTU, PLC)

    .. todo::

       * Document template format.
       * Does Slave or RegisterBlock need to hold the ModbusServerContext to read/write values?

    :param template: a specifically formatted template or file

    """
    def __init__(self, template='DEFAULT'):
        """
        See Slave docstring

        :param template: a specifically formatted template or file

        """
        self.template = template
        self.identity = ModbusDeviceIdentification()
        self.port = None
        self.ser_config = None
        self.mode = None
        self.slave_id = None
        self.zero_mode = True
        self.registers = []
        self.devices = []
        self.context = None
        self.parse_template()
        self.sparse = False
        # self.builder = BinaryPayloadBuilder(byteorder=Endian.Big, wordorder=Endian.Big)

    def parse_template(self):
        if self.template == 'DEFAULT':
            file = DEFAULT_TEMPLATE
        else:
            # TODO: validate and open file as string
            file = DEFAULT_TEMPLATE
        lines = file.splitlines()
        for line in lines:
            if line[0:len(TEMPLATE_PARSER_DESC)] == TEMPLATE_PARSER_DESC:
                id = line.split(TEMPLATE_PARSER_SEPARATOR)
                for i in id:
                    if i[0:len('VendorName')] == 'VendorName':
                        self.identity.VendorName = i[len('VendorName')+1:]
                    elif i[0:len('ProductCode')] == 'ProductCode':
                        self.identity.ProductCode = i[len('ProductCode')+1:]
                    elif i[0:len('VendorUrl')] == 'VendorUrl':
                        self.identity.VendorUrl = i[len('VendorUrl')+1:]
                    elif i[0:len('ProductName')] == 'ProductName':
                        self.identity.ProductName = i[len('ProductName')+1:]
                    elif i[0:len('ModelName')] == 'ModelName':
                        self.identity.ModelName = i[len('ModelName')+1:]
                    elif i[0:len('MajorMinorRevision')] == 'MajorMinorRevision':
                        self.identity.MajorMinorRevision = i[len('MajorMinorRevision')+1:]
                    elif i[0:len('sparse')].lower() == 'sparse':
                        self.sparse = bool(int(i[len('sparse')+1:]))
                if self.template == 'DEFAULT':
                    self.identity.MajorMinorRevision = pymodbus_version
            elif line[0:len(TEMPLATE_PARSER_NETWORK)] == TEMPLATE_PARSER_NETWORK:
                net_info = line.split(TEMPLATE_PARSER_SEPARATOR)
                for i in net_info:
                    if i[0:len('networkId')] == 'networkId':
                        net_id = int(i[len('networkId')+1:])
                        if net_id in range (1, 254):
                            self.slave_id = int(i[len('networkId')+1:])
                        else:
                            log.error("Invalid Modbus Slave ID {id}".format(id=net_id))
                    elif i[0:len('plcBaseAddress')] == 'plcBaseAddress':
                        plc = int(i[len('plcBaseAddress')+1:])
                        self.zero_mode = False if plc == 1 else True
            elif line[0:len(TEMPLATE_PARSER_PORT)] == TEMPLATE_PARSER_PORT:
                port_info = line.split(TEMPLATE_PARSER_SEPARATOR)
                for i in port_info:
                    if i[0:len('port')] == 'port':
                        log.info("port={port}".format(port=i[len('port')+1:]))
                        port = i[len('port')+1:].lower()
                        if port in ['tcp', 'udp']:
                            self.port = port
                        elif port in list_serial_ports():
                            self.ser_config = SerialPort()
                        else:
                            raise EnvironmentError("Undefined port read from {template}".format(template=self.template))
                    elif i[0:len('mode')] == 'mode':
                        log.info("mode={mode}".format(mode=i[len('mode')+1:]))
                        self.mode = i[len('mode')+1:]
                        if self.mode not in ['rtu', 'ascii', 'tcp']:
                            log.error("Undefined mode parsed from {template}".format(template=self.template))
                            raise EnvironmentError("Undefined Modbus mode read from {template}"
                                                   .format(template=self.template))
                    elif i[0:len('baudRate')] == 'baudRate':
                        log.info("baudRate={baudRate}".format(baudRate=i[len('baudRate')+1:]))
                        baudrate = int(i[len('baudRate')+1:])
                        if baudrate in self.ser_config.baudrates:
                            self.ser_config.baudrate = baudrate
                        else:
                            log.error("Unsupported baud rate {baud}".format(baud=str(baudrate)))
                    elif i[0:len('parity')] == 'parity':
                        log.info("parity={parity}".format(parity=i[len('parity')+1:]))
                        parity = i[len('parity')+1:]
                        if parity not in ['none', 'even', 'odd']:
                            self.ser_config.parity = parity
                        else:
                            log.error("Undefined parity {parity}".format(parity=parity))
            elif line[0:len(TEMPLATE_PARSER_REG_DESC)] == TEMPLATE_PARSER_REG_DESC:
                reg = self.RegisterBlock(parent=self)
                reg_desc = line.split(TEMPLATE_PARSER_SEPARATOR)
                for i in reg_desc:
                    if i[0:len('paramId')] == 'paramId':
                        reg.paramId = int(i[len('paramId')+1:])
                    elif i[0:len('address')].lower() == 'address':
                        addr = int(i[len('address') + 1:])
                        if addr in range(0, 99999):
                            reg.address = addr
                        else:
                            log.error("Invalid Modbus address {num}".format(num=addr))
                    elif i[0:len('name')].lower() == 'name':
                        reg.name = i[len('name') + 1:]
                    elif i[0:len('default')].lower() == 'default':
                        reg.default = i[len('default') + 1:]
                self.registers.append(reg)
            elif line[0:len(TEMPLATE_PARSER_REG)] == TEMPLATE_PARSER_REG:
                # TODO: assign to proper objects, sort/group addresses by type and min/max
                reg_config = line.split(TEMPLATE_PARSER_SEPARATOR)
                reg_exists = False
                this_reg = None
                for c in reg_config:
                    if c[0:len('paramId')] == 'paramId':
                        paramId = int(c[len('paramId')+1:])
                        for reg in self.registers:
                            if paramId == reg.paramId:
                                reg_exists = True
                        if not reg_exists:
                            reg = self.RegisterBlock(parent=self, paramId=paramId)
                            self.registers.append(reg)
                            reg_exists = True
                        this_reg = paramId
                    elif c[0:len('address')] == 'address':
                        addr = int(c[len('address')+1:])
                        if addr in range(0, 99999):
                            for reg in self.registers:
                                if reg.paramId == this_reg:
                                    reg.address = addr
                            if not reg_exists:
                                reg = self.RegisterBlock(parent=self, address=addr)
                                self.registers.append(reg)
                                reg_exists = True
                        else:
                            log.error("Invalid Modbus address {num}".format(num=addr))
                    elif c[0:len('registerType')] == 'registerType':
                        reg_type = c[len('registerType')+1:]
                        if reg_type in ['analog']:
                            for reg in self.registers:
                                if reg.paramId == this_reg:
                                    reg.type = 'ir'
                        elif reg_type in ['holding']:
                            for reg in self.registers:
                                if reg.paramId == this_reg:
                                    reg.type = 'hr'
                        elif reg_type in ['input']:
                            for reg in self.registers:
                                if reg.paramId == this_reg:
                                    reg.type = 'di'
                        elif reg_type in ['coil']:
                            for reg in self.registers:
                                if reg.paramId == this_reg:
                                    reg.type = 'co'
                        else:
                            log.error("Unsupported registerType {type}".format(type=reg_type))
                    elif c[0:len('encoding')] == 'encoding':
                        enc = c[len('encoding')+1:]
                        if enc in ['int16', 'int8', 'boolean']:
                            for reg in self.registers:
                                if reg.paramId == this_reg:
                                    reg.encoding = enc
                                    reg.default = int(reg.default)
                        elif enc in ['float32', 'int32']:
                            for reg in self.registers:
                                if reg.paramId == this_reg:
                                    reg.encoding = enc
                                    reg.length = 2
                                    reg.default = float(reg.default) if enc == 'float32' else int(reg.default)
                        else:
                            log.error("Unsupported encoding {type}".format(type=enc))
        hr_sequential = []
        hr_sparse_block = {}
        ir_sequential = []
        ir_sparse_block = {}
        di_sequential = []
        di_sparse_block = {}
        co_sequential = []
        co_sparse_block = {}
        for reg in self.registers:
            if reg.min is None:
                reg.min = reg._get_range()[0]
            if reg.max is None:
                reg.max = reg._get_range()[1]
            if reg.type == 'hr' and reg.address is not None:
                if self.sparse:
                    hr_sparse_block[reg.address] = 0
                else:
                    hr_sequential.append(reg.address)
            elif reg.type == 'ir' and reg.address is not None:
                if self.sparse:
                    ir_sparse_block[reg.address] = 0
                else:
                    ir_sequential.append(reg.address)
            elif reg.type == 'di' and reg.address is not None:
                if self.sparse:
                    di_sparse_block[reg.address] = 0
                else:
                    di_sequential.append(reg.address)
            elif reg.type == 'co' and reg.address is not None:
                if self.sparse:
                    co_sparse_block[reg.address] = 0
                else:
                    co_sequential.append(reg.address)
            else:
                log.error("Unhandled exception register {reg} addr=".format(reg=reg.name, addr=str(reg.address)))
        if self.sparse:
            hr_block = ModbusSparseDataBlock(hr_sparse_block)
            ir_block = ModbusSparseDataBlock(ir_sparse_block)
            di_block = ModbusSparseDataBlock(di_sparse_block)
            co_block = ModbusSparseDataBlock(co_sparse_block)
        else:
            hr_sequential.sort()
            hr_block = ModbusSequentialDataBlock(0, [0 for x in range(hr_sequential[0],
                                                                      hr_sequential[len(hr_sequential)-1])])
            ir_sequential.sort()
            ir_block = ModbusSequentialDataBlock(0, [0 for x in range(ir_sequential[0],
                                                                      ir_sequential[len(ir_sequential)-1])])
            di_sequential.sort()
            di_block = ModbusSequentialDataBlock(0, [0 for x in range(di_sequential[0],
                                                                      di_sequential[len(di_sequential)-1])])
            co_sequential.sort()
            co_block = ModbusSequentialDataBlock(0, [0 for x in range(co_sequential[0],
                                                                      co_sequential[len(co_sequential)-1])])
        self.context = ModbusSlaveContext(hr=hr_block, ir=ir_block, di=di_block, co=co_block, zero_mode=self.zero_mode)
        for reg in self.registers:
            reg.set_value(reg.default)

    class RegisterBlock(object):
        """
        .. todo::
           docstring
        """
        def __init__(self, parent, paramId=None, address=None, length=1, name=None, type=None, encoding=None,
                     default=0, min=None, max=None):
            self.parent = parent
            self.paramId = paramId
            self.address = address
            self.length = length
            self.name = name
            self.types = ['hr', 'ir', 'di', 'co']
            self.type = type
            self.encodings = ['uint8', 'uint16', 'uint32', 'int8', 'int16', 'int32', 'boolean', 'float32']
            self.encoding = encoding
            self.default = default
            self.min = min if min is not None else self._get_range()[0]
            self.max = max if max is not None else self._get_range()[1]
            # TODO: get Endianness from template
            self.byteorder=Endian.Big
            self.wordorder=Endian.Big
            self.value = None

        def _get_range(self):
            """Gets nominal max/min ranges based on encoding type"""
            min = None
            max = None
            if self.encoding is not None:
                if 'uint' in self.encoding[0:len('uint')]:
                    min = 0
                    bits = int(self.encoding[len('uint'):])
                    max = 2**bits - 1
                elif 'int' in self.encoding[0:len('int')]:
                    bits = int(self.encoding[len('int'):])
                    min = -(2**bits/2)
                    max = (2**bits/2) - 1
                elif self.encoding == 'boolean':
                    min = 0
                    max = 1
            return min, max

        def get_function_code(self, read=True):
            """
            Gets the Modbus function code for the register type and read/write operation.

            :param read: read/write
            :return: Modbus function code

            """
            if self.type in ['hr', 'co']:
                if read:
                    return READ_HR if self.type == 'hr' else READ_CO
                else:
                    return WRITE_SINGLE_HR if self.type == 'hr' else WRITE_SINGLE_CO
            elif self.type in ['ir', 'di']:
                if read:
                    return READ_IR if self.type == 'ir' else READ_DI
                else:
                    raise EnvironmentError("Illegal operation cannot write to {type}".format(type=self.type))

        def get_value(self):
            values = self.parent.context.getValues(self.get_function_code(), self.address, self.length)
            decoder = BinaryPayloadDecoder.fromRegisters(registers=values,
                                                         byteorder=self.byteorder, wordorder=self.wordorder)
            if self.encoding in ['int8', 'uint8']:
                decoded = decoder.decode_8bit_int() if self.encoding == 'int8' else decoder.decode_8bit_uint()
            elif self.encoding in ['int16', 'uint16']:
                decoded = decoder.decode_16bit_int() if self.encoding == 'int16' else decoder.decode_16bit_uint()
            elif self.encoding in ['int32', 'uint32']:
                decoded = decoder.decode_32bit_int() if self.encoding == 'int32' else decoder.decode_32bit_uint()
            elif self.encoding in ['float32', 'float64']:
                decoded = decoder.decode_32bit_float() if self.encoding == 'float32' else decoder.decode_64bit_float()
            elif self.encoding in ['int64', 'uint64']:
                decoded = decoder.decode_64bit_int() if self.encoding == 'int64' else decoder.decode_64bit_uint()
            elif self.encoding == 'boolean':
                decoded = decoder.decode_16bit_uint()
            elif self.encoding == 'string':
                decoded = decoder.decode_string()
            else:
                log.error("Unhandled encoding exception {enc}".format(enc=self.encoding))
                decoded = None
            log.info("Read {type} {addr} as {enc} {val} from {list}".format(type=self.type, addr=self.address,
                                                                            enc=self.encoding, val=decoded,
                                                                            list=str(values)))
            self.value = decoded
            return decoded

        def set_value(self, value):
            if value is not None:
                builder = BinaryPayloadBuilder(byteorder=self.byteorder, wordorder=self.wordorder)
                if self.encoding in ['int8', 'uint8']:
                    builder.add_8bit_int(value) if self.encoding == 'int8' else builder.add_8bit_uint(value)
                elif self.encoding in ['int16', 'uint16']:
                    builder.add_16bit_int(value) if self.encoding == 'int16' else builder.add_16bit_uint(value)
                elif self.encoding in ['int32', 'uint32']:
                    builder.add_32bit_int(value) if self.encoding == 'int32' else builder.add_32bit_uint(value)
                elif self.encoding in ['float32', 'float64']:
                    builder.add_32bit_float(value) if self.encoding == 'float32' else builder.add_64bit_float(value)
                elif self.encoding in ['int64', 'uint64']:
                    builder.add_64bit_int(value) if self.encoding == 'int64' else builder.add_64bit_uint(value)
                elif self.encoding == 'boolean':
                    builder.add_16bit_uint(value)
                elif self.encoding == 'string':
                    builder.add_string(value)
                else:
                    log.error("Unhandled encoding exception {enc}".format(enc=self.encoding))
                payload = builder.to_registers()
                log.info("Setting {type} {addr} to {enc} {val} as {list}".format(type=self.type, addr=self.address,
                                                                                 enc=self.encoding, val=value,
                                                                                 list=str(payload)))
                self.parent.context.setValues(self.get_function_code(), self.address, payload)
                self.value = value
            else:
                log.warning("Attempt to set {type} {addr} to None (default={default})".format(type=self.type,
                                                                                              addr=self.address,
                                                                                              default=self.default))


# From: https://github.com/SimplyAutomationized/modbus-tcp-tutorials/blob/master/tempSensors.py
'''
class Device(object):
    def __init__(self, path, devicetype, callback):
        self.path = path
        self.devicetype = devicetype
        self.callback = callback
        self.value = 0

    def update(self):
        self.value = self.callback(self.path)


def sensor_reading(sensor_path='some_path'):
    return 0


def read_device_map():
    rootpath = ''
    devices = {
        0x0001: Device(rootpath + 'unique_id', 'devicetype', sensor_reading)
    }
    return devices


class CallbackDataBlock(ModbusSparseDataBlock):
    def __init__(self, devices):
        self.devices = devices
        self.devices[0xbeef] = len(self.devices)
        # self.get_device_values()
        self.values = {k: 0 for k in self.devices.iterkeys()}
        super(CallbackDataBlock, self).__init__(self.values)

    def get_device_value(self):
        devices = []
        devices_registers = filter(lambda d: d != 0xbeef, self.devices)
        for r in devices_registers:
            if self.devices[r].devicetype == 'my_device':
                self.devices[r].register = r
                devices.append(self.devices[r])
        t = threads.deferToThread(self.get_devices_values, devices)
        t.addCallback(self.update_devices_values)

    def get_devices_values(self, devices):
        values = {}
        for d in devices:
            d.update()
            values[d.register] = d.value
        return values

    def update_device_values(self, values):
        for register in values:
            self.values[register] = values[register]
        self.get_device_value()
'''


def get_slave_context(device='DEFAULT'):
    """
    Creates a Modbus slave context based on an input template

    .. note::
       DEPRECATED / UNUSED

    :param device: a device template
    :return: ModbusSlaveContext

    """
    log.debug("Getting slave context")
    supported_devices = ['DEFAULT']
    if device not in supported_devices:
        raise EnvironmentError("Unsupported Device Template: " + device)
    default_hr = 10000
    default_ir = 10000
    default_di = 10000
    default_co = 10000
    default_hr_value = 3
    default_ir_value = 2
    default_di_value = 1
    default_co_value = 0
    if device in supported_devices:
        # TODO: Create reference/test devices
        hr_block = ModbusSequentialDataBlock(0, [default_hr_value for x in range(default_hr)])
        ir_block = ModbusSequentialDataBlock(0, [default_ir_value for x in range(default_ir)])
        di_block = ModbusSequentialDataBlock(0, [default_di_value for x in range(default_di)])
        co_block = ModbusSequentialDataBlock(0, [default_co_value for x in range(default_co)])
        return ModbusSlaveContext(hr=hr_block, ir=ir_block, di=di_block, co=co_block)
    else:
        raise NotImplementedError('Target Modbus device not supported.')


def update_values(server_context, slaves):
    """
    Updates the configured register values in the Modbus context.
    Increments or toggles values

    .. todo::

       * Proper striping of large values across 16-bit registers.
       * Support device simulation templates.

    :param server_context: a ``ModbusServerContext`` object
    :param slaves: a list of ``Slave`` objects

    """
    # context = server_context
    for slave in slaves:
        for reg in slave.registers:
            old_value = reg.get_value()
            if reg.type in ['hr', 'ir']:
                new_value = old_value + 1 if old_value < reg.max or reg.max is None else reg.min
            else:
                new_value = 0 if old_value == 1 else 1
            reg.set_value(new_value)


def list_serial_ports():
    """
    Lists serial port names.

    :raises EnvironmentError: On unsupported or unknown platforms
    :returns: A list of the serial ports available on the system

    """
    if sys.platform.startswith('win'):
        ports = ['COM%s' % (i + 1) for i in range(256)]
    elif sys.platform.startswith('linux') or sys.platform.startswith('cygwin'):
        # this excludes your current terminal "/dev/tty"
        ports = glob.glob('/dev/tty[A-Za-z]*')
    elif sys.platform.startswith('darwin'):
        ports = glob.glob('/dev/tty.*')
    else:
        raise EnvironmentError("Unsupported OS/platform")
    result = []
    for port in ports:
        try:
            s = serial.Serial(port)
            s.close()
            result.append(port)
        except (OSError, serial.SerialException):
            pass
    return result


class SerialPort(object):
    def __init__(self, port='/dev/ttyUSB0', baudrate=9600, bytesize=8, parity='none', stopbits=1):
        if port in list_serial_ports():
            self.port = port
            self.baudrate = baudrate
            self.baudrates = [2400, 4800, 9600, 19200, 38400, 57600, 115200]
            self.bytesize = bytesize
            self.parity = parity
            self.stopbits = stopbits
            self.timeout = None
            self.write_timeout = None
            self.inter_byte_timeout = None
            self.xonxoff = False
            self.rtscts = False
            self.dsrdtr = False
        else:
            raise EnvironmentError("Unable to find specified serial port {port}".format(port=port))

    def set_bps(self, shorthand):
        baudrate = int(shorthand.split("-")[0])
        framing = shorthand.split("-")[1]
        if baudrate in self.baudrates:
            self.baudrate = baudrate
        else:
            log.error("Attempt to configure unsupported baud rate {num}".format(num=baudrate))
        if framing == '8N1':
            self.bytesize = 8
            self.parity = 'none'
            self.stopbits = 1
        else:
            log.error("Attempt to configure unsupported framing {config}".format(config=framing))


def get_parser():
    """
    Parses the command line arguments.

    :returns: An argparse.ArgumentParser

    """
    parser = argparse.ArgumentParser(description="Modbus Slave Device.")

    port_choices = list_serial_ports() + ['tcp', 'udp']

    parser.add_argument('-t', '--template', dest='template', default='DEFAULT',
                        help="the template file to use")

    parser.add_argument('-p', '--port', dest='port', default=PORT_DEFAULT,
                        choices=port_choices,
                        help="tcp:502, udp:5020, or a USB/serial port name")

    parser.add_argument('-b', '--baud', default=9600, type=int,
                        choices=[2400, 4800, 9600, 19200, 38400, 57600, 115200],
                        help="baud rate (``int`` default 9600)", metavar="{2400..115200}")

    parser.add_argument('-f', '--framing', dest='framing', default='8N1',
                        choices=['8N1'],
                        help="serial port framing (byte size, parity, stop bits)")

    parser.add_argument('-m', '--mode', dest='mode', default=None,
                        choices=['rtu', 'ascii', 'tcp'],
                        help="Modbus framing mode RTU, ASCII or TCP")

    parser.add_argument('--logfile', default=None,
                        help="the log file name with optional extension (default extension .log)")

    parser.add_argument('--logsize', type=int, default=5,
                        help="the maximum log file size, in MB (default 5 MB)")

    parser.add_argument('--debug', action='store_true',
                        help="enable tick_log debug logging (default OFF)")

    return parser


def run_async_server():
    """
    Runs the Modbus asynchronous server
    """
    parser = get_parser()
    user_options = parser.parse_args()
    TCP_PORT = 502
    UDP_PORT = 5020

    # ----------------------------------------------------------------------- #
    # initialize your data store
    # ----------------------------------------------------------------------- #
    # The datastores only respond to the addresses that they are initialized to
    # Therefore, if you initialize a DataBlock to addresses from 0x00 to 0xFF,
    # a request to 0x100 will respond with an invalid address exception.
    # This is because many devices exhibit this kind of behavior (but not all)
    #
    #     block = ModbusSequentialDataBlock(0x00, [0]*0xff)
    #
    # Continuing, you can choose to use a sequential or a sparse DataBlock in
    # your data context.  The difference is that the sequential has no gaps in
    # the data while the sparse can. Once again, there are devices that exhibit
    # both forms of behavior::
    #
    #     block = ModbusSparseDataBlock({0x00: 0, 0x05: 1})
    #     block = ModbusSequentialDataBlock(0x00, [0]*5)
    #
    # Alternately, you can use the factory methods to initialize the DataBlocks
    # or simply do not pass them to have them initialized to 0x00 on the full
    # address range::
    #
    #     store = ModbusSlaveContext(di = ModbusSequentialDataBlock.create())
    #     store = ModbusSlaveContext()
    #
    # Finally, you are allowed to use the same DataBlock reference for every
    # table or you you may use a separate DataBlock for each table.
    # This depends if you would like functions to be able to access and modify
    # the same data or not::
    #
    #     block = ModbusSequentialDataBlock(0x00, [0]*0xff)
    #     store = ModbusSlaveContext(di=block, co=block, hr=block, ir=block)
    #
    # The server then makes use of a server context that allows the server to
    # respond with different slave contexts for different unit ids. By default
    # it will return the same context for every unit id supplied (broadcast
    # mode).
    # However, this can be overloaded by setting the single flag to False
    # and then supplying a dictionary of unit id to context mapping::
    #
    #     slaves  = {
    #         0x01: ModbusSlaveContext(...),
    #         0x02: ModbusSlaveContext(...),
    #         0x03: ModbusSlaveContext(...),
    #     }
    #     context = ModbusServerContext(slaves=slaves, single=False)
    #
    # The slave context can also be initialized in zero_mode which means that a
    # request to address(0-7) will map to the address (0-7). The default is
    # False which is based on section 4.4 of the specification, so address(0-7)
    # will map to (1-8)::
    #
    #     store = ModbusSlaveContext(..., zero_mode=True)
    # ----------------------------------------------------------------------- #

    # slaves = {
    #     0x01: get_slave_context(),
    # }
    slave = Slave(template=user_options.template)
    slaves = {
        slave.slave_id: slave.context
    }
    context = ModbusServerContext(slaves=slaves, single=False)

    # Set up looping call to update values
    UPDATE_INTERVAL = 5
    slave_list = [slave]
    slave_updater = RepeatingTimer(seconds=UPDATE_INTERVAL, name='slave_updater', defer=True,
                                   callback=update_values, **{'server_context': context, 'slaves': slave_list})
    slave_updater.start_timer()

    if (slave.mode == 'tcp' or user_options.mode == 'tcp'
            or user_options.mode is None and user_options.port.lower() == 'tcp'):
        framer = ModbusSocketFramer
    elif slave.mode == 'ascii' or user_options.mode == 'ascii':
        framer = ModbusAsciiFramer
    else:
        framer = ModbusRtuFramer

    # TODO: trap master connect/disconnect as INFO logs rather than DEBUG (default of pyModbus)
    if user_options.port.lower() == 'tcp':
        StartTcpServer(context, identity=slave.identity, address=("localhost", TCP_PORT), framer=framer)
    elif user_options.port.lower() == 'udp':
        StartUdpServer(context, identity=slave.identity, address=("127.0.0.1", UDP_PORT), framer=framer)
    else:
        ser = SerialPort()
        StartSerialServer(context, identity=slave.identity, framer=framer,
                          port=ser.port,
                          baudrate=ser.baudrate,
                          bytesize=ser.bytesize,
                          parity=ser.parity,
                          stopbits=ser.stopbits,)  # need timeout=X?
    # '''


if __name__ == "__main__":
    run_async_server()
