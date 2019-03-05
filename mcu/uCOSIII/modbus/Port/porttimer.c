/*
 * FreeModbus Libary: LPC214X Port
 * Copyright (C) 2007 Tiago Prado Lone <tiago@maxwellbohr.com.br>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * File: $Id: porttimer.c,v 1.1 2007/04/24 23:15:18 wolti Exp $
 */

/* ----------------------- Platform includes --------------------------------*/
#include "port.h"
#include "stm32f4xx_tim.h"
/* ----------------------- Modbus includes ----------------------------------*/
#include "mb.h"
#include "mbport.h"

/* ----------------------- Start implementation -----------------------------*/
BOOL
xMBPortTimersInit( USHORT usTim1Timerout50us )
{
	TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;

	//TIM_TimeBaseStructure.TIM_Period = 0x7E54;		  //CLK==24MHz ((1000000000/9600)*11*3.5)/(1000/24) == 0x7e54
	//TIM_TimeBaseStructure.TIM_Prescaler = 0x3;
	TIM_TimeBaseStructure.TIM_Period = 3999;		  
	TIM_TimeBaseStructure.TIM_Prescaler = 83; 
	TIM_TimeBaseStructure.TIM_ClockDivision = 0x0;
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseInit(TIM2, &TIM_TimeBaseStructure);
	TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE);
    TIM_Cmd(TIM2, ENABLE);
    return TRUE;
}


void
vMBPortTimersEnable(  )
{	
	TIM_ClearITPendingBit(TIM2, TIM_IT_Update);
	TIM_SetCounter(TIM2, 0);
	TIM_Cmd(TIM2, ENABLE);
	TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE);
}

void
vMBPortTimersDisable(  )
{
	TIM_SetCounter(TIM2, 0);
	TIM_Cmd(TIM2, DISABLE);
	TIM_ITConfig(TIM2, TIM_IT_Update, DISABLE);
}

void
TIMERExpiredISR( void )
{
    (void)pxMBPortCBTimerExpired();
}
