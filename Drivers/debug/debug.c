#include "sys_config.h"
#include "debug.h"
/* use UART4 as debug & program port */

uint8_t ComSendFlag = 0;
uint8_t ComSendSize = 0;
uint8_t ComRecvFlag = 0;
uint8_t ComRecvSize = 0;
uint8_t ComSendBuffer[COM_BUFFER_SIZE];
uint8_t ComRecvBuffer[COM_BUFFER_SIZE];

/* ���Դ�ӡ��� */
void DataPrint(uint8_t *data, uint8_t len)
{
  	uint8_t i;
	
	for (i = 0; i< len; i++)
	{
		printf("%02X ", *(data+i));
	}
	printf("\n");
}

/* USART4 ����ָ������������
 * ��ʼ�����ļ��������ò���ʱʹ��
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

/* UART4�ж� */
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
		DMA2_Channel3->CNDTR = COM_BUFFER_SIZE;	/* ����ָ��DMAͨ���Ļ���Ĵ�С */
		DMA_Cmd(DMA2_Channel3, ENABLE);
		ComRecvFlag = 1;
	}
}

/****************************************************************************************************************
** ��������:	DebugInit
** ��������:	Debug/UART4 ��ʼ������
** �������:	��
** �� �� ֵ:	��
** ��    ע:	��
****************************************************************************************************************/
void DebugInit(void)
{
	DMA_InitTypeDef   DMA_InitStruct;		// ����DMA�ṹ��
	NVIC_InitTypeDef  NVIC_InitStruct;		// ����NVIC��ʼ���ṹ��
	GPIO_InitTypeDef  GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;
	
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA2, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC | RCC_APB2Periph_AFIO, ENABLE);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_UART4, ENABLE);
	
	/* USART4�˿����� */
	GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_10;
	GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOC, &GPIO_InitStructure);
	
	GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_11;
	GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_IN_FLOATING;
	GPIO_Init(GPIOC, &GPIO_InitStructure);
	
    /* USART4 ����ģʽΪ 115200 8-N-1���жϽ��� */
	USART_InitStructure.USART_BaudRate = 115200;
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;
	USART_InitStructure.USART_StopBits = USART_StopBits_1;
	USART_InitStructure.USART_Parity = USART_Parity_No;
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
	USART_Init(UART4, &USART_InitStructure);
	
	/* �����ж�ʹ�� */
	NVIC_InitStruct.NVIC_IRQChannel = UART4_IRQn;
	NVIC_InitStruct.NVIC_IRQChannelPreemptionPriority = 2;
	NVIC_InitStruct.NVIC_IRQChannelSubPriority = 3;
	NVIC_InitStruct.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStruct);
	
	// DMA����
	DMA_DeInit(DMA2_Channel3);										/* USART2��RX��DMA1_Channel6���� */
	DMA_InitStruct.DMA_PeripheralBaseAddr = (u32)(&UART4->DR);		/* DMA�������ַ */
	DMA_InitStruct.DMA_MemoryBaseAddr = (u32)ComRecvBuffer;			/* �ڴ����ַ */
	DMA_InitStruct.DMA_DIR = DMA_DIR_PeripheralSRC;					/* ���ݴ��䷽��(�������)��DMA_DIR_PeripheralDST:������Ϊ���ݴ����Ŀ�ĵ�,DMA_DIR_PeripheralSRC:������Ϊ���ݴ������Դ */
	DMA_InitStruct.DMA_BufferSize = COM_BUFFER_SIZE;				/* ����ָ��DMAͨ����DMA����Ĵ�С�����ó�ÿһ��ѭ������Ҫ��������ݸ�����ͨ���������С���ó���ͬ�� */
	DMA_InitStruct.DMA_PeripheralInc = DMA_PeripheralInc_Disable;	/* ��ֹ�����ַ�Ĵ������� */
	DMA_InitStruct.DMA_MemoryInc = DMA_MemoryInc_Enable;			/* ʹ���ڴ��ַ�Ĵ������� */
	DMA_InitStruct.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;	/* �趨�������ݿ��(8λ) */
	DMA_InitStruct.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;	/* �趨�ڴ����ݿ��(8λ) */
	DMA_InitStruct.DMA_Mode = DMA_Mode_Normal;			/* DMA_Mode_Normal:��������ģʽ��DMA_Mode_Circular:ѭ������ģʽ */
	DMA_InitStruct.DMA_Priority = DMA_Priority_High;	/* �趨DMAͨ��x��������ȼ��� */
	DMA_InitStruct.DMA_M2M = DMA_M2M_Disable;			/* ��ֹDMAͨ�����ڴ浽�ڴ洫�� */
	DMA_Init(DMA2_Channel3, &DMA_InitStruct);			/* ����DMA_InitStruct��ָ���Ĳ�����ʼ��DMA_channel6 */
	
	DMA_Cmd(DMA2_Channel3, ENABLE);
	USART_DMACmd(UART4, USART_DMAReq_Rx, ENABLE);		/* ʹ��UART4���յ�DMA����ӳ�� */
	
	USART_Cmd(UART4, ENABLE);							/* ʹ�ܴ��� */
	USART_ITConfig(UART4, USART_IT_IDLE, ENABLE);		/* ʹ�ܴ������߿����ж� */
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
