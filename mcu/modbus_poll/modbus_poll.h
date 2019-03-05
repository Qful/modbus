#ifndef  __MODBUS_POLL_H__
#define  __MODBUS_POLL_H__

#define  MODBUS_POLL_SEND_TIMEOUT               5
#define  MODBUS_POLL_RESPONSE_TIMEOUT           1000
typedef enum
{
MODBUS_POLL_STATUS_ILLEGAL_FUNCTION        =    0x01,             
MODBUS_POLL_STATUS_ILLEGAL_DATA_ADDR       =    0x02,             
MODBUS_POLL_STATUS_ILLEGAL_DATA_VALUE      =    0x03,             
MODBUS_POLL_STATUS_SLAVE_DEVICE_FAILURE    =    0x04,             
MODBUS_POLL_STATUS_SUCCESS                 =    0x00,             
MODBUS_POLL_STATUS_INVALID_SLAVE_ID        =    0xE0,             
MODBUS_POLL_STATUS_INVALID_FUNCTION        =    0xE1,
MODBUS_POLL_STATUS_RESPONSE_TIMEOUT        =    0xE2,             
MODBUS_POLL_STATUS_INVALID_CRC             =    0xE3,
MODBUS_POLL_STATUS_SEND_TIMEOUT            =    0xE4,
MODBUS_POLL_STATUS_ERROR                   =    0xE5
}mb_poll_status_t;


typedef enum
{
  MB_POLL_TRUE=1,
  MB_POLL_FALSE=0
}mb_poll_bool_t;

#define  MB_POLL_SEND_OVER_EVT           (1<<0)
#define  MB_POLL_RESPONSE_EVT            (1<<1)
#define  MB_POLL_ALL_EVENTS              ((1<<2)-1)

void modbus_poll_init();
/*数据发送之前操作*/
void modbus_poll_pre_transmission(void (*preTransmission)());
/*数据发送之后操作*/
void modbus_poll_post_transmission(void (*postTransmission)());
/*功能码0x01 读取线圈状态*/
mb_poll_status_t modbus_poll_readCoils(uint8_t slave,uint16_t addr, uint16_t cnt,uint16_t *ptr_read_buff);
/*功能码0x02 读离散量输入*/
mb_poll_status_t modbus_poll_read_discrete_inputs(uint8_t slave,uint16_t addr,uint16_t cnt,uint16_t *ptr_read_buff);
/*功能码0x03 读保持寄存器*/
mb_poll_status_t modbus_poll_read_holding_registers(uint8_t slave,uint16_t addr,uint16_t cnt,uint16_t *ptr_read_buff);
/*功能码0x04 读输入寄存器*/
mb_poll_status_t modbus_poll_read_input_registers(uint8_t slave,uint16_t addr,uint8_t cnt,uint16_t *ptr_read_buff);
/*功能码0x05 写单个线圈*/
mb_poll_status_t modbus_poll_write_single_coil(uint8_t slave,uint16_t addr, uint8_t state);
/*功能码0x06 写单个寄存器 */
mb_poll_status_t modbus_poll_write_single_register(uint8_t slave,uint16_t addr,uint16_t value);
/*功能码0x0F 写多个线圈*/
mb_poll_status_t modbus_poll_write_multiple_coils(uint8_t slave,uint16_t addr,uint16_t cnt,uint16_t *ptr_write_buff);
/*功能码0x10 写多个寄存器*/
mb_poll_status_t modbus_poll_write_multiple_registers(uint8_t slave,uint16_t addr,uint16_t cnt,uint16_t *ptr_write_buff);
/*功能码0x16 屏蔽写寄存器*/
mb_poll_status_t modbus_poll_mask_write_register(uint8_t slave,uint16_t addr,uint16_t and_mask, uint16_t or_mask);
/*功能码0x17 读/写多个寄存器*/
mb_poll_status_t modbus_poll_read_write_multiple_registers(uint8_t slave,uint16_t r_addr,uint16_t r_cnt,uint16_t *ptr_read_buff,
                                                           uint16_t w_addr, uint16_t w_cnt,uint16_t *ptr_write_buff);




void mb_poll_byte_send(void);
void mb_poll_byte_receive(void);
void mb_poll_timer_35_expired();
void mb_poll_timer_response_expired();






#endif