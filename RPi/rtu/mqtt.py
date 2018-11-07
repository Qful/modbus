# coding=UTF-8
# import paho.mqtt.client as mqtt
import paho.mqtt.publish as publish
import paho.mqtt.client as mqtt
import json
import serial
import time
import random
import numpy
import pymodbus

from pymodbus.pdu import ModbusRequest
from pymodbus.client.sync import ModbusSerialClient as ModbusClient
from pymodbus.transaction import ModbusRtuFramer
from pymodbus.constants import Endian
from pymodbus.payload import BinaryPayloadDecoder as decode
from pymodbus.payload import BinaryPayloadBuilder as builder

HOST= "120.79.63.76"
PORT= 1883

SID= "device/qitas"

try:
	client = ModbusClient(method ='rtu',port='/dev/ttyUSB0',timeout=1,baudrate=4800) 
	client.connect()
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

	

while True:
	A = client.read_holding_registers(0,8,unit=1)#03H读保持寄存器(起始寄存器号，数量，从机号)->返回成功与否
	print(A.registers)  # 读出的数据列表
	#rq = client.write_register(0,31,unit=ID)  # 06H写保持寄存器(起始寄存器号，值，从机号)->返回写的数值
	#print(rq)  # 写入的数值
	#print(rq.function_code)  # 功能码
	#time.sleep(1)
	#write_data = [23] * 6
	#rq = client.write_registers(0, write_data)
	waterGage=random.uniform(0.5,1)
	voltage=random.uniform(3,5)
	dummy={"power":round(waterGage,2),"voltage":round(voltage,3)}
	jsondata=json.dumps(dummy)
	print("SID:"+SID)
	print(jsondata)
	client_id = time.strftime('%Y%m%d%H%M%S',time.localtime(time.time()))
	publish.single(SID, jsondata, qos = 1,hostname=HOST,port=PORT, client_id=client_id,auth = {'username':"admin", 'password':"public"})
	#print("qitas send")
	time.sleep(6)
	
client.close()
		
