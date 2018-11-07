
gcc client.c  -o client  `pkg-config --cflags --libs libmodbus`
gcc server.c  -o server  `pkg-config --cflags --libs libmodbus`
