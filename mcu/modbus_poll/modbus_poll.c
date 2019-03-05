/**
@file
St library for communicating with modbus slaves over RS232/485 (via RTU protocol).
*/

#include "FreeRTOS.h"
#include "task.h"
#include "cmsis_os.h"
#include "event_groups.h"
#include "modbus_poll.h"
#include "mb_poll_port_serial.h"
#include "mb_poll_port_timer.h"
#include "crc16.h"
#include "app_util.h"
#include "app_error.h"
#define APP_LOG_MODULE_NAME   "[mb_poll]"
#define APP_LOG_MODULE_LEVEL   APP_LOG_LEVEL_DEBUG    
#include "app_log.h"




#define  MODBUS_POLL_FUNC_CODE_READ_COILS                  0x01   
#define  MODBUS_POLL_FUNC_CODE_READ_DISCRETE_INPUTS        0x02   
#define  MODBUS_POLL_FUNC_CODE_WRITE_SINGLE_COIL           0x05  
#define  MODBUS_POLL_FUNC_CODE_WRITE_MULTIPLE_COILS        0x0F   

#define  MODBUS_POLL_FUNC_CODE_READ_HOLDING_REGISTERS      0x03    
#define  MODBUS_POLL_FUNC_CODE_READ_INPUT_REGISTERS        0x04 
#define  MODBUS_POLL_FUNC_CODE_WRITE_SINGLE_REGISTER       0x06    
#define  MODBUS_POLL_FUNC_CODE_WRITE_MULTIPLEREGISTERS     0x10        
#define  MODBUS_POLL_FUNC_CODE_MASK_WRITE_REGISTER         0x16    
#define  MODBUS_POLL_FUNC_CODE_RW_MULTIPLE_REGISTERS       0x17     

/*协议帧中响应或者传输最大的数据字节数*/
#define  MAX_ADU_BUFF_SIZE                                 96

osMutexId mb_poll_mutex_id;
EventGroupHandle_t mb_poll_events_group;

static   volatile uint8_t modbus_adu[MAX_ADU_BUFF_SIZE];
static   volatile uint8_t modbus_adu_size = 0;

static uint8_t  slave_addr;   
static uint8_t  func_code;
static uint16_t read_addr;   
static uint16_t read_cnt; 
static uint16_t *ptr_response_buff;   
static uint16_t write_addr;   
static uint16_t write_cnt;   
static uint16_t *ptr_transmit_buff;  

static void (*ptr_modbus_poll_pre_transmission)();
static void (*ptr_modbus_poll_post_transmission)();

static void modbus_poll_take_mutex();
static void modbus_poll_release_mutex();
static mb_poll_status_t modbus_poll();

/*modbus poll 初始化*/
void modbus_poll_init()
{
 /*默认无操作*/
 ptr_modbus_poll_pre_transmission = 0;
 ptr_modbus_poll_post_transmission = 0;
 /*创建互斥体*/
 osMutexDef(mb_poll_mutex);
 mb_poll_mutex_id=osMutexCreate(osMutex(mb_poll_mutex)); 
 APP_ASSERT(mb_poll_mutex_id);
 /*创建事件组*/
 mb_poll_events_group = xEventGroupCreate();
 APP_ASSERT(mb_poll_events_group);
 /*串口和定时器初始化*/
 mb_poll_port_serial_init();
 mb_poll_port_timer_init();
}

/*数据发送之前操作*/
void modbus_poll_pre_transmission(void (*preTransmission)())
{
  ptr_modbus_poll_pre_transmission = preTransmission;
}

/*数据发送之后操作*/
void modbus_poll_post_transmission(void (*postTransmission)())
{
  ptr_modbus_poll_post_transmission = postTransmission;
}

/*功能码0x01读取线圈状态*/
mb_poll_status_t modbus_poll_readCoils(uint8_t slave,uint16_t addr, uint16_t cnt,uint16_t *ptr_read_buff)
{
  mb_poll_status_t status;
  APP_ASSERT(ptr_read_buff);
  modbus_poll_take_mutex();
  slave_addr=slave;
  func_code=MODBUS_POLL_FUNC_CODE_READ_COILS;
  read_addr = addr;
  read_cnt = cnt;
  ptr_response_buff=ptr_read_buff;
  status= modbus_poll();
  modbus_poll_release_mutex();
  return status;
}


/*功能码0x02 读离散量输入*/
mb_poll_status_t modbus_poll_read_discrete_inputs(uint8_t slave,uint16_t addr,uint16_t cnt,uint16_t *ptr_read_buff)
{
  mb_poll_status_t status;
  APP_ASSERT(ptr_read_buff);
  modbus_poll_take_mutex();
  slave_addr=slave;
  func_code=MODBUS_POLL_FUNC_CODE_READ_DISCRETE_INPUTS;
  read_addr = addr;
  read_cnt = cnt;
  ptr_response_buff=ptr_read_buff;
  status= modbus_poll();
  modbus_poll_release_mutex();
  return status;
}


/*功能码0x03 读保持寄存器*/
mb_poll_status_t modbus_poll_read_holding_registers(uint8_t slave,uint16_t addr,uint16_t cnt,uint16_t *ptr_read_buff)
{
  mb_poll_status_t status;
  APP_ASSERT(ptr_read_buff);
  modbus_poll_take_mutex();
  slave_addr=slave;
  func_code=MODBUS_POLL_FUNC_CODE_READ_HOLDING_REGISTERS;
  read_addr = addr;
  read_cnt = cnt;
  ptr_response_buff=ptr_read_buff;
  status= modbus_poll();
  modbus_poll_release_mutex();
  return status;
}


/*功能码0x04 读输入寄存器*/
mb_poll_status_t modbus_poll_read_input_registers(uint8_t slave,uint16_t addr,uint8_t cnt,uint16_t *ptr_read_buff)
{
  mb_poll_status_t status;
  APP_ASSERT(ptr_read_buff);
  modbus_poll_take_mutex();
  slave_addr=slave;
  func_code=MODBUS_POLL_FUNC_CODE_READ_INPUT_REGISTERS;
  read_addr = addr;
  read_cnt = cnt;
  ptr_response_buff=ptr_read_buff;
  status= modbus_poll();
  modbus_poll_release_mutex();
  return status;
}


/*功能码0x05 写单个线圈*/
mb_poll_status_t modbus_poll_write_single_coil(uint8_t slave,uint16_t addr, uint8_t state)
{
  mb_poll_status_t status;
  modbus_poll_take_mutex();
  slave_addr=slave;
  func_code=MODBUS_POLL_FUNC_CODE_WRITE_SINGLE_COIL;
  write_addr = addr;
  write_cnt = (state ? 0xFF00 : 0x0000);
  status= modbus_poll();
  modbus_poll_release_mutex();
  return status;
}


/*功能码0x06 写单个寄存器*/
mb_poll_status_t modbus_poll_write_single_register(uint8_t slave,uint16_t addr,uint16_t value)
{
  mb_poll_status_t status;
  modbus_poll_take_mutex();
  slave_addr=slave;
  func_code=MODBUS_POLL_FUNC_CODE_WRITE_SINGLE_REGISTER;
  write_addr = addr;
  write_cnt = 0;
  ptr_transmit_buff = &value;
  status= modbus_poll();
  modbus_poll_release_mutex();
  return status;
}


/*功能码0x0F 写多个线圈*/
mb_poll_status_t modbus_poll_write_multiple_coils(uint8_t slave,uint16_t addr,uint16_t cnt,uint16_t *ptr_write_buff)
{
  mb_poll_status_t status;
  APP_ASSERT(ptr_write_buff);
  modbus_poll_take_mutex();
  slave_addr=slave;
  func_code=MODBUS_POLL_FUNC_CODE_WRITE_MULTIPLE_COILS;
  write_addr = addr;
  write_cnt = cnt;
  ptr_transmit_buff=ptr_write_buff;
  status= modbus_poll();
  modbus_poll_release_mutex();
  return status;
}


/*功能码0x10 写多个寄存器*/
mb_poll_status_t modbus_poll_write_multiple_registers(uint8_t slave,uint16_t addr,uint16_t cnt,uint16_t *ptr_write_buff)
{  
  mb_poll_status_t status;
  APP_ASSERT(ptr_write_buff);
  modbus_poll_take_mutex();
  slave_addr=slave;
  func_code=MODBUS_POLL_FUNC_CODE_WRITE_MULTIPLEREGISTERS;
  write_addr = addr;
  write_cnt = cnt;
  ptr_transmit_buff=ptr_write_buff;
  status= modbus_poll();
  modbus_poll_release_mutex();
  return status;
}



/*功能码0x16 屏蔽写寄存器*/
mb_poll_status_t modbus_poll_mask_write_register(uint8_t slave,uint16_t addr,uint16_t and_mask, uint16_t or_mask)
{
  mb_poll_status_t status;
  uint16_t write_buff[2];
  modbus_poll_take_mutex();
  slave_addr=slave;
  func_code=MODBUS_POLL_FUNC_CODE_MASK_WRITE_REGISTER;
  write_addr = addr;
  write_buff[0] = and_mask;
  write_buff[1] = or_mask;
  ptr_transmit_buff=write_buff;
  status= modbus_poll();
  modbus_poll_release_mutex();
  return status;
}

/*功能码0x17 读/写多个寄存器*/
mb_poll_status_t modbus_poll_read_write_multiple_registers(uint8_t slave,uint16_t r_addr,uint16_t r_cnt,uint16_t *ptr_read_buff,
                                                           uint16_t w_addr, uint16_t w_cnt,uint16_t *ptr_write_buff)
{
  mb_poll_status_t status;
  APP_ASSERT(ptr_read_buff);
  APP_ASSERT(ptr_write_buff);
  modbus_poll_take_mutex();
  slave_addr=slave;
  func_code=MODBUS_POLL_FUNC_CODE_RW_MULTIPLE_REGISTERS;
  read_addr = r_addr;
  read_cnt = r_cnt;
  write_addr = w_addr;
  write_cnt = w_cnt;
  ptr_response_buff=ptr_read_buff;
  ptr_transmit_buff=ptr_write_buff;
  status= modbus_poll();
  modbus_poll_release_mutex();
  return status;
}

/*获取互斥体*/
static void modbus_poll_take_mutex()
{
 if(osMutexWait(mb_poll_mutex_id,osWaitForever)!=osOK)
 {
  APP_ERROR_HANDLER(0);
 }
}

/*释放互斥体*/
static void modbus_poll_release_mutex()
{
 if(osMutexRelease(mb_poll_mutex_id)!=osOK)
 {
  APP_ERROR_HANDLER(0);
 }
}

/*数据发送和回应处理*/
static mb_poll_status_t modbus_poll()
{
  uint8_t  i, cnt;
  uint16_t crc;
  mb_poll_status_t status = MODBUS_POLL_STATUS_SUCCESS;
 
  modbus_adu[modbus_adu_size++] = slave_addr;
  modbus_adu[modbus_adu_size++] = func_code;
  
  switch(func_code)
  {
    case MODBUS_POLL_FUNC_CODE_READ_COILS:
    case MODBUS_POLL_FUNC_CODE_READ_DISCRETE_INPUTS:
    case MODBUS_POLL_FUNC_CODE_READ_INPUT_REGISTERS:
    case MODBUS_POLL_FUNC_CODE_READ_HOLDING_REGISTERS:
    case MODBUS_POLL_FUNC_CODE_RW_MULTIPLE_REGISTERS:
      modbus_adu[modbus_adu_size++] = HIGH_8BITS_OF_16(read_addr);
      modbus_adu[modbus_adu_size++] = LOW_8BITS_OF_16(read_addr);
      modbus_adu[modbus_adu_size++] = HIGH_8BITS_OF_16(read_cnt);
      modbus_adu[modbus_adu_size++] = LOW_8BITS_OF_16(read_cnt);
      break;
  }
  
  switch(func_code)
  {
    case MODBUS_POLL_FUNC_CODE_WRITE_SINGLE_COIL:
    case MODBUS_POLL_FUNC_CODE_MASK_WRITE_REGISTER:
    case MODBUS_POLL_FUNC_CODE_WRITE_MULTIPLE_COILS:
    case MODBUS_POLL_FUNC_CODE_WRITE_SINGLE_REGISTER:
    case MODBUS_POLL_FUNC_CODE_WRITE_MULTIPLEREGISTERS:
    case MODBUS_POLL_FUNC_CODE_RW_MULTIPLE_REGISTERS:
      modbus_adu[modbus_adu_size++] = HIGH_8BITS_OF_16(write_addr);
      modbus_adu[modbus_adu_size++] = LOW_8BITS_OF_16(write_addr);
      break;
  }
  
  switch(func_code)
  {
    case MODBUS_POLL_FUNC_CODE_WRITE_SINGLE_COIL:
      modbus_adu[modbus_adu_size++] = HIGH_8BITS_OF_16(write_cnt);
      modbus_adu[modbus_adu_size++] = LOW_8BITS_OF_16(write_cnt);
      break;
      
    case MODBUS_POLL_FUNC_CODE_WRITE_SINGLE_REGISTER:
      modbus_adu[modbus_adu_size++] = HIGH_8BITS_OF_16(ptr_transmit_buff[0]);
      modbus_adu[modbus_adu_size++] = LOW_8BITS_OF_16(ptr_transmit_buff[0]);
      break;
      
    case MODBUS_POLL_FUNC_CODE_WRITE_MULTIPLE_COILS:
      modbus_adu[modbus_adu_size++] = HIGH_8BITS_OF_16(write_cnt);
      modbus_adu[modbus_adu_size++] = LOW_8BITS_OF_16(write_cnt);
      cnt = (write_cnt % 8) ? ((write_cnt >> 3) + 1) : (write_cnt >> 3);
      modbus_adu[modbus_adu_size++] = cnt;
      for (i = 0; i < cnt; i++)
      {
        switch(i % 2)
        {
          case 0: // i is even
            modbus_adu[modbus_adu_size++] = LOW_8BITS_OF_16(ptr_transmit_buff[i >> 1]);
            break;
            
          case 1: // i is odd
            modbus_adu[modbus_adu_size++] = HIGH_8BITS_OF_16(ptr_transmit_buff[i >> 1]);
            break;
        }
      }
      break;
      
    case MODBUS_POLL_FUNC_CODE_WRITE_MULTIPLEREGISTERS:
    case MODBUS_POLL_FUNC_CODE_RW_MULTIPLE_REGISTERS:
      modbus_adu[modbus_adu_size++] = HIGH_8BITS_OF_16(write_cnt);
      modbus_adu[modbus_adu_size++] = LOW_8BITS_OF_16(write_cnt);
      modbus_adu[modbus_adu_size++] = LOW_8BITS_OF_16(write_cnt << 1);
      
      for (i = 0; i < LOW_8BITS_OF_16(write_cnt); i++)
      {
        modbus_adu[modbus_adu_size++] = HIGH_8BITS_OF_16(ptr_transmit_buff[i]);
        modbus_adu[modbus_adu_size++] = LOW_8BITS_OF_16(ptr_transmit_buff[i]);
      }
      break;
      
    case MODBUS_POLL_FUNC_CODE_MASK_WRITE_REGISTER:
      modbus_adu[modbus_adu_size++] = HIGH_8BITS_OF_16(ptr_transmit_buff[0]);
      modbus_adu[modbus_adu_size++] = LOW_8BITS_OF_16(ptr_transmit_buff[0]);
      modbus_adu[modbus_adu_size++] = HIGH_8BITS_OF_16(ptr_transmit_buff[1]);
      modbus_adu[modbus_adu_size++] = LOW_8BITS_OF_16(ptr_transmit_buff[1]);
      break;
  }
  
  // append CRC
  crc = 0xFFFF;
  for (i = 0; i < modbus_adu_size; i++)
  {
    crc = crc16_update(crc, modbus_adu[i]);
  }
  modbus_adu[modbus_adu_size++] = LOW_8BITS_OF_16(crc);
  modbus_adu[modbus_adu_size++] = HIGH_8BITS_OF_16(crc);
  modbus_adu[modbus_adu_size] = 0;

  // transmit request
  if (ptr_modbus_poll_pre_transmission)
  {
    ptr_modbus_poll_pre_transmission();
  }
  
  EventBits_t event;
  /*使能发送*/
  mb_poll_port_serial_enable(MB_POLL_FALSE,MB_POLL_TRUE);

  event = xEventGroupWaitBits(
					mb_poll_events_group,	// The event group being tested.
					MB_POLL_ALL_EVENTS,  	// The bits within the event group to wait for.
					pdTRUE,			        // should be cleared before returning.
					pdFALSE,		        // Don't wait for both bits, either bit will do.
					MODBUS_POLL_SEND_TIMEOUT);	
  
  if(!(event & MB_POLL_SEND_OVER_EVT))
  {
  APP_LOG_ERROR("MB_POLL 发送超时.\r\n");
  /*禁止发送，禁止接收*/
  mb_poll_port_serial_enable(MB_POLL_FALSE,MB_POLL_FALSE);
  status=MODBUS_POLL_STATUS_SEND_TIMEOUT;
  goto modbus_poll_err_handle;
  }
  
  if (ptr_modbus_poll_post_transmission)
  {
    ptr_modbus_poll_post_transmission();
  }
  event=0;
  modbus_adu_size=0;
  /*使能接收*/
  mb_poll_port_serial_enable(MB_POLL_TRUE,MB_POLL_FALSE);
  event = xEventGroupWaitBits(
					mb_poll_events_group,	// The event group being tested.
					MB_POLL_ALL_EVENTS,  	// The bits within the event group to wait for.
					pdTRUE,			        // should be cleared before returning.
					pdFALSE,		        // Don't wait for both bits, either bit will do.
					MODBUS_POLL_RESPONSE_TIMEOUT);	
  
  /*禁止发送，禁止接收*/
  mb_poll_port_serial_enable(MB_POLL_FALSE,MB_POLL_FALSE);
  
  if(!(event & MB_POLL_RESPONSE_EVT))
  {
  APP_LOG_ERROR("MB_POLL 响应超时.\r\n");
  status = MODBUS_POLL_STATUS_RESPONSE_TIMEOUT;
  goto modbus_poll_err_handle; 
  }
  
  // verify response is for correct Modbus slave
  if(modbus_adu[0] != slave_addr)
  {
   APP_LOG_ERROR("MB_POLL 响应为无效SLAVE.\r\n");
   status = MODBUS_POLL_STATUS_INVALID_SLAVE_ID;
   goto modbus_poll_err_handle; 
  }
      
  // verify response is for correct Modbus function code (mask exception bit 7)
  if ((modbus_adu[1] & 0x7F) != func_code)
  {
    APP_LOG_ERROR("MB_POLL 响应为无效功能码.\r\n");
   status = MODBUS_POLL_STATUS_INVALID_FUNCTION;
   goto modbus_poll_err_handle; 
  }
      
   // check whether Modbus exception occurred; return Modbus Exception Code
  if(IS_SET(modbus_adu[1], 7))
  {
   APP_LOG_ERROR("MB_POLL 功能异常：%d.\r\n",modbus_adu[2]);
   status = (mb_poll_status_t)modbus_adu[2];
   goto modbus_poll_err_handle; ;
  } 
  // verify response is large enough to inspect further
  if (modbus_adu_size >= 5)
  {
    // calculate CRC
   crc = 0xFFFF;
   for (i = 0; i < (modbus_adu_size - 2); i++)
   {
    crc = crc16_update(crc, modbus_adu[i]);
   }  
   // verify CRC
   if ((LOW_8BITS_OF_16(crc) != modbus_adu[modbus_adu_size - 2] ||
      HIGH_8BITS_OF_16(crc) != modbus_adu[modbus_adu_size - 1]))
   {
    APP_LOG_ERROR("MB_POLL 响应为无效CRC.\r\n");
    status = MODBUS_POLL_STATUS_INVALID_CRC;
    goto modbus_poll_err_handle; ;
   }
  }

  // disassemble ADU into words
  // evaluate returned Modbus function code
  switch(modbus_adu[1])
  {
  case MODBUS_POLL_FUNC_CODE_READ_COILS:
  case MODBUS_POLL_FUNC_CODE_READ_DISCRETE_INPUTS:
  // load bytes into word; response bytes are ordered L, H, L, H, ...
       for (i = 0; i < (modbus_adu[2] >> 1); i++)
       {
        if (i < (read_cnt/16+1))/*16bit为一组*/
        {
        ptr_response_buff[i] = uint16_decode((const uint8_t *)&modbus_adu[2 * i + 3]);
        }         
       }        
       // in the event of an odd number of bytes, load last byte into zero-padded word
       if (modbus_adu[2] % 2)
        {
         if (i < (read_cnt/16 +1))/*16bit为一组*/
         {
          uint8_t buff[2];
          buff[0]=modbus_adu[2 * i + 3];
          buff[1]=0;
          ptr_response_buff[i] = uint16_decode(buff);
         }       
        }
        break;       
  case MODBUS_POLL_FUNC_CODE_READ_INPUT_REGISTERS:
  case MODBUS_POLL_FUNC_CODE_READ_HOLDING_REGISTERS:
  case MODBUS_POLL_FUNC_CODE_RW_MULTIPLE_REGISTERS:
        // load bytes into word; response bytes are ordered H, L, H, L, ...
        for (i = 0; i < (modbus_adu[2] >> 1); i++)
        {
         if (i < read_cnt)
         {
          ptr_response_buff[i] = uint16_big_decode((const uint8_t *)&modbus_adu[2 * i + 3]);
         }      
        }
        break;
    }
modbus_poll_err_handle:
  modbus_adu_size=0;
  return status;
}


void mb_poll_byte_receive(void)
{
 uint8_t recv_byte;
 mb_poll_port_serial_get_byte(&recv_byte);
 if(modbus_adu_size>=MAX_ADU_BUFF_SIZE)
 {
 APP_LOG_ERROR("串口缓存溢出.本次数据无效.\r\n");
 modbus_adu_size=0;
 }
 modbus_adu[modbus_adu_size++]=recv_byte;
 /*重新开始t35定时器*/
 mb_poll_port_timer_35_start();
}

void mb_poll_byte_send(void)
{
static uint8_t send_idx=0;
if(modbus_adu_size!=0)
{
 mb_poll_port_serial_send_byte(*(uint8_t*)&modbus_adu[send_idx++]); 
 modbus_adu_size--;
}
else
{
 send_idx=0;
 /*禁止发送，禁止接收*/
 mb_poll_port_serial_enable(MB_POLL_FALSE,MB_POLL_FALSE); 
 xEventGroupSetBitsFromISR(mb_poll_events_group,MB_POLL_SEND_OVER_EVT,NULL);
}
}

void mb_poll_timer_35_expired()
{
 xEventGroupSetBits(mb_poll_events_group,MB_POLL_RESPONSE_EVT);  
}