#include "sys_config.h"
#include "debug.h"
/* use UART4 as debug & program port */

uint8_t ComSendFlag = 0;
uint8_t ComSendSize = 0;
uint8_t ComRecvFlag = 0;
uint8_t ComRecvSize = 0;
uint8_t ComSendBuffer[COM_BUFFER_SIZE];
uint8_t ComRecvBuffer[COM_BUFFER_SIZE];

/* 调试打印输出 */
void DataPrint(uint8_t *data, uint8_t len)
{
  	uint8_t i;
	
	for (i = 0; i< len; i++)
	{
		printf("%02X ", *(data+i));
	}
	printf("\n");
}

/* USART4 发送指定个数的数据
 * 初始化中心集中器配置参数时使用
 */
void DebugSendData(uint8_t *chr, uint8_t num)
{
	while(num != 0)
	{
		while(!USART_GetFlagStatus(UART4, USART_FLAG_TXE));
		USART_SendData(UART4, *chr);
		chr++;
		num--;
	}
}

/* UART4中断 */
void UART4_IRQHandler(void)
{
	uint8_t num;
	
	if (USART_GetITStatus(UART4, USART_IT_IDLE) == SET)
	{
		num = num;
		num = UART4->SR;
		num = UART4->DR;
		DMA_Cmd(DMA2_Channel3, DISABLE);
		ComRecvSize = COM_BUFFER_SIZE - DMA_GetCurrDataCounter(DMA2_Channel3);
		DMA2_Channel3->CNDTR = COM_BUFFER_SIZE;	/* 定义指定DMA通道的缓存的大小 */
		DMA_Cmd(DMA2_Channel3, ENABLE);
		ComRecvFlag = 1;
	}
}

/****************************************************************************************************************
** 函数名称:	DebugInit
** 功能描述:	Debug/UART4 初始化配置
** 输入参数:	无
** 返 回 值:	无
** 备    注:	无
****************************************************************************************************************/
void DebugInit(void)
{
	DMA_InitTypeDef   DMA_InitStruct;		// 定义DMA结构体
	NVIC_InitTypeDef  NVIC_InitStruct;		// 定义NVIC初始化结构体
	GPIO_InitTypeDef  GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;
	
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA2, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC | RCC_APB2Periph_AFIO, ENABLE);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_UART4, ENABLE);
	
	/* USART4端口配置 */
	GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_10;
	GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOC, &GPIO_InitStructure);
	
	GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_11;
	GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_IN_FLOATING;
	GPIO_Init(GPIOC, &GPIO_InitStructure);
	
    /* USART4 配置模式为 115200 8-N-1，中断接收 */
	USART_InitStructure.USART_BaudRate = 115200;
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;
	USART_InitStructure.USART_StopBits = USART_StopBits_1;
	USART_InitStructure.USART_Parity = USART_Parity_No;
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
	USART_Init(UART4, &USART_InitStructure);
	
	/* 串口中断使能 */
	NVIC_InitStruct.NVIC_IRQChannel = UART4_IRQn;
	NVIC_InitStruct.NVIC_IRQChannelPreemptionPriority = 2;
	NVIC_InitStruct.NVIC_IRQChannelSubPriority = 3;
	NVIC_InitStruct.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStruct);
	
	// DMA设置
	DMA_DeInit(DMA2_Channel3);										/* USART2的RX与DMA1_Channel6相连 */
	DMA_InitStruct.DMA_PeripheralBaseAddr = (u32)(&UART4->DR);		/* DMA外设基地址 */
	DMA_InitStruct.DMA_MemoryBaseAddr = (u32)ComRecvBuffer;			/* 内存基地址 */
	DMA_InitStruct.DMA_DIR = DMA_DIR_PeripheralSRC;					/* 数据传输方向(从外设读)，DMA_DIR_PeripheralDST:外设作为数据传输的目的地,DMA_DIR_PeripheralSRC:外设作为数据传输的来源 */
	DMA_InitStruct.DMA_BufferSize = COM_BUFFER_SIZE;				/* 定义指定DMA通道的DMA缓存的大小，设置成每一次循环中需要传输的数据个数。通常和数组大小设置成相同。 */
	DMA_InitStruct.DMA_PeripheralInc = DMA_PeripheralInc_Disable;	/* 禁止外设地址寄存器递增 */
	DMA_InitStruct.DMA_MemoryInc = DMA_MemoryInc_Enable;			/* 使能内存地址寄存器递增 */
	DMA_InitStruct.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;	/* 设定外设数据宽度(8位) */
	DMA_InitStruct.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;	/* 设定内存数据宽度(8位) */
	DMA_InitStruct.DMA_Mode = DMA_Mode_Normal;			/* DMA_Mode_Normal:正常缓存模式，DMA_Mode_Circular:循环缓存模式 */
	DMA_InitStruct.DMA_Priority = DMA_Priority_High;	/* 设定DMA通道x的软件优先级别 */
	DMA_InitStruct.DMA_M2M = DMA_M2M_Disable;			/* 禁止DMA通道的内存到内存传输 */
	DMA_Init(DMA2_Channel3, &DMA_InitStruct);			/* 根据DMA_InitStruct中指定的参数初始化DMA_channel6 */
	
	DMA_Cmd(DMA2_Channel3, ENABLE);
	USART_DMACmd(UART4, USART_DMAReq_Rx, ENABLE);		/* 使能UART4接收的DMA请求映像 */
	
	USART_Cmd(UART4, ENABLE);							/* 使能串口 */
	USART_ITConfig(UART4, USART_IT_IDLE, ENABLE);		/* 使能串口总线空闲中断 */
}

#if 0
void UART4_SendString(uint8_t *ch)
{
	while(*ch != 0)
	{
		while(!USART_GetFlagStatus(UART4, USART_FLAG_TXE));
		USART_SendData(UART4, *ch);
		ch++;
	}
}
#endif
