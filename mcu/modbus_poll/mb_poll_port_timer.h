#ifndef  __MB_POLL_PORT_TIMER_H__
#define  __MB_POLL_PORT_TIMER_H__

/*如果波特率大于19200 t3.5 最小等于3ms*/
#define  MB_POLL_PORT_TIMER_35_TIMEOUT         3


void mb_poll_port_timer_init();
void mb_poll_port_timer_35_start();
void mb_poll_port_timer_response_start();
void mb_poll_port_timer_stop();








#endif