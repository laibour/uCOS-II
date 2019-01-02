#include "sys_config.h"
#include "net.h"
#include "gprs.h"
#include "bluetooth.h"


/* 网络层设备名称 */
static char GPRS[] = "gprs";
static char BLUETOOTH[] = "bluetooth";

T_NetSendData NetSendData;
static PT_NetDriver g_ptNetDriverHead;
uint8_t BroadcastAddr[4] = {0xFF, 0xFF, 0xFF, 0xFF};

void GPRSPutProcess(void)
{
	uint16_t k;
	uint8_t MeterDataLen = 0;
	T_SystemTime tSystemTime = {0};
	static uint32_t SendTimeoutTimer;
	static uint8_t GPRSReportSendNum = 0;
	PT_Task ptNetCur = g_ptNetTaskHead;
	PT_NetSendData ptNetSendData = &NetSendData;
	
	while (ptNetCur)
	{
		if (ptNetCur->TaskState == TASK_READY)
		{
			GPRSReportSendNum = 0;
			memset(ptNetSendData->Data, 0x68, 1);						// Preamble
			memcpy(ptNetSendData->Data+1, ptNetCur->Data, 2);			// FuncCode
			memset(ptNetSendData->Data+3, 0x03, 1);						// CtrlArea=0x03
			memset(ptNetSendData->Data+4, 0, 1);						// FrameSeq
			memset(ptNetSendData->Data+5, 0, 1);						// KeyCode
			memcpy(ptNetSendData->Data+6,  ptNetCur->Data+6, 4);		// DstAddr
			memset(ptNetSendData->Data+10, 0, 4);						// SrcAddr
			memcpy(ptNetSendData->Data+14, ptNetCur->Data+2, 4);		// ConAddr
			memcpy(ptNetSendData->Data+18, ptNetCur->DataPacketID, 2);	// DataFrameID
			memset(ptNetSendData->Data+20, 0, 2);						// DataFrameSeq
			if (*(ptNetCur->Data+10) != 0)	/* 判断是否为超时指令 */
			{
				GPRSPutFlag = 1;
				ptNetCur->TaskState = TASK_RUNNING;
				ptNetSendData->Data[22] = ptNetCur->Data[10] - 0x80;  /* received DataLen */
				MeterDataLen = ptNetSendData->Data[22];
				switch(ptNetCur->ID)
				{
					case 0x2111:
						ptNetSendData->Data[22] += 2;						   // add time length
						memset(ptNetSendData->Data+23, 0x00, 2);	   		   // 2 bytes confirm
						memcpy(ptNetSendData->Data+25, ptNetCur->Data+11, 8);  // 1 byte data
						memset(ptNetSendData->Data+35, 0x16, 1);			   // end
						ptNetSendData->Len = 36;							   // send length
						break;
						
					case 0x2202:
					case 0x2211:
					case 0x2212:
					case 0x2213:
					case 0x2214:
					case 0x2215:
                    case 0x2216:
					case 0x2230:
					case 0x3004:
						ptNetSendData->Data[22] += 2;									  // DataLen+FuncCode				
						memcpy(ptNetSendData->Data+23, ptNetCur->Data, 2);				  // 2 bytes FuncCode
						memcpy(ptNetSendData->Data+25, ptNetCur->Data+11, MeterDataLen);  // Meter data
						memset(ptNetSendData->Data+MeterDataLen+27, 0x16, 1);			  // end，2bytes crc reserved before
						ptNetSendData->Len = MeterDataLen + 28;				  			  // send length
						break;
					
					case 0x2405:
						if ( (k = FindMeterID(ptNetCur->Data+6)) != METER_SUM )
						{
							MemoryBufferRead(MRAM, (uint8_t *)&tSystemTime, REPORT_PARM_ADDR+sizeof(T_ReportParm)*k+1, sizeof(T_SystemTime)); // 读报告上传时间
						}
						MeterDataLen -= 1;
						ptNetSendData->Data[22] = (ptNetSendData->Data[22]+15-1);		  // DataLen，无RSSI -1
						memset(ptNetSendData->Data+23, 0x01, 1);						  // 集中器状态字
						memset(ptNetSendData->Data+24, 0x00, 1);						  // 
						memcpy(ptNetSendData->Data+25, (uint8_t *)&MeterTotal, 2);		  // 上报表具总数量
						memcpy(ptNetSendData->Data+27, ptNetCur->Data+6, 4);			  // 表具编号，DstAddr
						memcpy(ptNetSendData->Data+31, (uint8_t *)&tSystemTime, 7); 	  // report time
						memcpy(ptNetSendData->Data+38, ptNetCur->Data+11, MeterDataLen);  // data
						memset(ptNetSendData->Data+MeterDataLen+40, 0x16, 1);			  // end，2bytes crc reserved
						ptNetSendData->Len = MeterDataLen + 41;
						break;
						
					case 0x3002:
						memcpy(ptNetCur->DataPacketID, InstallHead.DataPacketID, 2);
						memcpy(ptNetSendData->Data+18, InstallHead.DataPacketID, 2);  // DataFrameID
						ptNetSendData->Data[22] = 4;								  // DataLen+FuncCode
						memcpy(ptNetSendData->Data+23, ptNetCur->Data, 2);			  // 2 bytes FuncCode
						k = FindMeterID(ptNetCur->Data+6);
						memcpy(ptNetSendData->Data+25, (uint8_t *)&k, 2);			  // 2 bytes meter node
						memset(ptNetSendData->Data+29, 0x16, 1);			  		  // end，2bytes crc reserved before
						ptNetSendData->Len = 30;									  // send length
						break;
						
					case 0x4001:
						ptNetSendData->Data[22] += 13;									  // DataLen+FuncCode
						memcpy(ptNetSendData->Data+23, ptNetCur->Data, 2);				  // 2 bytes FuncCode
						memcpy(ptNetSendData->Data+25, (uint8_t *)&SystemTime, 7); 		  // system time
						memcpy(ptNetSendData->Data+32, ConParm.ReportBaseTime, 3);		  // report base time
						memcpy(ptNetSendData->Data+35, ptNetCur->Data+11, MeterDataLen);  // Data
						memset(ptNetSendData->Data+MeterDataLen+35, ConRssi, 1);		  // 1 bytes CON RSSI
						memset(ptNetSendData->Data+MeterDataLen+38, 0x16, 1);			  // end，2bytes crc reserved before
						ptNetSendData->Len = MeterDataLen + 39;							  // send length
						break;
					
					case 0x2207:
					case 0x4002:
					case 0x4003:
					case 0x4004:
					case 0x4005:
					case 0x4006:
					case 0x4007:
					case 0x4008:
					case 0x4009:
					case 0x4010:
					case 0x5002:
					case 0x5004:
					case 0x5005:
					case 0x5006:
					case 0x5007:
					case 0x5008:
					case 0x5009:
					case 0x5010:
						ptNetSendData->Data[22] += 2;									  // DataLen+FuncCode
						memcpy(ptNetSendData->Data+23, ptNetCur->Data, 2);				  // 2 bytes FuncCode
						memcpy(ptNetSendData->Data+25, ptNetCur->Data+11, MeterDataLen);  // Data
						memset(ptNetSendData->Data+MeterDataLen+27, 0x16, 1);			  // end，2bytes crc reserved before
						ptNetSendData->Len = MeterDataLen + 28;							  // send length
						break;
					
					default:
						GPRSPutFlag = 0;
						DelTask(NET_TASK, ptNetCur);  // delete invalid net task
						ptNetCur = g_ptNetTaskHead;
						break;
				}
			}
			else  /* 2603超时 */
			{
				ptNetSendData->Data[22] = 2;			  // DataLen
				memset(ptNetSendData->Data+23, 0x03, 1);  // 2 bytes response code
				memset(ptNetSendData->Data+24, 0x26, 1);  // 
				memset(ptNetSendData->Data+27, 0x16, 1);  // end，2bytes crc reserved before
				ptNetSendData->Len = 28;				  // send length
				ptNetCur->TaskState = TASK_RUNNING;
				GPRSPutFlag = 1;
			}
			break;
		}
		else if (ptNetCur->TaskState == TASK_RUNNING)  /* task is running */
		{
			if (GPRSOnline == 1)
			{
				if ((GET_TICK_COUNT() - SendTimeoutTimer) > TICK_RATE_MS(GPRS_REPORT_TIME))
				{
					GPRSReportSendNum++;
					if (GPRSReportSendNum >= 2)  /* GPRS报告重发到2次 */
					{
						GPRSReportSendNum = 0;
						ptNetCur->ProcTime++;
						if (ptNetCur->ProcTime == NET_TASK_PROC_TIME)
						{
							DelTask(NET_TASK, ptNetCur);  /* delete net task */
						}
						else
						{
							ptNetCur->TaskState = TASK_SUSPEND;
						}
					}
					else  /* 重发GPRS报告 */
					{
						GPRSPutFlag = 1;
					}
				}
			}
			break;
		}
		else
		{
			ptNetCur = ptNetCur->ptNext;
		}
	}
	
	/* one cycle completed, set the suspend net tasks to ready. */
	if (ptNetCur ==  NULL)
	{
		ptNetCur = g_ptNetTaskHead;
		while (ptNetCur)
		{
			if (ptNetCur->TaskState == TASK_SUSPEND)
			{
				ptNetCur->TaskState = TASK_READY;	
			}
			else
			{
				ptNetCur = ptNetCur->ptNext;
			}
		}
	}
	
	/* conditions ready, and send the net report */
	if (GPRSOnline==1 && GPRSPutFlag==1)
	{
		GPRSPutFlag = 0;
		SendTimeoutTimer = GET_TICK_COUNT();
		NetSendPacket(GPRS, ptNetSendData->Data, ptNetSendData->Len);
	}
}

/* 将GPRS网络收到的数组缓存到射频处理数组中 */
void GPRSGetProcess(void)
{
	uint8_t i;
	uint16_t k;
	T_ESTASK tESTask;
	PT_Task ptRadioNew;
	PT_NetHead ptNetHead;
	T_RealPrice tRealPrice;
	T_PresetPrice tPresetPrice;
	uint8_t GPRSRecvSize = 0;
	uint8_t GPRSRecvBuffer[GPRS_BUFFER_SIZE];
	PT_NetSendData ptNetSendData = &NetSendData;
	
	if (GPRSGetFlag == 1)
	{
		NetRecvData(GPRS, GPRSRecvBuffer, (uint8_t*)&GPRSRecvSize);  /* 获取GPRS接收数据 */
		
		ptNetHead = (PT_NetHead)GPRSRecvBuffer;
		if ((ptNetHead->FuncCode==0x3004) || (ptNetHead->FuncCode==0x5004) || (ptNetHead->FuncCode==0x5005) || (ptNetHead->FuncCode==0x5007) || (ptNetHead->FuncCode==0x5009))
		{
			if (ptNetHead->FuncCode == 0x5004)  /* 按价格类别保存实时调价数据 */
			{
				memcpy(tRealPrice.Data, GPRSRecvBuffer+23, 14);
				MemoryBufferWrite(MRAM, (uint8_t *)&tRealPrice, REAL_PRICE_ADDR + sizeof(T_RealPrice)*GPRSRecvBuffer[23], sizeof(T_RealPrice));
			}
			else if (ptNetHead->FuncCode == 0x5005)  /* 按价格类别保存预调价数据 */
			{
				memcpy(tPresetPrice.Data, GPRSRecvBuffer+23, 15);
				MemoryBufferWrite(MRAM, (uint8_t *)&tPresetPrice, PRESET_PRICE_ADDR + sizeof(T_PresetPrice)*GPRSRecvBuffer[23], sizeof(T_PresetPrice));
			}
			
			if (ptNetHead->FuncCode==0x5004 && CompareData(ptNetHead->DstAddr, BroadcastAddr, 4)==true)
			{
				for (k = 0; k < METER_SUM; k++)
				{
					MemoryBufferRead(MRAM, (uint8_t *)&tESTask, ESTASK_ADDR + sizeof(T_ESTASK)*k, sizeof(T_ESTASK));
					if (tESTask.PriceCalss == GPRSRecvBuffer[23])  /* 价格类别 */
					{
						tESTask.Sign |= 0x02;
						memcpy(tESTask.RPID, ptNetHead->DataPacketID, 2);
						MemoryBufferWrite(MRAM, (uint8_t *)&tESTask, ESTASK_ADDR + sizeof(T_ESTASK)*k, sizeof(T_ESTASK));
					}
				}
			}
			else if (ptNetHead->FuncCode==0x5005 && CompareData(ptNetHead->DstAddr, BroadcastAddr, 4)==true)
			{
				for (k = 0; k < METER_SUM; k++)
				{
					MemoryBufferRead(MRAM, (uint8_t *)&tESTask, ESTASK_ADDR + sizeof(T_ESTASK)*k, sizeof(T_ESTASK));
					if (tESTask.PriceCalss == GPRSRecvBuffer[23])  /* 价格类别 */
					{
						tESTask.Sign |= 0x10;
						memcpy(tESTask.PPID, ptNetHead->DataPacketID, 2);
						MemoryBufferWrite(MRAM, (uint8_t *)&tESTask, ESTASK_ADDR + sizeof(T_ESTASK)*k, sizeof(T_ESTASK));
					}
				}
			}
			else
			{
				if ((k = FindMeterID(ptNetHead->DstAddr)) != METER_SUM)
				{
					MemoryBufferRead(MRAM, (uint8_t *)&tESTask, ESTASK_ADDR + sizeof(T_ESTASK)*k, sizeof(T_ESTASK));
					if (ptNetHead->FuncCode == 0x3004)
					{
						tESTask.Sign |= 0x01;
						tESTask.PriceCalss = GPRSRecvBuffer[23];  /* 保存单表价格类别 */
						memcpy(tESTask.OGID, ptNetHead->DataPacketID, 2);
						memcpy(&tESTask.OpenGas, GPRSRecvBuffer+23, 28);
					}
					else if (ptNetHead->FuncCode == 0x5004)
					{
						tESTask.Sign |= 0x02;
						tESTask.PriceCalss = GPRSRecvBuffer[23];  /* 保存单表价格类别 */
						memcpy(tESTask.RPID, ptNetHead->DataPacketID, 2);
					}
					else if (ptNetHead->FuncCode == 0x5005)
					{
						tESTask.Sign |= 0x10;
						tESTask.PriceCalss = GPRSRecvBuffer[23];  /* 保存单表价格类别 */
						memcpy(tESTask.PPID, ptNetHead->DataPacketID, 2);
					}
					else if (ptNetHead->FuncCode == 0x5007)
					{
						tESTask.Sign |= 0x04;
						memcpy(tESTask.REID, ptNetHead->DataPacketID, 2);
						memcpy(tESTask.Recharge, GPRSRecvBuffer+23, 12);
					}
					else  // 5009
					{
						tESTask.Sign |= 0x08;
						memcpy(tESTask.CSID, ptNetHead->DataPacketID, 2);
						memcpy(&tESTask.ClearSign, GPRSRecvBuffer+23, 1);
					}
					MemoryBufferWrite(MRAM, (uint8_t *)&tESTask, ESTASK_ADDR+sizeof(T_ESTASK)*k, sizeof(T_ESTASK));
				}
			}
		}
		else
		{
			for (i = 0; i < TASK_SIZE; i++)
			{
				if (RadioTaskArray[i].TaskState == TASK_IDLE)
				{
					ptRadioNew = &RadioTaskArray[i];
					ptRadioNew->Data = (uint8_t *)malloc(sizeof(uint8_t)*GPRSRecvSize);
					if (!ptRadioNew->Data)
					{
						free(ptRadioNew->Data);
						ptRadioNew->Data = NULL;
						DBG_PRINTF("ptRadioNew->Data malloc error!\n");
						return;
					}
					memcpy(ptRadioNew->Data, GPRSRecvBuffer, GPRSRecvSize);		// RadioData
					memcpy(&ptRadioNew->DataPacketID, GPRSRecvBuffer+18, 2);	// 数据包ID
					memcpy(ptRadioNew->DstAddr, GPRSRecvBuffer+6, 4);			// 目标（表具）地址
					memcpy(&ptRadioNew->ID, GPRSRecvBuffer+1, 2);				// ID(FuncCode)
					ptRadioNew->Src       = AT_NET;
					ptRadioNew->Dest      = AT_RADIO;
					ptRadioNew->ProcTime  = 0;
					ptRadioNew->TaskState = TASK_READY;
					ptRadioNew->ptPre 	  = NULL;
					ptRadioNew->ptNext    = NULL;
					AddTask(RADIO_TASK, ptRadioNew);
					break;
				}
			}
			
			/* 任务满，返回busy error到中心 */
			if (i == TASK_SIZE)
			{
				memcpy(ptNetSendData->Data, GPRSRecvBuffer, 23);	// net head
				memset(ptNetSendData->Data+3,  0x03, 1);			// CtrlArea=0x03
				memset(ptNetSendData->Data+22, 0x02, 1);			// DataLen=0x02
				memset(ptNetSendData->Data+23, 0x21, 1);			// 0121 busy code
				memset(ptNetSendData->Data+24, 0x01, 1);			// 
				memset(ptNetSendData->Data+25, 0x00, 2);			// 2bytes CRC
				memset(ptNetSendData->Data+27, 0x16, 1);			// end
				ptNetSendData->Len = 28;
				NetSendPacket(GPRS, ptNetSendData->Data, ptNetSendData->Len);
			}
		}
	}
}


void GPRSPutData(uint8_t *data, uint8_t size)
{
	if (GPRSPutFlag == 1)
	{
		GPRSPutFlag = 0;
		NetSendPacket(GPRS, data, size);
	}
}


void NetTaskProcess(void)
{
	NetProcess(GPRS);
	
	NetProcess(BLUETOOTH);
	
	GPRSGetProcess();
	
	GPRSPutProcess();
	
	GPRSMultiPacketProcess();
	
	BlueMultiPacketProcess();
}


void NetProcess(char *pcName)
{
	PT_NetDriver ptTmp = g_ptNetDriverHead;
	
	while (ptTmp)
	{
		if (strcmp(ptTmp->name, pcName) == 0)
		{
			ptTmp->Process();
		}
		ptTmp = ptTmp->ptNext;
	}
}


int NetConnect(char *pcName)
{
	int iError = -1;
	PT_NetDriver ptTmp = g_ptNetDriverHead;
	
	while (ptTmp)
	{
		if (strcmp(ptTmp->name, pcName) == 0)
		{
			return ptTmp->Connect();
		}
		ptTmp = ptTmp->ptNext;
	}
	
	return iError;
}


// 从网络设备pcName发送数据包
void NetSendPacket(char *pcName, uint8_t *buffer, uint8_t size)
{
	PT_NetDriver ptTmp = g_ptNetDriverHead;
	
	while (ptTmp)
	{
		if (strcmp(ptTmp->name, pcName) == 0)
		{
			ptTmp->SendData(buffer, size);
		}
		ptTmp = ptTmp->ptNext;
	}
}


// 从网络设备pcName接收数据包
uint8_t NetRecvData(char *pcName, uint8_t *data, uint8_t *size)
{
	uint8_t iError = 0;
	PT_NetDriver ptTmp = g_ptNetDriverHead;
	
	while (ptTmp)
	{
		if (strcmp(ptTmp->name, pcName) == 0)
		{
			ptTmp->RecvData(data, size);
		}
		ptTmp = ptTmp->ptNext;
	}
	
	return iError;
}


// 注册PT_NetDriver结构体
int RegisterNetDriver(PT_NetDriver ptNetDriver)
{
	PT_NetDriver ptTmp;

	if (!g_ptNetDriverHead)
	{
		g_ptNetDriverHead   = ptNetDriver;
		ptNetDriver->ptNext = NULL;
	}
	else
	{
		ptTmp = g_ptNetDriverHead;
		while (ptTmp->ptNext)
		{
			ptTmp = ptTmp->ptNext;
		}
		ptTmp->ptNext	    = ptNetDriver;
		ptNetDriver->ptNext = NULL;
	}

	return 0;
}


// 注册所有的网络备结构体
void NetRegister(void)
{
	int iError;
	
	iError = iError;
	iError = GPRSRegister();
	iError |= BluetoothRegister();
}


// 初始化所有的网络设备
void NetDeviceInit(void)
{
	PT_NetDriver ptTmp = g_ptNetDriverHead;

	while (ptTmp)
	{
		ptTmp->Init();
		ptTmp = ptTmp->ptNext;
	}
}


/* CRC字节校验 */
uint16_t ComputeCrc(uint16_t crc, uint8_t dataByte, uint16_t polynomial)
{
	uint8_t i = 0;
	
	for(i=0; i<8; i++)
	{
		if ((((crc&0x8000)>>8)^(dataByte&0x80)) != 0)
		{
			crc <<= 1;			// shift left once
			crc  ^= polynomial;	// XOR with polynomial
		}
		else
		{
			crc <<= 1;	// shift left once
		}
		
		dataByte <<= 1;	// Next data bit
	}
	
	return crc;
}


/* 生成网络数组CRC校验 */
void NetComputeCRC(uint8_t *buffer, uint8_t length, uint8_t *pcrc)
{
	uint8_t i = 0;
	uint16_t crc, polynomial;
	
	crc = CRC_CCITT_SEED;
	polynomial = POLYNOMIAL_CCITT;
	
	for (i=0; i<length; i++)
	{
		crc = ComputeCrc(crc, buffer[i], polynomial);
	}
	
	crc = ((uint16_t)(~crc));
	pcrc[0] = (uint8_t)(crc & 0xFF);
	pcrc[1] = (uint8_t)(crc >> 8);
}


/* 校验网络数组CRC */
bool NetCheckCRC(uint8_t *buffer, uint8_t length, uint16_t *pcrc)
{
	uint8_t i = 0;
	uint16_t crc, polynomial;
	
	crc = CRC_CCITT_SEED;
	polynomial = POLYNOMIAL_CCITT;
	
	for (i=0; i<length; i++)
	{
		crc = ComputeCrc(crc, buffer[i], polynomial);
	}
	
	if (*pcrc == ((uint16_t)(~crc)))
	{
		return true;
	}
	else
	{
		return false;
	}
}

/* 网络数据正确性校验 */
bool NetDataCheck(uint8_t *buffer, uint8_t len)
{
	if (buffer[0]!=0x68 || buffer[len-1]!=0x16)
	{
		return false;
	}
	
	if ( CompareData(buffer+14, ConParm.ID, 4) == false)
	{
		return false;
	}
	
	if ( NetCheckCRC(buffer, len-3, (uint16_t *)(buffer+len-3)) )
	{
		return true;
	}
	else
	{
		return false;
	}
}
