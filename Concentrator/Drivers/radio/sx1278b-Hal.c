#include "platform.h"
#include "sx1278b-Hal.h"
#include "spib.h"

/*!
 * SX1278B RESET I/O definitions
 */
#define RESET_IOPORT                                GPIOF
#define RESET_PIN                                   GPIO_Pin_11

/*!
 * SX1278B SPI NSS I/O definitions
 */
#define NSS_IOPORT                                  GPIOB
#define NSS_PIN                                     GPIO_Pin_12     // GPIO_Pin_12

/*!
 * SX1278B DIO pins  I/O definitions
 */
#define DIO0_IOPORT                                 GPIOB
#define DIO0_PIN                                    GPIO_Pin_0

#define DIO1_IOPORT                                 GPIOB
#define DIO1_PIN                                    GPIO_Pin_1

#define DIO2_IOPORT                                 GPIOF
#define DIO2_PIN                                    GPIO_Pin_6

#define DIO3_IOPORT                                 GPIOF
#define DIO3_PIN                                    GPIO_Pin_7

#define DIO4_IOPORT                                 GPIOF
#define DIO4_PIN                                    GPIO_Pin_6

#define DIO5_IOPORT                                 GPIOF
#define DIO5_PIN                                    GPIO_Pin_7


/* Y模式扩频输出功率放大器控制端口初始化 */
void B_RFPA0133_ON(void)
{
	GPIO_SetBits(GPIOB, GPIO_Pin_2);	// 输出高
	GPIO_ResetBits(GPIOC, GPIO_Pin_5);	// G8
	GPIO_SetBits(GPIOC, GPIO_Pin_4);	// G16
}


void B_RFPA0133_OFF(void)
{
	GPIO_ResetBits(GPIOB, GPIO_Pin_2); 				// 输出低
	GPIO_ResetBits(GPIOC, GPIO_Pin_4 | GPIO_Pin_5);	// 输出低
}


/* 功放输出使能初始化 */
void B_RFPA0133_Init(void)
{
	GPIO_InitTypeDef  GPIO_InitStructure; 	
 	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB | RCC_APB2Periph_GPIOC, ENABLE);	 // 使能PE端口时钟

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4 | GPIO_Pin_5;	// 推挽输出  
 	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;		// 推挽输出
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;   	// 速度50MHz
 	GPIO_Init(GPIOC, &GPIO_InitStructure);
	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;				// 推挽输出  
 	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;		// 推挽输出
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;   	// 速度50MHz
 	GPIO_Init(GPIOB, &GPIO_InitStructure);
 	
	B_RFPA0133_OFF();										// 默认值关闭
}


void SX1278BInitIo( void )
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
	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7;		// RF2_SW ON
    GPIO_Init( GPIOG, &GPIO_InitStructure );
	GPIO_WriteBit( GPIOG, GPIO_Pin_7, Bit_SET );
	
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
	
	SpibInit( );		// SPI初始化
	
	B_RFPA0133_Init();
}

void SX1278BSetReset( uint8_t state )
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

void SX1278BWrite( uint8_t addr, uint8_t data )
{
    SX1278BWriteBuffer( addr, &data, 1 );
}

void SX1278BRead( uint8_t addr, uint8_t *data )
{
    SX1278BReadBuffer( addr, data, 1 );
}

void SX1278BWriteBuffer( uint8_t addr, uint8_t *buffer, uint8_t size )
{
    uint8_t i;

    //NSS = 0;
    GPIO_WriteBit( NSS_IOPORT, NSS_PIN, Bit_RESET );

    SpibInOut( addr | 0x80 );
    for( i = 0; i < size; i++ )
    {
        SpibInOut( buffer[i] );
    }

    //NSS = 1;
    GPIO_WriteBit( NSS_IOPORT, NSS_PIN, Bit_SET );
}

void SX1278BReadBuffer( uint8_t addr, uint8_t *buffer, uint8_t size )
{
    uint8_t i;

    //NSS = 0;
    GPIO_WriteBit( NSS_IOPORT, NSS_PIN, Bit_RESET );

    SpibInOut( addr & 0x7F );

    for( i = 0; i < size; i++ )
    {
        buffer[i] = SpibInOut( 0 );
    }

    //NSS = 1;
    GPIO_WriteBit( NSS_IOPORT, NSS_PIN, Bit_SET );
}

void SX1278BWriteFifo( uint8_t *buffer, uint8_t size )
{
    SX1278BWriteBuffer( 0, buffer, size );
}

void SX1278BReadFifo( uint8_t *buffer, uint8_t size )
{
    SX1278BReadBuffer( 0, buffer, size );
}

inline uint8_t SX1278BReadDio0( void )
{
    return GPIO_ReadInputDataBit( DIO0_IOPORT, DIO0_PIN );
}

inline uint8_t SX1278BReadDio1( void )
{
    return GPIO_ReadInputDataBit( DIO1_IOPORT, DIO1_PIN );
}

inline uint8_t SX1278BReadDio2( void )
{
    return GPIO_ReadInputDataBit( DIO2_IOPORT, DIO2_PIN );
}

inline uint8_t SX1278BReadDio3( void )
{
    return GPIO_ReadInputDataBit( DIO3_IOPORT, DIO3_PIN );
}

inline uint8_t SX1278BReadDio4( void )
{
    return GPIO_ReadInputDataBit( DIO4_IOPORT, DIO4_PIN );
}

inline uint8_t SX1278BReadDio5( void )
{
    return GPIO_ReadInputDataBit( DIO5_IOPORT, DIO5_PIN );
}


//射频芯片收发切换
inline void SX1278BWriteRxTx( uint8_t txEnable )
{
	;
}

