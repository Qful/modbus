#include "FreeRTOS.h"
#include "task.h"
#include "cmsis_os.h"
#include "modbus_poll.h"
#include "mb_poll_port_timer.h"
#define APP_LOG_MODULE_NAME   "[mb_poll_port_timer]"
#define APP_LOG_MODULE_LEVEL   APP_LOG_LEVEL_ERROR    
#include "app_log.h"
#include "app_error.h"


static void mb_poll_port_timer_expired(void const * argument);
osTimerId mb_poll_timer_id;

void mb_poll_port_timer_init()
{
 osTimerDef(host_comm_timer,mb_poll_port_timer_expired);
 mb_poll_timer_id=osTimerCreate(osTimer(host_comm_timer),osTimerOnce,0);
 APP_ASSERT(mb_poll_timer_id);
}

void mb_poll_port_timer_35_start()
{
 APP_LOG_DEBUG("timer35 start.\r\n");
 osTimerStart(mb_poll_timer_id,MB_POLL_PORT_TIMER_35_TIMEOUT);
}

void mb_poll_port_timer_stop()
{
 osTimerStop(mb_poll_timer_id);
}

static void mb_poll_port_timer_expired(void const * argument)
{
 mb_poll_timer_35_expired();
}
