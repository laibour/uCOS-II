#include "sys_config.h"
#include "sx1278a.h"
#include "sx1278b.h"
#include "radio.h"


static char SX1278A[] = "SX1278A";
static char SX1278B[] = "SX1278B";

static uint8_t SX1278ASendSize  = 0;				   /* RFA send size */
static uint16_t SX1278ARecvSize = 0;				   /* RFA receive size */
static uint8_t SX1278ASendBuffer[SX1278_BUFFER_SIZE];  /* RFA send buffer */
static uint8_t SX1278ARecvBuffer[SX1278_BUFFER_SIZE];  /* RFA receive buffer */

T_XmodeCfg XmodeCfg;								   /* X_mode 配置数据结构体 */
T_RadioState RadioState;							   /* SX1278A 发送模式、阶段、步骤结构体 */
static T_RFLED RFLED;								   /* RF up/down led blink */
static T_RadioSendData RadioSendData;				   /* Radio 待发送数据结构体 */
static PT_RadioDriver g_ptRadioDriverHead;
static tSX1278States SX1278ATaskState = SX1278_STATE_IDLE;
static tSX1278States SX1278BTaskState = SX1278_STATE_RX_INIT;

void RadioPutProcess(void)
{
	PT_Task ptRadioTmp = g_ptRadioTaskHead;
	
	switch(RadioState.Mode)
	{
		case S_MODE:
			switch(RadioState.Stage)
			{
				case STAGE_ENTER:
					if (ConParm.RFChannel != InstallParm.MeterInstallParm.RFChannel)  /* 需要更新中心频点 */
					{
						ConParm.RFChannel = InstallParm.MeterInstallParm.RFChannel;
						MemoryBufferWrite(MRAM, (uint8_t *) &ConParm, CON_PARM_ADDR, sizeof(ConParm));
						Y_Wakeup_LoRaSettings.RFFrequency = LORA_FRE_BASE + (ConParm.RFChannel * LORA_FRE_STEP);
						Y_Normal_LoRaSettings.RFFrequency = Y_Wakeup_LoRaSettings.RFFrequency;
					}
					RFChannelFlag = 0;
					ScanDataConfig();
					LoRaSettings = Y_Normal_LoRaSettings;
					SX1278ALoRaInit();
					SX1278ATaskState = SX1278_STATE_TX_INIT;
					RadioState.Stage = STAGE_RUNNING;
					break;
				
				case STAGE_RUNNING:
					if (SystemTime.hour==0x23 && SystemTime.minute==0x59)  /* 每天23:59时主动退出扫频模式 */
					{
                        RadioState.Stage = STAGE_EXIT;
					}
					break;
					
				case STAGE_EXIT:
					RadioState.Mode  = Y_MODE;
					RadioState.Stage = STAGE_ENTER;
					SX1278ATaskState = SX1278_STATE_IDLE;
					break;
				
				default:
					break;
			}
			break;
			
		case X_MODE:
			switch(RadioState.Stage)
			{
				case STAGE_ENTER:
					if ( XmodeDataConfig(XmodeCfg.Data) )
					{
						LoRaSettings = X_Wakeup_LoRaSettings;
						SX1278ALoRaInit();
						SX1278ASendSize = 1;
						memset(SX1278ASendBuffer, 0x03, 1);	 /* 仅为唤醒作用，0x03无意义 */
						SX1278ATaskState = SX1278_STATE_TX_INIT;
						RadioState.Step  = STEP_WAKEUP;
						RadioState.Stage = STAGE_RUNNING;
					}
					else
					{
						RadioState.Mode  = Y_MODE;
						RadioState.Stage = STAGE_ENTER;
						SX1278ATaskState = SX1278_STATE_IDLE;
					}
					break;
				
				case STAGE_RUNNING:
					if (XmodeCfg.Data[0] == 0)
					{
						RadioState.Mode  = Y_MODE;
						RadioState.Stage = STAGE_ENTER;
						SX1278ATaskState = SX1278_STATE_IDLE;
					}
					break;
				
				default:
					break;
			}
			break;
		
		case Y_MODE:
			switch(RadioState.Stage)
			{
				case STAGE_ENTER:
					RadioDeviceInit();
					SX1278ATaskState = SX1278_STATE_RX_INIT;
					RadioState.Stage = STAGE_RUNNING;
					break;
				
				case STAGE_RUNNING:
					if (SystemTime.hour>=0x06 && SystemTime.hour<=0x23)	 /* 在X、Z模式时间段判断是否进入X、Z模式 */
					{
						if (SystemTime.second==0x58 && (SystemTime.minute&0x0F)==0x09 && g_ptRadioTaskHead)	 /* Z模式 */
						{
							RadioState.Mode  = Z_MODE;
							RadioState.Stage = STAGE_ENTER;
							SX1278ATaskState = SX1278_STATE_IDLE;
						}
						else if (XmodeCfg.Data[0] != 0)	 /* X模式 */
						{
							RadioState.Mode  = X_MODE;
							RadioState.Stage = STAGE_ENTER;
							SX1278ATaskState = SX1278_STATE_IDLE;
						}
					}
					else if (XmodeCfg.Data[0] != 0)	 /* X模式 */
					{
						RadioState.Mode  = X_MODE;
						RadioState.Stage = STAGE_ENTER;
						SX1278ATaskState = SX1278_STATE_IDLE;
					}
					break;
				
				default:
					break;
			}
			break;
			
		case Z_MODE:
			switch(RadioState.Stage)
			{
				case STAGE_ENTER:
					if (GetRadioData() == TASK_DATA_DONE)
					{
						LoRaSettings = Y_Wakeup_LoRaSettings;
						SX1278ALoRaInit();
						SX1278ASendSize = 1;
						memset(SX1278ASendBuffer, 0x03, 1);
						SX1278ATaskState = SX1278_STATE_TX_INIT;
						RadioState.Step  = STEP_WAKEUP;
						RadioState.Stage = STAGE_RUNNING;
					}
					else
					{
						RadioState.Mode  = Y_MODE;
						RadioState.Stage = STAGE_ENTER;
						SX1278ATaskState = SX1278_STATE_IDLE;
					}
					break;
				
				case STAGE_RUNNING:
					if (GetRadioData() == TASK_DATA_DONE)
					{
						LoRaSettings = Y_Normal_LoRaSettings;
						SX1278ALoRaInit();
						memcpy(SX1278ASendBuffer, RadioSendData.Data, RadioSendData.Len);
						SX1278ASendSize  = RadioSendData.Len;
						SX1278ATaskState = SX1278_STATE_TX_INIT;
						RadioState.Step  = STEP_HANDLE;
					}
					else if (GetRadioData() == TASK_DATA_NULL)
					{
						/* 一轮完成后，将挂起的任务置为准备状态 */
						while (ptRadioTmp)
						{
							if (ptRadioTmp->TaskState == TASK_SUSPEND)
							{
								ptRadioTmp->TaskState = TASK_READY;
							}
							else
							{
								ptRadioTmp = ptRadioTmp->ptNext;
							}
						}
						
						RadioState.Mode  = Y_MODE;
						RadioState.Stage = STAGE_ENTER;
						SX1278ATaskState = SX1278_STATE_IDLE;
					}
					break;
				
				default:
					break;
			}
			break;
		
		default:
			break;
	}
}

/* SX1278A task process */
void SX1278ATaskProcess(void)
{
	uint16_t TempID;
	static uint8_t RadioSendNum = 0;
	static uint32_t SX1278ATimeoutTimer;
	PT_Task ptRadioCur = g_ptRadioTaskHead;
	
	switch(SX1278ATaskState)
	{
		case SX1278_STATE_IDLE:
			 break;
		
		case SX1278_STATE_TX_INIT:
			RadioSetTxPacket(SX1278A, SX1278ASendBuffer, SX1278ASendSize);
			DATA_PRINTF(SX1278ASendBuffer, SX1278ASendSize);
			SX1278ATimeoutTimer = GET_TICK_COUNT();
			SX1278ATaskState    = SX1278_STATE_TX_RUNNING;
			break;
			
		case SX1278_STATE_TX_RUNNING:
			if ((GET_TICK_COUNT() - SX1278ATimeoutTimer) < TICK_RATE_MS(10000))
			{
				if (RadioProcess(SX1278A) == RF_TX_DONE)
				{
					SX1278ATimeoutTimer = GET_TICK_COUNT();
					SX1278ATaskState    = SX1278_STATE_TX_DONE;
				}
			}
			else
			{
				SX1278ATaskState = SX1278_STATE_TX_TIMEOUT;
			}
			break;
		
		case SX1278_STATE_TX_DONE:
			RFLED.UpFlag = 1;
			switch(RadioState.Mode)
			{
				case X_MODE:
					if (RadioState.Step == STEP_WAKEUP)
					{
						if ((GET_TICK_COUNT() - SX1278ATimeoutTimer) > TICK_RATE_MS(400))
						{
							LoRaSettings = X_Normal_LoRaSettings;
							SX1278ALoRaInit();
							memcpy(SX1278ASendBuffer, XmodeCfg.Wakeup, 9);
							SX1278ASendSize  = 9;
							SX1278ATaskState = SX1278_STATE_TX_INIT;
							RadioState.Step  = STEP_SETTING;
						}
					}
					else if (RadioState.Step == STEP_SETTING)
					{
						if ((GET_TICK_COUNT() - SX1278ATimeoutTimer) > TICK_RATE_MS(400))
						{
							RadioSendData.Data[5] = 0xE2;
							if (*((uint16_t *)(RadioSendData.Data)) == 0x2211)
							{
								memcpy(RadioSendData.Data+11, (uint8_t *)&SystemTime, 7);  /* system time */
							}
							GenerateSum(RadioSendData.Data, RadioSendData.Len - 1);
							memcpy(SX1278ASendBuffer, RadioSendData.Data, RadioSendData.Len);
							SX1278ASendSize  = RadioSendData.Len;
							SX1278ATaskState = SX1278_STATE_TX_INIT;
							RadioState.Step  = STEP_HANDLE;
						}
					}
					else
					{
						RadioState.Step = STEP_IDLE;
						SX1278ATaskState = SX1278_STATE_RX_INIT;
					}
					break;
				
				case Z_MODE:
					if (RadioState.Step == STEP_WAKEUP)
					{
						LoRaSettings = Y_Normal_LoRaSettings;
						SX1278ALoRaInit();
						memcpy(SX1278ASendBuffer, RadioSendData.Data, RadioSendData.Len);
						SX1278ASendSize  = RadioSendData.Len;
						SX1278ATaskState = SX1278_STATE_TX_INIT;
						RadioState.Step  = STEP_HANDLE;
					}
					else
					{
						SX1278ATaskState = SX1278_STATE_RX_INIT;
					}
					break;
				
				default:
					SX1278ATaskState = SX1278_STATE_RX_INIT;
					break;
			}
			break;
		
		case SX1278_STATE_TX_TIMEOUT:
			SX1278ATaskState = SX1278_STATE_TX_INIT;
			break;
		
		case SX1278_STATE_RX_INIT:
			RadioSetRxMode(SX1278A);
			SX1278ATimeoutTimer = GET_TICK_COUNT();
			SX1278ATaskState    = SX1278_STATE_RX_RUNNING;
			break;
		
		case SX1278_STATE_RX_RUNNING:
			if (RadioState.Mode == Y_MODE)
			{
				if (RadioProcess(SX1278A) == RF_RX_DONE)
				{
					SX1278ATaskState = SX1278_STATE_RX_DONE;
				}
				else if (RFChannelFlag == 1)  /* 修改中心频点 */
				{
					RFChannelFlag = 0;
					Y_Wakeup_LoRaSettings.RFFrequency = LORA_FRE_BASE + (ConParm.RFChannel * LORA_FRE_STEP);
					Y_Normal_LoRaSettings.RFFrequency = Y_Wakeup_LoRaSettings.RFFrequency;
					LoRaSettings = Y_Normal_LoRaSettings;
					SX1278ALoRaInit();
					SX1278ATaskState = SX1278_STATE_RX_INIT;
				}
			}
			else if (RadioState.Mode == S_MODE)	 /* 扫频时等待4.2s */
			{
				if ((GET_TICK_COUNT() - SX1278ATimeoutTimer) < TICK_RATE_MS(4200))
				{
					if (RadioProcess(SX1278A) == RF_RX_DONE)
					{
						SX1278ATaskState = SX1278_STATE_RX_DONE;
					}
				}
				else
				{
					SX1278ATaskState = SX1278_STATE_RX_TIMEOUT;
				}
			}
			else
			{
				if ((GET_TICK_COUNT() - SX1278ATimeoutTimer) < TICK_RATE_MS(3000))
				{
					if (RadioProcess(SX1278A) == RF_RX_DONE)
					{
						SX1278ATaskState = SX1278_STATE_RX_DONE;
					}
				}
				else
				{
					SX1278ATaskState = SX1278_STATE_RX_TIMEOUT;
				}
			}
			break;
		
		case SX1278_STATE_RX_DONE:
			RadioGetRxPacket(SX1278A, SX1278ARecvBuffer, (uint16_t*)&SX1278ARecvSize);
			if ( CheckSum(SX1278ARecvBuffer, SX1278ARecvSize-1) )
			{
				RFLED.DownFlag = 1;
				memcpy(&TempID, SX1278ARecvBuffer, 2);
				switch(RadioState.Mode)
				{
					case S_MODE:
                        if (TempID == 0x2405)  /* 2405 feedback */
						{
							if ( SX1278AFeedback(SX1278ARecvBuffer) )
							{
								SX1278ATaskState = SX1278_STATE_TX_INIT;  /* 表具存在，回复 */
							}
							else
							{
								SX1278ATaskState = SX1278_STATE_RX_INIT;  /* 表具不存在，不回复 */
							}
							
							RadioGetProcess(SX1278ARecvBuffer, SX1278ARecvSize);
						}
						else if (TempID==0x3002 && SX1278ARecvBuffer[11]==0x64)  /* 收到扫频请求数据 */
						{
							InstallDataConfig(SX1278ARecvBuffer);
							memcpy(SX1278ASendBuffer, RadioSendData.Data, RadioSendData.Len);
							SX1278ASendSize  = RadioSendData.Len;
							SX1278ATaskState = SX1278_STATE_TX_INIT;
						}
						else  /* 扫频成功或收到其他信息，处理后继续扫频 */
						{
							ScanDataConfig();
							RadioGetProcess(SX1278ARecvBuffer, SX1278ARecvSize);
							SX1278ATaskState = SX1278_STATE_TX_INIT;
						}
						break;
					
					case X_MODE:
						SX1278ARecvBuffer[5] = 0xA1;
						RadioGetProcess(SX1278ARecvBuffer, SX1278ARecvSize);
						memset(XmodeCfg.Data, 0, X_MODE_SIZE);
						SX1278ATaskState = SX1278_STATE_IDLE;  /* SX1278A设置为空闲态 */
						break;
					
					case Y_MODE:
						if (TempID == 0x2405)  /* 2405 feedback */
						{
							if ( SX1278AFeedback(SX1278ARecvBuffer) )
							{
								SX1278ATaskState = SX1278_STATE_TX_INIT;  /* 表具存在，回复 */
							}
							else
							{
								SX1278ATaskState = SX1278_STATE_RX_INIT;  /* 表具不存在，不回复 */
							}
							
							RadioGetProcess(SX1278ARecvBuffer, SX1278ARecvSize);
						}
						else if (TempID == 0x2233)  /* 手持器信号测试 */
						{
							SX1278ARssiBack(SX1278ARecvBuffer);
							SX1278ATaskState = SX1278_STATE_TX_INIT;  /* 回复 */
						}
						else
						{
							RadioGetProcess(SX1278ARecvBuffer, SX1278ARecvSize);
							SX1278ATaskState = SX1278_STATE_RX_INIT;  /* 启动SX1278A接收 */
						}
						break;
					
					case Z_MODE:
						RadioSendNum = 0;
						RadioGetProcess(SX1278ARecvBuffer, SX1278ARecvSize);
						SX1278ATaskState = SX1278_STATE_IDLE;
						break;
					
					default:
						break;
				}
			}
			else  /* 校验错误 */
			{
				SX1278ATaskState = SX1278_STATE_RX_INIT;
			}
			break;
		
		case SX1278_STATE_RX_TIMEOUT:
			switch(RadioState.Mode)
			{
				case S_MODE:
					ScanDataConfig();
					SX1278ATaskState = SX1278_STATE_TX_INIT;
					break;
				
				case X_MODE:
					memcpy(SX1278ARecvBuffer,    XmodeCfg.Data+1,  2);  /* 超时无响应 */
					memcpy(SX1278ARecvBuffer+2,  XmodeCfg.Data+14, 4);
					memcpy(SX1278ARecvBuffer+6,  XmodeCfg.Data+6,  4);
					memset(SX1278ARecvBuffer+10, 0x00, 2);
					SX1278ARecvSize = 12;
					RadioGetProcess(SX1278ARecvBuffer, SX1278ARecvSize);
					memset(SX1278ARecvBuffer, 0, SX1278ARecvSize);
					memset(XmodeCfg.Data, 0, X_MODE_SIZE);
					SX1278ATaskState = SX1278_STATE_IDLE;
					break;
				
				case Y_MODE:
					SX1278ATaskState = SX1278_STATE_RX_INIT;
					break;
				
				case Z_MODE:
					while (ptRadioCur)
					{
						if (ptRadioCur->TaskState == TASK_RUNNING)
						{
							RadioSendNum++;
							if (RadioSendNum == 2)
							{
								RadioSendNum = 0;
								ptRadioCur->ProcTime += 1;  /* 处理次数 */
								if (ptRadioCur->ProcTime == RADIO_TASK_PROC_TIME)
								{
									memcpy(SX1278ARecvBuffer,    ptRadioCur->Data+1,  2);  /* 超时无响应 */
									memcpy(SX1278ARecvBuffer+2,  ptRadioCur->Data+14, 4);
									memcpy(SX1278ARecvBuffer+6,  ptRadioCur->Data+6,  4); 
									memset(SX1278ARecvBuffer+10, 0x00, 2);
									SX1278ARecvSize = 12;
									RadioGetProcess(SX1278ARecvBuffer, SX1278ARecvSize);
									memset(SX1278ARecvBuffer, 0, SX1278ARecvSize);
									break;
								}
								else
								{
									ptRadioCur->TaskState = TASK_SUSPEND;  /* 将任务挂起 */
								}
							}
							else
							{
								ptRadioCur->TaskState = TASK_READY;			// 将任务置为准备状态
							}
							break;
						}
						else
						{
							ptRadioCur = ptRadioCur->ptNext;
						}
					}
					
					SX1278ATaskState = SX1278_STATE_IDLE;
					break;
				
				default:
					break;
			}
			break;
		
		default:
			break;
	}
}

/* 2405接收数据处理 */
bool SX1278AFeedback(uint8_t *buffer)
{
	uint16_t k;
	T_ESTASK tESTask;
	T_RealPrice tRealPrice;
	T_PresetPrice tPresetPrice;
	
	/* 判断计算出节点地址 */
	if ( (k = FindMeterID(buffer+6)) != METER_SUM )
	{
		SaveReportPram(buffer, k);	/* 保存即时上传2405数据 */
		
		SaveMeterPram(buffer, k);	/* 保存打包上传2405数据 */
		
		MemoryBufferRead(MRAM, (uint8_t *)&tESTask, ESTASK_ADDR + k*sizeof(T_ESTASK), sizeof(T_ESTASK));
		if (tESTask.Sign != 0)
		{
			memcpy(SX1278ASendBuffer, buffer, 10);
			if ((tESTask.Sign & 0x01) == 0x01)	 // 3004开户
			{
				memset(SX1278ASendBuffer+10, 29, 1);				// data len
				memset(SX1278ASendBuffer+11, 0x04, 1);				// task flag
				memcpy(SX1278ASendBuffer+12, tESTask.OpenGas, 28);	// net receive data
				SX1278ASendSize = 41;
			}
			else if ((tESTask.Sign & 0x02) == 0x02)  // 5004即时调价
			{
				memset(SX1278ASendBuffer+10, 15, 1);
				memset(SX1278ASendBuffer+11, 0x05, 1);
				MemoryBufferRead(MRAM, (uint8_t *)&tRealPrice, REAL_PRICE_ADDR + tESTask.PriceCalss*sizeof(T_RealPrice), sizeof(T_RealPrice));  /* 读取价格 */
				memcpy(SX1278ASendBuffer+12, &tRealPrice, 14);  // net receive data
				SX1278ASendSize = 27;
			}
			else if ((tESTask.Sign & 0x04) == 0x04)	 // 5007充值
			{
				memset(SX1278ASendBuffer+10, 13, 1);				 // data len
				memset(SX1278ASendBuffer+11, 0x01, 1);				 // task flag
				memcpy(SX1278ASendBuffer+12, tESTask.Recharge, 12);	 // net receive data
				memcpy(SX1278ASendBuffer+17, (uint8_t *)&SystemTime, 7);  /* system time */
				SX1278ASendSize = 25;
			}
			else if ((tESTask.Sign & 0x08) == 0x08)  // 5009清标志
			{
				memset(SX1278ASendBuffer+10, 2, 1);					 // data len
				memset(SX1278ASendBuffer+11, 0x02, 1);				 // task flag
				memset(SX1278ASendBuffer+12, tESTask.ClearSign, 1);	 // net receive data
				SX1278ASendSize = 14;
			}
			else if ((tESTask.Sign & 0x10) == 0x10)  // 5005预调价
			{
				memset(SX1278ASendBuffer+10, 16, 1);	// data len
				memset(SX1278ASendBuffer+11, 0x03, 1);  // task flag
				MemoryBufferRead(MRAM, (uint8_t *)&tPresetPrice, PRESET_PRICE_ADDR + tESTask.PriceCalss*sizeof(T_PresetPrice), sizeof(T_PresetPrice));  /* 读取价格 */
				memcpy(SX1278ASendBuffer+12, &tPresetPrice, 15);  // net receive data
				SX1278ASendSize = 28;
			}
			else
			{
				tESTask.Sign = 0;
				MemoryBufferWrite(MRAM, (uint8_t *)&tESTask, ESTASK_ADDR+k*sizeof(T_ESTASK), sizeof(T_ESTASK));
				DBG_PRINTF("Received ID is not available.\n");
				return false;
			}
			GenerateSum(SX1278ASendBuffer, SX1278ASendSize-1);  // 生成校验和
			return true;
		}
		
		/* 2405主动上传没有任务发送，则正常回复 */
		memcpy(SX1278ASendBuffer, buffer, 10);					 // head
		memset(SX1278ASendBuffer+10, 0x08, 1);					 // length
		memset(SX1278ASendBuffer+11, 0x00, 1);					 // task flag
		memcpy(SX1278ASendBuffer+12, (uint8_t *)&SystemTime, 7); // system time
		SX1278ASendSize = 20;
		GenerateSum(SX1278ASendBuffer, SX1278ASendSize-1); 		 // Generate checksum
	}
	else
	{
		return false;
	}
	
	return true;
}

/* 2233手持器信号强度回复 */
void SX1278ARssiBack(uint8_t *buffer)
{
	uint8_t i, RecvRssi;
	
	memcpy(SX1278ASendBuffer, buffer, 10);	// head
	memset(SX1278ASendBuffer+10, 0xA0, 1);  // length
	for (i = 0; i < 4; i++)
	{
		SX1278ASendBuffer[11+i] = 0xA1 + i;
		SX1278ASendBuffer[15+i] = 0xB1 + i;
		SX1278ASendBuffer[19+i] = 0xC1 + i;
		SX1278ASendBuffer[23+i] = 0xD1 + i;
		SX1278ASendBuffer[27+i] = 0xE1 + i;
		SX1278ASendBuffer[31+i] = 0xF1 + i;
	}
	memcpy(SX1278ASendBuffer+35, (uint8_t *)&SystemTime, 7);  // system time
	SX1278ARead( REG_LR_PKTRSSIVALUE, &RecvRssi );
	memset(SX1278ASendBuffer+42, RecvRssi, 1);		  		  // 1 bytes recv rssi
	SX1278ASendSize = 44;
	GenerateSum(SX1278ASendBuffer, SX1278ASendSize-1); 		  // Generate checksum
}

/* 射频接收数据处理 */
void RadioGetProcess(uint8_t *buffer, uint8_t size)
{
	uint8_t i, temp;
	uint16_t k, TempID;
	T_ESTASK tESTask;
	PT_Task ptNetNew;
	uint8_t DataPacketFlag = 0;
	uint8_t DataPacketID[2] = {0};
	PT_RadioHead ptRadioHead;
	PT_Task ptRadioDel = g_ptRadioTaskHead;
	
	ptRadioHead = (PT_RadioHead)buffer;
	
	if ((ptRadioHead->FuncCode==0x3004) || (ptRadioHead->FuncCode==0x5004) || (ptRadioHead->FuncCode==0x5005) || (ptRadioHead->FuncCode==0x5007) || (ptRadioHead->FuncCode==0x5009))
	{
		if ( (k = FindMeterID(buffer+6)) != METER_SUM )
		{
			MemoryBufferRead(MRAM, (uint8_t *)&tESTask, ESTASK_ADDR + k*sizeof(T_ESTASK), sizeof(T_ESTASK));
			if (ptRadioHead->FuncCode == 0x3004)
			{
				tESTask.Sign &= 0xFE;
				memcpy(DataPacketID, tESTask.OGID, 2);  /* 缓存数据包ID */
			}
			else if (ptRadioHead->FuncCode == 0x5004)
			{
				tESTask.Sign &= 0xFD;
				memcpy(DataPacketID, tESTask.RPID, 2);
			}
			else if (ptRadioHead->FuncCode == 0x5005)
			{
				tESTask.Sign &= 0xEF;
				memcpy(DataPacketID, tESTask.PPID, 2);
			}
			else if (ptRadioHead->FuncCode == 0x5007)
			{
				tESTask.Sign &= 0xFB;
				memcpy(DataPacketID, tESTask.REID, 2);
			}
			else  // 5009
			{
				tESTask.Sign &= 0xF7;
				memcpy(DataPacketID, tESTask.CSID, 2);
			}
			MemoryBufferWrite(MRAM, (uint8_t *)&tESTask, ESTASK_ADDR + k*sizeof(T_ESTASK), sizeof(T_ESTASK));
		}
	}
	else
	{
		while (ptRadioDel)
		{
			memcpy(&TempID, buffer, 2);
			if (ptRadioDel->TaskState==TASK_RUNNING && ptRadioDel->ID==TempID && CompareData(buffer+6, ptRadioDel->DstAddr, 4)==true)
			{
				DataPacketFlag = 1;
				memcpy(DataPacketID, ptRadioDel->DataPacketID, 2);	/* 缓存数据包ID */
				DelTask(RADIO_TASK, ptRadioDel);  /* delete completed radio task */
				break;
			}
			else
			{
				ptRadioDel = ptRadioDel->ptNext;
			}
		}
		
		if (DataPacketFlag == 0)
		{
			g_DataPacketID++;
			memcpy(DataPacketID, (uint8_t *)&g_DataPacketID, 2);
		}
	}
	
	if (RadioState.Mode == X_MODE)
	{
		memcpy(DataPacketID, XmodeCfg.Data + 18, 2);  /* X模式数据包ID */
	}
	
	if (ptRadioHead->FuncCode == 0x4001)  /* 若为4001，rssi test*/
	{
		uint8_t MeterDataLen = 0;
		
		SX1278ARead( REG_LR_PKTRSSIVALUE, &ConRssi );
		memset(ComSendBuffer, 0x68, 1);			// Preamble
		memcpy(ComSendBuffer+1, buffer, 2);		// FuncCode
		memset(ComSendBuffer+3, 0x03, 1);		// CtrlArea=0x03
		memset(ComSendBuffer+4, 0, 2);			// FrameSeq & KeyCode
		memcpy(ComSendBuffer+6,  buffer+6, 4);	// DstAddr
		memset(ComSendBuffer+10, 0, 4);			// SrcAddr
		memcpy(ComSendBuffer+14, buffer+2, 4);	// ConAddr
		memset(ComSendBuffer+18, 0, 2);			// DataFrameID
		memset(ComSendBuffer+20, 0, 2);			// DataFrameSeq
		memcpy(ComSendBuffer+22, buffer+10, 1);	// recv DataLen
		ComSendBuffer[22] -= 0x80;
		MeterDataLen = ComSendBuffer[22];
		if (buffer[10] != 0)  /* 判断是否为超时指令 */
		{
			ComSendBuffer[22] += 13;								// DataLen+FuncCode
			memcpy(ComSendBuffer+23, buffer, 2);					// 2 bytes FuncCode
			memcpy(ComSendBuffer+25, (uint8_t *)&SystemTime, 7); 	// system time
			memcpy(ComSendBuffer+32, ConParm.ReportBaseTime, 3);	// report base time
			memcpy(ComSendBuffer+35, buffer+11, MeterDataLen);  	// Data
			memset(ComSendBuffer+MeterDataLen+35, ConRssi, 1);		// 1 bytes CON RSSI
			memset(ComSendBuffer+MeterDataLen+38, 0x16, 1);			// end，2bytes crc reserved before
			ComSendSize = MeterDataLen + 39;						// send length
			ComSendFlag = 1;
		}
		else
		{
			ComSendBuffer[22] = 2;					// DataLen
			memset(ComSendBuffer+23, 0x03, 1);		// 2 bytes response code
			memset(ComSendBuffer+24, 0x26, 1);		// 
			memset(ComSendBuffer+27, 0x16, 1);		// end，2bytes crc reserved before
			ComSendSize = 28;						// send length
			ComSendFlag = 1;
		}
	}
	
	/* 增加新的NetTaskArray任务 */
	for (i = 0; i < TASK_SIZE; i++)
	{
		if (NetTaskArray[i].TaskState == TASK_IDLE)
		{
			if ((ptRadioHead->FuncCode == 0x2405) && ((k = FindMeterID(buffer+6)) != METER_SUM))  /* 若为2405，则删除存储的任务标志 */
			{
				temp = 0;
				MemoryBufferWrite(MRAM, &temp, REPORT_PARM_ADDR+sizeof(T_ReportParm)*k, 1);
			}
			else if (ptRadioHead->FuncCode == 0x4001)  /* 若为4001，读取RSSI */
			{
				SX1278ARead( REG_LR_PKTRSSIVALUE, &ConRssi );
			}
			
			/* 将任务添加了链表 */
			ptNetNew = &NetTaskArray[i];
			ptNetNew->Data = (uint8_t *)malloc(sizeof(uint8_t)*size);
			if (!ptNetNew->Data)
			{
				free(ptNetNew->Data);
				ptNetNew->Data = NULL;
				DBG_PRINTF("ptNetNew->Data malloc error!\n");
				return;
			}
			memcpy(&ptNetNew->ID,  buffer, 2);
			memcpy(ptNetNew->Data, buffer, size);
			memcpy(ptNetNew->DstAddr, buffer+6, 4);
			memcpy(ptNetNew->DataPacketID, DataPacketID, 2);
			ptNetNew->ProcTime  = 0;
			ptNetNew->Src       = AT_RADIO;
			ptNetNew->Dest      = AT_NET;
			ptNetNew->TaskState = TASK_READY;
			ptNetNew->ptPre     = NULL;
			ptNetNew->ptNext    = NULL;
			AddTask(NET_TASK, ptNetNew);
			break;
		}
	}
}

/* 获取射频发送数据 */
uint8_t GetRadioData(void)
{
	uint8_t result = TASK_DATA_NULL;
	PT_Task ptRadioCur = g_ptRadioTaskHead;
	PT_RadioSendData ptRadioSendData = &RadioSendData;
	
	while (ptRadioCur)
	{
		if (ptRadioCur->TaskState == TASK_READY)  /* task is ready */
		{
			memcpy(ptRadioSendData->Data,   ptRadioCur->Data+1,  2);  // 功能码
			memcpy(ptRadioSendData->Data+2, ptRadioCur->Data+14, 4);  // 集中器地址
			memcpy(ptRadioSendData->Data+6, ptRadioCur->Data+6,  4);  // 表具地址
			switch(ptRadioCur->ID)
			{
				case 0x2202:
				case 0x2212:
				case 0x2214:
					result = TASK_DATA_DONE;
					memset(ptRadioSendData->Data+10, 0x00, 1);  // data len (0 bytes data len)
					ptRadioSendData->Len = 12;
					break;
				
				case 0x4001:
				case 0x4002:
				case 0x4003:
				case 0x4004:
				case 0x4005:
				case 0x4006:
				case 0x4007:
				case 0x4008:
				case 0x4009:
				case 0x4010:
					result = TASK_DATA_DONE;
					memset(ptRadioSendData->Data+10, 0x07, 1);					  // data len (7 bytes data len)
					memcpy(ptRadioSendData->Data+11, (uint8_t *)&SystemTime, 7);  // system time
					ptRadioSendData->Len = 19;
					break;
				
				case 0x5006:
					result = TASK_DATA_DONE;
					memset(RadioSendData.Data+10, *(ptRadioCur->Data+22)-1, 1);	 // radio data len except 4 bytes MAC addr
					memcpy(RadioSendData.Data+11, ptRadioCur->Data+23, 12);  	 // real data
					memset(RadioSendData.Data+23, 0, 3);  						 // reserved
					memset(RadioSendData.Data+26, *(ptRadioCur->Data+35), 1);  	 // real data
					RadioSendData.Len = *(ptRadioCur->Data+22) - 1 + 12;
					break;
					
				case 0x5002:
				case 0x5004:
				case 0x5008:
				case 0x5010:
					result = TASK_DATA_DONE;
					memset(ptRadioSendData->Data+10, *(ptRadioCur->Data+22)-4, 1);					  // data len (13 bytes data len)
					memcpy(ptRadioSendData->Data+11, ptRadioCur->Data+23, *(ptRadioCur->Data+22)-4);  // real data
					ptRadioSendData->Len = 11 + (*(ptRadioCur->Data+22)-4) + 1;
					break;
					
				default:
					DelTask(RADIO_TASK, ptRadioCur);  // delete invalid video task
					ptRadioCur = g_ptRadioTaskHead;
					break;
			}
			
			if (result == TASK_DATA_DONE)
			{
				ptRadioCur->TaskState = TASK_RUNNING;
				GenerateSum(ptRadioSendData->Data, ptRadioSendData->Len - 1);  // generate checksum
				break;
			}
		}
		else if (ptRadioCur->TaskState == TASK_RUNNING)	 // task is running
		{
			result = TASK_DATA_BUSY;
			break;
		}
		else
		{
			ptRadioCur = ptRadioCur->ptNext;
		}
	}
	
	return result;
}

/* 将存储的即时上报2405放入空闲任务中 */
void ActiveDataReport(void)
{
	uint16_t k;
	uint8_t i, temp;
	PT_Task ptNetNew;
	T_ReportParm tReportParm;
	static uint32_t ActiveTimer = 0;
	
	if ( (GET_TICK_COUNT() - ActiveTimer) >= TICK_RATE_MS(2000) )
	{
		ActiveTimer = GET_TICK_COUNT();
		
		for (k = 0; k < METER_SUM; k++)
		{
			MemoryBufferRead(MRAM, &temp, REPORT_PARM_ADDR + sizeof(T_ReportParm)*k, 1);  /* 读2405标志 */
			if (temp == 1)
			{
				/* 增加新的NetTaskArray任务 */
				for (i = 0; i < TASK_SIZE; i++)
				{
					if (NetTaskArray[i].TaskState == TASK_IDLE)
					{
						temp = 0;
						MemoryBufferWrite(MRAM, &temp, REPORT_PARM_ADDR+sizeof(tReportParm)*k, 1);  /* 清2405标志 */
						MemoryBufferRead(MRAM, (uint8_t *)&tReportParm, REPORT_PARM_ADDR+sizeof(T_ReportParm)*k, sizeof(T_ReportParm));
						
						/* 将任务添加了链表中 */
						ptNetNew = &NetTaskArray[i];
						ptNetNew->Data = (uint8_t *)malloc(sizeof(uint8_t)*33);  /* 2405 data length */
						if (!ptNetNew->Data)
						{
							free(ptNetNew->Data);
							ptNetNew->Data = NULL;
							DBG_PRINTF("ptNetNew->Data malloc error!\n");
							return;
						}
						memcpy(&ptNetNew->ID,  (uint8_t *)&tReportParm.ReportHead.FuncCode, 2);
						memcpy(ptNetNew->Data, (uint8_t *)&tReportParm.ReportHead, 33);
						memcpy(ptNetNew->DstAddr, (uint8_t *)&tReportParm.ReportHead.DstAddr, 4);
						g_DataPacketID++;
						memcpy(ptNetNew->DataPacketID, (uint8_t *)&g_DataPacketID, 2);
						ptNetNew->ProcTime  = 0;
						ptNetNew->Src       = AT_RADIO;
						ptNetNew->Dest      = AT_NET;
						ptNetNew->TaskState = TASK_READY;
						ptNetNew->ptPre     = NULL;
						ptNetNew->ptNext    = NULL;
						AddTask(NET_TASK, ptNetNew);
						break;
					}
				}
				break;
			}
		}
	}
}

/* X模式数据配置 */
uint8_t XmodeDataConfig(uint8_t *buffer)
{
	uint16_t TempID;
	uint8_t result = 1;
	
	memcpy(RadioSendData.Data,   buffer+1,  2);	 // 功能码
	memcpy(RadioSendData.Data+2, buffer+14, 4);	 // 集中器地址
	memcpy(RadioSendData.Data+6, buffer+6,  4);	 // 表具地址
	memcpy(&TempID, RadioSendData.Data, 2);
	switch(TempID)
	{
		case 0x2202:
		case 0x2212:
		case 0x2214:
		case 0x2215:
        case 0x2216:
		case 0x2230:
			memset(RadioSendData.Data+10, 0x00, 1);	 // data len (0 bytes data len)
			RadioSendData.Len = 12;
			break;
		
		case 0x2211:
		case 0x4001:
		case 0x4002:
		case 0x4003:
		case 0x4004:
		case 0x4005:
		case 0x4006:
		case 0x4007:
		case 0x4008:
		case 0x4009:
		case 0x4010:
			memset(RadioSendData.Data+10, 0x07, 1);					   // data len (7 bytes data len)
			memcpy(RadioSendData.Data+11, (uint8_t *)&SystemTime, 7);  // system time
			RadioSendData.Len = 19;
			break;
		
		case 0x5006:
			memset(RadioSendData.Data+10, *(buffer+22)-1, 1);  // radio data len except 4 bytes MAC addr
			memcpy(RadioSendData.Data+11, buffer+23, 12);  	   // real data
			memset(RadioSendData.Data+23, 0, 3);  			   // reserved
			memset(RadioSendData.Data+26, *(buffer+35), 1);    // real data
			RadioSendData.Len = *(buffer+22) - 1 + 12;
			break;
		
		case 0x2207:
		case 0x2213:
		case 0x5002:
		case 0x5004:
		case 0x5005:
		case 0x5007:
		case 0x5008:
		case 0x5009:
		case 0x5010:
			memset(RadioSendData.Data+10, *(buffer+22)-4, 1);		   // radio data len except 4 bytes MAC addr
			memcpy(RadioSendData.Data+11, buffer+23, *(buffer+22)-4);  // real data
			RadioSendData.Len = *(buffer+22) - 4 + 12;
			break;
			
		default:
			memset(XmodeCfg.Data, 0, X_MODE_SIZE);
			result = 0;
			break;
	}
	
	if (result)
	{
		memcpy(XmodeCfg.Wakeup,   RadioSendData.Data+2, 4);	 // 集中器地址
		memcpy(XmodeCfg.Wakeup+4, RadioSendData.Data+6, 4);	 // 表具地址
		memset(XmodeCfg.Wakeup+3, 0xE2, 1);				  	 // 模拟手持器时，A1 -> E2
		if (TempID==0x2211 && CompareData(buffer+6, BroadcastAddr, 4)==true)  /* X mode 群校时 */
		{
			memset(XmodeCfg.Wakeup+8, 0x03, 1);  // 群唤醒
		}
		else
		{
			memset(XmodeCfg.Wakeup+8, 0x01, 1);  // 单唤醒
		}
	}
	
	return result;
}

/* 集中器扫频参数配置 */
void ScanDataConfig(void)
{
	memset(SX1278ASendBuffer,    0x01,  1);					  // 功能码
	memset(SX1278ASendBuffer+1,  0x30,  1);					  // 功能码
	memcpy(SX1278ASendBuffer+2,  ConParm.ID, 4);			  // 集中器地址
	memset(SX1278ASendBuffer+6,  0xFF,  4);					  // 表具广播地址
	memset(SX1278ASendBuffer+10, 0x08, 1);					  // data len
	memcpy(SX1278ASendBuffer+11, (uint8_t *)&SystemTime, 7);  // system time
	memset(SX1278ASendBuffer+18, ConParm.RFChannel, 1);		  // RFChannel
	SX1278ASendSize = 20;									  // report len
	GenerateSum(SX1278ASendBuffer, SX1278ASendSize - 1);	  // generate checksum;
}

/* 集中器扫频时安装参数配置 */
void InstallDataConfig(uint8_t *buffer)
{
	uint16_t k;
	uint8_t	MeterAddrTemp[4];
	
	if ( (k = FindMeterID(buffer+6)) == METER_SUM )  /* 存储器中没有相同的表具地址，则寻找空节点 */
	{
		for (k = 0; k < METER_SUM; k++)
		{
			MemoryBufferRead(MRAM, MeterAddrTemp, METER_ADDR+4*k, 4);
			if (MeterAddrTemp[0] == 0xFF )
			{
				break;
			}
		}
		
		if (k != METER_SUM)	 /* 存储器中有空节点 */
		{
			MeterTotal++;
			MemoryBufferWrite(MRAM, buffer+6, METER_ADDR+4*k, 4);
			MemoryBufferWrite(MRAM, (uint8_t *)&MeterTotal, METER_TOTAL_ADDR, 2);
		}
	}
	
	/* 编译安装数据 */
	InstallParm.MeterInstallParm.NodeAddr[0] = (uint8_t)k;		   // 节点地址
	InstallParm.MeterInstallParm.NodeAddr[1] = (uint8_t)(k>>8);
	memcpy(RadioSendData.Data,    buffer,     2);				   // 功能码3002
	memcpy(RadioSendData.Data+2,  ConParm.ID, 4);				   // 集中器地址
	memcpy(RadioSendData.Data+6,  buffer+6,   4);				   // 表具地址
	memset(RadioSendData.Data+10, sizeof(T_MeterInstallParm), 1);  // data len
	memcpy(RadioSendData.Data+11, &InstallParm.MeterInstallParm, sizeof(T_MeterInstallParm));  // real data
	RadioSendData.Len = 11 + sizeof(T_MeterInstallParm) + 1;
	GenerateSum(RadioSendData.Data, RadioSendData.Len - 1);		   // generate checksum
}

/***************************************** 射频任务处理 *****************************************/
void RadioTaskProcess(void)
{
	RadioPutProcess();
	
	SX1278ATaskProcess();
	
	if (SX1278BCongfig.Enable == 1)
	{
		SX1278BTaskProcess();
	}
	else
	{
		SX1278BTaskState = SX1278_STATE_RX_INIT;
	}
	
	ActiveDataReport();
	
	RF_LED_Blink();
}

// RF发送、接收灯闪烁
void RF_LED_Blink(void)
{
	static uint8_t UP_LED_EN = 0;
	static uint8_t DOWN_LED_EN = 0;
	static uint32_t UPLEDTimer = 0;
	static uint32_t DOWNLEDTimer = 0;
	
	/* RF data up(send data) */
	if (RFLED.UpFlag == 1)
	{
		UP_LED_ON();
		UP_LED_EN    = 1;
		RFLED.UpFlag = 0;
		UPLEDTimer   = GET_TICK_COUNT();
	}
	else if (UP_LED_EN==1 && (GET_TICK_COUNT()-UPLEDTimer)>=TICK_RATE_MS(300))
	{
		UP_LED_OFF();
		UP_LED_EN = 0;
	}
	
	/* RF data down(recevie data) */
	if (RFLED.DownFlag == 1)
	{
		DOWN_LED_ON();
		DOWN_LED_EN    = 1;
		RFLED.DownFlag = 0;
		DOWNLEDTimer   = GET_TICK_COUNT();
	}
	else if (DOWN_LED_EN==1 && (GET_TICK_COUNT()-DOWNLEDTimer)>=TICK_RATE_MS(300))
	{
		DOWN_LED_OFF();
		DOWN_LED_EN = 0;
	}
}

// 注册PT_RadioDriver结构体
int RegisterRadioDriver(PT_RadioDriver ptRadioDriver)
{
	PT_RadioDriver ptTmp;

	if (!g_ptRadioDriverHead)
	{
		g_ptRadioDriverHead   = ptRadioDriver;
		ptRadioDriver->ptNext = NULL;
	}
	else
	{
		ptTmp = g_ptRadioDriverHead;
		while (ptTmp->ptNext)
		{
			ptTmp = ptTmp->ptNext;
		}
		ptTmp->ptNext	      = ptRadioDriver;
		ptRadioDriver->ptNext = NULL;
	}

	return 0;
}

// 注册所有的射频设备结构体
void RadioRegister(void)
{
	int iError;
	
	iError = SX1278ARegister();
	iError |= SX1278BRegister();
	
	if (iError)
	{
		DBG_PRINTF("RadioRegister error!\n");
	}
}

// 初始化所有的射频设备
void RadioDeviceInit(void)
{
	PT_RadioDriver ptTmp = g_ptRadioDriverHead;
	
	while (ptTmp)
	{
		if (0 != ptTmp->Init())
		{
			DBG_PRINTF("%s initialization fail!\n", ptTmp->name);
		}
		ptTmp = ptTmp->ptNext;
	}
}

// 射频设备pcName处理函数
uint32_t RadioProcess(char *pcName)
{
	uint32_t iError = 0;
	PT_RadioDriver ptTmp = g_ptRadioDriverHead;
	
	while (ptTmp)
	{
		if (strcmp(ptTmp->name, pcName) == 0)
		{
			return ptTmp->Process();
		}
		ptTmp = ptTmp->ptNext;
	}
	
	return iError;
}

// 设置射频设备pcName为接收状态
int RadioSetRxMode(char *pcName)
{
	int iError = -1;
	
	PT_RadioDriver ptTmp = g_ptRadioDriverHead;
	
	while (ptTmp)
	{
		if (strcmp(ptTmp->name, pcName) == 0)
		{
			ptTmp->StartRx();
			
			return 0;
		}
		ptTmp = ptTmp->ptNext;
	}
	
	return iError;
}

// 获取射频设备pcName的接收数据包
void RadioGetRxPacket(char *pcName, uint8_t *buffer, uint16_t* size)
{
	PT_RadioDriver ptTmp = g_ptRadioDriverHead;
	
	while (ptTmp)
	{
		if (strcmp(ptTmp->name, pcName) == 0)
		{
			ptTmp->GetRxPacket(buffer, size);
		}
		ptTmp = ptTmp->ptNext;
	}
}

// 从射频设备pcName发送数据包
void RadioSetTxPacket(char *pcName, const void *buffer, uint16_t size)
{
	PT_RadioDriver ptTmp = g_ptRadioDriverHead;
	
	while (ptTmp)
	{
		if (strcmp(ptTmp->name, pcName) == 0)
		{
			ptTmp->SetTxPacket(buffer, size);
		}
		ptTmp = ptTmp->ptNext;
	}
}

/***************************************** SX1278B监控模块 *****************************************/
void SX1278BDataProcess(uint8_t *buffer, uint8_t size)
{
	uint16_t TempID;
	uint8_t  TempSize;
	uint8_t  MeterDataLen;
	uint8_t  TempBuffer[100];
	
	memcpy(&TempID, buffer, 2);
	if (TempID == 0x2405)
	{
		memset(TempBuffer, 0x68, 1);			// Preamble
		memcpy(TempBuffer+1, buffer, 2);		// FuncCode
		memset(TempBuffer+3, 0x03, 1);			// CtrlArea=0x03
		memset(TempBuffer+4, 0, 1);				// FrameSeq
		memset(TempBuffer+5, 0, 1);				// KeyCode
		memcpy(TempBuffer+6,  buffer+6, 4);		// DstAddr
		memset(TempBuffer+10, 0, 4);			// SrcAddr
		memcpy(TempBuffer+14, buffer+2, 4);		// ConAddr
		memset(TempBuffer+18, 0, 2);			// DataFrameID
		memset(TempBuffer+20, 0, 2);			// DataFrameSeq
		memcpy(TempBuffer+22, buffer+10, 1);	// recv DataLen
		TempBuffer[22] -= 0x80;
		MeterDataLen = TempBuffer[22] - 1;		// DataLen，无RSSI -1
		
		TempBuffer[22] = (TempBuffer[22]+15-1);				
		memset(TempBuffer+23, 0x01, 1);						// 集中器状态字
		memset(TempBuffer+24, 0x00, 1);						// 
		memcpy(TempBuffer+25, (uint8_t *)&MeterTotal, 2);	// 上报表具总数量
		memcpy(TempBuffer+27, buffer+6, 4);					// 表具编号，DstAddr
		memcpy(TempBuffer+31, (uint8_t *)&SystemTime, 7);	// system time
		memcpy(TempBuffer+38, buffer+11, MeterDataLen);		// data
		memset(TempBuffer+MeterDataLen+40, 0x16, 1);		// end，2bytes crc reserved before
		TempSize = MeterDataLen + 41;
		NetComputeCRC(TempBuffer, TempSize-3, TempBuffer+TempSize-3);  // create CRC
		USART2_SendData(TempBuffer, TempSize);  // 发送监测数据
	}
}

static uint8_t SX1278BSendSize  = 0;			// RFB send size
static uint16_t SX1278BRecvSize = 0;			// RFB receive size
uint8_t SX1278BSendBuffer[SX1278_BUFFER_SIZE];	// RFB send buffer
uint8_t SX1278BRecvBuffer[SX1278_BUFFER_SIZE];	// RFB receive buffer
T_SX1278BCongfig SX1278BCongfig = {0, 24, 0};	/* SX1278 监控模块状态设置 */

// SX1278B task process
void SX1278BTaskProcess(void)
{
	static uint32_t SX1278BTimeoutTimer;
	
	switch(SX1278BTaskState)
	{
		case SX1278_STATE_IDLE:
			 break;
		
		case SX1278_STATE_TX_INIT:
			RadioSetTxPacket(SX1278B, SX1278BSendBuffer, SX1278BSendSize);
			SX1278BTimeoutTimer = GET_TICK_COUNT();
			SX1278BTaskState    = SX1278_STATE_TX_RUNNING;
			break;
			
		case SX1278_STATE_TX_RUNNING:
			if ((GET_TICK_COUNT()-SX1278BTimeoutTimer) < TICK_RATE_MS(10000))
			{
				if (RadioProcess(SX1278B) == RF_TX_DONE)
				{
					SX1278BTimeoutTimer = GET_TICK_COUNT();
					SX1278BTaskState    = SX1278_STATE_TX_DONE;
				}
			}
			else
			{
				SX1278BTaskState = SX1278_STATE_TX_TIMEOUT;
			}
			break;
		
		case SX1278_STATE_TX_DONE:
			RFLED.UpFlag = 1;
			SX1278BTaskState = SX1278_STATE_RX_INIT;
			break;
		
		case SX1278_STATE_TX_TIMEOUT:
			SX1278BTaskState = SX1278_STATE_TX_INIT;
			break;
		
		case SX1278_STATE_RX_INIT:
			SX1278B_LoRaSettings.RFFrequency = LORA_FRE_BASE + SX1278BCongfig.RFChannel * LORA_FRE_STEP;
			BLoRaSettings = SX1278B_LoRaSettings;
			SX1278BLoRaInit();
			RadioSetRxMode(SX1278B);
			SX1278BTimeoutTimer = GET_TICK_COUNT();
			SX1278BTaskState    = SX1278_STATE_RX_RUNNING;
			break;
		
		case SX1278_STATE_RX_RUNNING:
			if (RadioProcess(SX1278B) == RF_RX_DONE)
			{
				SX1278BTaskState = SX1278_STATE_RX_DONE;
			}
			else if (SX1278BCongfig.RFChannelFlag == 1)
			{
				SX1278BCongfig.RFChannelFlag = 0;
				SX1278BTaskState = SX1278_STATE_RX_INIT;
			}
			break;
		
		case SX1278_STATE_RX_DONE:
			RadioGetRxPacket(SX1278B, SX1278BRecvBuffer, (uint16_t*)&SX1278BRecvSize);
			
			if (SX1278BCheckSum(SX1278BRecvBuffer, SX1278BRecvSize-1))	// 校验(不校验集中器号)
			{
				RFLED.DownFlag = 1;
				SX1278BDataProcess(SX1278BRecvBuffer, SX1278BRecvSize);
				SX1278BTaskState = SX1278_STATE_RX_INIT;		// 启动SX1278B接收
			}
			else	// 校验错误
			{
				SX1278BTaskState = SX1278_STATE_RX_INIT;
			}
			break;
		
		case SX1278_STATE_RX_TIMEOUT:
			SX1278BTaskState = SX1278_STATE_RX_INIT;
			break;
		
		default:
			break;
	}
}
