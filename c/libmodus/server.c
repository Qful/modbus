
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


#define LOOP             1
#define SERVER_ID        1
#define ADDRESS_START    0
#define ADDRESS_END      9

uint16_t *tab_rp_registers;

int main(int argc, char *argv[])
{
    int s = -1;
	int nb;
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
	nb = ADDRESS_END - ADDRESS_START;
	tab_rp_registers = (uint16_t *) malloc(nb * sizeof(uint16_t));
    memset(tab_rp_registers, 0, nb * sizeof(uint16_t));
    for(;;) 
	{
/*         uint8_t query[MODBUS_TCP_MAX_ADU_LENGTH];
        rc = modbus_receive(ctx, query);
        if (rc > 0) 
		{
            modbus_reply(ctx, query, rc, mb_mapping);
        }
		else if(rc  == -1){
			printf("modbus receive error\n\n");
            break;
        } */
		
		rc = modbus_read_registers(ctx, ADDRESS_START, nb, tab_rp_registers);
		if (rc != nb) {
			printf("ERROR modbus_read_registers (%d)\n", rc);
			printf("Address = %d, nb = %d\n", ADDRESS_START, nb);
		} else {
/* 			if (tab_rw_rq_registers[i] != tab_rp_registers[i]) {
				printf("Address = %d, value %d (0x%X) != %d (0x%X)\n",
					   ADDRESS_START, tab_rw_rq_registers[i], tab_rw_rq_registers[i],
					   tab_rp_registers[i], tab_rp_registers[i]);
			} */
			printf("value 1= %d \n",tab_rp_registers[0]);
		}
    }
    printf("Quit the loop: %s\n", modbus_strerror(errno));
    modbus_mapping_free(mb_mapping);
	free(tab_rp_registers);
    if(s != -1) {
        close(s);
    }
    modbus_close(ctx);
    modbus_free(ctx);

    return 0;
}
