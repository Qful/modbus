# coding=UTF-8

from __future__ import division

#import pylibmodbus

import time
import serial
import pymodbus
import unittest
from pymodbus.pdu import ModbusRequest
from pymodbus.client.sync import ModbusSerialClient as ModbusClient
from pymodbus.transaction import ModbusRtuFramer
from pymodbus.constants import Endian
from pymodbus.payload import BinaryPayloadDecoder as decode
from pymodbus.payload import BinaryPayloadBuilder as builder

#Diagnosis messages not requires.

#from pymodbus.diag_message import *
#from pymodbus.file_message import *
#from pymodbus.other_message import *
#from pymodbus.mei_message import *

#Endian library for decoding HEX to Float
#logging not required. 
#import logging
#logging.basicConfig()
#log=logging.getLogger()
#log.setLevel(logging.DEBUG)
ID = 0x1

try:
	client = ModbusClient(method ='rtu',port='/dev/ttyUSB0',timeout=1,baudrate=4800) 
	client.connect()
except:
	#log.info("connect serial error")
	print "connect serial error" 
	time.sleep(1)
	
while 1:
	A = client.read_holding_registers(0, 8, unit=ID)  # 03H读保持寄存器(起始寄存器号，数量，从机号)->返回成功与否
	print(A)
	print(A.registers)  # 读出的数据列表
	rq = client.write_register(0,31,unit=ID)  # 06H写保持寄存器(起始寄存器号，值，从机号)->返回写的数值
	print(rq)  # 写入的数值
	print(rq.function_code)  # 功能码
	time.sleep(1)
	write_data = [23] * 6
	rq = client.write_registers(0, write_data)
	#print(rq)  # 写入的数值
	#print(rq.function_code)  # 功能码
	time.sleep(1)
	read_data = client.read_registers(0, 8)
	print(read_data)
	print(read_data.registers)  # 读出的数据列表
	timestamp = time.strftime('%H:%M:%S %d-%m-%Y')
	print timestamp
    #assert (rq.function_code < 0x80)  # test that we are not an error
    #assert (rr.registers[1] == 666)  # test the expected value

client.close()

