#include "sys_config.h"
#include "net.h"
#include "bluetooth.h"
#include "bluetooth-Hal.h"
#include "bluetooth-Misc.h"
#include "radio.h"


static uint8_t ResetFlag = 0;
static uint8_t BlueRecvFlag = 0;
static uint8_t BlueRecvSize = 0;
uint8_t BlueRecvBuffer[BLUE_BUFFER_SIZE];

static uint8_t BlueSendFlag = 0;
static uint8_t BlueSendSize = 0;
static uint8_t BlueSendBuffer[BLUE_BUFFER_SIZE];

static uint8_t AppSendSize = 0;
static uint8_t AppSendBuffer[BLUE_BUFFER_SIZE];
static T_MultiPacket BlueMultiPacket;	/* 多帧传输 */

static void BluetoothInit(void);
static void BluetoothProcess(void);
static void BlueReplyHandle(uint8_t *buffer, uint8_t size);
static void BluetoothSendData(uint8_t *data, uint8_t size);
static void BluetoothRecvData(uint8_t *data, uint8_t *size);

static T_NetDriver BluetoothDriver = {
	.name	    = "bluetooth",
	.Init       = BluetoothInit,
// 	.Disconnect = BluetoothDisconnect,
	.SendData	= BluetoothSendData,
 	.RecvData	= BluetoothRecvData,
	.Process	= BluetoothProcess,
};

/* 蓝牙通信处理函数 */
void BluetoothProcess(void)
{
	uint8_t ret;
	static uint32_t BluetoothTick;
	static tBluetoothStates BlueState = BLUETOOTH_STATE_IDLE;
	
	switch(BlueState)
	{
		case BLUETOOTH_STATE_IDLE:
			if (ADErrorFlag == 0)
			{
				BlueState = BLUETOOTH_STATE_INIT;
			}
			break;
		
		case BLUETOOTH_STATE_INIT:
			BLUETOOTH_POWER_OFF();
			BluetoothTick = GET_TICK_COUNT();
			BlueState = BLUETOOTH_STATE_POWER;
			break;
			
		case BLUETOOTH_STATE_POWER:
			if ((GET_TICK_COUNT() - BluetoothTick) > TICK_RATE_MS(5000))
			{
				BLUETOOTH_POWER_ON();
				BlueState = BLUETOOTH_STATE_CFG;
			}
			break;
			 
		case BLUETOOTH_STATE_CFG:
			ret = BluetoothCfg();
			if (ret == BLUETOOTH_DONE)
			{
				BlueState = BLUETOOTH_STATE_RUNNING;
			}
			else if (ret == BLUETOOTH_ERROR)
			{
				BlueState = BLUETOOTH_STATE_IDLE;
			}
			break;
			
		case BLUETOOTH_STATE_RUNNING:
			if (BlueRecvFlag == 1)  /* 接收Bluetooth数据 */
			{
				BlueRecvFlag = 0;
				DATA_PRINTF(BlueRecvBuffer, BlueRecvSize);
				if (NetCheckCRC(BlueRecvBuffer, BlueRecvSize-3, (uint16_t *)(BlueRecvBuffer+BlueRecvSize-3)) && \
					BlueRecvBuffer[0]==0x68 && BlueRecvBuffer[BlueRecvSize-1]==0x16)  /* recieved bluetooth data */
				{
					BlueReplyHandle(BlueRecvBuffer, BlueRecvSize);
				}
				else
				{
					memset(BlueRecvBuffer, 0, BLUE_BUFFER_SIZE);
				}
			}
			
			if (BlueSendFlag == 1)  /* 发送Bluetooth数据 */
			{
				BlueSendFlag = 0;
				NetComputeCRC(BlueSendBuffer, BlueSendSize-3, BlueSendBuffer+BlueSendSize-3);  // CRC
				USART1_SendData(BlueSendBuffer, BlueSendSize);
				if (ResetFlag == 1)
				{
					Soft_delay_ms(2000);
					SoftReset();
				}
			}
			
			/* 电池低电压故障时关闭蓝牙模块 */
			if (ADErrorFlag == 1)
			{
				BLUETOOTH_POWER_OFF();
				BlueState = BLUETOOTH_STATE_IDLE;
			}
			break;
			
		default:
			break;
	}
}

/* BLUETOOTH确认回复 */
void BlueReplyHandle(uint8_t *buffer, uint8_t size)
{
	uint8_t i, temp;
	uint16_t k, TempID;
	PT_Task ptRadioTmp;
	uint8_t MeterAddSum;
	uint8_t CommonBack = 1;
	uint8_t NetSendFlag = 1;
	uint8_t	MeterAddrTemp[4];
	PT_Task ptRadioDel = g_ptRadioTaskHead;
	PT_NetHead ptNetHead = (PT_NetHead)AppSendBuffer;
	
	memcpy(&TempID, buffer+1, 2);
	memcpy(AppSendBuffer, buffer, 23);
	if (TempID == 0x1003)  /* set system initial parameter config */
	{
		memcpy(ConParm.ID, buffer+23, 4);
		memcpy(&ConParm.RFChannel, buffer+27, 1);
		MemoryBufferWrite(MRAM, buffer+28, SOCKET_ADDR, sizeof(T_Socket));
		memcpy(ConParm.MAC, buffer+73, 4);
		MemoryBufferWrite(MRAM, (uint8_t *)&ConParm, CON_PARM_ADDR, sizeof(ConParm));	/* write CON parameters */
		ResetFlag = 1;
	}
	else if (TempID == 0x1004)  /* read system initial parameter */
	{
		CommonBack = 0;
		ptNetHead->DataLen = 56;
		memset(AppSendBuffer+23, 0x00, 2);
		memcpy(AppSendBuffer+25, ConParm.ID, 4);
		memcpy(AppSendBuffer+29, &ConParm.RFChannel, 1);
		MemoryBufferRead(MRAM, AppSendBuffer+30, SOCKET_ADDR, sizeof(T_Socket));  /* read socket parameter */
		memcpy(AppSendBuffer+75, ConParm.MAC, 4);
		memset(AppSendBuffer+81, 0x16, 1);		   // end
		AppSendSize = 82;
	}
	else if (TempID == 0x1005)  /* set GPRS heartbeat time */
	{
		ConParm.GPRSHeartTime = (*((uint16_t *)(buffer+23)) * 1000);  /* 心跳周期(ms) */
		MemoryBufferWrite(MRAM, (uint8_t *)&ConParm, CON_PARM_ADDR, sizeof(ConParm));	/*  write CON parameters */
	}
	else if (TempID == 0x1006)  /* read GPRS heart time */
	{
		CommonBack = 0;
		ptNetHead->DataLen = 0x04;
		memset(AppSendBuffer+23, 0x00, 2);
		*((uint16_t *)(AppSendBuffer+25)) = (uint16_t)(ConParm.GPRSHeartTime/1000);
		memset(AppSendBuffer+29, 0x16, 1);
		AppSendSize = 30;
	}
	else if (TempID == 0x1008)  /* read the concentrator vision */
	{
		CommonBack = 0;
		ptNetHead->DataLen = 10;
		memset(AppSendBuffer+23, 0x00, 2);
		memcpy(AppSendBuffer+25, (uint8_t *)&RELEASE_TIME, 4);  /* release time */
		memset(AppSendBuffer+29, SOFTWARE_VERSION, 1);  /* 1 bytes software version */
		memset(AppSendBuffer+30, HARDWARE_VERSION, 1);  /* 1 bytes hardware version */
		memset(AppSendBuffer+31, 3, 1);
		memset(AppSendBuffer+32, NetMode, 1);
		memset(AppSendBuffer+35, 0x16, 1);
		AppSendSize = 36;
	}
	else if (TempID == 0x2100)	/* 集中器恢复出厂设置 */
	{
		RTCInit();			/* Initialize RTC time value */
		AllMemoryBufferReset(MRAM);
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
		memset(AppSendBuffer+23, 0x00, 2);
		memcpy(AppSendBuffer+25, &SystemTime, 7);  /* 7 bytes clock data */
		memset(AppSendBuffer+34, 0x16, 1);
		AppSendSize = 35;
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
		memset(AppSendBuffer+23, 0x00, 2);
		memset(AppSendBuffer+25, ConParm.RFChannel, 1);
		memset(AppSendBuffer+28, 0x16, 1);
		AppSendSize = 29;
	}
	else if (TempID == 0x2109)	/* 设置集中器ID */
	{
		memcpy(ConParm.ID, buffer+23, 3);
		MemoryBufferWrite(MRAM, (uint8_t *)&ConParm, CON_PARM_ADDR, sizeof(ConParm));
		ResetFlag = 1;
	}
	else if (TempID == 0x2110)	/* 读取集中器ID */
	{
		CommonBack = 0;
		ptNetHead->DataLen = 0x06;
		memset(AppSendBuffer+23, 0x00, 2);		  // 2bytes confirm
		memcpy(AppSendBuffer+25, ConParm.ID, 4);  // Concentrator addr
		memset(AppSendBuffer+31, 0x16, 1);		  // end
		AppSendSize = 32;
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
	else if(TempID == 0x2120)	/* 集中器X模式设置 */
	{
		if (*(buffer+23) == 1)	/* 进入X模式 */
		{
			XmodeCfg.Enable = 1;
		}
		else
		{
			XmodeCfg.Enable = 0;  /* 退出X模式 */
			memset(XmodeCfg.Data, 0, X_MODE_SIZE);
		}
	}
	else if (TempID == 0x2122)	/* 使能集中器进入扫描安装模式 */
	{
		if (InstallParm.Flag == 0)	/* 没有配置安装数据，返回错误 */
		{
			CommonBack = 0;
			ptNetHead->DataLen = 0x02;
			memset(AppSendBuffer+23, 0x06, 1);  /* error code */
			memset(AppSendBuffer+24, 0x01, 1);
			memset(AppSendBuffer+27, 0x16, 1);
			AppSendSize = 28;
		}
		else
		{
			RadioState.Mode  = S_MODE;
			RadioState.Stage = STAGE_ENTER;
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
		memset(AppSendBuffer+23, 0x00, 2);  // 2bytes confirm
		MemoryBufferRead(MRAM, AppSendBuffer+25, METER_INSTALL_ADDR+1, sizeof(T_InstallParm)-1);
		memset(AppSendBuffer+sizeof(T_InstallParm)+26, 0x16, 1);  // end，2bytes crc reserved
		AppSendSize = sizeof(T_InstallParm) + 27;
	}
	else if (TempID == 0x2130)	// 读集中器软件版本
	{
		CommonBack = 0;
		ptNetHead->DataLen = 0x03;
		memset(AppSendBuffer+23, 0x00, 2);	 		    // 2 bytes confirm
		memset(AppSendBuffer+25, SOFTWARE_VERSION, 1);  // 1 bytes version number
		memset(AppSendBuffer+28, 0x16, 1);	            // end
		AppSendSize = 29;
	}
	else if (TempID == 0x2131)	// 清集中器Y/Z任务参数
	{
		uint8_t BroadcastAddr[4] = {0};
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
					DelTask(RADIO_TASK, ptRadioDel);  // delete radio task
					ptRadioDel = ptRadioTmp;
				}
				else
				{
					ptRadioDel = ptRadioDel->ptNext;
				}
			}
		}
	}
	else if (TempID == 0x2303)	// 增加表具
	{
		uint8_t m = 0;
		
		MeterAddSum = (buffer[22] - 8) / 4;
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
						memcpy(AppSendBuffer+26+6*m, buffer+27+4*i, 4);  /* 集中器地址 */
						memcpy(AppSendBuffer+30+6*m, (uint8_t *)&k, 2);  /* 节点地址 */
						m++;
						break;
					}
				}
			}
			else  /* 存储器中已存在要增加的表号 */
			{
				memcpy(AppSendBuffer+26+6*m, buffer+27+4*i, 4);  /* 集中器地址 */
				memcpy(AppSendBuffer+30+6*m, (uint8_t *)&k, 2);  /* 节点地址 */
				m++;
			}
		}
		MemoryBufferWrite(MRAM, (uint8_t *)&MeterTotal, METER_TOTAL_ADDR, 2);  /* 保存管理表具总数量 */
		
		CommonBack = 0;
		ptNetHead->DataLen = 6*m + 3;
		memset(AppSendBuffer+23, 0x00, 2);  	// 2 bytes confirm
		memset(AppSendBuffer+25, m, 1);     	// meter number
		memset(AppSendBuffer+28+6*m, 0x16, 1);
		AppSendSize = 6*m + 29;
	}
	else if (TempID == 0x2304)	// 删除表具
	{
		MeterAddSum = (buffer[22] - 4) / 4;
		for (i = 0; i < MeterAddSum; i++)
		{
			if ( (k = FindMeterID(buffer+23+4*i)) != METER_SUM )
			{
				MeterTotal--;
				memset(MeterAddrTemp, 0xFF, 4);
				MemoryBufferWrite(MRAM, MeterAddrTemp, METER_ADDR+4*k, 4);
				
				// 清除3004、5005、5007、5009任务标志及数据内容
				MRAMWriteData(ESTASK_ADDR+sizeof(T_ESTASK)*k, 0x00, sizeof(T_ESTASK));
				
				// 清除存储的2405数据
				MRAMWriteData(METER_PARM_ADDR + sizeof(T_MeterParm)*k, 0, sizeof(T_MeterParm));
			}
		}
		MemoryBufferWrite(MRAM, (uint8_t *)&MeterTotal, METER_TOTAL_ADDR, 2);  /* 保存管理表具总数量 */
	}
	else if (TempID == 0x2305)	// 配置集中器管理表具地址及节点
	{
		if (ptNetHead->DataPacketSeq[0] == 0x01)  // 第一帧，清0表具总数量及复位表具地址存储区
		{
			unsigned char *p_extram = (unsigned char *)(STM32_EXT_SRAM_BEGIN+METER_ADDR);
			
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
				*((uint16_t *)(AppSendBuffer+23)) = 0x0115;
				memset(AppSendBuffer+37, 0x16, 1);
				AppSendSize = 28;
				break;
			}
			MemoryBufferWrite(MRAM, buffer+23+6*i, METER_ADDR+4*k, 4);
			MeterTotal++;
		}
		MemoryBufferWrite(MRAM, (uint8_t *)&MeterTotal, METER_TOTAL_ADDR, 2);  // 保存管理表具总数量
	}
	else if (TempID == 0x2306)	// 读取集中器管理表具节点号
	{
		CommonBack  = 0;
		NetSendFlag = 0;
		memset(&BlueMultiPacket, 0, sizeof(T_MultiPacket));
		memcpy((uint8_t *)&BlueMultiPacket.NetHead, buffer, 23);
		BlueMultiPacket.NetHead.CtrlArea = 0x02;
		if (MeterTotal%NODE_PACKET_NUM)
		{
			BlueMultiPacket.NetHead.DataPacketSeq[1] = MeterTotal/NODE_PACKET_NUM + 1;
		}
		else
		{
			BlueMultiPacket.NetHead.DataPacketSeq[1] = MeterTotal/NODE_PACKET_NUM;
		}
		BlueMultiPacket.PacketType  = PACKET_NODE;
		BlueMultiPacket.PacketState = PACKET_STATE_INIT;
		if (BlueMultiPacket.NetHead.DataPacketSeq[1] == 0)  /* 集中器没有管理表具 */
		{
			NetSendFlag = 1;
			ptNetHead->DataLen = 0x02;
			memset(AppSendBuffer+23, 0x02, 1);  // error code
			memset(AppSendBuffer+24, 0x01, 1);
			memset(AppSendBuffer+27, 0x16, 1);
			AppSendSize = 28;
			memset(&BlueMultiPacket, 0, sizeof(T_MultiPacket));
		}
	}
	else if (TempID == 0x2402)	// 中心主动获取集中器管理的表具上报信息
	{
		uint16_t sum = 0;
		uint32_t ReadAddr;
		T_MeterParm tMeterParm;
		
		CommonBack  = 0;
		NetSendFlag = 0;
		memset(&BlueMultiPacket, 0, sizeof(T_MultiPacket));
		memcpy((uint8_t *)&BlueMultiPacket.NetHead, buffer, 23);
		BlueMultiPacket.NetHead.CtrlArea = 0x02;
		
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
			BlueMultiPacket.NetHead.DataPacketSeq[1] = sum/FETCH_PACKET_NUM + 1;
		}
		else
		{
			BlueMultiPacket.NetHead.DataPacketSeq[1] = sum/FETCH_PACKET_NUM;
		}
		BlueMultiPacket.PacketType  = PACKET_FETCH;
		BlueMultiPacket.PacketState = PACKET_STATE_INIT;
		if (BlueMultiPacket.NetHead.DataPacketSeq[1] == 0)  /* 集中器没有管理表具 */
		{
			NetSendFlag = 1;
			ptNetHead->DataLen = 0x02;  // 2 bytes length
			AppSendBuffer[23]  = 0x00;
			memset(AppSendBuffer+24, 0x02, 1);  // error code
			memset(AppSendBuffer+25, 0x01, 1);
			memset(AppSendBuffer+28, 0x16, 1);  // end
			AppSendSize = 29;
			memset(&BlueMultiPacket, 0, sizeof(T_MultiPacket));
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
				memcpy(AppSendBuffer+29, (uint8_t *)&tMeterParm.MeterAddr, sizeof(T_MeterParm)-1);
				ptNetHead->DataLen  = sizeof(T_MeterParm) + 5;
				memset(AppSendBuffer+23, 0x00, 2);		  			    // 2 bytes confirm
				memcpy(AppSendBuffer+25, ConStatus, 2);	  			    // concentrator status
				memcpy(AppSendBuffer+27, (uint8_t *)&MeterTotal, 2);    // meter total numbers
				memset(AppSendBuffer+sizeof(T_MeterParm)+30, 0x16, 1);  // end
				AppSendSize = sizeof(T_MeterParm) + 31;
			}
			else
			{
				ptNetHead->DataLen = 0x02;
				*((uint16_t *)(AppSendBuffer+23)) = 0x0110;  /* 2 bytes error code */
				memset(AppSendBuffer+37, 0x16, 1);
				AppSendSize = 28;
			}
		}
		else
		{
			ptNetHead->DataLen = 0x02;
			*((uint16_t *)(AppSendBuffer+23)) = 0x0118;  // 2 bytes error code
			memset(AppSendBuffer+37, 0x16, 1);		     // end
			AppSendSize = 28;
		}
	}
	else if (TempID == 0x2601)	// 集中器异常事件上传
	{
		ptNetHead->DataLen = 0x02;
		memcpy(AppSendBuffer+23, ConStatus, 2);  // 2 bytes concentrator status code
		memset(AppSendBuffer+27, 0x16, 1);		 // end
		AppSendSize = 28;
	}
	else if (TempID == 0x3002)	// 表具安装参数配置
	{
		if (SystemTime.hour>=0x06 && SystemTime.hour<=0x23)	// 判断是否为扫频时间段
		{
			InstallParm.Flag = 1;
			RadioState.Mode  = S_MODE;
			RadioState.Stage = STAGE_ENTER;
			memcpy(&InstallHead, buffer, sizeof(T_NetHead));
			memcpy(&InstallParm.MeterInstallParm, buffer+23, sizeof(T_MeterInstallParm));
			MemoryBufferWrite(MRAM, (uint8_t *) &InstallParm, METER_INSTALL_ADDR, sizeof(T_InstallParm));
		}
		else
		{
			CommonBack = 0;
			ptNetHead->DataLen = 0x02;
			memset(AppSendBuffer+23, 0x13, 1);	 // time error code
			memset(AppSendBuffer+24, 0x01, 1);
			memset(AppSendBuffer+27, 0x16, 1);	 // end，2bytes crc reserved before
			AppSendSize = 28;
		}
	}
	else if (TempID == 0x3003)	// 退出表具安装模式
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
				// GPRSGetFlag = 1;  /* 表号存在，有Radio任务需要处理 */
			}
			else
			{
				CommonBack = 0;
				ptNetHead->DataLen = 0x02;
				memset(AppSendBuffer+23, 0x18, 1);	/* error code */
				memset(AppSendBuffer+24, 0x01, 1);
				memset(AppSendBuffer+27, 0x16, 1);
				AppSendSize = 28;
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
				memset(AppSendBuffer+23, 0x21, 1);  /* busy code */
				memset(AppSendBuffer+24, 0x01, 1);
				memset(AppSendBuffer+27, 0x16, 1);
				AppSendSize = 28;
			}
		}
	}
	
	if (CommonBack == 1)  // common reply
	{
		ptNetHead->DataLen = 0x02;
		memset(AppSendBuffer+23, 0x00, 2);	 // 2 bytes confirm
		memset(AppSendBuffer+27, 0x16, 1);	 // end
		AppSendSize = 28;
	}
	
	if (NetSendFlag == 1)  // send GPRS data
	{
		ptNetHead->CtrlArea = 0x02;
		NetComputeCRC(AppSendBuffer, AppSendSize-3, AppSendBuffer+AppSendSize-3);  // create CRC
		USART1_SendData(AppSendBuffer, AppSendSize);  // send GPRS data
		DATA_PRINTF(AppSendBuffer, AppSendSize);
		if (ResetFlag == 1)
		{
			Soft_delay_ms(2500);
			SoftReset();
		}
	}
}

/* 打包上传至中心 */
void BlueMultiPacketProcess(void)
{
	uint8_t m;
	uint32_t ReadAddr;
	T_MeterParm tMeterParm;
	uint8_t	MeterAddrTemp[4];
	static uint32_t MultiPacketTimer;
	
	if ((BlueMultiPacket.PacketType != PACKET_IDLE) && (BlueMultiPacket.PacketState != PACKET_STATE_IDLE))
	{
		if (BlueMultiPacket.PacketState == PACKET_STATE_INIT)
		{
			MultiPacketTimer = GET_TICK_COUNT();
			BlueMultiPacket.PacketState = PACKET_STATE_RUN;
		}
			
		if (BlueMultiPacket.PacketType == PACKET_NODE)  // 2306提取表具地址及节点
		{
			if ((GET_TICK_COUNT() - MultiPacketTimer) > TICK_RATE_MS(20))
			{
				MultiPacketTimer = GET_TICK_COUNT();
				BlueMultiPacket.NetHead.DataPacketSeq[0]++;
				for (m = 0; BlueMultiPacket.Node<METER_SUM && m<NODE_PACKET_NUM; BlueMultiPacket.Node++)
				{
					MemoryBufferRead(MRAM, MeterAddrTemp, METER_ADDR+4*BlueMultiPacket.Node, 4);
					if (MeterAddrTemp[0] != 0xFF)
					{
						memcpy(AppSendBuffer+26+6*m, MeterAddrTemp, 4);
						*((uint16_t *)(AppSendBuffer+30+6*m)) = BlueMultiPacket.Node;
						m++;
					}
				}
				BlueMultiPacket.NetHead.DataLen = 6*m + 3;
				memcpy(AppSendBuffer, (uint8_t *)&BlueMultiPacket.NetHead, 23);
				memset(AppSendBuffer+23, 0x00, 2);		// 2 bytes confirm
				memset(AppSendBuffer+25, m, 1);		    // 本包上传表具数量
				memset(AppSendBuffer+28+6*m, 0x16, 1);	// end
				AppSendSize = 29 + 6*m;
				NetComputeCRC(AppSendBuffer, AppSendSize-3, AppSendBuffer+AppSendSize-3);  // create CRC
				USART1_SendData(AppSendBuffer, AppSendSize);
				DATA_PRINTF(AppSendBuffer, AppSendSize);
			}
		}
		else if (BlueMultiPacket.PacketType == PACKET_FETCH)  // 2402提取表具信息
		{
			if ((GET_TICK_COUNT() - MultiPacketTimer) > TICK_RATE_MS(20))
			{
				MultiPacketTimer = GET_TICK_COUNT();
				BlueMultiPacket.NetHead.DataPacketSeq[0]++;
				for (m = 0; BlueMultiPacket.Node<METER_SUM && m<FETCH_PACKET_NUM; BlueMultiPacket.Node++)
				{
					ReadAddr = METER_PARM_ADDR + sizeof(T_MeterParm)*BlueMultiPacket.Node;
					MemoryBufferRead(MRAM, (uint8_t *)&tMeterParm, ReadAddr, sizeof(T_MeterParm));
					if (tMeterParm.Flag != 0)
					{
						memcpy(AppSendBuffer+31+m*(sizeof(T_MeterParm)-1), (uint8_t *)&tMeterParm.MeterAddr, sizeof(T_MeterParm)-1);
						m++;
					}
				}
				*((uint16_t *)(AppSendBuffer+22)) = m*(sizeof(T_MeterParm)-1) + 7;
				memcpy(AppSendBuffer, (uint8_t *)&BlueMultiPacket.NetHead, 22);
				memset(AppSendBuffer+24, 0x00, 2);		                      // 2 bytes confirm
				memcpy(AppSendBuffer+26, ConStatus, 2);                       // concentrator status
				memcpy(AppSendBuffer+28, (uint8_t *)&MeterTotal, 2);          // meter total numbers
				memset(AppSendBuffer+30, m, 1);		                          // current packet meter number
				memset(AppSendBuffer+33+m*(sizeof(T_MeterParm)-1), 0x16, 1);  // end
				AppSendSize = 34 + m*(sizeof(T_MeterParm)-1);
				NetComputeCRC(AppSendBuffer, AppSendSize-3, AppSendBuffer+AppSendSize-3);  // create CRC
				USART1_SendData(AppSendBuffer, AppSendSize);
				DATA_PRINTF(AppSendBuffer, AppSendSize);
			}
		}
		
		if ((BlueMultiPacket.NetHead.DataPacketSeq[0] >= BlueMultiPacket.NetHead.DataPacketSeq[1]) || \
			(GET_TICK_COUNT() - MultiPacketTimer) > TICK_RATE_MS(5000))  /* 5s over time */
		{
			memset(&BlueMultiPacket, 0, sizeof(T_MultiPacket));
		}
	}
}


uint8_t BluetoothSendRec(uint8_t *str, uint8_t *find, uint16_t time, uint8_t num)
{
	static uint8_t SendNum = 0;
	uint8_t result = BLUETOOTH_BUSY;
	static uint32_t TxTimeoutTimer;
    static enum
    {
        SEND_STATE = 0,
        WAIT_STATE
    }GPRS_SEND_RECV_STATE = SEND_STATE;
	
    switch(GPRS_SEND_RECV_STATE)
	{
		case SEND_STATE:
			USART1_SendString(str);
			TxTimeoutTimer = GET_TICK_COUNT();
			GPRS_SEND_RECV_STATE = WAIT_STATE;
			break;
		
		case WAIT_STATE:
			if((GET_TICK_COUNT()-TxTimeoutTimer) < TICK_RATE_MS(time))
			{
				if(BlueRecvFlag==1 && LookForStr(BlueRecvBuffer, find)!=FULL)
				{
					SendNum = 0;
					BlueRecvFlag = 0;
					//DBG_PRINTF("%s \n", BlueRecvBuffer);
					memset(BlueRecvBuffer, 0, BLUE_BUFFER_SIZE);
					GPRS_SEND_RECV_STATE = SEND_STATE;
					result = BLUETOOTH_DONE;
				}
			}
			else
			{
				SendNum++;
				if (SendNum == num)
				{
					SendNum = 0;
					DBG_PRINTF("%s send error!\n", str);
					memset(BlueRecvBuffer, 0, BLUE_BUFFER_SIZE);
					result = BLUETOOTH_ERROR;
				}
				BlueRecvFlag = 0;
				GPRS_SEND_RECV_STATE = SEND_STATE;
			}
			break;
		
		default:
			break;
	}
	
	return result;
}


void BluetoothSendData(uint8_t *data, uint8_t size)
{
	BlueSendFlag = 1;
	BlueSendSize = size;
	memcpy((void *)BlueSendBuffer, (void *)data, (size_t)BlueSendSize);
}


void BluetoothRecvData(uint8_t *data, uint8_t *size)
{
	if (BlueRecvFlag == 1)
	{
		BlueRecvFlag = 0;
		*size = BlueRecvSize;
		memcpy((void *)data, (void *)BlueRecvBuffer, (size_t)BlueRecvSize);
	}
}


// 蓝牙模块初始化
void BluetoothInit(void)
{
	USART1_Init();
	
	BluetoothIOInit();
}


int BluetoothRegister(void)
{
	return RegisterNetDriver(&BluetoothDriver);
}


// USART1中断
void USART1_IRQHandler(void)
{
	uint8_t num;
	
	if (USART_GetITStatus(USART1, USART_IT_IDLE) == SET)
	{
		num = num;
		num = USART1->SR;
		num = USART1->DR;
		DMA_Cmd(DMA1_Channel5, DISABLE);
		BlueRecvSize = BLUE_BUFFER_SIZE - DMA_GetCurrDataCounter(DMA1_Channel5);
		DMA1_Channel5->CNDTR = BLUE_BUFFER_SIZE;	/* 定义指定DMA通道的缓存的大小 */
		DMA_Cmd(DMA1_Channel5, ENABLE);
		BlueRecvFlag = 1;
	}
}
