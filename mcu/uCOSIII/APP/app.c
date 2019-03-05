/*
*********************************************************************************************************
*                                              EXAMPLE CODE
*
*                             (c) Copyright 2013; Micrium, Inc.; Weston, FL
*
*                   All rights reserved.  Protected by international copyright laws.
*                   Knowledge of the source code may not be used to write a similar
*                   product.  This file may only be used in accordance with a license
*                   and should not be redistributed in any way.
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*
*                                            EXAMPLE CODE
*
*                                     ST Microelectronics STM32
*                                              on the
*
*                                           STM3240G-EVAL
*                                         Evaluation Board
*
* Filename      : app.c
* Version       : V1.00
* Programmer(s) : EHS
*                 DC
*********************************************************************************************************
*/
/*
*********************************************************************************************************
*                                             INCLUDE FILES
*********************************************************************************************************
*/

#include  <app_cfg.h>
#include  <includes.h>

/*
*********************************************************************************************************
*                                            LOCAL DEFINES
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                       LOCAL GLOBAL VARIABLES
*********************************************************************************************************
*/

/* ----------------- APPLICATION GLOBALS ------------------ */
static  OS_TCB   AppTaskStartTCB;
static  OS_TCB   AppTaskMODBUSTCB;

static  OS_TCB   AppTaskOneTCB;
static  OS_TCB   AppTaskTwoTCB;


static  CPU_STK  AppTaskStartStk[APP_CFG_TASK_START_STK_SIZE];
static  CPU_STK  AppTaskMODBUSStk[APP_CFG_TASK_MODBUS_STK_SIZE];

static  CPU_STK  AppTaskOneStk[APP_CFG_TASK_One_STK_SIZE];
static  CPU_STK  AppTaskTwoStk[APP_CFG_TASK_Two_STK_SIZE];

/*
*********************************************************************************************************
*                                         FUNCTION PROTOTYPES
*********************************************************************************************************
*/

static  void  AppTaskStart          (void     *p_arg);
void TaskModbus             (void *parg);

void  AppTaskOne       (void     *p_arg);
void  AppTaskTwo       (void     *p_arg);


static  void  AppTaskCreate         (void);
static  void  AppObjCreate          (void);

/*
*********************************************************************************************************
*                                                main()
*
* Description : This is the standard entry point for C code.  It is assumed that your code will call
*               main() once you have performed all necessary initialization.
*
* Arguments   : none
*
* Returns     : none
*********************************************************************************************************
*/

int main(void)
{
    OS_ERR  err;


    BSP_IntDisAll();                                            /* Disable all interrupts.                              */
    
    OSInit(&err);                                               /* Init uC/OS-III.                                      */

    OSTaskCreate((OS_TCB       *)&AppTaskStartTCB,              /* Create the start task                                */
                 (CPU_CHAR     *)"App Task Start",
                 (OS_TASK_PTR   )AppTaskStart, 
                 (void         *)0,
                 (OS_PRIO       )APP_CFG_TASK_START_PRIO,
                 (CPU_STK      *)&AppTaskStartStk[0],
                 (CPU_STK_SIZE  )AppTaskStartStk[APP_CFG_TASK_START_STK_SIZE / 10],
                 (CPU_STK_SIZE  )APP_CFG_TASK_START_STK_SIZE,
                 (OS_MSG_QTY    )0,
                 (OS_TICK       )0,
                 (void         *)0,
                 (OS_OPT        )(OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
                 (OS_ERR       *)&err);

    OSStart(&err);                                              /* Start multitasking (i.e. give control to uC/OS-III). */
    
    (void)&err;
    
    return (0);
}
/*
*********************************************************************************************************
*                                          STARTUP TASK
*
* Description : This is an example of a startup task.  As mentioned in the book's text, you MUST
*               initialize the ticker only once multitasking has started.
*
* Arguments   : p_arg   is the argument passed to 'AppTaskStart()' by 'OSTaskCreate()'.
*
* Returns     : none
*
* Notes       : 1) The first line of code is used to prevent a compiler warning because 'p_arg' is not
*                  used.  The compiler should not generate any code for this statement.
*********************************************************************************************************
*/
static  void  AppTaskStart (void *p_arg)
{
    OS_ERR      err;


   (void)p_arg;

    BSP_Init();                                                 /* Initialize BSP functions                             */
    CPU_Init();                                                 /* Initialize the uC/CPU services                       */
    
    BSP_Tick_Init();                                            /* Start Tick Initialization                            */
    
    Mem_Init();                                                 /* Initialize Memory Management Module                  */
    Math_Init();                                                /* Initialize Mathematical Module                       */


#if OS_CFG_STAT_TASK_EN > 0u
    OSStatTaskCPUUsageInit(&err);                               /* Compute CPU capacity with no task running            */
#endif

#ifdef CPU_CFG_INT_DIS_MEAS_EN
    CPU_IntDisMeasMaxCurReset();
#endif

      
#if (APP_CFG_SERIAL_EN == DEF_ENABLED)    
    App_SerialInit();                                           /* Initialize Serial Communication                      */
#endif

    APP_TRACE_DBG(("Creating Application kernel objects\n\r"));
    AppObjCreate();                                             /* Create Applicaiton kernel objects                    */
    
    APP_TRACE_DBG(("Creating Application Tasks\n\r"));
    AppTaskCreate();                                            /* Create Application tasks                             */

    while (DEF_TRUE) {                                          /* Task body, always written as an infinite loop.       */
       // BSP_LED_Toggle(0);
        OSTimeDlyHMSM(0, 0, 0, 100, 
                      OS_OPT_TIME_HMSM_STRICT, 
                      &err);
    }
}
/*
*********************************************************************************************************
*                                          AppTaskCreate()
*
* Description : Create application tasks.
*
* Argument(s) : none
*
* Return(s)   : none
*
* Caller(s)   : AppTaskStart()
*
* Note(s)     : none.
*********************************************************************************************************
*/

static  void  AppTaskCreate (void)
{
   OS_ERR   err;
   OSTaskCreate((OS_TCB       *)&AppTaskOneTCB,              /* Create the task One                               */
                 (CPU_CHAR     *)"App One Start",
                 (OS_TASK_PTR   )AppTaskOne, 
                 (void         *)0,
                 (OS_PRIO       )APP_CFG_TASK_One_PRIO,
                 (CPU_STK      *)&AppTaskOneStk[0],
                 (CPU_STK_SIZE  )AppTaskOneStk[APP_CFG_TASK_One_STK_SIZE / 10],
                 (CPU_STK_SIZE  )APP_CFG_TASK_One_STK_SIZE,
                 (OS_MSG_QTY    )0,
                 (OS_TICK       )0,
                 (void         *)0,
                 (OS_OPT        )(OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
                 (OS_ERR       *)&err);
   OSTaskCreate((OS_TCB       *)&AppTaskTwoTCB,              /* Create the task Two                               */
                 (CPU_CHAR     *)"App Two Start",
                 (OS_TASK_PTR   )AppTaskTwo, 
                 (void         *)0,
                 (OS_PRIO       )APP_CFG_TASK_Two_PRIO,
                 (CPU_STK      *)&AppTaskTwoStk[0],
                 (CPU_STK_SIZE  )AppTaskTwoStk[APP_CFG_TASK_Two_STK_SIZE / 10],
                 (CPU_STK_SIZE  )APP_CFG_TASK_Two_STK_SIZE,
                 (OS_MSG_QTY    )0,
                 (OS_TICK       )0,
                 (void         *)0,
                 (OS_OPT        )(OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
                 (OS_ERR       *)&err);
                 OSTaskCreate((OS_TCB       *)&AppTaskMODBUSTCB,/* Create the task MODBUS                               */
                 (CPU_CHAR     *)"App TASK MODBUS Start",
                 (OS_TASK_PTR   )TaskModbus, 
                 (void         *)0,
                 (OS_PRIO       )APP_CFG_TASK_MODBUS_PRIO,
                 (CPU_STK      *)&AppTaskMODBUSStk[0],
                 (CPU_STK_SIZE  )AppTaskMODBUSStk[APP_CFG_TASK_MODBUS_STK_SIZE / 10],
                 (CPU_STK_SIZE  )APP_CFG_TASK_MODBUS_STK_SIZE,
                 (OS_MSG_QTY    )0,
                 (OS_TICK       )0,
                 (void         *)0,
                 (OS_OPT        )(OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
                 (OS_ERR       *)&err);
}


/*
*********************************************************************************************************
*                                          AppObjCreate()
*
* Description : Create application kernel objects tasks.
*
* Argument(s) : none
*
* Return(s)   : none
*
* Caller(s)   : AppTaskStart()
*
* Note(s)     : none.
*********************************************************************************************************
*/
static  void  AppObjCreate (void)
{

}

