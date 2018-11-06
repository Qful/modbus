#include <errno.h>      // Error number definitions
#include <stdint.h>     // C99 fixed data types
#include <stdio.h>      // Standard input/output definitions
#include <stdlib.h>     // C standard library
#include <string.h>     // String function definitions
#include <unistd.h>     // UNIX standard function definitions
#include <fcntl.h>      // File control definitions
#include <termios.h>    // POSIX terminal control definitions
#include <sys/types.h>


#ifdef RPI
#include <wiringPi.h>
#endif

#include <time.h>
#include <stdarg.h>
#include <pthread.h>


// defines RTS pin number (wiring gpio 0, pin 17 PHY)
#define RTS 0


#define SERIAL_SPD B9600

// Time in msec to transfer 1 byte
// 173 - for 57600 (10000000/57600)
// 1040 - for 9600
// etc
#define SERIAL_SPD_1BYTE 1040


char *serial_port = "/dev/ttyS0";
int fd = 0;     // File descriptor



// Open usb-serial port for reading & writing
int open_port(void) {

    int fd;    // File descriptor for the port
    fd = open(serial_port, O_RDWR | O_NOCTTY | O_SYNC);

    if (fd == -1) {
        fprintf(stderr, "open_port: Unable to open %s: %s\n", serial_port, strerror(errno));
        exit(EXIT_FAILURE);
    }

    return fd;
}


unsigned char hexval(unsigned char c)
{
    if ('0' <= c && c <= '9')
        return c - '0';
    else if ('a' <= c && c <= 'f')
        return c - 'a' + 10;
    else if ('A' <= c && c <= 'F')
        return c - 'A' + 10;
    else abort();
}

void say(char *_msg, ...) {
    struct timespec gettime_now;
    long int start_time;
    char *msg = malloc(strlen(_msg)*2);

    va_list args;
    va_start(args, _msg);
    int wb = vsprintf(msg, _msg, args);
    va_end(args);


    unsigned char crc = 0xff;
    for (int i=1; i < wb - 1;i += 2) {
        int d = (hexval(msg[i]) << 4) + hexval(msg[i+1]);
        crc += d;
    }
    crc = (0xff - crc);
//    printf("CRC: (%s %X)\n", msg, crc);

#ifdef RPI
    digitalWrite(RTS, 1);
#endif

    wb = dprintf(fd, "%s%X\r\n", msg, crc);

    // tcdrain not working, using sleep to wait for transmission
    if (wb > 0) {
        usleep(wb * SERIAL_SPD_1BYTE);
    }
#ifdef RPI
    digitalWrite(RTS, 0);
#endif

    printf("\nSaid: %s%X\r\n", msg, crc);

}


#define ST_WAIT 0
#define ST_ADDR 1
#define ST_PID 2
#define ST_RECV 3

void ProcessMessages() {
    char buf[512];
    int buf_pos = 0;


    buf[0] = 0;
    int ppos = 0;
    int state = ST_WAIT;
    int rv;
    char readyFlag;

    long int id = 0;

    fd_set set;
    FD_ZERO(&set); /* clear the set */
    FD_SET(fd, &set); /* add our file descriptor to the set */

    printf("Read response\r\n");
    int timeout = 100;
    while (--timeout > 0) {

        ssize_t inp_size = read(fd, &buf, 512);
        if (inp_size > 0) {
            buf[inp_size] = 0;


            printf("\r\ninp size: %d ", inp_size, buf[0]);
            for (int i=0;i<inp_size;i++) {
                printf("[%d]", buf[i]);
            }
        }

        usleep(100 * 100);


    }
}


int main(int argc, char *argv[]) {
    int opt = 0;
    uint32_t freq = 0;
    int id = 1;

    char action = ' ';
    while ((opt = getopt(argc, argv, "eds:f:i:")) != -1) {
        switch (opt) {
            case 'e':
            case 'd':
                action = opt;
                break;
            case 'f':
                action = opt;
                freq = atoi(optarg);
                break;
            case 'i':
                id = atoi(optarg);
                break;
            case 's':
                serial_port = malloc(strlen(optarg) + 1);
                strcpy(serial_port, optarg);
                printf("Serial=%s\n", serial_port);
                break;
        }
    }


    struct termios options;    // Terminal options
    int rc;         // Return value

    fd = open_port();            // Open tty device for RD and WR

    // Get the current options for the port
    if ((rc = tcgetattr(fd, &options)) < 0) {
        fprintf(stderr, "failed to get attr: %d, %s\n", fd, strerror(errno));
        exit(EXIT_FAILURE);
    }
#ifdef RPI
    printf("RPI version\n");
    wiringPiSetup();
    pinMode(RTS, OUTPUT);
    digitalWrite(RTS, 0);
#endif

    // Set the baud rates
    cfsetispeed(&options, SERIAL_SPD);

    // Set the baud rates
    cfsetospeed(&options, SERIAL_SPD);

    cfmakeraw(&options);
    options.c_cflag |= (CLOCAL | CREAD);    /* ignore modem controls */
    options.c_cflag &= ~CSIZE;
    options.c_cflag |= CS8;         /* 8-bit characters */
    options.c_cflag &= ~PARENB;     /* no parity bit */
    options.c_cflag &= ~CSTOPB;     /* only need 1 stop bit */
    options.c_cflag &= ~CRTSCTS;    /* no hardware flowcontrol (internal RTS doesn't work on RPI 2/3) */

    /* setup for non-canonical mode */
    options.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL | IXON);
    options.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
    options.c_oflag &= ~OPOST;

    /* fetch bytess as they become available */
    options.c_cc[VMIN] = 0;
    options.c_cc[VTIME] = 0;


    // Set the new attributes
    if ((rc = tcsetattr(fd, TCSANOW, &options)) < 0) {
        fprintf(stderr, "failed to set attr: %d, %s\n", fd, strerror(errno));
        exit(EXIT_FAILURE);
    }

    switch (action) {
        case 'e':
            say(":%02X0620000002", id); // start // D7
            break;
        case 'd':
            say(":%02X0620000001", id); // stop // D8
            break;
        case 'f':
            say(":%02X062001%04X", id, freq * 10);
    }



    ProcessMessages();


    // Close file descriptor & exit
    close(fd);

    return EXIT_SUCCESS;
}

