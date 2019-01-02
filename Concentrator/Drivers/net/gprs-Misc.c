#include "sys_config.h"
#include "gprs.h"
#include "gprs-Hal.h"
#include "gprs-Misc.h"


static uint8_t OK[] = "OK";
static uint8_t AT[] = "AT\r";
static uint8_t Ready[] = "PB DONE";
//static uint8_t Ready[] = "SMS Ready";
//static uint8_t Ready[] = "Call Ready";

uint8_t GPRSOpen(void)
{
	uint8_t FAIL[] = "FAIL";
	uint8_t ERROR[] = "ERROR";
	uint8_t CONNECTOK[] = "CONNECT";
	uint8_t result = GPRS_BUSY;
	static uint32_t GPRSOpenTimer;
//	uint8_t GPRSOpenTest[]  = "AT+CIPOPEN=0,\"TCP\",\"time.nist.gov\",13\r";	   // time test
//	uint8_t GPRSOpenData1[] = "AT+CIPOPEN=0,\"TCP\",\"119.006.100.234\",05223\r";  // 8081 port
//	uint8_t GPRSOpenData2[] = "AT+CIPOPEN=0,\"TCP\",\"101.204.248.138\",08168\r";  // new server test
//	uint8_t GPRSOpenData3[] = "AT+CIPOPEN=0,\"TCP\",\"120.26.240.190\",08169\r";  // new server

    static enum
    {
		GPRS_OPEN_INIT,
		GPRS_OPEN_WAIT,
        GPRS_OPEN_SEND,
        GPRS_OPEN_RUNNING,
		GPRS_OPEN_SHUT,
    }GPRS_OPEN_STATE = GPRS_OPEN_INIT;
	
    switch(GPRS_OPEN_STATE)
	{
		case GPRS_OPEN_INIT:
			GPRSOpenTimer   = GET_TICK_COUNT();
			GPRS_OPEN_STATE = GPRS_OPEN_WAIT;
			break;
		
		case GPRS_OPEN_WAIT:
			if ((GET_TICK_COUNT() - GPRSOpenTimer) > TICK_RATE_MS(1500))
			{
				GPRS_OPEN_STATE = GPRS_OPEN_SEND;
			}
			break;
			
		case GPRS_OPEN_SEND:
			memset(GPRSRecvBuffer, 0, GPRS_BUFFER_SIZE);
			USART2_SendString(GPRSOpenData);
			GPRSOpenTimer   = GET_TICK_COUNT();
			GPRS_OPEN_STATE = GPRS_OPEN_RUNNING;
			break;
		
		case GPRS_OPEN_RUNNING:
			if ((GET_TICK_COUNT()-GPRSOpenTimer) < TICK_RATE_MS(20000))	// 20000
			{
				if (GPRSRecvFlag == 1)
				{
					GPRSRecvFlag = 0;
					DBG_PRINTF("%s \n", GPRSRecvBuffer);
					
					if ( LookForStr(GPRSRecvBuffer, FAIL)!=FULL || LookForStr(GPRSRecvBuffer, ERROR)!=FULL )	// LookForStr(GPRSRecvBuffer, CLOSE)!=FULL )
					{
						memset(GPRSRecvBuffer, 0, GPRS_BUFFER_SIZE);
						GPRS_OPEN_STATE = GPRS_OPEN_INIT;
						result = GPRS_ERROR;
					}
					else if (LookForStr(GPRSRecvBuffer, CONNECTOK) != FULL)
					{
						memset(GPRSRecvBuffer, 0, GPRS_BUFFER_SIZE);
						GPRS_OPEN_STATE = GPRS_OPEN_INIT;
						result = GPRS_DONE;
					}
					else
					{
						memset(GPRSRecvBuffer, 0, GPRS_BUFFER_SIZE);
					}
				}
			}
			else
			{
				GPRSRecvFlag = 0;
				GPRS_OPEN_STATE = GPRS_OPEN_INIT;
				result = GPRS_ERROR;
			}
			break;
		
		default:
			break;
	}
	
	return result;
}


bool GPRSPowerOn(void)
{
	uint8_t ret;
	uint8_t result = false;
	static uint32_t startTick;
	static tGPRSPowerStates GPRSPowerState = GPRS_POWER_INIT;
	
	switch(GPRSPowerState)
	{
		case GPRS_POWER_IDLE:
			break;
		
		case GPRS_POWER_INIT:
			GPRS_POWER_OFF();
			GPRS_PWRKEY_OFF();
			startTick = GET_TICK_COUNT();
			GPRSPowerState = GPRS_POWER_ON;
			break;
			
		case GPRS_POWER_ON:
			if ((GET_TICK_COUNT() - startTick) > TICK_RATE_MS(3000))
			{
				GPRS_POWER_ON();
				Soft_delay_ms(5);
				GPRS_PWRKEY_ON();
				startTick = GET_TICK_COUNT();
				GPRSPowerState = GPRS_POWER_KEY;
			}
			break;
			
		case GPRS_POWER_KEY:
			if ((GET_TICK_COUNT() - startTick) > TICK_RATE_MS(10000))
			{
				GPRS_PWRKEY_OFF();
				GPRSPowerState = GPRS_POWER_AT;
			}
			break;
			
		case GPRS_POWER_AT:
			ret = GPRSSendRec(AT, OK, 500, 10);
			if (ret == GPRS_DONE)
			{
				startTick = GET_TICK_COUNT();
				GPRSPowerState = GPRS_POWER_WAIT;
			}
			else if (ret == GPRS_ERROR)
			{
				GPRSPowerState = GPRS_POWER_INIT;
			}
			break;
			
		case GPRS_POWER_WAIT:
			if ((GET_TICK_COUNT()-startTick) < TICK_RATE_MS(40000))	// wait 40s
			{
				if (GPRSRecvFlag == 1)	// 接收GPRS数据
				{
					GPRSRecvFlag = 0;
					DBG_PRINTF("%s \n", GPRSRecvBuffer);
					
					if (LookForStr(GPRSRecvBuffer, Ready) != FULL)
					{
						GPRSPowerState = GPRS_POWER_DONE;
					}
				}
			}
			else
			{
				GPRSPowerState = GPRS_POWER_INIT;
			}
			break;
		
		case GPRS_POWER_DONE:
			result = true;
			memset(GPRSRecvBuffer, 0, GPRS_BUFFER_SIZE);
			GPRSPowerState = GPRS_POWER_INIT;
			break;
			
		default:
			break;
	}
	
	return result;
}


uint8_t GPRSCfg(void)
{
	uint8_t ret;
	uint8_t result = GPRS_BUSY;
//	static uint8_t GPRSCfgNum = 0;
    
    uint8_t AT_CGATT[]      = "AT+CGATT?\r";
    uint8_t AT_CGSOCKCONT[] = "AT+CGSOCKCONT=1,\"IP\",\"CMNET\"\r";
	uint8_t AT_TIMESET[]    = "AT+CIPTIMEOUT=30000,20000,40000\r";
    uint8_t AT_CIPMODE[]    = "AT+CIPMODE=1\r";
    uint8_t AT_ATD[]        = "AT&D0\r";
    uint8_t AT_NETOPEN[]    = "AT+NETOPEN\r";
    uint8_t AT_IPADDR[]     = "AT+IPADDR\r";
    uint8_t AT_NETCLOSE[]   = "AT+NETCLOSE\r";
	uint8_t AT_CNSMOD[]     = "AT+CNSMOD?\r";
	
	uint8_t CGATT_OK[] = ": 1";
	uint8_t IPADDR_OK[] = ".";
	uint8_t CNSMOD_OK[] = "CNSMOD: 0,8";
	uint8_t NETOPEN_OK[] = "NETOPEN: 0";
	uint8_t NETCLOSE_OK[] = "NETCLOSE: 0";
	
	static enum
    {
        GPRS_CFG_CGATT,
		GPRS_CFG_CNSMOD,
        GPRS_CFG_CONT,
		GPRS_CFG_TIMESET,
        GPRS_CFG_MODE,
		GPRS_CFG_ATD,
		GPRS_CFG_NETOPEN,
		GPRS_CFG_IPADDR,
		GPRS_CFG_ERROR,
		GPRS_CFG_WAIT,
		GPRS_CFG_CLOSE
    }GPRS_CFG_STATE = GPRS_CFG_CGATT;
	
	switch(GPRS_CFG_STATE)
	{
		case GPRS_CFG_CGATT:
			ret = GPRSSendRec(AT_CGATT, CGATT_OK, 2000, 30);
			if (ret == GPRS_DONE)
			{
				GPRS_CFG_STATE = GPRS_CFG_CNSMOD;
			}
			else if (ret == GPRS_ERROR)
			{
				GPRS_CFG_STATE = GPRS_CFG_ERROR;
			}
			break;
			
		case GPRS_CFG_CNSMOD:
			ret = GPRSSendRec(AT_CNSMOD, CNSMOD_OK, 1000, 3);
			if (ret == GPRS_DONE)
			{
				NetMode = 4;
				GPRS_CFG_STATE = GPRS_CFG_CONT;
			}
			else if (ret == GPRS_ERROR)
			{
				NetMode = 2;
				GPRS_CFG_STATE = GPRS_CFG_CONT;
			}
			break;
            
        case GPRS_CFG_CONT:
			ret = GPRSSendRec(AT_CGSOCKCONT, OK, 1000, 3);
			if (ret == GPRS_DONE)
			{
				GPRS_CFG_STATE = GPRS_CFG_TIMESET;
			}
			else if (ret == GPRS_ERROR)
			{
				GPRS_CFG_STATE = GPRS_CFG_ERROR;
			}
			break;
		
		case GPRS_CFG_TIMESET:
			ret = GPRSSendRec(AT_TIMESET, OK, 1000, 3);
			if (ret == GPRS_DONE)
			{
				GPRS_CFG_STATE = GPRS_CFG_MODE;
			}
			else if (ret == GPRS_ERROR)
			{
				GPRS_CFG_STATE = GPRS_CFG_ERROR;
			}
			break;
			
		case GPRS_CFG_MODE:
			ret = GPRSSendRec(AT_CIPMODE, OK, 1000, 3);
			if (ret == GPRS_DONE)
			{
				GPRS_CFG_STATE = GPRS_CFG_ATD;
			}
			else if (ret == GPRS_ERROR)
			{
				GPRS_CFG_STATE = GPRS_CFG_ERROR;
			}
			break;
			
		case GPRS_CFG_ATD:
			ret = GPRSSendRec(AT_ATD, OK, 1000, 3);
			if (ret == GPRS_DONE)
			{
				GPRS_CFG_STATE = GPRS_CFG_NETOPEN;
			}
			else if (ret == GPRS_ERROR)
			{
				GPRS_CFG_STATE = GPRS_CFG_ERROR;
			}
			break;
			
		case GPRS_CFG_NETOPEN:
			ret = GPRSSendRec(AT_NETOPEN, NETOPEN_OK, 30000, 1);
			if (ret == GPRS_DONE)
			{
				GPRS_CFG_STATE = GPRS_CFG_IPADDR;
			}
			else if (ret == GPRS_ERROR)
			{
				GPRS_CFG_STATE = GPRS_CFG_ERROR;
			}
			break;
			
		case GPRS_CFG_IPADDR:
			ret = GPRSSendRec(AT_IPADDR, IPADDR_OK, 5000, 3);
			if (ret == GPRS_DONE)
			{
				result = GPRS_DONE;
				GPRS_CFG_STATE = GPRS_CFG_CGATT;
			}
			else if (ret == GPRS_ERROR)
			{
				GPRS_CFG_STATE = GPRS_CFG_ERROR;
			}
			break;
			
		case GPRS_CFG_ERROR:
			ret = GPRSSendRec(AT_NETCLOSE, NETCLOSE_OK, 5000, 1);
			if (ret == GPRS_DONE)
			{
				GPRS_CFG_STATE = GPRS_CFG_CGATT;
			}
			else if (ret == GPRS_ERROR)
			{
				result = GPRS_ERROR;
				GPRS_CFG_STATE = GPRS_CFG_CGATT;
			}
			break;
		
		default:
			break;
	}
	
	return result;
}

/* 退出透传模式 */
bool GPRSExit(void)
{
	bool result = false;
	static uint8_t ExitNum = 0;
	static uint32_t ExitTimeoutTimer;
	static enum
    {
		GPRS_EXIT_INIT,
		GPRS_EXIT_WAIT,
        GPRS_EXIT_RUNNING,
		GPRS_EXIT_END,
    }GPRS_EXIT_STATE = GPRS_EXIT_INIT;
	
	switch(GPRS_EXIT_STATE)
	{
		case GPRS_EXIT_INIT:
			ExitTimeoutTimer = GET_TICK_COUNT();
			GPRS_EXIT_STATE  = GPRS_EXIT_WAIT;
			break;
		
		case GPRS_EXIT_WAIT:
			if ((GET_TICK_COUNT() - ExitTimeoutTimer) > TICK_RATE_MS(1000))
			{
				ExitNum = 0;
				ExitTimeoutTimer = GET_TICK_COUNT();
				GPRS_EXIT_STATE  = GPRS_EXIT_RUNNING;
			}
			break;
			
		case GPRS_EXIT_RUNNING:
			if ((GET_TICK_COUNT() - ExitTimeoutTimer) > TICK_RATE_MS(500))
			{
				USART2_SendString((uint8_t *)"+");
				DBG_PRINTF("+");
				ExitTimeoutTimer = GET_TICK_COUNT();
				ExitNum++;
				if (ExitNum == 3)
				{
					ExitNum = 0;
					GPRS_EXIT_STATE  = GPRS_EXIT_END;
				}
			}
			break;
		
		case GPRS_EXIT_END:
			if ((GET_TICK_COUNT() - ExitTimeoutTimer) > TICK_RATE_MS(1000))
			{
				ExitTimeoutTimer = GET_TICK_COUNT();
				GPRS_EXIT_STATE  = GPRS_EXIT_INIT;
				result = true;
			}
			break;
		
		default:
			break;
	}
	
	return result;
}
