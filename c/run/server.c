
#include <stdio.h>

#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <modbus.h>

#if defined(_WIN32)
#define close closesocket
#endif

#ifndef _MSC_VER
#include <unistd.h>
#endif

int main(int argc, char *argv[])
{
    int s = -1;
    modbus_t *ctx = NULL;
    modbus_mapping_t *mb_mapping = NULL;
    int rc;
    int use_backend;

    ctx = modbus_new_rtu("/dev/ttyUSB0", 4800, 'N', 8, 1);
    modbus_set_slave(ctx, 1);
    modbus_connect(ctx);
    mb_mapping = modbus_mapping_new(MODBUS_MAX_READ_BITS, 0, MODBUS_MAX_READ_REGISTERS, 0);
	
    if (mb_mapping == NULL) 
	{
        fprintf(stderr, "Failed to allocate the mapping: %s\n", modbus_strerror(errno));
        modbus_free(ctx);
        return -1;
    }
    printf("qitas ready to echo \n\n");
    for(;;) 
	{
        uint8_t query[MODBUS_TCP_MAX_ADU_LENGTH];
        rc = modbus_receive(ctx, query);
        if (rc > 0) {
            modbus_reply(ctx, query, rc, mb_mapping);
        }
		else if(rc  == -1){
			printf("modbus receive error\n\n");
            break;
        }
    }
    printf("Quit the loop: %s\n", modbus_strerror(errno));
    modbus_mapping_free(mb_mapping);
    if(s != -1) {
        close(s);
    }
    /* For RTU, skipped by TCP (no TCP connect) */
    modbus_close(ctx);
    modbus_free(ctx);

    return 0;
}
