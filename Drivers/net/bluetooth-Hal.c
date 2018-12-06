#include "sys_config.h"
#include "bluetooth.h"
#include "bluetooth-Hal.h"


/****************************************************************************************************************
** ��������:	USART1_SendString
** ��������:	USART1�����ַ���
** �������:	*ch - �����͵��ַ���ָ��
** �� �� ֵ:	��
** ��    ע:	��
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
** ��������:	USART1_SendData
** ��������:	USART1����ָ������������
** �������:	*ch - �����͵��ַ���ָ��
**				num - ���͸���
** �� �� ֵ:	��
** ��    ע:	��
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


// USART1��ʼ��
void USART1_Init(void)
{
	DMA_InitTypeDef  DMA_InitStruct;		// ����DMA�ṹ��
	NVIC_InitTypeDef  NVIC_InitStruct;		// ����NVIC��ʼ���ṹ��
	GPIO_InitTypeDef  GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;
	
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_USART1 | RCC_APB2Periph_AFIO, ENABLE);
	
    /* USART1�˿����ã�PA9 TX �������������PA10 RX ��������ģʽ */
	GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_9;
	GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	
	GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_10;
	GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_IN_FLOATING;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	
    /* USART1���� */
	USART_InitStructure.USART_BaudRate = 9600;
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;
	USART_InitStructure.USART_StopBits = USART_StopBits_1;
	USART_InitStructure.USART_Parity = USART_Parity_No;
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_Init(USART1, &USART_InitStructure);
	
	// �����ж�ʹ��
	NVIC_InitStruct.NVIC_IRQChannel = USART1_IRQn;
	NVIC_InitStruct.NVIC_IRQChannelPreemptionPriority = 1;
	NVIC_InitStruct.NVIC_IRQChannelSubPriority = 1;
	NVIC_InitStruct.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStruct);
	
	// DMA����
	DMA_DeInit(DMA1_Channel5);									/* USART1��RX��DMA1_Channel5���� */
	DMA_InitStruct.DMA_PeripheralBaseAddr = (u32)(&USART1->DR);	/* DMA�������ַ */
	DMA_InitStruct.DMA_MemoryBaseAddr = (u32)BlueRecvBuffer;	/* �ڴ����ַ */
	DMA_InitStruct.DMA_DIR = DMA_DIR_PeripheralSRC;				/* ���ݴ��䷽��(�������)��DMA_DIR_PeripheralDST:������Ϊ���ݴ����Ŀ�ĵ�,DMA_DIR_PeripheralSRC:������Ϊ���ݴ������Դ */
	DMA_InitStruct.DMA_BufferSize = BLUE_BUFFER_SIZE;				/* ����ָ��DMAͨ����DMA����Ĵ�С�����ó�ÿһ��ѭ������Ҫ��������ݸ�����ͨ���������С���ó���ͬ�� */
	DMA_InitStruct.DMA_PeripheralInc = DMA_PeripheralInc_Disable;	/* ��ֹ�����ַ�Ĵ������� */
	DMA_InitStruct.DMA_MemoryInc = DMA_MemoryInc_Enable;			/* ʹ���ڴ��ַ�Ĵ������� */
	DMA_InitStruct.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;	/* �趨�������ݿ��(8λ) */
	DMA_InitStruct.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;	/* �趨�ڴ����ݿ��(8λ) */
	DMA_InitStruct.DMA_Mode = DMA_Mode_Normal;			/* DMA_Mode_Normal:��������ģʽ��DMA_Mode_Circular:ѭ������ģʽ */
	DMA_InitStruct.DMA_Priority = DMA_Priority_High;	/* �趨DMAͨ��x��������ȼ��� */
	DMA_InitStruct.DMA_M2M = DMA_M2M_Disable;			/* ��ֹDMAͨ�����ڴ浽�ڴ洫�� */
	DMA_Init(DMA1_Channel5, &DMA_InitStruct);			/* ����DMA_InitStruct��ָ���Ĳ�����ʼ��DMA_channel5 */
	
	DMA_Cmd(DMA1_Channel5, ENABLE);
	USART_DMACmd(USART1, USART_DMAReq_Rx, ENABLE);		/* ʹ��USART1���յ�DMA����ӳ�� */
	
	USART_Cmd(USART1, ENABLE);							/* ʹ�ܴ��� */
	USART_ITConfig(USART1, USART_IT_IDLE, ENABLE);		/* ʹ�ܴ������߿����ж� */
	
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

