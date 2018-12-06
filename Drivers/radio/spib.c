#include "spib.h"


// SPI2 I/O PORT
#define SPIB_INTERFACE                              SPI2
#define SPIB_CLK                                    RCC_APB1Periph_SPI2

#define SPIB_PIN_SCK_PORT                           GPIOB
#define SPIB_PIN_SCK_PORT_CLK                       RCC_APB2Periph_GPIOB
#define SPIB_PIN_SCK                                 GPIO_Pin_13

#define SPIB_PIN_MISO_PORT                           GPIOB
#define SPIB_PIN_MISO_PORT_CLK                       RCC_APB2Periph_GPIOB
#define SPIB_PIN_MISO                                GPIO_Pin_14

#define SPIB_PIN_MOSI_PORT                           GPIOB
#define SPIB_PIN_MOSI_PORT_CLK                       RCC_APB2Periph_GPIOB
#define SPIB_PIN_MOSI                                GPIO_Pin_15


/****************************************************************************************************************
** 函数名称:	SpiInit
** 功能描述:	SPI2初始化
** 输入参数:	无
** 返 回 值:	无
** 备    注:	无
****************************************************************************************************************/
void SpibInit( void )
{
    SPI_InitTypeDef SPI_InitStructure;
    GPIO_InitTypeDef GPIO_InitStructure;
	
    /* Enable peripheral clocks */
    /* Enable SPI2 clock and GPIO clock for SPI2 */
    RCC_APB1PeriphClockCmd( SPIB_PIN_MISO_PORT_CLK | SPIB_PIN_MOSI_PORT_CLK |
                            SPIB_PIN_SCK_PORT_CLK, ENABLE );
    RCC_APB1PeriphClockCmd( SPIB_CLK, ENABLE );

    /* GPIO configuration */
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;

    GPIO_InitStructure.GPIO_Pin = SPIB_PIN_SCK;
    GPIO_Init( SPIB_PIN_SCK_PORT, &GPIO_InitStructure );

    GPIO_InitStructure.GPIO_Pin = SPIB_PIN_MOSI;
    GPIO_Init( SPIB_PIN_MOSI_PORT, &GPIO_InitStructure );

    GPIO_InitStructure.GPIO_Pin = SPIB_PIN_MISO;
    GPIO_Init( SPIB_PIN_MISO_PORT, &GPIO_InitStructure );

    // 禁用JTAG
	RCC_APB2PeriphClockCmd( RCC_APB2Periph_AFIO, ENABLE );
    GPIO_PinRemapConfig( GPIO_Remap_SWJ_JTAGDisable, ENABLE );

    /* SPIB_INTERFACE Config */
    SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
    SPI_InitStructure.SPI_Mode = SPI_Mode_Master;
    SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b;
    SPI_InitStructure.SPI_CPOL = SPI_CPOL_Low;
    SPI_InitStructure.SPI_CPHA = SPI_CPHA_1Edge;
    SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;
    SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_8; // 72/8 MHz
    SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;
    SPI_InitStructure.SPI_CRCPolynomial = 7;
    SPI_Init( SPIB_INTERFACE, &SPI_InitStructure );
    SPI_Cmd( SPIB_INTERFACE, ENABLE );
}


uint8_t SpibInOut( uint8_t outData )
{
    /* Send SPIy data */
    SPI_I2S_SendData( SPIB_INTERFACE, outData );
    while( SPI_I2S_GetFlagStatus( SPIB_INTERFACE, SPI_I2S_FLAG_RXNE ) == RESET );
    return SPI_I2S_ReceiveData( SPIB_INTERFACE );
}

