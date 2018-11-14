# coding=UTF-8
# import paho.mqtt.client as mqtt
import paho.mqtt.publish as publish
import paho.mqtt.client as mqtt
import json
import serial
import time
import random
import numpy
import logging

import pymodbus
from pymodbus.pdu import ModbusRequest
from pymodbus.client.sync import ModbusSerialClient as ModbusClient
from pymodbus.transaction import ModbusRtuFramer
from pymodbus.constants import Endian
from pymodbus.payload import BinaryPayloadDecoder as decode
from pymodbus.payload import BinaryPayloadBuilder as builder
from pymodbus.exceptions import NotImplementedException
from pymodbus.exceptions import ParameterException
from pymodbus.utilities import pack_bitstring, unpack_bitstring

#HOST= "120.79.63.76"
HOST= "192.168.1.250"
PORT= 1883

SID= "device/qitas"
DEVID = 0x1


try:
	rs485 = ModbusClient(method ='rtu',port='/dev/ttyUSB0',timeout=1,baudrate=4800) 
	rs485.connect()
except:
	#log.info("connect serial error")
	print("connect serial error") 
	time.sleep(1)
	
# 打开串口
# ser = serial.Serial("/dev/ttyUSB0", 115200)


def on_connect(client, userdata, flags, rc):
    print("Connected with result code "+str(rc))
    client.subscribe(SID)

def on_message(client, userdata, msg):
    print(msg.topic+" "+msg.payload.decode("utf-8"))

def get_GST5000(client,ID,reg,len):
	A = client.read_holding_registers(reg,len,unit=ID) #03H读保持寄存器(起始寄存器号，数量，从机号)->返回成功与否
	#print(A.registers)  # 读出的数据列表
	return A.registers
	
def set_GST5000(client,ID,reg,value):
	B = client.write_register(reg,value,unit=ID)  # 06H写保持寄存器(起始寄存器号，值，从机号)->返回写的数值
	print(B)  # 写入的数值
	return B
	
def sub_port(client, userdata, msg):
    # client.publish("test", "你好 MQTT", qos=0, retain=False)  # 发布消息
    # ser.write("qitas mqtt publish \n".encode())
    # publish.single("qitas", "qitas MQTT", qos = 1,hostname=HOST,port=PORT, client_id=client_id,auth = {'username':"Qitas", 'password':"123456"})
    client_id = time.strftime('%Y%m%d%H%M%S',time.localtime(time.time()))
    print("client_id："+client_id)
    client = mqtt.Client(client_id)    # ClientId不能重复，所以使用当前时间	
    client.username_pw_set("qitas", "qitas")  # 必须设置，否则会返回「Connected with result code 4」
    client.on_connect = on_connect
    client.on_message = on_message
    client.connect(HOST, PORT, 60)
    client.loop_forever()

while True:
	
	cnt=0
	while (cnt<10):
		ret = get_GST5000(rs485,DEVID,cnt*10,10)
		#set_GST5000(rs485,DEVID,cnt*10,ret[cnt]+1)
		cnt=cnt+1
		index=0
		while (index<10):
			if ret[index] > 0:
				print("result: at %d with %d " %((index+cnt*10),ret[index]))			
			index=index+1
		index=0
	'''
	ret = get_GST5000(rs485,DEVID,0,100)
	
	
	set_GST5000(rs485,DEVID,0,ret[0]+1)
	'''
	#print(rq.function_code)  # 功能码
	#time.sleep(1)
	#write_data = [23] * 6
	#rq = client.write_registers(0, write_data)
	dummy={"GST5000 EVENT":ret[0],"GST5000 POWER":ret[1]}
	jsondata=json.dumps(dummy)
	print("SID:"+SID)
	print(jsondata)
	client_id = time.strftime('%Y%m%d%H%M%S',time.localtime(time.time()))
	print(client_id)
	#publish.single(SID, jsondata, qos = 1,hostname=HOST,port=PORT, client_id=client_id,auth = {'username':"admin", 'password':"public"})
	#print("qitas send")
	#time.sleep(5)
	
client.close()
		
