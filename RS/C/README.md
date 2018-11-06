# pi-rs485-driver-modbus-ascii

Driver for innovert 3-phase engine controller

Tested on [ISD222M21B](https://www.prst.ru/preobrazovatel/innovert/)

Special build for Raspberry Pi, using onboard serial and software RTS flow-control

Install
-----------------
### Raspberry-Pi

Get dependencies
```{r, engine='bash'}
sudo apt-get install gcc wiringpi
```

Build
```{r, engine='bash'}
./build.sh
```

Usage samples
-----------------

```{r, engine='bash'}
# Start engine (enable)
./driver -e
# Stop engine (disable)
./driver -d
# Set 40 Hz frequency
./driver -f 40

# Start second controller (id 2)
./driver -e -i 2

# Use another uart
./driver -e -s /dev/ttyAMA0
```

Sample device
-----------------

![my device](https://github.com/ramzes642/pi-rs485-driver-modbus-ascii/raw/master/sample-device.JPG)

Wiring
-----------------
Use chip [ADM1485JNZ](https://www.chipdip.ru/product/adm1485jn) or similar

```
TX(8 pin) to DI
RX(10 pin) to RO
RTS(11 pin) to RE/DE (connect them together on chip)
VCC to raspberry's 5V, GND
```

A/B then can be connected to engine controller RS+/RS-

Controller setup
-----------------
```
PH00 = 1 (9600 spd)
PH01 = 0 (8N1 ASCII)
PH02 = 1 (controller id)
Pb01 = 5 (frequency setup via rs485)
Pb02 = 2 (start/stop via rs485)
```
