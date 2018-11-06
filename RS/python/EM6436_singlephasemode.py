#ModBUS Communication between Schneider EM6436 Meter and Raspberry Pi
#First beta version.
#The meter is set with the following settings
#Communication : (RS484 to RS232 to USB) - BaudRate = 19200, Parity = N, Stopbits = 1, Device ID=1 (Hardcode in meter)
#Electical Settings: APri:50, Asec: 5, VPri: 415, Vsec:415, SYS: SINGLE
#To use the meter in Single Phase mode, Some address has to be commented.
#This program was tested on RPi3 running Rasbian Jessie Pixel from Noobs V2
#Debian Kernel = Linux raspberrypi 4.4.38-v7+ #938 SMP Thu Dec 15 15:22:21 GMT 2016 armv7l GNU/Linux

#Additional Packages: pymodbus,pyserial. (available in pyPi repo)
#V1.0b Feb2,2017
#Code by Sai Shibu (AWNA/058/15)
#Copyrights AmritaWNA Smartgrid Tag

import time
import pymodbus 
import serial
from pymodbus.pdu import ModbusRequest
from pymodbus.client.sync import ModbusSerialClient as ModbusClient
from pymodbus.transaction import ModbusRtuFramer

#Diagnosis messages not requires.

#from pymodbus.diag_message import *
#from pymodbus.file_message import *
#from pymodbus.other_message import *
#from pymodbus.mei_message import *

#Endian library for decoding HEX to Float
from pymodbus.constants import Endian
from pymodbus.payload import BinaryPayloadDecoder as decode
from pymodbus.payload import BinaryPayloadBuilder as builder


#logging not required. 
#import logging
#logging.basicConfig()
#log=logging.getLogger()
#log.setLevel(logging.DEBUG)

#EM6436 is defined as client
client = ModbusClient(method ='rtu',port='/dev/ttyUSB0',timeout=0.05) 
client.connect()

while 1:
	
####################################################
#Read Whole Block (Bugs while decoding with Endian!!!)
#	T_RMS=client.read_holding_registers(0xbb8,20,unit=1)
#	R_RMS=client.read_holding_registers(0xbd6,20,unit=1) #Total R Phase RMS Block
#	Y_RMS=client.read_holding_registers(0xbd6,20,unit=1) #Total Y Phase RMS Block
#	B_RMS=client.read_holding_registers(0xbd6,20,unit=1) #Total B Phase RMS Block
#####################################################

	#Current Values
	A=client.read_holding_registers(3912,2,unit=1)
	

        A_d = decode.fromRegisters(A.registers, endian=Endian.Little)
        A_d ={'float':A_d.decode_32bit_float(),}

        
#####################################################
	#Voltage Values
	 
	VLN=client.read_holding_registers(3910,2,unit=1)
	
        VLN_d = decode.fromRegisters(VLN.registers, endian=Endian.Little)
        VLN_d ={'float':VLN_d.decode_32bit_float(),}

######################################################
	#Power Values 
	#NOTE: EM6436 does not give VAR Values!!!

	W=client.read_holding_registers(3902,2,unit=1) 
	
	VA=client.read_holding_registers(3900,2,unit=1)


        W_d = decode.fromRegisters(W.registers, endian=Endian.Little)
        W_d ={'float':W_d.decode_32bit_float(),}

        VA_d = decode.fromRegisters(VA.registers, endian=Endian.Little)
        VA_d ={'float':VA_d.decode_32bit_float(),}

######################################################
	#Power Factor Values
	PF=client.read_holding_registers(3906,2,unit=1)

	PF_d = decode.fromRegisters(PF.registers, endian=Endian.Little)
        PF_d ={'float':PF_d.decode_32bit_float(),}

######################################################
	#Frequency Value
	F=client.read_holding_registers(3914,2,unit=1)
	F_d = decode.fromRegisters(F.registers, endian=Endian.Little)
        F_d ={'float':F_d.decode_32bit_float(),}
######################################################
	#Energy Value
	VAH=client.read_holding_registers(3958,2,unit=1) 
	WH=client.read_holding_registers(3960,2,unit=1) 
	VAH_d = decode.fromRegisters(VAH.registers, endian=Endian.Little)
        VAH_d ={'float':VAH_d.decode_32bit_float(),}
        WH_d = decode.fromRegisters(WH.registers, endian=Endian.Little)
        WH_d ={'float':WH_d.decode_32bit_float(),}
######################################################
	#Power Interruptions count
	intr=client.read_holding_registers(3998,2,unit=1) 
	intr_d = decode.fromRegisters(intr.registers, endian=Endian.Little)
        intr_d ={'16uint':intr_d.decode_16bit_uint(),}
 
######################################################

        print "-" * 100
        timestamp = time.strftime('%H:%M:%S %d-%m-%Y')
        print timestamp
	print "Current Values"

	for i, value in A_d.iteritems():
		print value
		A=value

        print "-" * 100
	print "Voltage Values"

        

        for i, value in VLN_d.iteritems():
                print value
		VLN=value

        
	print "-" * 100
	print "Power Factor Values"

        for i, value in PF_d.iteritems():
                print value
                PF=value
        
	print "-" * 100
	print "Frequency Value"

        for i, value in F_d.iteritems():
                print value
                F=value

	print "-" * 100
	print "Power Values"

        for i, value in W_d.iteritems():
                print value
                W=value

        for i, value in VA_d.iteritems():
                print value
                VA=value

        print "-" * 100
	print "Energy Value"

        for i, value in VAH_d.iteritems():
                print value
                VAH=value
        for i, value in WH_d.iteritems():
                print value
                WH=value

	print "-" * 100
	print "interruption"

        for i, value in intr_d.iteritems():
                print value
                intr=value

	print "-" * 100
	

client.close()

