#include "platform.h"
#include "radio.h"
#include "sx1278b-Hal.h"
#include "sx1278b-LoRa.h"
#include "sx1278b.h"


// Y_mode LoRa settings
T_BLoRaSettings SX1278B_LoRaSettings =
{
	477459330, 		  // RFFrequency
    2,                // Power
    6,                // SignalBw [0: 7.8kHz, 1: 10.4 kHz, 2: 15.6 kHz, 3: 20.8 kHz, 4: 31.2 kHz,
                      // 5: 41.6 kHz, 6: 62.5 kHz, 7: 125 kHz, 8: 250 kHz, 9: 500 kHz, other: Reserved]
    10,               // SpreadingFactor [6: 64, 7: 128, 8: 256, 9: 512, 10: 1024, 11: 2048, 12: 4096  chips]
    2,                // ErrorCoding [1: 4/5, 2: 4/6, 3: 4/7, 4: 4/8]
    true,             // CrcOn [0: OFF, 1: ON]
    false,            // ImplicitHeaderOn [0: OFF, 1: ON]
	6,				  // PreambleLength
	0x12,			  // SyncWord
    0,                // RxSingleOn [0: Continuous, 1 Single]
    0,                // FreqHopOn [0: OFF, 1: ON]，跳频
    4,                // HopPeriod Hops every frequency hopping period symbols
    100,              // TxPacketTimeout
    100,              // RxPacketTimeout
    0xFF,             // PayloadLength (used for implicit header mode)
};

// SPIB using LoRa settings
T_BLoRaSettings BLoRaSettings =
{
    482080000,	  	  // RFFrequency
    2,                // Power
    6,                // SignalBw [0: 7.8kHz, 1: 10.4 kHz, 2: 15.6 kHz, 3: 20.8 kHz, 4: 31.2 kHz,
                      // 5: 41.6 kHz, 6: 62.5 kHz, 7: 125 kHz, 8: 250 kHz, 9: 500 kHz, other: Reserved]
    10,               // SpreadingFactor [6: 64, 7: 128, 8: 256, 9: 512, 10: 1024, 11: 2048, 12: 4096  chips]
    2,                // ErrorCoding [1: 4/5, 2: 4/6, 3: 4/7, 4: 4/8]
    true,             // CrcOn [0: OFF, 1: ON]
    false,            // ImplicitHeaderOn [0: OFF, 1: ON]
	6,			  	  // PreambleLength
	0x12,			  // SyncWord
    0,                // RxSingleOn [0: Continuous, 1 Single]
    0,                // FreqHopOn [0: OFF, 1: ON]
    4,                // HopPeriod Hops every frequency hopping period symbols
    100,              // TxPacketTimeout
    100,              // RxPacketTimeout
    0xFF,             // PayloadLength (used for implicit header mode)
};

/*!
 * SX1278B registers variable
 */
uint8_t SX1278BRegs[0x70];

static bool LoRaOn = false;
static bool LoRaOnState = false;

static void RFBIOInit(void);


static T_RadioDriver SX1278BDriver = {
	.name	     = "SX1278B",
	.Init        = SX1278BInit,
    .Reset       = SX1278BReset,
    .StartRx     = SX1278BStartRx,
    .GetRxPacket = SX1278BGetRxPacket,
    .SetTxPacket = SX1278BSetTxPacket,
    .Process     = SX1278BProcess,
};


int SX1278BRegister(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;

    RCC_APB2PeriphClockCmd( RCC_APB2Periph_GPIOG | RCC_APB2Periph_AFIO, ENABLE );

    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7;
    GPIO_Init(GPIOG, &GPIO_InitStructure);
	
	if (SX1278BCongfig.Enable == 1)
	{
		return RegisterRadioDriver(&SX1278BDriver);
	}
	else
	{
		GPIO_WriteBit(GPIOG, GPIO_Pin_7, Bit_RESET);	// RF2_SW OFF
		return 0;
	}
}


int SX1278BInit( void )
{
	uint8_t TempReg;
		
	RFBIOInit();
	
    // Initialize LoRa registers structure
    SX1278BLR = ( tSX1278BLR* )SX1278BRegs;	// SX1278B寄存器映射
	
    SX1278BInitIo( );		// SX1278B端口初始化
    
    SX1278BReset( );		// SX1278B复位
	
	// for test hard spi
	SX1278BRead(0x06, &TempReg);
	if (TempReg != 0x6C)
	{
		return -1;
	}
	
    // REMARK: After radio reset the default modem is FSK
    LoRaOn = true;          	// LORA = 1
	LoRaOnState = false;
	
    SX1278BSetLoRaOn(LoRaOn);	// 设置为扩频模式
	
	/* 设置RF的中心频点 */
	SX1278B_LoRaSettings.RFFrequency = LORA_FRE_BASE + SX1278BCongfig.RFChannel * LORA_FRE_STEP;
	
	BLoRaSettings = SX1278B_LoRaSettings;
    
    SX1278BLoRaInit();			// Initialize LoRa modem
	
	RF2_LED_ON();
	
	return 0;
}


void SX1278BReset( void )
{
	uint32_t startTick;  

	SX1278BSetReset( RADIO_RESET_ON );
    
    // Wait 1ms
    startTick = GET_TICK_COUNT( );
    while( ( GET_TICK_COUNT( ) - startTick ) < TICK_RATE_MS( 1 ) );    

    SX1278BSetReset( RADIO_RESET_OFF );
    
    // Wait 6ms
    startTick = GET_TICK_COUNT( );
    while( ( GET_TICK_COUNT( ) - startTick ) < TICK_RATE_MS( 6 ) );    
}


void RFBIOInit(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOF | RCC_APB2Periph_AFIO, ENABLE);
	
	GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_11;		// RF POWER PORT
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOF, &GPIO_InitStructure);
	
	RFB_POWER_ON();										// RF POWER ON
}


void SX1278BSetLoRaOn( bool enable )
{
    if( LoRaOnState == enable )
    {
        return;
    }
    LoRaOnState = enable;
    LoRaOn = enable;

	SX1278BLoRaSetOpMode( RFLR_OPMODE_SLEEP );
	
	/* 设置为扩频模式 */
	SX1278BLR->RegOpMode = ( SX1278BLR->RegOpMode & RFLR_OPMODE_LONGRANGEMODE_MASK ) | RFLR_OPMODE_LONGRANGEMODE_ON;
	SX1278BWrite( REG_LR_OPMODE, SX1278BLR->RegOpMode );
	
	SX1278BLoRaSetOpMode( RFLR_OPMODE_STANDBY );
									// RxDone               RxTimeout                   FhssChangeChannel           CadDone
	SX1278BLR->RegDioMapping1 = RFLR_DIOMAPPING1_DIO0_00 | RFLR_DIOMAPPING1_DIO1_00 | RFLR_DIOMAPPING1_DIO2_00 | RFLR_DIOMAPPING1_DIO3_00;
									// CadDetected          ModeReady
	SX1278BLR->RegDioMapping2 = RFLR_DIOMAPPING2_DIO4_00 | RFLR_DIOMAPPING2_DIO5_00;
	SX1278BWriteBuffer( REG_LR_DIOMAPPING1, &SX1278BLR->RegDioMapping1, 2 );
	
	SX1278BReadBuffer( REG_LR_OPMODE, SX1278BRegs + 1, 0x70 - 1 );
}


bool SX1278BGetLoRaOn( void )
{
    return LoRaOn;
}


void SX1278BSetOpMode( uint8_t opMode )
{
	SX1278BLoRaSetOpMode( opMode );
}


uint8_t SX1278BGetOpMode( void )
{
	return SX1278BLoRaGetOpMode( );
}


double SX1278BReadRssi( void )
{
	return SX1278BLoRaReadRssi( );
}


uint8_t SX1278BReadRxGain( void )
{
	return SX1278BLoRaReadRxGain( );
}


uint8_t SX1278BGetPacketRxGain( void )
{
	return SX1278BLoRaGetPacketRxGain(  );
}


int8_t SX1278BGetPacketSnr( void )
{
	return SX1278BLoRaGetPacketSnr(  );
}


double SX1278BGetPacketRssi( void )
{
	return SX1278BLoRaGetPacketRssi( );
}


uint32_t SX1278BGetPacketAfc( void )
{
	 while( 1 )
	 {
		 // Useless in LoRa mode
		 // Block program here
	 }
}


void SX1278BStartRx( void )
{
	SX1278BLoRaSetRFState( RFLR_STATE_RX_INIT );    //LoRa 中断接收状态
}


void SX1278BGetRxPacket( void *buffer, uint16_t *size )
{
	SX1278BLoRaGetRxPacket( buffer, size );
}


void SX1278BSetTxPacket( const void *buffer, uint16_t size )
{
	SX1278BLoRaSetTxPacket( buffer, size );
}


uint8_t SX1278BGetRFState( void )
{
	return SX1278BLoRaGetRFState( );
}


void SX1278BSetRFState( uint8_t state )
{
	SX1278BLoRaSetRFState( state );
}


uint32_t SX1278BProcess( void )
{
	return SX1278BLoRaProcess( );
}

