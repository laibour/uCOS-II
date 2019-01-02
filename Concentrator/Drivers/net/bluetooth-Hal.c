#include "sys_config.h"
#include "bluetooth.h"
#include "bluetooth-Hal.h"


/****************************************************************************************************************
** 函数名称:	USART1_SendString
** 功能描述:	USART1发送字符串
** 输入参数:	*ch - 待发送的字符串指针
** 返 回 值:	无
** 备    注:	无
****************************************************************************************************************/
void USART1_SendString(uint8_t *ch)
{
	while(*ch != 0)
	{
		while(!USART_GetFlagStatus(USART1, USART_FLAG_TXE));
		USART_SendData(USART1, *ch);
		ch++;
	}
}

/****************************************************************************************************************
** 函数名称:	USART1_SendData
** 功能描述:	USART1发送指定个数的数据
** 输入参数:	*ch - 待发送的字符串指针
**				num - 发送个数
** 返 回 值:	无
** 备    注:	无
****************************************************************************************************************/
void USART1_SendData(uint8_t *ch, uint8_t num)
{
	while(num != 0)
	{
		while(!USART_GetFlagStatus(USART1, USART_FLAG_TXE));
		USART_SendData(USART1, *ch);
		ch++;
		num--;
	}
}


// USART1初始化
void USART1_Init(void)
{
	DMA_InitTypeDef  DMA_InitStruct;		// 定义DMA结构体
	NVIC_InitTypeDef  NVIC_InitStruct;		// 定义NVIC初始化结构体
	GPIO_InitTypeDef  GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;
	
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_USART1 | RCC_APB2Periph_AFIO, ENABLE);
	
    /* USART1端口配置，PA9 TX 复用推挽输出、PA10 RX 浮空输入模式 */
	GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_9;
	GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	
	GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_10;
	GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_IN_FLOATING;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	
    /* USART1配置 */
	USART_InitStructure.USART_BaudRate = 9600;
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;
	USART_InitStructure.USART_StopBits = USART_StopBits_1;
	USART_InitStructure.USART_Parity = USART_Parity_No;
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_Init(USART1, &USART_InitStructure);
	
	// 串口中断使能
	NVIC_InitStruct.NVIC_IRQChannel = USART1_IRQn;
	NVIC_InitStruct.NVIC_IRQChannelPreemptionPriority = 1;
	NVIC_InitStruct.NVIC_IRQChannelSubPriority = 1;
	NVIC_InitStruct.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStruct);
	
	// DMA设置
	DMA_DeInit(DMA1_Channel5);									/* USART1的RX与DMA1_Channel5相连 */
	DMA_InitStruct.DMA_PeripheralBaseAddr = (u32)(&USART1->DR);	/* DMA外设基地址 */
	DMA_InitStruct.DMA_MemoryBaseAddr = (u32)BlueRecvBuffer;	/* 内存基地址 */
	DMA_InitStruct.DMA_DIR = DMA_DIR_PeripheralSRC;				/* 数据传输方向(从外设读)，DMA_DIR_PeripheralDST:外设作为数据传输的目的地,DMA_DIR_PeripheralSRC:外设作为数据传输的来源 */
	DMA_InitStruct.DMA_BufferSize = BLUE_BUFFER_SIZE;				/* 定义指定DMA通道的DMA缓存的大小，设置成每一次循环中需要传输的数据个数。通常和数组大小设置成相同。 */
	DMA_InitStruct.DMA_PeripheralInc = DMA_PeripheralInc_Disable;	/* 禁止外设地址寄存器递增 */
	DMA_InitStruct.DMA_MemoryInc = DMA_MemoryInc_Enable;			/* 使能内存地址寄存器递增 */
	DMA_InitStruct.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;	/* 设定外设数据宽度(8位) */
	DMA_InitStruct.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;	/* 设定内存数据宽度(8位) */
	DMA_InitStruct.DMA_Mode = DMA_Mode_Normal;			/* DMA_Mode_Normal:正常缓存模式，DMA_Mode_Circular:循环缓存模式 */
	DMA_InitStruct.DMA_Priority = DMA_Priority_High;	/* 设定DMA通道x的软件优先级别 */
	DMA_InitStruct.DMA_M2M = DMA_M2M_Disable;			/* 禁止DMA通道的内存到内存传输 */
	DMA_Init(DMA1_Channel5, &DMA_InitStruct);			/* 根据DMA_InitStruct中指定的参数初始化DMA_channel5 */
	
	DMA_Cmd(DMA1_Channel5, ENABLE);
	USART_DMACmd(USART1, USART_DMAReq_Rx, ENABLE);		/* 使能USART1接收的DMA请求映像 */
	
	USART_Cmd(USART1, ENABLE);							/* 使能串口 */
	USART_ITConfig(USART1, USART_IT_IDLE, ENABLE);		/* 使能串口总线空闲中断 */
	
//	DMA_ITConfig(DMA1_Channel4, DMA_IT_TC, ENABLE);
//	USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);
}


void BluetoothIOInit(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	
	RCC_APB2PeriphClockCmd(BLUETOOTH_POWER_RCC | RCC_APB2Periph_AFIO, ENABLE);
	
	GPIO_InitStructure.GPIO_Pin   = BLUETOOTH_POWER_PIN;	// BLUETOOTH POWER PIN
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(BLUETOOTH_POWER_GPIO, &GPIO_InitStructure);
	
	BLUETOOTH_POWER_OFF();									// BLUETOOTH POWER OFF
}

