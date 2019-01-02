#include "sys_config.h"
#include "memory.h"
#include "mram.h"


static void MRAMInit(void);
//static void sram_test(void);
static void MRAMBufferReset(void);
static void MRAMBufferRead(uint8_t* pBuffer, uint32_t ReadAddr, uint16_t NumByteToRead);
static void MRAMBufferWrite(uint8_t* pBuffer, uint32_t WriteAddr, uint16_t NumByteToWrite);

T_MemoryDriver MRAMDriver = {
	.name	        = "MRAM",
	.Init		    = MRAMInit,
	.BufferRead     = MRAMBufferRead,
	.BufferWrite    = MRAMBufferWrite,
	.AllBufferReset = MRAMBufferReset,
};

void MRAMBufferReset(void)
{
	uint16_t k;
	uint32_t temp;
	uint8_t ID[4];
	uint8_t RFChannel;
	T_Socket CentreSocket;
	uint8_t *p_extram = (uint8_t *)STM32_EXT_SRAM_BEGIN;
	
	memcpy(ID, ConParm.ID, 4);
	RFChannel = ConParm.RFChannel;
	MemoryBufferRead(MRAM, (uint8_t *)&CentreSocket, SOCKET_ADDR, sizeof(T_Socket));
	for (temp = 0; temp < (STM32_EXT_SRAM_END - STM32_EXT_SRAM_BEGIN); temp++)
	{
		*p_extram++ = (uint8_t)0x00;
	}
	
	p_extram = (uint8_t *)(STM32_EXT_SRAM_BEGIN + METER_ADDR);
	memset(&ConParm, 0, sizeof(T_ConParm));
	ConParm.Flag = 1;
	memcpy(ConParm.ID, ID, 4);
	ConParm.RFChannel = RFChannel;
	ConParm.GPRSHeartTime = 30000;
	/* 把表具地址存储区设置为0xFF */
	for (k = 0; k < 4*METER_SUM; k++)
	{
		*p_extram++ = (uint8_t)0xFF;
	}
	MemoryBufferWrite(MRAM, (uint8_t *)&ConParm, CON_PARM_ADDR, sizeof(T_ConParm));    /* 写入集中器参数 */
	MemoryBufferWrite(MRAM, (uint8_t *)&CentreSocket, SOCKET_ADDR, sizeof(T_Socket));  /* 写入IP参数 */
}


void MRAMWriteData(uint32_t WriteAddr, uint8_t data, uint16_t NumByteToWrite)
{
	unsigned int i;
	unsigned char *p_extram = (unsigned char *)(STM32_EXT_SRAM_BEGIN + WriteAddr);
	
	if ((STM32_EXT_SRAM_BEGIN + WriteAddr) >= STM32_EXT_SRAM_END)
	{
		DBG_PRINTF("MRAMWriteData Over memory limited!\n");
		while(1);
	}
	
	for (i=0; i<NumByteToWrite; i++)
	{
		*p_extram++ = data;
	}
}


void MRAMBufferRead(uint8_t* pBuffer, uint32_t ReadAddr, uint16_t NumByteToRead)
{
	unsigned int i;
	unsigned char *p_extram = (unsigned char *)(STM32_EXT_SRAM_BEGIN + ReadAddr);
	
	if ((STM32_EXT_SRAM_BEGIN + ReadAddr) >= STM32_EXT_SRAM_END)
	{
		DBG_PRINTF("MRAMBufferRead Over memory limited!\n");
		while(1);
	}
	
	for (i=0; i<NumByteToRead; i++)
	{
		*pBuffer++ = *p_extram++;
	}
}


void MRAMBufferWrite(uint8_t* pBuffer, uint32_t WriteAddr, uint16_t NumByteToWrite)
{
	unsigned int i;
	unsigned char *p_extram = (unsigned char *)(STM32_EXT_SRAM_BEGIN + WriteAddr);
	
	if ((STM32_EXT_SRAM_BEGIN + WriteAddr) >= STM32_EXT_SRAM_END)
	{
		DBG_PRINTF("MRAMBufferWrite over memory limited!\n");
		while(1);
	}
	
	for (i=0; i<NumByteToWrite; i++)
	{
		*p_extram++ = *pBuffer++;
	}
}


void MRAMRegister(void)
{
	RegisterMemoryDriver(&MRAMDriver);
}


void MRAMInit(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    FSMC_NORSRAMInitTypeDef  FSMC_NORSRAMInitStructure;
    FSMC_NORSRAMTimingInitTypeDef  p;

    /* FSMC GPIO configure */
    {
        RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOD | RCC_APB2Periph_GPIOE | RCC_APB2Periph_GPIOF
                               | RCC_APB2Periph_GPIOG, ENABLE);
        RCC_AHBPeriphClockCmd(RCC_AHBPeriph_FSMC, ENABLE);

        GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF_PP;
        GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;

        /*
        FSMC_D0 ~ FSMC_D3
        PD14 FSMC_D0   PD15 FSMC_D1   PD0  FSMC_D2   PD1  FSMC_D3
        */
        GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_14 | GPIO_Pin_15;
        GPIO_Init(GPIOD, &GPIO_InitStructure);

        /*
        FSMC_D4 ~ FSMC_D12
        PE7 ~ PE15  FSMC_D4 ~ FSMC_D12
        */
        GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7 | GPIO_Pin_8 | GPIO_Pin_9 | GPIO_Pin_10
                                      | GPIO_Pin_11 | GPIO_Pin_12 | GPIO_Pin_13 | GPIO_Pin_14 | GPIO_Pin_15;
        GPIO_Init(GPIOE, &GPIO_InitStructure);

        /* FSMC_D13 ~ FSMC_D15   PD8 ~ PD10 */
        GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8 | GPIO_Pin_9 | GPIO_Pin_10;
        GPIO_Init(GPIOD, &GPIO_InitStructure);

        /*
        FSMC_A0 ~ FSMC_A5   FSMC_A6 ~ FSMC_A9
        PF0     ~ PF5       PF12    ~ PF15
        */
        GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_2 | GPIO_Pin_3
                                      | GPIO_Pin_4 | GPIO_Pin_5 | GPIO_Pin_12 | GPIO_Pin_13 | GPIO_Pin_14 | GPIO_Pin_15;
        GPIO_Init(GPIOF, &GPIO_InitStructure);

        /* FSMC_A10 ~ FSMC_A15  PG0 ~ PG5 */
        GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_2 | GPIO_Pin_3 | GPIO_Pin_4 | GPIO_Pin_5;
        GPIO_Init(GPIOG, &GPIO_InitStructure);

        /* FSMC_A16 ~ FSMC_A18  PD11 ~ PD13 */
        GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11 | GPIO_Pin_12 | GPIO_Pin_13;
        GPIO_Init(GPIOD, &GPIO_InitStructure);
		
		/* FSMC_A19  PE3 */
        GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;
        GPIO_Init(GPIOE, &GPIO_InitStructure);

        /* RD-PD4 WR-PD5 */
        GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4 | GPIO_Pin_5;
        GPIO_Init(GPIOD, &GPIO_InitStructure);

        /* NBL0-PE0 NBL1-PE1 */
        GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1;
        GPIO_Init(GPIOE, &GPIO_InitStructure);

        /* NE1 */
        GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7;
        GPIO_Init(GPIOD, &GPIO_InitStructure);
    }
    /* FSMC GPIO configure */

    /*-- FSMC Configuration ------------------------------------------------------*/
    p.FSMC_AddressSetupTime = 0;
    p.FSMC_AddressHoldTime = 0;
    p.FSMC_DataSetupTime = 2;
    p.FSMC_BusTurnAroundDuration = 0;
    p.FSMC_CLKDivision = 0;
    p.FSMC_DataLatency = 0;
    p.FSMC_AccessMode = FSMC_AccessMode_A;

    FSMC_NORSRAMInitStructure.FSMC_Bank = FSMC_Bank1_NORSRAM1;
    FSMC_NORSRAMInitStructure.FSMC_DataAddressMux = FSMC_DataAddressMux_Disable;
    FSMC_NORSRAMInitStructure.FSMC_MemoryType = FSMC_MemoryType_SRAM;
    FSMC_NORSRAMInitStructure.FSMC_MemoryDataWidth = FSMC_MemoryDataWidth_16b;
    FSMC_NORSRAMInitStructure.FSMC_BurstAccessMode = FSMC_BurstAccessMode_Disable;
    FSMC_NORSRAMInitStructure.FSMC_AsynchronousWait = FSMC_AsynchronousWait_Disable;
    FSMC_NORSRAMInitStructure.FSMC_WaitSignalPolarity = FSMC_WaitSignalPolarity_Low;
    FSMC_NORSRAMInitStructure.FSMC_WrapMode = FSMC_WrapMode_Disable;
    FSMC_NORSRAMInitStructure.FSMC_WaitSignalActive = FSMC_WaitSignalActive_BeforeWaitState;
    FSMC_NORSRAMInitStructure.FSMC_WriteOperation = FSMC_WriteOperation_Enable;
    FSMC_NORSRAMInitStructure.FSMC_WaitSignal = FSMC_WaitSignal_Disable;
    FSMC_NORSRAMInitStructure.FSMC_ExtendedMode = FSMC_ExtendedMode_Disable;
    FSMC_NORSRAMInitStructure.FSMC_WriteBurst = FSMC_WriteBurst_Disable;
    FSMC_NORSRAMInitStructure.FSMC_ReadWriteTimingStruct = &p;
    FSMC_NORSRAMInitStructure.FSMC_WriteTimingStruct = &p;

    FSMC_NORSRAMInit(&FSMC_NORSRAMInitStructure);
    /* Enable FSMC Bank1_SRAM Bank */
    FSMC_NORSRAMCmd(FSMC_Bank1_NORSRAM1, ENABLE);

//	sram_test();
}

#if 0
/* memtest */
void sram_test(void)
{
	unsigned int temp;
	unsigned char *p_extram = (unsigned char *)STM32_EXT_SRAM_BEGIN;
	
	DBG_PRINTF("mram testing...\n");
	p_extram = (unsigned char *)STM32_EXT_SRAM_BEGIN;
	for(temp = 0; temp < (STM32_EXT_SRAM_END - STM32_EXT_SRAM_BEGIN); temp++)
	{
		*p_extram++ = (unsigned char)temp;
	}
	
	p_extram = (unsigned char *)STM32_EXT_SRAM_BEGIN;
	for(temp = 0; temp < (STM32_EXT_SRAM_END - STM32_EXT_SRAM_BEGIN); temp++)
	{
		if( *p_extram++ != (unsigned char)temp )
		{
			DBG_PRINTF("mram test error!\n");
			while(1);
		}
	}
	
	DBG_PRINTF("mram test passed.\n");
}
#endif
