#include "sys_config.h"
#include "net.h"
#include "gprs.h"
#include "gprs-Hal.h"
#include "gprs-Misc.h"
#include "radio.h"


uint8_t Deact[] = "DEACT";	/* PDP Context Deactivate */
uint8_t Close[] = "CLOSED";	/* TCP socket was shut */

uint8_t GPRSOnline = 0;     /* GPRS在线标志 */
uint8_t GPRSGetFlag = 0;
uint8_t GPRSPutFlag = 0;

uint8_t GPRSRecvFlag = 0;
uint8_t GPRSRecvSize = 0;
uint8_t GPRSRecvBuffer[GPRS_BUFFER_SIZE];

static uint8_t ResetFlag = 0;
static uint8_t GPRSSendFlag = 0;
static uint8_t GPRSSendSize = 0;
static uint8_t GPRSSendBuffer[GPRS_BUFFER_SIZE];

static uint8_t NetSendSize = 0;
static uint8_t NetSendBuffer[GPRS_BUFFER_SIZE];
static T_MultiPacket GPRSMultiPacket;	/* 多帧传输 */

static void GPRSInit(void);
static void GetRFState(void);
static void GPRSProcess(void);
static void GPRSSendData(uint8_t *data, uint8_t size);
static void GPRSRecvData(uint8_t *data, uint8_t *size);
static void GPRSReplyHandle(uint8_t *buffer, uint8_t size);

static T_NetDriver GPRSDriver = {
	.name	  = "gprs",
	.Init     = GPRSInit,
 	.SendData = GPRSSendData,
 	.RecvData = GPRSRecvData,
	.Process  = GPRSProcess,
};

void GPRSProcess(void)
{
	uint8_t return_val;
	static T_HeartBeat Register;
	static uint8_t RegisterNum = 0;
	static uint32_t TxTimeoutTimer;
	static uint32_t HeartTimeoutTimer;
	static uint32_t RegisterTimeoutTimer;
	static tGPRSStates GPRSState = GPRS_STATE_POWERON;
	
	switch(GPRSState)
	{
		case GPRS_STATE_IDLE:
			 break;
			 
		case GPRS_STATE_POWERON:
			if ( GPRSPowerOn() )
			{
				GPRSState = GPRS_STATE_CFG;
			}
			break;
			
		case GPRS_STATE_CFG:
			return_val = GPRSCfg();
			if (return_val == GPRS_DONE)
			{
				GPRSState = GRPS_STATE_OPEN;
			}
			else if (return_val == GPRS_ERROR)
			{
				GPRSState = GPRS_STATE_POWERON;
			}
			break;
			
		case GRPS_STATE_OPEN:
			return_val = GPRSOpen();
			if (return_val == GPRS_DONE)
			{
				Register = HeartBeat;
				Register.Head.FuncCode = 0x1001;
				NetComputeCRC((uint8_t *)&Register, sizeof(Register)-3, (uint8_t *)&Register.Crc);  // create CRC
				USART2_SendData((uint8_t *)&Register, sizeof(Register));
				DATA_PRINTF((uint8_t *)&Register, sizeof(Register));
				RegisterTimeoutTimer = GET_TICK_COUNT();
				GPRSState = GPRS_STATE_REGISTER;
			}
			else if (return_val == GPRS_ERROR)
			{
				GPRSState = GPRS_STATE_EXIT;
			}
			break;
			
		case GPRS_STATE_REGISTER:
			if ((GET_TICK_COUNT() - RegisterTimeoutTimer) < TICK_RATE_MS(10000))  /* 注册包10s回复限时 */
			{
				if (GPRSRecvFlag == 1)	/* 接收到GPRS数据 */
				{
					GPRSRecvFlag = 0;
					DATA_PRINTF(GPRSRecvBuffer, GPRSRecvSize);
					if ( NetDataCheck(GPRSRecvBuffer, GPRSRecvSize) && GPRSRecvBuffer[1]==0x01 && GPRSRecvBuffer[2]==0x10)  /* check crc and register */
					{
						RegisterNum = 0;
						Soft_delay_ms(5);
						RTCWrite((PT_SystemTime)(GPRSRecvBuffer+25));
						TxTimeoutTimer    = GET_TICK_COUNT();
						HeartTimeoutTimer = GET_TICK_COUNT();
						GPRSState         = GPRS_STATE_RUNNING;
					}
				}
			}
			else
			{
				RegisterNum++;
				if (RegisterNum < 3)
				{
					USART2_SendData((uint8_t *)&Register, sizeof(Register));
					DATA_PRINTF((uint8_t *)&Register, sizeof(Register));
					RegisterTimeoutTimer = GET_TICK_COUNT();
				}
				else
				{
					RegisterNum = 0;
					GPRSState = GPRS_STATE_EXIT;
				}
			}
			break;
		
		case GPRS_STATE_RUNNING:
			if ((GET_TICK_COUNT() - TxTimeoutTimer) < TICK_RATE_MS(ConParm.GPRSHeartTime))
			{
				if (GPRSRecvFlag == 1)	// 接收GPRS数据
				{
					GPRSRecvFlag = 0;
					DATA_PRINTF(GPRSRecvBuffer, GPRSRecvSize);
					if ( NetDataCheck(GPRSRecvBuffer, GPRSRecvSize) )  /* check receive data */
					{
						if (GPRSRecvBuffer[1]==0x02 && GPRSRecvBuffer[2]==0x10)  /* heartbeat */
						{
							HeartTimeoutTimer = GET_TICK_COUNT();
						}
						else  /* net data */
						{
							TxTimeoutTimer    = GET_TICK_COUNT();
							HeartTimeoutTimer = GET_TICK_COUNT();
							GPRSReplyHandle(GPRSRecvBuffer, GPRSRecvSize);
						}
					}
					else if (LookForStr(GPRSRecvBuffer, Close)!=FULL || LookForStr(GPRSRecvBuffer, Deact)!=FULL)  /* off line data */
					{
						GPRSState = GRPS_STATE_OPEN;
					}
					else
					{
						memset(GPRSRecvBuffer, 0, GPRS_BUFFER_SIZE);
					}
				}
				
				if (GPRSSendFlag == 1)	/* 发送GPRS数据 */
				{
					GPRSSendFlag = 0;
					NetComputeCRC(GPRSSendBuffer, GPRSSendSize-3, GPRSSendBuffer+GPRSSendSize-3);
					USART2_SendData(GPRSSendBuffer, GPRSSendSize);  // send GPRS data
					//DATA_PRINTF(GPRSSendBuffer, GPRSSendSize);
					if (ResetFlag == 1)
					{
						Soft_delay_ms(2500);
						SoftReset();
					}
				}
				
				if (GPIO_ReadInputDataBit(GPRS_DCD_GPIO, GPRS_DCD_PIN) == Bit_SET)
				{
					GPRSState = GRPS_STATE_OPEN;
				}
			}
			else
			{
				GetRFState();
				NetComputeCRC((uint8_t *)&HeartBeat, sizeof(T_HeartBeat) - 3, (uint8_t *)&HeartBeat.Crc);
				USART2_SendData((uint8_t *)&HeartBeat, sizeof(T_HeartBeat));  /* 发送心跳包 */
				//DATA_PRINTF((uint8_t *)&HeartBeat, sizeof(T_HeartBeat));
				TxTimeoutTimer = GET_TICK_COUNT();
			}
			
			if ((GET_TICK_COUNT() - HeartTimeoutTimer) > TICK_RATE_MS(240000))  /* 4min断线检测 */
			{
				GPRSState = GRPS_STATE_OPEN;  /* 没有收到中心数据，重连 */
			}
			break;
			
		case GPRS_STATE_EXIT:  /* 退出透传模式 */
			if (GPRSExit())
			{
				GPRSState = GPRS_STATE_CFG;
			}
			break;
			
		default:
			break;
	}
	
	if (GPRSState == GPRS_STATE_RUNNING || GPRSState == GPRS_STATE_REGISTER)  /* GPRS在线指示灯 */
	{
		GPRS_LED_ON();
		GPRSOnline = 1;
	}
	else
	{
		GPRS_LED_OFF();
		GPRSOnline = 0;
	}
}

/* GPRS确认回复 */
void GPRSReplyHandle(uint8_t *buffer, uint8_t size)
{
	uint8_t i, temp;
	uint16_t k, TempID;
	PT_Task ptRadioTmp;
	uint8_t MeterAddSum;
	uint8_t CommonBack = 1;
	uint8_t NetSendFlag = 1;
	uint8_t	MeterAddrTemp[4];
	PT_Task ptNetDel = g_ptNetTaskHead;
	PT_Task ptRadioDel = g_ptRadioTaskHead;
	PT_NetHead ptNetHead = (PT_NetHead)NetSendBuffer;
	
	memcpy(&TempID, buffer+1, 2);
	memcpy(NetSendBuffer, buffer, 23);
	if (buffer[3] == 0x01)  /* 收到中心发送的指令 */
	{
		if (TempID == 0x1003)  /* set system initial parameter config */
		{
			memcpy(ConParm.ID, buffer+23, 4);
			memcpy(&ConParm.RFChannel, buffer+27, 1);
			MemoryBufferWrite(MRAM, buffer+28, SOCKET_ADDR, sizeof(T_Socket));
			memcpy(ConParm.MAC, buffer+73, 4);
			MemoryBufferWrite(MRAM, (uint8_t *)&ConParm, CON_PARM_ADDR, sizeof(ConParm));  /*  write CON parameters */
			ResetFlag = 1;
		}
		else if (TempID == 0x1004)  /* read system initial parameter */
		{
			CommonBack = 0;
			ptNetHead->DataLen = 56;
			memset(NetSendBuffer+23, 0x00, 2);
			memcpy(NetSendBuffer+25, ConParm.ID, 4);
			memcpy(NetSendBuffer+29, &ConParm.RFChannel, 1);
			MemoryBufferRead(MRAM, NetSendBuffer+30, SOCKET_ADDR, sizeof(T_Socket));  /* read socket parameter */
			memcpy(NetSendBuffer+75, ConParm.MAC, 4);
			memset(NetSendBuffer+81, 0x16, 1);
			NetSendSize = 82;
		}
		else if (TempID == 0x1005)  /* set GPRS heartbeat time */
		{
			ConParm.GPRSHeartTime = (*((uint16_t *)(buffer+23)) * 1000);  /* 心跳周期(ms) */
			MemoryBufferWrite(MRAM, (uint8_t *)&ConParm, CON_PARM_ADDR, sizeof(ConParm));  /* write CON parameters */
		}
		else if (TempID == 0x1006)  /* read GPRS heart time */
		{
			CommonBack = 0;
			ptNetHead->DataLen = 0x04;
			memset(NetSendBuffer+23, 0x00, 2);
			*((uint16_t *)(NetSendBuffer+25)) = (uint16_t)(ConParm.GPRSHeartTime/1000);
			memset(NetSendBuffer+29, 0x16, 1);
			NetSendSize = 30;
		}
		else if (TempID == 0x1008)  /* read the concentrator vision */
		{
			CommonBack = 0;
			ptNetHead->DataLen = 10;
			memset(NetSendBuffer+23, 0x00, 2);
			memcpy(NetSendBuffer+25, (uint8_t *)&RELEASE_TIME, 4);  // release time
			memset(NetSendBuffer+29, SOFTWARE_VERSION, 1);  // 1 bytes version number
			memset(NetSendBuffer+30, HARDWARE_VERSION, 1);  // 1 bytes version number
			memset(NetSendBuffer+31, 3, 1);
			memset(NetSendBuffer+32, NetMode, 1);
			memset(NetSendBuffer+35, 0x16, 1);
			NetSendSize = 36;
		}
		else if (TempID == 0x2100)	/* 集中器恢复出厂设置 */
		{
			RTCInit();  			/* Initialize RTC time value */
			AllMemoryBufferReset(MRAM);
			temp = 0;
			MemoryBufferWrite(EEPROM, &temp, 0x01, 1);
			Soft_delay_ms(5);
			ResetFlag = 1;
		}
		else if (TempID == 0x2103)	/* 设置时钟 */
		{
			Soft_delay_ms(5);
			RTCWrite((PT_SystemTime)(buffer+23));
		}
		else if (TempID == 0x2104)	/* 读取时钟 */
		{
			CommonBack = 0;
			ptNetHead->DataLen = 0x09;
			memset(NetSendBuffer+23, 0x00, 2);
			memcpy(NetSendBuffer+25, &SystemTime, 7);
			memset(NetSendBuffer+34, 0x16, 1);
			NetSendSize = 35;
		}
		else if (TempID == 0x2105)	/* 设置射频频段 */
		{
			RFChannelFlag = 1;
			ConParm.RFChannel = *(buffer+23);
			MemoryBufferWrite(MRAM, (uint8_t *)&ConParm, CON_PARM_ADDR, sizeof(ConParm));
			/* 修改3002扫频数据中的射频参数 */
			MemoryBufferRead(MRAM, (uint8_t *)&InstallParm, METER_INSTALL_ADDR, sizeof(T_InstallParm));
			InstallParm.MeterInstallParm.RFChannel = ConParm.RFChannel;
			MemoryBufferWrite(MRAM, (uint8_t *)&InstallParm, METER_INSTALL_ADDR, sizeof(T_InstallParm));
		}
		else if (TempID == 0x2106)	/* 读取射频频段 */
		{
			CommonBack = 0;
			ptNetHead->DataLen = 0x03;
			memset(NetSendBuffer+23, 0x00, 2);
			memset(NetSendBuffer+25, ConParm.RFChannel, 1);
			memset(NetSendBuffer+28, 0x16, 1);
			NetSendSize = 29;
		}
		else if (TempID == 0x2109)	/* 设置集中器ID */
		{
			temp = 0;
			MemoryBufferWrite(EEPROM, &temp, 0x01, 1);
			memcpy(ConParm.ID, buffer+23, 3);
			MemoryBufferWrite(MRAM, (uint8_t *)&ConParm, CON_PARM_ADDR, sizeof(ConParm));
			ResetFlag = 1;
		}
		else if (TempID == 0x2110)	/* 读取集中器ID */
		{
			CommonBack = 0;
			ptNetHead->DataLen = 0x06;
			memset(NetSendBuffer+23, 0x00, 2);
			memcpy(NetSendBuffer+25, ConParm.ID, 4);
			memset(NetSendBuffer+31, 0x16, 1);
			NetSendSize = 32;
		}
		else if (TempID == 0x2118)	/* 集中器自动升级指令 */
		{
			temp = 1;
			MemoryBufferWrite(EEPROM, &temp, 0x01, 1);
			Soft_delay_ms(5);
			MemoryBufferWrite(EEPROM, buffer+23, 0x10, 60);
			Soft_delay_ms(5);
			RTCWrite((PT_SystemTime)(buffer+83));
			ResetFlag = 1;
		}
		else if(TempID == 0x2120)   /* 集中器X模式设置 */
		{
			if (*(buffer+23) == 1)	/* 进入X模式 */
			{
				XmodeCfg.Enable = 1;
			}
			else  /* 退出X模式 */
			{
				XmodeCfg.Enable = 0;
                memset(XmodeCfg.Data, 0, X_MODE_SIZE);
			}
		}
		else if (TempID == 0x2122)	/* 使能集中器进入扫频安装模式 */
		{
			if (SystemTime.hour>=0x06 && SystemTime.hour<=0x23)  /* 判断是否为扫频时间段 */
			{
				if (InstallParm.Flag == 0)	/* 没有配置扫频数据，返回错误 */
				{
					CommonBack = 0;
					ptNetHead->DataLen = 0x02;
					memset(NetSendBuffer+23, 0x06, 1);
					memset(NetSendBuffer+24, 0x01, 1);
					memset(NetSendBuffer+27, 0x16, 1);
					NetSendSize = 28;
				}
				else
				{
					for (k = 0; k < METER_SUM; k++)
					{
						MemoryBufferRead(MRAM, MeterAddrTemp, METER_ADDR+4*k, 4);
						if (MeterAddrTemp[0] == 0xFF)  /* 为空节点 */
						{
							break;
						}
					}
					
					if (k != METER_SUM)
					{
						RadioState.Mode  = S_MODE;
						RadioState.Stage = STAGE_ENTER;
					}
					else  /* 集中器管理表具数量已满 */
					{
						CommonBack = 0;
						ptNetHead->DataLen = 0x02;
						*((uint16_t *)(NetSendBuffer+23)) = 0x0101;  /* 2 bytes error code */
						memset(NetSendBuffer+27, 0x16, 1);
						NetSendSize = 28;
					}
				}
			}
			else
			{
				CommonBack = 0;
				ptNetHead->DataLen = 0x02;
				memset(NetSendBuffer+23, 0x13, 1);  /* time error code */
				memset(NetSendBuffer+24, 0x01, 1);
				memset(NetSendBuffer+27, 0x16, 1);
				NetSendSize = 28;
			}
		}
		else if (TempID == 0x2123)  /* 配置集中器的表具安装参数(3002参数) */
		{
			InstallParm.Flag = 1;
			memcpy(&InstallParm.MeterInstallParm, buffer+23, sizeof(T_MeterInstallParm));
			MemoryBufferWrite(MRAM, (uint8_t *) &InstallParm, METER_INSTALL_ADDR, sizeof(T_InstallParm));
		}
		else if (TempID == 0x2124)  /* 读取集中器存储的表具安装参数(3002参数) */
		{
			CommonBack = 0;
			ptNetHead->DataLen  = sizeof(T_InstallParm) + 1;
			memset(NetSendBuffer+23, 0x00, 2);
			MemoryBufferRead(MRAM, NetSendBuffer+25, METER_INSTALL_ADDR+1, sizeof(T_InstallParm)-1);
			memset(NetSendBuffer+sizeof(T_InstallParm)+26, 0x16, 1);
			NetSendSize = sizeof(T_InstallParm) + 27;
		}
		else if (TempID == 0x2130)	/* 读集中器软件版本 */
		{
			CommonBack = 0;
			ptNetHead->DataLen = 0x03;
			memset(NetSendBuffer+23, 0x00, 2);
			memset(NetSendBuffer+25, SOFTWARE_VERSION, 1);  /* 1 bytes software version */
			memset(NetSendBuffer+28, 0x16, 1);
			NetSendSize = 29;
		}
		else if (TempID == 0x2131)	/* 清集中器Y/Z任务参数 */
		{
			ptRadioDel = g_ptRadioTaskHead;
			if ( CompareData(ptNetHead->DstAddr, BroadcastAddr, 4) )  /* delete all radio task */
			{
				while (ptRadioDel)
				{
					ptRadioTmp = ptRadioDel->ptNext;
					DelTask(RADIO_TASK, ptRadioDel);
					ptRadioDel = ptRadioTmp;
				}
			}
			else  /* delete specific radio task */
			{
				while (ptRadioDel)
				{
					if ( CompareData(ptRadioDel->DstAddr, ptNetHead->DstAddr, 4) == true )
					{
						ptRadioTmp = ptRadioDel->ptNext;
						DelTask(RADIO_TASK, ptRadioDel);
						ptRadioDel = ptRadioTmp;
					}
					else
					{
						ptRadioDel = ptRadioDel->ptNext;
					}
				}
			}
		}
		else if (TempID == 0x2303)	/* 增加表具 */
		{
			uint8_t m = 0;
			
			MeterAddSum = (buffer[22] - 8) / 4;
			if ((METER_SUM - MeterTotal) >= MeterAddSum)
			{
				for (i = 0; i < MeterAddSum; i++)
				{
					if ( (k = FindMeterID(buffer+27+4*i)) == METER_SUM )  /* 存储器中不存在要增加的表号 */
					{
						for (k = 0; k < METER_SUM; k++)
						{
							MemoryBufferRead(MRAM, MeterAddrTemp, METER_ADDR+4*k, 4);
							if (MeterAddrTemp[0] == 0xFF)  /* 为空节点 */
							{
								MeterTotal++;
								MemoryBufferWrite(MRAM, buffer+27+4*i, METER_ADDR+4*k, 4);
								memcpy(NetSendBuffer+26+6*m, buffer+27+4*i, 4);  /* 集中器地址 */
								memcpy(NetSendBuffer+30+6*m, (uint8_t *)&k, 2);  /* 节点地址 */
								m++;
								break;
							}
						}
					}
					else  /* 存储器中已存在要增加的表号 */
					{
						memcpy(NetSendBuffer+26+6*m, buffer+27+4*i, 4);  /* 集中器地址 */
						memcpy(NetSendBuffer+30+6*m, (uint8_t *)&k, 2);  /* 节点地址 */
						m++;
					}
				}
				MemoryBufferWrite(MRAM, (uint8_t *)&MeterTotal, METER_TOTAL_ADDR, 2);  /* 保存管理表具总数量 */
				
				CommonBack = 0;
				ptNetHead->DataLen = 6*m + 3;
				memset(NetSendBuffer+23, 0x00, 2);
				memset(NetSendBuffer+25, m, 1);    /* 表具数量 */
				memset(NetSendBuffer+28+6*m, 0x16, 1);
				NetSendSize = 6*m + 29;
			}
			else
			{
				CommonBack = 0;
				ptNetHead->DataLen = 0x02;
				*((uint16_t *)(NetSendBuffer+23)) = 0x0101;  /* 2 bytes error code */
				memset(NetSendBuffer+27, 0x16, 1);
				NetSendSize = 28;
			}
		}
		else if (TempID == 0x2304)	/* 删除表具 */
		{
			MeterAddSum = (buffer[22] - 4) / 4;
			for (i = 0; i < MeterAddSum; i++)
			{
				if ( (k = FindMeterID(buffer+23+4*i)) != METER_SUM )
				{
					/* 表具删除后，清除这支表的所有Z任务 */
					ptRadioDel = g_ptRadioTaskHead;
					while (ptRadioDel)
					{
						if ( CompareData(ptRadioDel->DstAddr, buffer+23+4*i, 4) == true )
						{
							ptRadioTmp = ptRadioDel->ptNext;
							DelTask(RADIO_TASK, ptRadioDel);
							ptRadioDel = ptRadioTmp;
						}
						else
						{
							ptRadioDel = ptRadioDel->ptNext;
						}
					}
					
					MeterTotal--;
					memset(MeterAddrTemp, 0xFF, 4);
					MemoryBufferWrite(MRAM, MeterAddrTemp, METER_ADDR+4*k, 4);
					
					/* 清除3004、5005、5007、5009任务标志及数据内容 */
					MRAMWriteData(ESTASK_ADDR+sizeof(T_ESTASK)*k, 0x00, sizeof(T_ESTASK));
					
					/* 清除存储的2405数据 */
					MRAMWriteData(METER_PARM_ADDR + sizeof(T_MeterParm)*k, 0, sizeof(T_MeterParm));
				}
			}
			MemoryBufferWrite(MRAM, (uint8_t *)&MeterTotal, METER_TOTAL_ADDR, 2);  /* 保存管理表具总数量 */
		}
		else if (TempID == 0x2305)	/* 配置集中器管理表具地址及节点 */
		{
			if (ptNetHead->DataPacketSeq[0] == 0x01)  /* 第一帧，清0表具总数量及复位表具地址存储区 */
			{
				unsigned char *p_extram = (unsigned char *)(STM32_EXT_SRAM_BEGIN + METER_ADDR);
				
				MeterTotal = 0;
				for (k = 0; k < 4*METER_SUM; k++)
				{
					*p_extram++ = (unsigned char)0xFF;
				}
			}
			
			MeterAddSum = buffer[22]/6;
			for (i = 0; i < MeterAddSum; i++)
			{
				k = *((uint16_t *)(buffer+27+6*i));
				if (k > METER_SUM)
				{
					CommonBack = 0;
					ptNetHead->DataLen = 0x02;
					*((uint16_t *)(NetSendBuffer+23)) = 0x0115;
					memset(NetSendBuffer+27, 0x16, 1);
					NetSendSize = 28;
					break;
				}
				MemoryBufferWrite(MRAM, buffer+23+6*i, METER_ADDR+4*k, 4);
				MeterTotal++;
			}
			MemoryBufferWrite(MRAM, (uint8_t *)&MeterTotal, METER_TOTAL_ADDR, 2);  /* 保存管理表具总数量 */
		}
		else if (TempID == 0x2306)	/* 读取集中器管理表具节点号 */
		{
			CommonBack  = 0;
			NetSendFlag = 0;
			memset(&GPRSMultiPacket, 0, sizeof(T_MultiPacket));
			memcpy((uint8_t *)&GPRSMultiPacket.NetHead, buffer, 23);
			GPRSMultiPacket.NetHead.CtrlArea = 0x02;
			if (MeterTotal%NODE_PACKET_NUM)
			{
				GPRSMultiPacket.NetHead.DataPacketSeq[1] = MeterTotal/NODE_PACKET_NUM + 1;
			}
			else
			{
				GPRSMultiPacket.NetHead.DataPacketSeq[1] = MeterTotal/NODE_PACKET_NUM;
			}
			GPRSMultiPacket.PacketType  = PACKET_NODE;
			GPRSMultiPacket.PacketState = PACKET_STATE_INIT;
			if (GPRSMultiPacket.NetHead.DataPacketSeq[1] == 0)  /* 集中器没有管理表具 */
			{
				NetSendFlag = 1;
				ptNetHead->DataLen = 0x02;
				*((uint16_t *)(NetSendBuffer+23)) = 0x0115;  /* error code */
				memset(NetSendBuffer+27, 0x16, 1);
				NetSendSize = 28;
				memset(&GPRSMultiPacket, 0, sizeof(T_MultiPacket));
			}
		}
		else if (TempID == 0x2307)  /* 配置表具价格类别 */
		{
			uint8_t m = 0;
			T_ESTASK tESTask;
			
			MeterAddSum = (buffer[22] - 4) / 5;
			for (i = 0; i < MeterAddSum; i++)
			{
				if ( (k = FindMeterID(buffer+23+5*i)) != METER_SUM )  /* 存储器中存在要增加的表号 */
				{
					MemoryBufferRead(MRAM, (uint8_t *)&tESTask, ESTASK_ADDR + sizeof(T_ESTASK)*k, sizeof(T_ESTASK));
					tESTask.PriceCalss = *(buffer+27+5*i);
					MemoryBufferWrite(MRAM, (uint8_t *)&tESTask, ESTASK_ADDR+sizeof(T_ESTASK)*k, sizeof(T_ESTASK));
				}
				else  /* 存储器中不存在要增加的表号 */
				{
					CommonBack = 0;
					memcpy(NetSendBuffer+26+4*m, buffer+23+5*i, 4);  /* 集中器地址 */
					m++;
				}
			}
			
			if (CommonBack == 0)
			{
				ptNetHead->DataLen = 4*m + 3;
				*((uint16_t *)(NetSendBuffer+23)) = 0x0118;  /* error code */
				memset(NetSendBuffer+23, 0x00, 2);
				memset(NetSendBuffer+25, m, 1);    /* 表具数量 */
				memset(NetSendBuffer+28+4*m, 0x16, 1);
				NetSendSize = 4*m + 29;
			}
		}
		else if (TempID == 0x2308)  /* 查询表具价格类别 */
		{
			uint8_t m = 0;
			T_ESTASK tESTask;
			
			MeterAddSum = buffer[22] / 4;
			for (i = 0; i < MeterAddSum; i++)
			{
				if ( (k = FindMeterID(buffer+23+4*i)) != METER_SUM )  /* 存储器中存在查询的表号 */
				{
					MemoryBufferRead(MRAM, (uint8_t *)&tESTask, ESTASK_ADDR + sizeof(T_ESTASK)*k, sizeof(T_ESTASK));
					memcpy(NetSendBuffer+26+5*m, buffer+23+4*i, 4);  /* 集中器地址 */
					*(NetSendBuffer+30+5*i) = tESTask.PriceCalss;
					m++;
				}
				else  /* 存储器中不存在查询的表号 */
				{
					memcpy(NetSendBuffer+26+5*m, buffer+23+4*i, 4);  /* 集中器地址 */
					*(NetSendBuffer+30+5*i) = 0x00;
					m++;
				}
			}
			CommonBack = 0;
			ptNetHead->DataLen = 5*m + 3;
			memset(NetSendBuffer+23, 0x00, 2);
			memset(NetSendBuffer+25, m, 1);    /* 表具数量 */
			memset(NetSendBuffer+28+5*m, 0x16, 1);
			NetSendSize = 5*m + 29;
		}
		else if (TempID == 0x2402)	/* 中心主动获取集中器管理的表具上报信息 */
		{
			uint16_t sum = 0;
			uint32_t ReadAddr;
			T_MeterParm tMeterParm;
			
			CommonBack  = 0;
			NetSendFlag = 0;
			memset(&GPRSMultiPacket, 0, sizeof(T_MultiPacket));
			memcpy((uint8_t *)&GPRSMultiPacket.NetHead, buffer, 23);
			GPRSMultiPacket.NetHead.CtrlArea = 0x02;
			for (k = 0; k < METER_SUM; k++)
			{
				ReadAddr = METER_PARM_ADDR + sizeof(T_MeterParm)*k;
				MemoryBufferRead(MRAM, (uint8_t *)&tMeterParm, ReadAddr, sizeof(T_MeterParm));
				if (tMeterParm.Flag != 0)
				{
					sum++;
				}
			}
			
			if (sum%FETCH_PACKET_NUM)
			{
				GPRSMultiPacket.NetHead.DataPacketSeq[1] = sum/FETCH_PACKET_NUM + 1;
			}
			else
			{
				GPRSMultiPacket.NetHead.DataPacketSeq[1] = sum/FETCH_PACKET_NUM;
			}
			GPRSMultiPacket.PacketType  = PACKET_FETCH;
			GPRSMultiPacket.PacketState = PACKET_STATE_INIT;
			if (GPRSMultiPacket.NetHead.DataPacketSeq[1] == 0)  /* 集中器没有管理表具 */
			{
				NetSendFlag = 1;
				ptNetHead->DataLen = 0x02;
				NetSendBuffer[23]  = 0x00;			/* 2 bytes length */
				memset(NetSendBuffer+24, 0x02, 1);  /* error code 0121 */
				memset(NetSendBuffer+25, 0x01, 1);
				memset(NetSendBuffer+28, 0x16, 1);
				NetSendSize = 29;
				memset(&GPRSMultiPacket, 0, sizeof(T_MultiPacket));
			}
		}
		else if (TempID == 0x2406)  /* take single meter report information */
		{
			uint32_t ReadAddr;
			T_MeterParm tMeterParm;
			
			CommonBack = 0;
			if ( (k = FindMeterID(ptNetHead->DstAddr)) != METER_SUM )
			{
				ReadAddr = METER_PARM_ADDR + sizeof(T_MeterParm)*k;
				MemoryBufferRead(MRAM, (uint8_t *)&tMeterParm, ReadAddr, sizeof(T_MeterParm));
				if (tMeterParm.Flag != 0)
				{
					memcpy(NetSendBuffer+29, (uint8_t *)&tMeterParm.MeterAddr, sizeof(T_MeterParm)-1);
					ptNetHead->DataLen  = sizeof(T_MeterParm) + 5;
					memset(NetSendBuffer+23, 0x00, 2);		  			    // 2 bytes confirm
					memcpy(NetSendBuffer+25, ConStatus, 2);	  			    // concentrator status
					memcpy(NetSendBuffer+27, (uint8_t *)&MeterTotal, 2);    // meter total numbers
					memset(NetSendBuffer+sizeof(T_MeterParm)+30, 0x16, 1);  // end
					NetSendSize = sizeof(T_MeterParm) + 31;
				}
				else
				{
					ptNetHead->DataLen = 0x02;
					*((uint16_t *)(NetSendBuffer+23)) = 0x0110;  /* 2 bytes error code */
					memset(NetSendBuffer+27, 0x16, 1);
					NetSendSize = 28;
				}
			}
			else
			{
				ptNetHead->DataLen = 0x02;
				*((uint16_t *)(NetSendBuffer+23)) = 0x0118;  /* 2 bytes error code */
				memset(NetSendBuffer+27, 0x16, 1);
				NetSendSize = 28;
			}
		}
		else if (TempID == 0x2601)	/* 集中器异常事件上传 */
		{
			ptNetHead->DataLen = 0x02;
			memcpy(NetSendBuffer+23, ConStatus, 2);  /* 2 bytes concentrator status code */
			memset(NetSendBuffer+27, 0x16, 1);
			NetSendSize = 28;
		}
		else if (TempID == 0x3002)	/* 表具安装参数配置 */
		{
			if (SystemTime.hour>=0x06 && SystemTime.hour<=0x23)	 /* 判断是否为扫频时间段 */
			{
				for (k = 0; k < METER_SUM; k++)
				{
					MemoryBufferRead(MRAM, MeterAddrTemp, METER_ADDR+4*k, 4);
					if (MeterAddrTemp[0] == 0xFF)  /* 为空节点 */
					{
						break;
					}
				}
				
				if (k != METER_SUM)
				{
					InstallParm.Flag = 1;
					RadioState.Mode  = S_MODE;
					RadioState.Stage = STAGE_ENTER;
					memcpy(&InstallHead, buffer, sizeof(T_NetHead));
					memcpy(&InstallParm.MeterInstallParm, buffer+23, sizeof(T_MeterInstallParm));
					MemoryBufferWrite(MRAM, (uint8_t *)&InstallParm, METER_INSTALL_ADDR, sizeof(T_InstallParm));
				}
				else  /* 集中器管理表具数量已满 */
				{
					CommonBack = 0;
					ptNetHead->DataLen = 0x02;
					*((uint16_t *)(NetSendBuffer+23)) = 0x0101;  /* 2 bytes error code */
					memset(NetSendBuffer+27, 0x16, 1);
					NetSendSize = 28;
				}
			}
			else
			{
				CommonBack = 0;
				ptNetHead->DataLen = 0x02;
				memset(NetSendBuffer+23, 0x13, 1);  /* time error code */
				memset(NetSendBuffer+24, 0x01, 1);
				memset(NetSendBuffer+27, 0x16, 1);
				NetSendSize = 28;
			}
		}
		else if (TempID == 0x3003)	/* 退出表具安装模式 */
		{
			RadioState.Mode  = S_MODE;
			RadioState.Stage = STAGE_EXIT;
		}
		else
		{
			if (XmodeCfg.Enable == 0)
			{
				if ( FindMeterID(ptNetHead->DstAddr) != METER_SUM )
				{
					GPRSGetFlag = 1;  /* 表号存在，有Radio任务需要处理 */
				}
				else
				{
					CommonBack = 0;
					ptNetHead->DataLen = 0x02;
					memset(NetSendBuffer+23, 0x18, 1);	/* error code */
					memset(NetSendBuffer+24, 0x01, 1);
					memset(NetSendBuffer+27, 0x16, 1);
					NetSendSize = 28;
				}
			}
			else
			{
				if (XmodeCfg.Data[0] == 0)
				{
					memcpy(XmodeCfg.Data, buffer, size);
				}
				else
				{
					CommonBack = 0;
					ptNetHead->DataLen = 0x02;
					memset(NetSendBuffer+23, 0x21, 1);  /* busy code */
					memset(NetSendBuffer+24, 0x01, 1);
					memset(NetSendBuffer+27, 0x16, 1);
					NetSendSize = 28;
				}
			}
		}
		
		if (CommonBack == 1)  /* common reply */
		{
			ptNetHead->DataLen = 0x02;
			memset(NetSendBuffer+23, 0x00, 2);
			memset(NetSendBuffer+27, 0x16, 1);
			NetSendSize = 28;
		}
		
		if (NetSendFlag == 1)  /* send GPRS data */
		{
			ptNetHead->CtrlArea = 0x02;
			NetComputeCRC(NetSendBuffer, NetSendSize-3, NetSendBuffer+NetSendSize-3);
			USART2_SendData(NetSendBuffer, NetSendSize);
			DATA_PRINTF(NetSendBuffer, NetSendSize);
			if (ResetFlag == 1)
			{
				Soft_delay_ms(2500);
				SoftReset();
			}
		}
	}
	else if (buffer[3] == 0x04)	 /* 收到中心的确认，删除上传任务 */
	{
		while (ptNetDel)
		{
			if (ptNetDel->ID==TempID && CompareData(ptNetDel->DstAddr, buffer+6, 4)==true && CompareData(ptNetDel->DataPacketID, buffer+18, 2)==true)
			{
				DelTask(NET_TASK, ptNetDel);  /* delete net task */
				break;
			}
			else
			{
				ptNetDel = ptNetDel->ptNext;
			}
		}
	}
}

/* 打包上传至中心 */
void GPRSMultiPacketProcess(void)
{
	uint8_t m;
	uint32_t ReadAddr;
	T_MeterParm tMeterParm;
	uint8_t	MeterAddrTemp[4];
	static uint32_t MultiPacketTimer;
	
	if ((GPRSMultiPacket.PacketType != PACKET_IDLE) && (GPRSMultiPacket.PacketState != PACKET_STATE_IDLE))
	{
		if (GPRSMultiPacket.PacketState == PACKET_STATE_INIT)
		{
			MultiPacketTimer = GET_TICK_COUNT();
			GPRSMultiPacket.PacketState = PACKET_STATE_RUN;
		}
			
		if (GPRSMultiPacket.PacketType == PACKET_NODE)  /* 2306提取表具地址及节点 */
		{
			if ((GET_TICK_COUNT() - MultiPacketTimer) > TICK_RATE_MS(20))
			{
				MultiPacketTimer = GET_TICK_COUNT();
				GPRSMultiPacket.NetHead.DataPacketSeq[0]++;
				for (m = 0; GPRSMultiPacket.Node<METER_SUM && m<NODE_PACKET_NUM; GPRSMultiPacket.Node++)
				{
					MemoryBufferRead(MRAM, MeterAddrTemp, METER_ADDR+4*GPRSMultiPacket.Node, 4);
					if (MeterAddrTemp[0] != 0xFF)
					{
						memcpy(NetSendBuffer+26+6*m, MeterAddrTemp, 4);
						*((uint16_t *)(NetSendBuffer+30+6*m)) = GPRSMultiPacket.Node;
						m++;
					}
				}
				GPRSMultiPacket.NetHead.DataLen = 6*m + 3;
				memcpy(NetSendBuffer, (uint8_t *)&GPRSMultiPacket.NetHead, 23);
				memset(NetSendBuffer+23, 0x00, 2);		// 2 bytes confirm
				memset(NetSendBuffer+25, m, 1);		    // 本包上传表具数量
				memset(NetSendBuffer+28+6*m, 0x16, 1);	// end
				NetSendSize = 29 + 6*m;
				NetComputeCRC(NetSendBuffer, NetSendSize-3, NetSendBuffer+NetSendSize-3);  // create CRC
				USART2_SendData(NetSendBuffer, NetSendSize);
				DATA_PRINTF(NetSendBuffer, NetSendSize);
			}
		}
		else if (GPRSMultiPacket.PacketType == PACKET_FETCH)  /* 2402提取表具信息 */
		{
			if ((GET_TICK_COUNT() - MultiPacketTimer) > TICK_RATE_MS(20))
			{
				MultiPacketTimer = GET_TICK_COUNT();
				GPRSMultiPacket.NetHead.DataPacketSeq[0]++;
				for (m = 0; GPRSMultiPacket.Node<METER_SUM && m<FETCH_PACKET_NUM; GPRSMultiPacket.Node++)
				{
					ReadAddr = METER_PARM_ADDR + sizeof(T_MeterParm)*GPRSMultiPacket.Node;
					MemoryBufferRead(MRAM, (uint8_t *)&tMeterParm, ReadAddr, sizeof(T_MeterParm));
					if (tMeterParm.Flag != 0)
					{
						memcpy(NetSendBuffer+31+m*(sizeof(T_MeterParm)-1), (uint8_t *)&tMeterParm.MeterAddr, sizeof(T_MeterParm)-1);
						m++;
					}
				}
				*((uint16_t *)(NetSendBuffer+22)) = m*(sizeof(T_MeterParm)-1) + 7;
				memcpy(NetSendBuffer, (uint8_t *)&GPRSMultiPacket.NetHead, 22);
				memset(NetSendBuffer+24, 0x00, 2);		                      // 2 bytes confirm
				memcpy(NetSendBuffer+26, ConStatus, 2);                       // concentrator status
				memcpy(NetSendBuffer+28, (uint8_t *)&MeterTotal, 2);          // meter total numbers
				memset(NetSendBuffer+30, m, 1);		                          // current packet meter number
				memset(NetSendBuffer+33+m*(sizeof(T_MeterParm)-1), 0x16, 1);  // end
				NetSendSize = 34 + m*(sizeof(T_MeterParm)-1);
				NetComputeCRC(NetSendBuffer, NetSendSize-3, NetSendBuffer+NetSendSize-3);  // create CRC
				USART2_SendData(NetSendBuffer, NetSendSize);
				DATA_PRINTF(NetSendBuffer, NetSendSize);
			}
		}
		
		if ((GPRSMultiPacket.NetHead.DataPacketSeq[0] == GPRSMultiPacket.NetHead.DataPacketSeq[1]) || \
			(GET_TICK_COUNT() - MultiPacketTimer) > TICK_RATE_MS(30000))  /* 30s over time */
		{
			memset(&GPRSMultiPacket, 0, sizeof(T_MultiPacket));
		}
	}
}


uint8_t GPRSSendRec(uint8_t *str, uint8_t *find, uint16_t time, uint8_t num)
{
	uint8_t result = GPRS_BUSY;
	static uint8_t  SendNum = 0;
	static uint32_t TxTimeoutTimer;
    static enum
    {
        STATE_SEND,
        STATE_RUNNING
    }GPRS_SEND_RECV_STATE = STATE_SEND;
	
    switch(GPRS_SEND_RECV_STATE)
	{
		case STATE_SEND:
			memset(GPRSRecvBuffer, 0, GPRS_BUFFER_SIZE);
			USART2_SendString(str);
			TxTimeoutTimer = GET_TICK_COUNT();
			GPRS_SEND_RECV_STATE = STATE_RUNNING;
			break;
		
		case STATE_RUNNING:
			if ((GET_TICK_COUNT()-TxTimeoutTimer) < TICK_RATE_MS(time))
			{
				if (GPRSRecvFlag == 1)
				{
					GPRSRecvFlag = 0;
					DBG_PRINTF("%s \n", GPRSRecvBuffer);
					
					if (LookForStr(GPRSRecvBuffer, find) != FULL)
					{
						SendNum = 0;
						memset(GPRSRecvBuffer, 0, GPRS_BUFFER_SIZE);
						GPRS_SEND_RECV_STATE = STATE_SEND;
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
				SendNum++;
				if (SendNum == num)
				{
					SendNum = 0;
					DBG_PRINTF("%s send error!\n", str);
					memset(GPRSRecvBuffer, 0, GPRS_BUFFER_SIZE);
					result = GPRS_ERROR;
				}
				GPRSRecvFlag = 0;
				GPRS_SEND_RECV_STATE = STATE_SEND;
			}
			break;
		
		default:
			break;
	}
	
	return result;
}


void GPRSSendData(uint8_t *data, uint8_t size)
{
	GPRSSendFlag = 1;
	GPRSSendSize = size;
	memcpy((void *)GPRSSendBuffer, (void *)data, (size_t)GPRSSendSize);
}


void GPRSRecvData(uint8_t *data, uint8_t *size)
{
	if (GPRSGetFlag == 1)
	{
		GPRSGetFlag = 0;
		*size = GPRSRecvSize;
		memcpy((void *)data, (void *)GPRSRecvBuffer, (size_t)GPRSRecvSize);
	}
}


void GPRSInit(void)
{
	USART2_Init();
	
	GPRSIOInit();
}


int GPRSRegister(void)
{
	return RegisterNetDriver(&GPRSDriver);
}


/* 获取集中器RF模块所处状态 */
void GetRFState(void)
{
	if (RadioState.Mode == S_MODE)
	{
		HeartBeat.RFState = 0x01;
	}
	else if (XmodeCfg.Enable == 1)
	{
		HeartBeat.RFState = 0x02;
	}
	else
	{
		HeartBeat.RFState = 0x00;
	}
}


// USART2中断
void USART2_IRQHandler(void)
{
	uint8_t num;
	
	if (USART_GetITStatus(USART2, USART_IT_IDLE) == SET)
	{
		num = num;
		num = USART2->SR;
		num = USART2->DR;
		DMA_Cmd(DMA1_Channel6, DISABLE);
		GPRSRecvSize = GPRS_BUFFER_SIZE - DMA_GetCurrDataCounter(DMA1_Channel6);
		DMA1_Channel6->CNDTR = GPRS_BUFFER_SIZE;	/* 定义指定DMA通道的缓存的大小 */
		DMA_Cmd(DMA1_Channel6, ENABLE);
		GPRSRecvFlag = 1;
	}
}
