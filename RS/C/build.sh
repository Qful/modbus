#!/bin/sh

# Non raspberry build
#cc -o driver driver.c -std=gnu99 -Wno-declaration-after-statement  -v -I /usr/local/lib

cc -o driver driver.c -std=gnu99 -Wno-declaration-after-statement -v -I /usr/local/lib -lwiringPi -DRPI
