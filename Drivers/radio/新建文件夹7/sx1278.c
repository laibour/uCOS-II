#include "platform.h"
#include "radio.h"
#include "sx1278a.h"
#include "sx1278a-Hal.h"
#include "sx1278a-LoRa.h"


/*!
 * SX1278A registers variable
 */
uint8_t SX1278ARegs[0x70];

static bool LoRaOn = false;
static bool LoRaOnState = false;


void RFIOInit(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC | RCC_APB2Periph_AFIO, ENABLE);
	
	GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_6;			// RF POWER PORT
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOC, &GPIO_InitStructure);
	
	RF_POWER_ON();										// RF POWER ON
}


uint8_t SX1278AInit( void )
{
	uint8_t TempReg;
	
	RFIOInit();
	
    // Initialize LoRa registers structure
    SX1278ALR = ( tSX1278ALR* )SX1278ARegs;	// SX1278A寄存器映射

    SX1278AInitIo( );	// SX1278A端口初始化
    
    SX1278AReset( );		// SX1278A复位
	
	// for test hard spi
	SX1278ARead(0x06, &TempReg);
	while(TempReg != 0x6C)
	{
		printf("Hard SPI Err!\r\n");
		Soft_delay_ms(100);
	}
	
    // REMARK: After radio reset the default modem is FSK
    LoRaOn = true;          	// LORA = 1
	
    SX1278ASetLoRaOn( LoRaOn );	// 设置为扩频模式
	
	LoRaSettings = Y_Normal_LoRaSettings;
    
    SX1278ALoRaInit( );			// Initialize LoRa modem
	
	return 0;
}


void SX1278AReset( void )
{
	uint32_t startTick;  

	SX1278ASetReset( RADIO_RESET_ON );
    
    // Wait 1ms
    startTick = GET_TICK_COUNT( );
    while( ( GET_TICK_COUNT( ) - startTick ) < TICK_RATE_MS( 1 ) );    

    SX1278ASetReset( RADIO_RESET_OFF );
    
    // Wait 6ms
    startTick = GET_TICK_COUNT( );
    while( ( GET_TICK_COUNT( ) - startTick ) < TICK_RATE_MS( 6 ) );    
}


void SX1278ASetLoRaOn( bool enable )
{
    if( LoRaOnState == enable )
    {
        return;
    }
    LoRaOnState = enable;
    LoRaOn = enable;

	SX1278ALoRaSetOpMode( RFLR_OPMODE_SLEEP );
	
	/* 设置为扩频模式 */
	SX1278ALR->RegOpMode = ( SX1278ALR->RegOpMode & RFLR_OPMODE_LONGRANGEMODE_MASK ) | RFLR_OPMODE_LONGRANGEMODE_ON;
	SX1278AWrite( REG_LR_OPMODE, SX1278ALR->RegOpMode );
	
	SX1278ALoRaSetOpMode( RFLR_OPMODE_STANDBY );
									// RxDone               RxTimeout                   FhssChangeChannel           CadDone
	SX1278ALR->RegDioMapping1 = RFLR_DIOMAPPING1_DIO0_00 | RFLR_DIOMAPPING1_DIO1_00 | RFLR_DIOMAPPING1_DIO2_00 | RFLR_DIOMAPPING1_DIO3_00;
									// CadDetected          ModeReady
	SX1278ALR->RegDioMapping2 = RFLR_DIOMAPPING2_DIO4_00 | RFLR_DIOMAPPING2_DIO5_00;
	SX1278AWriteBuffer( REG_LR_DIOMAPPING1, &SX1278ALR->RegDioMapping1, 2 );
	
	SX1278AReadBuffer( REG_LR_OPMODE, SX1278ARegs + 1, 0x70 - 1 );
}


bool SX1278AGetLoRaOn( void )
{
    return LoRaOn;
}


void SX1278ASetOpMode( uint8_t opMode )
{
	SX1278ALoRaSetOpMode( opMode );
}


uint8_t SX1278AGetOpMode( void )
{
	return SX1278ALoRaGetOpMode( );
}


double SX1278AReadRssi( void )
{
	return SX1278ALoRaReadRssi( );
}


uint8_t SX1278AReadRxGain( void )
{
	return SX1278ALoRaReadRxGain( );
}


uint8_t SX1278AGetPacketRxGain( void )
{
	return SX1278ALoRaGetPacketRxGain(  );
}


int8_t SX1278AGetPacketSnr( void )
{
	return SX1278ALoRaGetPacketSnr(  );
}


double SX1278AGetPacketRssi( void )
{
	return SX1278ALoRaGetPacketRssi( );
}


uint32_t SX1278AGetPacketAfc( void )
{
	 while( 1 )
	 {
		 // Useless in LoRa mode
		 // Block program here
	 }
}


void SX1278AStartRx( void )
{
	SX1278ALoRaSetRFState( RFLR_STATE_RX_INIT );    //LoRa 中断接收状态
}


void SX1278AGetRxPacket( void *buffer, uint16_t *size )
{
	SX1278ALoRaGetRxPacket( buffer, size );
}


void SX1278ASetTxPacket( const void *buffer, uint16_t size )
{
	SX1278ALoRaSetTxPacket( buffer, size );
}


uint8_t SX1278AGetRFState( void )
{
	return SX1278ALoRaGetRFState( );
}


void SX1278ASetRFState( uint8_t state )
{
	SX1278ALoRaSetRFState( state );
}


uint32_t SX1278AProcess( void )
{
	return SX1278ALoRaProcess( );
}

