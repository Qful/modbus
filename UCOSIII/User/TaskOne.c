#include  <includes.h>






/*
*******************************************************************************
**************************
*                                            App_TaskONE()
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

void  AppTaskOne(void *p_arg)
{
    OS_ERR err;
    float i = 1.5;
    INT8U j = 0;
    float Temp = 0;
    while(1)
    { 
        Temp = i * j ;
        if(j > 20)
        {
            j = 0;
        }
        if(Temp > 15)
        {
          GPIO_SetBits(GPIOF, GPIO_Pin_10);  
        }
        else
        {
          GPIO_ResetBits(GPIOF, GPIO_Pin_10);  
        }
        OSTimeDlyHMSM(0, 0, 0, 50,OS_OPT_TIME_HMSM_STRICT,&err);
        j++;  
    }
}

