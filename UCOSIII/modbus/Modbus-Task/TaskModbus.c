/*
;************************************************************************************************************
;*				        		       
;*
;*                                 	     
;*
;*--------------------------------------------- 文件信息 ----------------------------------------------------                                      
;*
;* 文件名称 : 	
;* 文件功能 : MODBUS通讯	显示信息
;*            
;* 补充说明 : 
;*-------------------------------------------- 最新版本信息 -------------------------------------------------
;* 修改作者 : 
;* 修改日期 : 
;* 版本声明 : 
;*-------------------------------------------- 历史版本信息 -------------------------------------------------
;* 文件作者 : 
;* 创建日期 : 
;* 版本声明 :  
;*-----------------------------------------------------------------------------------------------------------
;*-----------------------------------------------------------------------------------------------------------
;************************************************************************************************************
*/

#include "includes.h"

/* ----------------------- Modbus includes ----------------------------------*/
#include "mb.h"
#include "mbport.h"
#include "mbutils.h"

#include "TaskModbus.h"



/* ----------------------- Static variables ---------------------------------*/
/*0X表示读控制器的输出信号
  1X表示读控制器的输入信号
  3X和4X指控制器的寄存器地址类型，4X可读可写，3X只读
*/
INT16U   usRegInputStart = REG_INPUT_START;
INT16U   usRegInputBuf[REG_INPUT_NREGS];

INT16U   usRegHoldingStart = REG_HOLDING_START;
INT16U	usRegHoldingBuf[REG_HOLDING_NREGS];

INT16U   ucRegCoilsStart = REG_COILS_START;
INT8U	ucRegCoilsBuf[REG_COILS_NREGS];	//一个线圈管理8个输出，0X

INT16U   ucRegDiscStart = REG_DISC_START;	
INT8U	ucRegDiscBuf[REG_DISC_NREGS];		// 1X





/*********************************************************************************************************
**                            TaskStart(void *pdata)
********************************************************************************************************/


void TaskModbus(void *parg)
{
	eMBErrorCode    eStatus;
	OS_ERR err;
	eStatus = eStatus;	
	(void)parg;

	eStatus = eMBInit( MB_RTU, 0x01, 0, 9600, MB_PAR_EVEN );

	/* Enable the Modbus Protocol Stack. */
	eStatus = eMBEnable();

	for( ;; )
	{
		eMBPoll();
		OSTimeDly(5,OS_OPT_TIME_DLY,&err);
		
	}
} 

eMBErrorCode
eMBRegInputCB( UCHAR * pucRegBuffer, USHORT usAddress, USHORT usNRegs )
{
    eMBErrorCode    eStatus = MB_ENOERR;
    int             iRegIndex;

    if( ( usAddress >= REG_INPUT_START )
        && ( usAddress + usNRegs <= REG_INPUT_START + REG_INPUT_NREGS ) )
    {
        iRegIndex = ( int )( usAddress - usRegInputStart );
        while( usNRegs > 0 )
        {
            *pucRegBuffer++ = ( unsigned char )( usRegInputBuf[iRegIndex] >> 8 );
            *pucRegBuffer++ = ( unsigned char )( usRegInputBuf[iRegIndex] & 0xFF );
            iRegIndex++;
            usNRegs--;
        }
    }
    else
    {
        eStatus = MB_ENOREG;
    }

    return eStatus;
}

eMBErrorCode eMBRegHoldingCB( UCHAR * pucRegBuffer, USHORT usAddress, USHORT usNRegs,
                 eMBRegisterMode eMode )
{
    eMBErrorCode    eStatus = MB_ENOERR;
    int             iRegIndex;
    USHORT			usTmp;

    if( ( usAddress >= REG_HOLDING_START )
        && ( usAddress + usNRegs <= REG_HOLDING_START + REG_HOLDING_NREGS ) )
    {
        iRegIndex = ( int )( usAddress - usRegHoldingStart );
        switch(eMode)
        {
        	case MB_REG_READ:
        	{
		        while( usNRegs > 0 )
		        {
		            *pucRegBuffer++ =
		                ( UCHAR )( usRegHoldingBuf[iRegIndex] >> 8 );
		            *pucRegBuffer++ =
		                ( UCHAR )( usRegHoldingBuf[iRegIndex] & 0xFF );
		            iRegIndex++;
		            usNRegs--;
		        }
		        break;
	        }
	        case MB_REG_WRITE:
	        {
	        	while( usNRegs > 0 )
		        {
		            usTmp = (USHORT)(*pucRegBuffer++);
		            usTmp <<= 8;
		            usTmp += (USHORT)(*pucRegBuffer++);
		            usRegHoldingBuf[iRegIndex] = usTmp;
		            iRegIndex++;
		            usNRegs--;
		        }
	        	break;
	        }
	        default:
               {
                	eStatus = MB_ENOREG;
                	break;
               }
        }
    }
    else
    {
        eStatus = MB_ENOREG;
    }

    return eStatus;
}



eMBErrorCode eMBRegCoilsCB( UCHAR * pucRegBuffer, USHORT usAddress, USHORT usNCoils,
               eMBRegisterMode eMode )
{
    eMBErrorCode    eStatus = MB_ENOERR;
    SHORT           iNCoils = ( SHORT )usNCoils;
    USHORT  usBitOffset;

    /* Check if we have registers mapped at this block. */
    if( ( usAddress >= REG_COILS_START ) &&
        ( usAddress + usNCoils <= REG_COILS_START + REG_COILS_NREGS) )
    {
        usBitOffset = ( USHORT )( usAddress - ucRegCoilsStart );
        switch ( eMode )
        {
                /* Read current values and pass to protocol stack. */
            case MB_REG_READ:
			{
			    while( iNCoils > 0 )
			    {
			        *pucRegBuffer++ =
			            xMBUtilGetBits( ucRegCoilsBuf, usBitOffset,
			                            ( UCHAR )( iNCoils > 8 ? 8 : iNCoils ) );
			        iNCoils -= 8;
			        usBitOffset += 8;
			    }
			    break;
			}

                /* Update current register values. */
            case MB_REG_WRITE:
            {
                while( iNCoils > 0 )
                {
                    xMBUtilSetBits( ucRegCoilsBuf, usBitOffset,
                                    ( UCHAR )( iNCoils > 8 ? 8 : iNCoils ),
                                    *pucRegBuffer++ );
                    iNCoils -= 8;
                }
                break;
            }
            //>>ASU_2010823_V0 For Issue : ADD default
            default:
            {
            	eStatus = MB_ENOREG;
            	break;
            }
            //<<ASU_2010823_V0 For Issue : ADD default
        }

    }
    else
    {
        eStatus = MB_ENOREG;
    }
    return eStatus;
}

eMBErrorCode eMBRegDiscreteCB( UCHAR * pucRegBuffer, USHORT usAddress, USHORT usNDiscrete )
{
    eMBErrorCode    eStatus = MB_ENOERR;
    SHORT           iNDiscrete = ( SHORT )usNDiscrete;
    USHORT  usBitOffset;

    /* Check if we have registers mapped at this block. */
    if( ( usAddress >= REG_DISC_START ) &&
        ( usAddress + usNDiscrete <= REG_DISC_START + REG_DISC_NREGS) )
    {
        usBitOffset = ( USHORT )( usAddress - ucRegDiscStart );

	    while( iNDiscrete > 0 )
	    {
	        *pucRegBuffer++ =
	            xMBUtilGetBits( ucRegDiscBuf, usBitOffset,
	                            ( UCHAR )( iNDiscrete > 8 ? 8 : iNDiscrete ) );
	        iNDiscrete -= 8;
	        usBitOffset += 8;
	    }
    }
    else
    {
        eStatus = MB_ENOREG;
    }
    return eStatus;
}



/*----------------------------------END OF FILE-------------------------------*/   	              
