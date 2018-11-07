#!/bin/bash
./gen.sh && ./configure
make && sudo make install 
sudo ldconfig

# python install 
sudo apt install python-dev -y
pip install pylibmodbus
pip3 install pylibmodbus