#include "spia.h"


// SPI1
#define SPIA_INTERFACE                               SPI1
#define SPIA_CLK                                     RCC_APB2Periph_SPI1

#define SPIA_PIN_SCK_PORT                            GPIOA
#define SPIA_PIN_SCK_PORT_CLK                        RCC_APB2Periph_GPIOA
#define SPIA_PIN_SCK                                 GPIO_Pin_5

#define SPIA_PIN_MISO_PORT                           GPIOA
#define SPIA_PIN_MISO_PORT_CLK                       RCC_APB2Periph_GPIOA
#define SPIA_PIN_MISO                                GPIO_Pin_6

#define SPIA_PIN_MOSI_PORT                           GPIOA
#define SPIA_PIN_MOSI_PORT_CLK                       RCC_APB2Periph_GPIOA
#define SPIA_PIN_MOSI                                GPIO_Pin_7


/****************************************************************************************************************
** 函数名称:	SpiaInit
** 功能描述:	SPI1初始化
** 输入参数:	无
** 返 回 值:	无
** 备    注:	无
****************************************************************************************************************/
void SpiaInit( void )
{
    SPI_InitTypeDef SPI_InitStructure;
    GPIO_InitTypeDef GPIO_InitStructure;
	
    /* Enable peripheral clocks */
    /* Enable SPI2 clock and GPIO clock for SPI2 */
    RCC_APB2PeriphClockCmd( SPIA_PIN_MISO_PORT_CLK | SPIA_PIN_MOSI_PORT_CLK |
                            SPIA_PIN_SCK_PORT_CLK, ENABLE );
    RCC_APB2PeriphClockCmd( SPIA_CLK, ENABLE );

    /* GPIO configuration */
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;

    GPIO_InitStructure.GPIO_Pin = SPIA_PIN_SCK;
    GPIO_Init( SPIA_PIN_SCK_PORT, &GPIO_InitStructure );

    GPIO_InitStructure.GPIO_Pin = SPIA_PIN_MOSI;
    GPIO_Init( SPIA_PIN_MOSI_PORT, &GPIO_InitStructure );

    GPIO_InitStructure.GPIO_Pin = SPIA_PIN_MISO;
    GPIO_Init( SPIA_PIN_MISO_PORT, &GPIO_InitStructure );

    // 禁用JTAG
	RCC_APB2PeriphClockCmd( RCC_APB2Periph_AFIO, ENABLE );
    GPIO_PinRemapConfig( GPIO_Remap_SWJ_JTAGDisable, ENABLE );

    /* SPIA_INTERFACE Config */
    SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
    SPI_InitStructure.SPI_Mode = SPI_Mode_Master;
    SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b;
    SPI_InitStructure.SPI_CPOL = SPI_CPOL_Low;
    SPI_InitStructure.SPI_CPHA = SPI_CPHA_1Edge;
    SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;
    SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_8; // 72/8 MHz
    SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;
    SPI_InitStructure.SPI_CRCPolynomial = 7;
    SPI_Init( SPIA_INTERFACE, &SPI_InitStructure );
    SPI_Cmd( SPIA_INTERFACE, ENABLE );
}


uint8_t SpiaInOut( uint8_t outData )
{
    /* Send SPIy data */
    SPI_I2S_SendData( SPIA_INTERFACE, outData );
    while( SPI_I2S_GetFlagStatus( SPIA_INTERFACE, SPI_I2S_FLAG_RXNE ) == RESET );
    return SPI_I2S_ReceiveData( SPIA_INTERFACE );
}

