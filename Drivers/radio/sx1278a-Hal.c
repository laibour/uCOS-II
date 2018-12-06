#include "platform.h"
#include "sx1278a-Hal.h"
#include "spia.h"

/*!
 * SX1278A RESET I/O definitions
 */
#define RESET_IOPORT                                GPIOC
#define RESET_PIN                                   GPIO_Pin_6

/*!
 * SX1278A SPI NSS I/O definitions
 */
#define NSS_IOPORT                                  GPIOA
#define NSS_PIN                                     GPIO_Pin_4     // GPIO_Pin_12

/*!
 * SX1278A DIO pins  I/O definitions
 */
#define DIO0_IOPORT                                 GPIOB
#define DIO0_PIN                                    GPIO_Pin_7

#define DIO1_IOPORT                                 GPIOB
#define DIO1_PIN                                    GPIO_Pin_8

#define DIO2_IOPORT                                 GPIOF
#define DIO2_PIN                                    GPIO_Pin_6

#define DIO3_IOPORT                                 GPIOF
#define DIO3_PIN                                    GPIO_Pin_7

#define DIO4_IOPORT                                 GPIOF
#define DIO4_PIN                                    GPIO_Pin_6

#define DIO5_IOPORT                                 GPIOF
#define DIO5_PIN                                    GPIO_Pin_7


/* Y模式扩频输出功率放大器控制端口初始化 */
void A_RFPA0133_ON(void)
{
	GPIO_SetBits(GPIOB, GPIO_Pin_9);	// 输出高
	GPIO_ResetBits(GPIOB, GPIO_Pin_6);	// G8
	GPIO_SetBits(GPIOB, GPIO_Pin_5);	// G16
}


void A_RFPA0133_OFF(void)
{
	GPIO_ResetBits(GPIOB, GPIO_Pin_9); 				// 输出低
	GPIO_ResetBits(GPIOB, GPIO_Pin_5 | GPIO_Pin_6);	// 输出低
}


/* 功放输出使能初始化 */
void A_RFPA0133_Init(void)
{
	GPIO_InitTypeDef  GPIO_InitStructure; 	
 	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB | RCC_APB2Periph_GPIOC, ENABLE);	 // 使能PE端口时钟

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5 | GPIO_Pin_6;	// 推挽输出  
 	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;		// 推挽输出
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;   	// 速度50MHz
 	GPIO_Init(GPIOB, &GPIO_InitStructure);
	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;				// 推挽输出  
 	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;		// 推挽输出
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;   	// 速度50MHz
 	GPIO_Init(GPIOB, &GPIO_InitStructure);
 	
	A_RFPA0133_OFF();										// 默认值关闭
}


void SX1278AInitIo( void )
{
    GPIO_InitTypeDef GPIO_InitStructure;


    RCC_APB2PeriphClockCmd( RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB | RCC_APB2Periph_GPIOC | 
                            RCC_APB2Periph_GPIOD | RCC_APB2Periph_GPIOF | RCC_APB2Periph_AFIO, ENABLE );

    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    // Configure SPI-->NSS as output
    GPIO_InitStructure.GPIO_Pin = NSS_PIN;
    GPIO_Init( NSS_IOPORT, &GPIO_InitStructure );
	GPIO_WriteBit( NSS_IOPORT, NSS_PIN, Bit_SET );
	
	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7;		// RF1_SW ON
    GPIO_Init( GPIOC, &GPIO_InitStructure );
	GPIO_WriteBit( GPIOC, GPIO_Pin_7, Bit_SET );
	
	
    // Configure radio DIO as inputs
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;

    // Configure DIO0
    GPIO_InitStructure.GPIO_Pin =  DIO0_PIN;
    GPIO_Init( DIO0_IOPORT, &GPIO_InitStructure );
    
    // Configure DIO1
    GPIO_InitStructure.GPIO_Pin =  DIO1_PIN;
    GPIO_Init( DIO1_IOPORT, &GPIO_InitStructure );
    
    // Configure DIO2
    GPIO_InitStructure.GPIO_Pin =  DIO2_PIN;
    GPIO_Init( DIO2_IOPORT, &GPIO_InitStructure );
    
    // REAMARK: DIO3/4/5 configured are connected to IO expander

    // Configure DIO3 as input
    GPIO_InitStructure.GPIO_Pin =  DIO3_PIN;
    GPIO_Init( DIO3_IOPORT, &GPIO_InitStructure );
    // Configure DIO4 as input
    GPIO_InitStructure.GPIO_Pin =  DIO4_PIN;
    GPIO_Init( DIO4_IOPORT, &GPIO_InitStructure );
    // Configure DIO5 as input
	GPIO_InitStructure.GPIO_Pin =  DIO5_PIN;
    GPIO_Init( DIO5_IOPORT, &GPIO_InitStructure );
	
	SpiaInit( );		// SPI初始化
	
	A_RFPA0133_Init();
}

void SX1278ASetReset( uint8_t state )
{
    GPIO_InitTypeDef GPIO_InitStructure;

    if( state == RADIO_RESET_ON )
    {
        // Configure RESET as output
		GPIO_InitStructure.GPIO_Pin = RESET_PIN;
		GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
        GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_Out_PP;
        GPIO_Init( RESET_IOPORT, &GPIO_InitStructure );
		
		// Set RESET pin to 0
        GPIO_WriteBit( RESET_IOPORT, RESET_PIN, Bit_RESET );
    }
    else
    {
		GPIO_InitStructure.GPIO_Pin =  RESET_PIN;
        GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
		GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
        GPIO_Init( RESET_IOPORT, &GPIO_InitStructure );
		
		// Set RESET pin to 1
        GPIO_WriteBit( RESET_IOPORT, RESET_PIN, Bit_SET );

    }
}

void SX1278AWrite( uint8_t addr, uint8_t data )
{
    SX1278AWriteBuffer( addr, &data, 1 );
}

void SX1278ARead( uint8_t addr, uint8_t *data )
{
    SX1278AReadBuffer( addr, data, 1 );
}

void SX1278AWriteBuffer( uint8_t addr, uint8_t *buffer, uint8_t size )
{
    uint8_t i;

    //NSS = 0;
    GPIO_WriteBit( NSS_IOPORT, NSS_PIN, Bit_RESET );

    SpiaInOut( addr | 0x80 );
    for( i = 0; i < size; i++ )
    {
        SpiaInOut( buffer[i] );
    }

    //NSS = 1;
    GPIO_WriteBit( NSS_IOPORT, NSS_PIN, Bit_SET );
}

void SX1278AReadBuffer( uint8_t addr, uint8_t *buffer, uint8_t size )
{
    uint8_t i;

    //NSS = 0;
    GPIO_WriteBit( NSS_IOPORT, NSS_PIN, Bit_RESET );

    SpiaInOut( addr & 0x7F );

    for( i = 0; i < size; i++ )
    {
        buffer[i] = SpiaInOut( 0 );
    }

    //NSS = 1;
    GPIO_WriteBit( NSS_IOPORT, NSS_PIN, Bit_SET );
}

void SX1278AWriteFifo( uint8_t *buffer, uint8_t size )
{
    SX1278AWriteBuffer( 0, buffer, size );
}

void SX1278AReadFifo( uint8_t *buffer, uint8_t size )
{
    SX1278AReadBuffer( 0, buffer, size );
}

inline uint8_t SX1278AReadDio0( void )
{
    return GPIO_ReadInputDataBit( DIO0_IOPORT, DIO0_PIN );
}

inline uint8_t SX1278AReadDio1( void )
{
    return GPIO_ReadInputDataBit( DIO1_IOPORT, DIO1_PIN );
}

inline uint8_t SX1278AReadDio2( void )
{
    return GPIO_ReadInputDataBit( DIO2_IOPORT, DIO2_PIN );
}

inline uint8_t SX1278AReadDio3( void )
{
    return GPIO_ReadInputDataBit( DIO3_IOPORT, DIO3_PIN );
}

inline uint8_t SX1278AReadDio4( void )
{
    return GPIO_ReadInputDataBit( DIO4_IOPORT, DIO4_PIN );
}

inline uint8_t SX1278AReadDio5( void )
{
    return GPIO_ReadInputDataBit( DIO5_IOPORT, DIO5_PIN );
}


//射频芯片收发切换
inline void SX1278AWriteRxTx( uint8_t txEnable )
{
	;
}

