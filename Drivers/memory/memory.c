#include "sys_config.h"
#include "memory.h"
#include "i2c_ee.h"
#include "mram.h"


volatile uint8_t I2C_DEVICE_ADDR;
static PT_MemoryDriver g_ptMemoryDriverHead;

// 注册所有存储设备
void MemoryRegister(void)
{
	MRAMRegister();
	
	EEPROMRegister();
}


void AllMemoryBufferReset(char *pcName)
{
	PT_MemoryDriver ptTmp = g_ptMemoryDriverHead;
	
	while (ptTmp)
	{
		if (strcmp(ptTmp->name, pcName) == 0)
		{
			ptTmp->AllBufferReset();
		}
		ptTmp = ptTmp->ptNext;
	}
}


void MemoryBufferRead(char *pcName, uint8_t* pBuffer, uint32_t ReadAddr, uint16_t NumByteToRead)
{
	PT_MemoryDriver ptTmp = g_ptMemoryDriverHead;
	
	while (ptTmp)
	{
		if (strcmp(ptTmp->name, pcName) == 0)
		{
			ptTmp->BufferRead(pBuffer, ReadAddr, NumByteToRead);
		}
		ptTmp = ptTmp->ptNext;
	}
}


void MemoryBufferWrite(char *pcName, uint8_t* pBuffer, uint32_t WriteAddr, uint16_t NumByteToWrite)
{
	PT_MemoryDriver ptTmp = g_ptMemoryDriverHead;
	
	while (ptTmp)
	{
		if (strcmp(ptTmp->name, pcName) == 0)
		{
			ptTmp->BufferWrite(pBuffer, WriteAddr, NumByteToWrite);
		}
		ptTmp = ptTmp->ptNext;
	}
}


// 初始化所有的网络设备
void MemoryDeviceInit(void)
{
	PT_MemoryDriver ptTmp = g_ptMemoryDriverHead;

	while (ptTmp)
	{
		ptTmp->Init();
		ptTmp = ptTmp->ptNext;
	}
}


// 注册PT_MemoryDriver结构体
void RegisterMemoryDriver(PT_MemoryDriver ptMemoryDriver)
{
	PT_MemoryDriver ptTmp;
	
	if (!g_ptMemoryDriverHead)
	{
		g_ptMemoryDriverHead   = ptMemoryDriver;
		ptMemoryDriver->ptNext = NULL;
	}
	else
	{
		ptTmp = g_ptMemoryDriverHead;
		while (ptTmp->ptNext)
		{
			ptTmp = ptTmp->ptNext;
		}
		ptTmp->ptNext	       = ptMemoryDriver;
		ptMemoryDriver->ptNext = NULL;
	}
}

#if 0
void MemoryTest(void)
{
	uint8_t MRAMBuffer[255] = {0};
	
	memset(MRAMBuffer, 0xAA, sizeof(MRAMBuffer));
	
	MemoryBufferWrite(MRAM, MRAMBuffer, 0, sizeof(MRAMBuffer));
	
	memset(MRAMBuffer, 0, sizeof(MRAMBuffer));
	
	Soft_delay_ms(5);
	
	MemoryBufferRead(MRAM, MRAMBuffer, 0, sizeof(MRAMBuffer));
}
#endif
