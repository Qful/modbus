# coding=UTF-8

from __future__ import division



import time
import serial
import unittest
import pylibmodbus

ID = 0x1


def test_get_set_timeout(self):
	old_response_timeout = self.mb.get_response_timeout()
	self.mb.set_response_timeout(old_response_timeout + 1)

	new_response_timeout = self.mb.get_response_timeout()
	self.assertEqual(new_response_timeout, old_response_timeout + 1)

def test_read_and_write(self):
	nb = 5

	# Write [0, 0, 0, 0, 0]
	write_data = [0] * nb
	self.mb.write_registers(0, write_data)

	# Read
	read_data = self.mb.read_registers(0, nb)
	self.assertListEqual(write_data, list(read_data))

	# Write [0, 1, 2, 3, 4]
	write_data = list(range(nb))
	self.mb.write_registers(0, write_data)

	# Read
	read_data = self.mb.read_registers(0, nb)
	self.assertListEqual(write_data, list(read_data))

def test_write_and_read_registers(self):
	write_data = list(range(5))
	# Write 5 registers and read 3 from address 2
	read_data = self.mb.write_and_read_registers(0, write_data, 2, 3)
	self.assertListEqual(list(read_data), write_data[2:])	


try:
	client = pylibmodbus.ModbusTcp("127.0.0.1", 1502)ModbusClient(method ='rtu',port='/dev/ttyUSB0',timeout=1,baudrate=4800) 
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

