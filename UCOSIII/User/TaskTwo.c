#include  <includes.h>






/*
*******************************************************************************
**************************
*                                            App_TaskTwo()
*
* Description : 
*
* Argument(s) : p_arg       Argument passed to 'App_TaskONE()' by '
OSTaskCreate()'.
*
* Return(s)   : none.
*
* Caller(s)   : This is a task.
*
* Note(s)     : none.
*******************************************************************************
**************************
*/

void  AppTaskTwo(void *p_arg)
{
    OS_ERR err;
        
    while(1)
    { 
        
        
          GPIO_SetBits(GPIOC, GPIO_Pin_1);  
          OSTimeDlyHMSM(0, 0, 0, 500,OS_OPT_TIME_HMSM_STRICT,&err);
          GPIO_ResetBits(GPIOC, GPIO_Pin_1);  
          OSTimeDlyHMSM(0, 0, 0, 500,OS_OPT_TIME_HMSM_STRICT,&err);
        
    }
}

