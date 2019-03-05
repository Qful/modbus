#ifndef  __MB_POLL_PORT_SERIAL_H__
#define  __MB_POLL_PORT_SERIAL_H__



#define  MB_POLL_PORT_SERIAL_BAUDRATE         115200



void mb_poll_port_serial_init();
void mb_poll_port_serial_enable(mb_poll_bool_t rx_bool,mb_poll_bool_t tx_bool);
/*串口接收一个字节*/
void mb_poll_port_serial_send_byte(uint8_t send_byte);
/*串口发送一个字节*/
void mb_poll_port_serial_get_byte(uint8_t *ptr_byte);

/*串口中断处理*/
void mb_poll_port_serial_isr(void);




#endif