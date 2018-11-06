#ModBUS Communication between Schneider EM6436 Meter and Raspberry Pi
#First beta version.
#The meter is set with the following settings
#Communication : (RS484 to RS232 to USB) - BaudRate = 19200, Parity = N, Stopbits = 1, Device ID=1 (Hardcode in meter)
#Electical Settings: APri:50, Asec: 5, VPri: 415, Vsec:415, SYS: Star
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
	print "new set"
	
####################################################
#Read Whole Block (Bugs while decoding with Endian!!!)
#	T_RMS=client.read_holding_registers(0xbb8,20,unit=1)
#	R_RMS=client.read_holding_registers(0xbd6,20,unit=1) #Total R Phase RMS Block
#	Y_RMS=client.read_holding_registers(0xbd6,20,unit=1) #Total Y Phase RMS Block
#	B_RMS=client.read_holding_registers(0xbd6,20,unit=1) #Total B Phase RMS Block
#####################################################

	#Current Values
	A=client.read_holding_registers(3912,2,unit=1)
	A1=client.read_holding_registers(3928,2,unit=1) 
	A2=client.read_holding_registers(3942,2,unit=1) 
	A3=client.read_holding_registers(3956,2,unit=1) 

        A_d = decode.fromRegisters(A.registers, endian=Endian.Little)
        A_d ={'float':A_d.decode_32bit_float(),}

        A1_d = decode.fromRegisters(A1.registers, endian=Endian.Little)
        A1_d ={'float':A1_d.decode_32bit_float(),}

        A2_d = decode.fromRegisters(A2.registers, endian=Endian.Little)
        A2_d ={'float':A2_d.decode_32bit_float(),}

        A3_d = decode.fromRegisters(A3.registers, endian=Endian.Little)
        A3_d ={'float':A3_d.decode_32bit_float(),}

#####################################################
	#Voltage Values
	VLL=client.read_holding_registers(3908,2,unit=1) 
	VLN=client.read_holding_registers(3910,2,unit=1)
	V12=client.read_holding_registers(3924,2,unit=1) 
	V23=client.read_holding_registers(3938,2,unit=1) 
	V31=client.read_holding_registers(3952,2,unit=1) 
	V1=client.read_holding_registers(3926,2,unit=1) 
	V2=client.read_holding_registers(3940,2,unit=1) 
	V3=client.read_holding_registers(3954,2,unit=1) 

        VLN_d = decode.fromRegisters(VLN.registers, endian=Endian.Little)
        VLN_d ={'float':VLN_d.decode_32bit_float(),}

        VLL_d = decode.fromRegisters(VLL.registers, endian=Endian.Little)
        VLL_d ={'float':VLL_d.decode_32bit_float(),}

        V12_d = decode.fromRegisters(V12.registers, endian=Endian.Little)
        V12_d ={'float':V12_d.decode_32bit_float(),}

        V23_d = decode.fromRegisters(V23.registers, endian=Endian.Little)
        V23_d ={'float':V23_d.decode_32bit_float(),}

        V31_d = decode.fromRegisters(V31.registers, endian=Endian.Little)
        V31_d ={'float':V31_d.decode_32bit_float(),}

        V1_d = decode.fromRegisters(V1.registers, endian=Endian.Little)
        V1_d ={'float':V1_d.decode_32bit_float(),}

        V2_d = decode.fromRegisters(V2.registers, endian=Endian.Little)
        V2_d ={'float':V2_d.decode_32bit_float(),}

        V3_d = decode.fromRegisters(V3.registers, endian=Endian.Little)
        V3_d ={'float':V3_d.decode_32bit_float(),}

######################################################
	#Power Values 
	#NOTE: EM6436 does not give VAR Values!!!

	W=client.read_holding_registers(3902,2,unit=1) 
	W1=client.read_holding_registers(3918,2,unit=1)  
	W2=client.read_holding_registers(3932,2,unit=1)  
	W3=client.read_holding_registers(3946,2,unit=1)
	VA=client.read_holding_registers(3900,2,unit=1)
	VA1=client.read_holding_registers(3916,2,unit=1) 
	VA2=client.read_holding_registers(3930,2,unit=1) 
	VA3=client.read_holding_registers(3944,2,unit=1) 

        W_d = decode.fromRegisters(W.registers, endian=Endian.Little)
        W_d ={'float':W_d.decode_32bit_float(),}

        W1_d = decode.fromRegisters(W1.registers, endian=Endian.Little)
        W1_d ={'float':W1_d.decode_32bit_float(),}

        W2_d = decode.fromRegisters(W2.registers, endian=Endian.Little)
        W2_d ={'float':W2_d.decode_32bit_float(),}

        W3_d = decode.fromRegisters(W3.registers, endian=Endian.Little)
        W3_d ={'float':W3_d.decode_32bit_float(),}

        VA_d = decode.fromRegisters(VA.registers, endian=Endian.Little)
        VA_d ={'float':VA_d.decode_32bit_float(),}

        VA1_d = decode.fromRegisters(VA1.registers, endian=Endian.Little)
        VA1_d ={'float':VA1_d.decode_32bit_float(),}

        VA2_d = decode.fromRegisters(VA2.registers, endian=Endian.Little)
        VA2_d ={'float':VA2_d.decode_32bit_float(),}

        VA3_d = decode.fromRegisters(VA3.registers, endian=Endian.Little)
        VA3_d ={'float':VA3_d.decode_32bit_float(),}

######################################################
	#Power Factor Values
	PF=client.read_holding_registers(3906,2,unit=1)
	PF1=client.read_holding_registers(3922,2,unit=1) 
	PF2=client.read_holding_registers(3936,2,unit=1) 
	PF3=client.read_holding_registers(3950,2,unit=1) 

        PF_d = decode.fromRegisters(PF.registers, endian=Endian.Little)
        PF_d ={'float':PF_d.decode_32bit_float(),}

        PF1_d = decode.fromRegisters(PF1.registers, endian=Endian.Little)
        PF1_d ={'float':PF1_d.decode_32bit_float(),}

        PF2_d = decode.fromRegisters(PF2.registers, endian=Endian.Little)
        PF2_d ={'float':PF2_d.decode_32bit_float(),}

        PF3_d = decode.fromRegisters(PF3.registers, endian=Endian.Little)
        PF3_d ={'float':PF3_d.decode_32bit_float(),}

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

        for i, value in A1_d.iteritems():
                print value
		A1=value

        for i, value in A2_d.iteritems():
                print value
		A2=value

        for i, value in A3_d.iteritems():
                print value
		A3=value
	A_RMS=[A,A1,A2,A3]
	
	print "-" * 100
	print "Voltage Values"

        for i, value in VLL_d.iteritems():
                print value
		VLL=value

        for i, value in VLN_d.iteritems():
                print value
		VLN=value

        for i, value in V12_d.iteritems():
                print value
		V12=value

        for i, value in V23_d.iteritems():
                print value
		V23=value

        for i, value in V31_d.iteritems():
                print value
		V31=value

        for i, value in V1_d.iteritems():
                print value
		V1=value

        for i, value in V2_d.iteritems():
                print value
		V2=value

        for i, value in V3_d.iteritems():
                print value
		V3=value
        V_RMS=[VLL,VLN,V12,V23,V31,V1,V2,V3]
	print "-" * 100
	print "Power Factor Values"

        for i, value in PF_d.iteritems():
                print value
                PF=value
        for i, value in PF1_d.iteritems():
                print value
                PF1=value
        for i, value in PF2_d.iteritems():
                print value
                PF2=value
        for i, value in PF3_d.iteritems():
                print value
                PF3=value
        PF_RMS=[PF,PF1,PF2,PF3]
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

        for i, value in W1_d.iteritems():
                print value
                W1=value

        for i, value in W2_d.iteritems():
                print value
                W2=value

        for i, value in W3_d.iteritems():
                print value
                W3=value
        W_RMS=[W,W1,W2,W3]
        for i, value in VA_d.iteritems():
                print value
                VA=value

        for i, value in VA1_d.iteritems():
                print value
                VA1=value

        for i, value in VA2_d.iteritems():
                print value
                VA2=value

        for i, value in VA3_d.iteritems():
                print value
                VA3=value

        VA_RMS=[VA,VA1,VA2,VA3]
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

