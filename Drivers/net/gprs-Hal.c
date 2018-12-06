#include "sys_config.h"
#include "gprs.h"
#include "gprs-Hal.h"


void USART2_Init(void)
{
	DMA_InitTypeDef  DMA_InitStruct;		// 定义DMA结构体
	NVIC_InitTypeDef  NVIC_InitStruct;		// 定义NVIC初始化结构体
	GPIO_InitTypeDef  GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;
	
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_AFIO, ENABLE);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);

	/* USART2端口配置，PD5 TX 复用推挽输场、PD6 RX 浮空输入模式 */
	//GPIO_PinRemapConfig(GPIO_Remap_USART2, ENABLE);

	GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_2;
	GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_3;
	GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_IN_FLOATING;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
    
    /* USART2配置 */
	USART_InitStructure.USART_BaudRate = 115200;
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;
	USART_InitStructure.USART_StopBits = USART_StopBits_1;
	USART_InitStructure.USART_Parity = USART_Parity_No;
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
	USART_Init(USART2, &USART_InitStructure);
	
	/* 串口中断使能 */
	NVIC_InitStruct.NVIC_IRQChannel = USART2_IRQn;
	NVIC_InitStruct.NVIC_IRQChannelPreemptionPriority = 2;
	NVIC_InitStruct.NVIC_IRQChannelSubPriority = 1;
	NVIC_InitStruct.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStruct);
	
	// DMA设置
	DMA_DeInit(DMA1_Channel6);									/* USART2的RX与DMA1_Channel6相连 */
	DMA_InitStruct.DMA_PeripheralBaseAddr = (u32)(&USART2->DR);	/* DMA外设基地址 */
	DMA_InitStruct.DMA_MemoryBaseAddr = (u32)GPRSRecvBuffer;		/* 内存基地址 */
	DMA_InitStruct.DMA_DIR = DMA_DIR_PeripheralSRC;				/* 数据传输方向(从外设读)，DMA_DIR_PeripheralDST:外设作为数据传输的目的地,DMA_DIR_PeripheralSRC:外设作为数据传输的来源 */
	DMA_InitStruct.DMA_BufferSize = GPRS_BUFFER_SIZE;				/* 定义指定DMA通道的DMA缓存的大小，设置成每一次循环中需要传输的数据个数。通常和数组大小设置成相同。 */
	DMA_InitStruct.DMA_PeripheralInc = DMA_PeripheralInc_Disable;	/* 禁止外设地址寄存器递增 */
	DMA_InitStruct.DMA_MemoryInc = DMA_MemoryInc_Enable;			/* 使能内存地址寄存器递增 */
	DMA_InitStruct.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;	/* 设定外设数据宽度(8位) */
	DMA_InitStruct.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;	/* 设定内存数据宽度(8位) */
	DMA_InitStruct.DMA_Mode = DMA_Mode_Normal;			/* DMA_Mode_Normal:正常缓存模式，DMA_Mode_Circular:循环缓存模式 */
	DMA_InitStruct.DMA_Priority = DMA_Priority_High;	/* 设定DMA通道x的软件优先级别 */
	DMA_InitStruct.DMA_M2M = DMA_M2M_Disable;			/* 禁止DMA通道的内存到内存传输 */
	DMA_Init(DMA1_Channel6, &DMA_InitStruct);			/* 根据DMA_InitStruct中指定的参数初始化DMA_channel6 */
	
	DMA_Cmd(DMA1_Channel6, ENABLE);
	USART_DMACmd(USART2, USART_DMAReq_Rx, ENABLE);		/* 使能USART2接收的DMA请求映像 */
	
	USART_Cmd(USART2, ENABLE);							/* 使能串口 */
	USART_ITConfig(USART2, USART_IT_IDLE, ENABLE);		/* 使能串口总线空闲中断 */
	
//	DMA_ITConfig(DMA1_Channel6, DMA_IT_TC, ENABLE);
//	USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);
}

/* USART2 发送一个字符串 */
void USART2_SendString(uint8_t *ch)
{
	while(*ch != 0)
	{
		while(!USART_GetFlagStatus(USART2, USART_FLAG_TXE));
		USART_SendData(USART2, *ch);
		ch++;
	}
}

/* USART2 发送指定个数的数据 */
void USART2_SendData(uint8_t *chr, uint8_t num)
{
	while(num != 0)
	{
		while(!USART_GetFlagStatus(USART2, USART_FLAG_TXE));
		USART_SendData(USART2, *chr);
		chr++;
		num--;
	}
}


void GPRSIOInit(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	
	RCC_APB2PeriphClockCmd(GPRS_LED_RCC | GPRS_DCD_RCC | GPRS_DTR_RCC | GPRS_POWER_RCC | GPRS_PWRKEY_RCC, ENABLE);
	
	GPIO_InitStructure.GPIO_Pin   = GPRS_PWRKEY_PIN;	// GPRS PWRKEY
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPRS_PWRKEY_GPIO, &GPIO_InitStructure);
	
	GPIO_InitStructure.GPIO_Pin   = GPRS_POWER_PIN;		// GPRS POWER
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPRS_POWER_GPIO, &GPIO_InitStructure);
	
	GPIO_InitStructure.GPIO_Pin   = GPRS_LED_PIN; 		// GPRS LED
 	GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
 	GPIO_Init(GPRS_LED_GPIO, &GPIO_InitStructure);
	   
	GPIO_InitStructure.GPIO_Pin   = GPRS_DCD_PIN;		// GPRS DCD
	GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_IN_FLOATING;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPRS_DCD_GPIO, &GPIO_InitStructure);
	
	GPIO_InitStructure.GPIO_Pin   = GPRS_DTR_PIN; 		// GPRS DTR
 	GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
 	GPIO_Init(GPRS_DTR_GPIO, &GPIO_InitStructure);
	
	GPRS_LED_OFF();										// TURN OFF GPRS LED
//	GPRS_DTR_OFF();										// turn off gprs DTR pin
	GPRS_DTR_ON();
	GPRS_POWER_OFF();									// GPRS POWER OFF
	GPRS_PWRKEY_OFF();									// GPRS POWKEY OFF
}
