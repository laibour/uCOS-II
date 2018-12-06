#include "platform.h"
#include "sx1278a.h"
#include "sx1278a-Hal.h"
#include "sx1278a-LoRa.h"
#include "sx1278a-LoRaMisc.h"
#include "debug.h"


/*!
 * Constant values need to compute the RSSI value
 */
#define RSSI_OFFSET_LF                              -155.0
#define RSSI_OFFSET_HF                              -150.0

#define NOISE_ABSOLUTE_ZERO                         -174.0

#define NOISE_FIGURE_LF                                4.0
#define NOISE_FIGURE_HF                                6.0


// X_mode wakeup LoRa settings
T_LoRaSettings X_Wakeup_LoRaSettings =
{
    484200000,	  	  // RFFrequency
    2,                // Power
    7,                // SignalBw [0: 7.8kHz, 1: 10.4 kHz, 2: 15.6 kHz, 3: 20.8 kHz, 4: 31.2 kHz,
                      // 5: 41.6 kHz, 6: 62.5 kHz, 7: 125 kHz, 8: 250 kHz, 9: 500 kHz, other: Reserved]
    8,                // SpreadingFactor [6: 64, 7: 128, 8: 256, 9: 512, 10: 1024, 11: 2048, 12: 4096  chips]
    1,                // ErrorCoding [1: 4/5, 2: 4/6, 3: 4/7, 4: 4/8]
    true,             // CrcOn [0: OFF, 1: ON]
    false,            // ImplicitHeaderOn [0: OFF, 1: ON]
	1700,			  // PreambleLength
	0x56,			  // SyncWord
    0,                // RxSingleOn [0: Continuous, 1 Single]
    0,                // FreqHopOn [0: OFF, 1: ON]
    4,                // HopPeriod Hops every frequency hopping period symbols
    100,              // TxPacketTimeout
    100,              // RxPacketTimeout
    0xFF,             // PayloadLength (used for implicit header mode)
};

// X_mode normal LoRa settings
T_LoRaSettings X_Normal_LoRaSettings =
{
    485000000,	  	  // RFFrequency
    2,                // Power
    7,                // SignalBw [0: 7.8kHz, 1: 10.4 kHz, 2: 15.6 kHz, 3: 20.8 kHz, 4: 31.2 kHz,
                      // 5: 41.6 kHz, 6: 62.5 kHz, 7: 125 kHz, 8: 250 kHz, 9: 500 kHz, other: Reserved]
    8,                // SpreadingFactor [6: 64, 7: 128, 8: 256, 9: 512, 10: 1024, 11: 2048, 12: 4096  chips]
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


// Y_mode wakeup LoRa settings
T_LoRaSettings Y_Wakeup_LoRaSettings =
{
	477459330, 		  // RFFrequency
    2,                // Power
    6,                // SignalBw [0: 7.8kHz, 1: 10.4 kHz, 2: 15.6 kHz, 3: 20.8 kHz, 4: 31.2 kHz,
                      // 5: 41.6 kHz, 6: 62.5 kHz, 7: 125 kHz, 8: 250 kHz, 9: 500 kHz, other: Reserved]
    10,               // SpreadingFactor [6: 64, 7: 128, 8: 256, 9: 512, 10: 1024, 11: 2048, 12: 4096  chips]
    2,                // ErrorCoding [1: 4/5, 2: 4/6, 3: 4/7, 4: 4/8]
    true,             // CrcOn [0: OFF, 1: ON]
    false,            // ImplicitHeaderOn [0: OFF, 1: ON]
	450,			  // PreambleLength
	0x12,			  // SyncWord
    0,                // RxSingleOn [0: Continuous, 1 Single]
    0,                // FreqHopOn [0: OFF, 1: ON]，跳频
    4,                // HopPeriod Hops every frequency hopping period symbols
    100,              // TxPacketTimeout
    100,              // RxPacketTimeout
    0xFF,             // PayloadLength (used for implicit header mode)
};

// Y_mode LoRa settings
T_LoRaSettings Y_Normal_LoRaSettings =
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

// SPIA using LoRa settings
T_LoRaSettings LoRaSettings =
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
 * Precomputed signal bandwidth log values
 * Used to compute the Packet RSSI value.
 */
const double A_SignalBwLog[] =
{
    3.8927900303521316335038277369285,  // 7.8 kHz
    4.0177301567005500940384239336392,  // 10.4 kHz
    4.193820026016112828717566631653,   // 15.6 kHz
    4.31875866931372901183597627752391, // 20.8 kHz
    4.4948500216800940239313055263775,  // 31.2 kHz
    4.6197891057238405255051280399961,  // 41.6 kHz
    4.795880017344075219145044421102,   // 62.5 kHz
    5.0969100130080564143587833158265,  // 125 kHz
    5.397940008672037609572522210551,   // 250 kHz
    5.6989700043360188047862611052755   // 500 kHz
};

const double A_RssiOffsetLF[] =
{   // These values need to be specify in the Lab
    -155.0,
    -155.0,
    -155.0,
    -155.0,
    -155.0,
    -155.0,
    -155.0,
    -155.0,
    -155.0,
    -155.0,
};

const double A_RssiOffsetHF[] =
{   // These values need to be specify in the Lab
    -150.0,
    -150.0,
    -150.0,
    -150.0,
    -150.0,
    -150.0,
    -150.0,
    -150.0,
    -150.0,
    -150.0,
};

/*!
 * Frequency hopping frequencies table
 */
const int32_t A_HoppingFrequencies[] =
{
    916500000,
    923500000,
    906500000,
    917500000,
    917500000,
    909000000,
    903000000,
    916000000,
    912500000,
    926000000,
    925000000,
    909500000,
    913000000,
    918500000,
    918500000,
    902500000,
    911500000,
    926500000,
    902500000,
    922000000,
    924000000,
    903500000,
    913000000,
    922000000,
    926000000,
    910000000,
    920000000,
    922500000,
    911000000,
    922000000,
    909500000,
    926000000,
    922000000,
    918000000,
    925500000,
    908000000,
    917500000,
    926500000,
    908500000,
    916000000,
    905500000,
    916000000,
    903000000,
    905000000,
    915000000,
    913000000,
    907000000,
    910000000,
    926500000,
    925500000,
    911000000,
};


/*!
 * SX1278A LoRa registers variable
 */
tSX1278ALR* SX1278ALR;

/*!
 * Local RF buffer for communication support
 */
static uint8_t RFBuffer[RF_BUFFER_SIZE];

/*!
 * RF state machine variable
 */
static uint8_t RFLRState = RFLR_STATE_IDLE;

/*!
 * Rx management support variables
 */
static uint16_t RxPacketSize = 0;
static int8_t RxPacketSnrEstimate;
static double RxPacketRssiValue;
static uint8_t RxGain = 1;
static uint32_t RxTimeoutTimer = 0;
/*!
 * PacketTimeout Stores the Rx window time value for packet reception
 */
static uint32_t PacketTimeout;

/*!
 * Tx management support variables
 */
static uint16_t TxPacketSize = 0;


void SX1278ALoRaSetSyncWord( uint8_t value )
{
    SX1278AWrite( 0x39, value );
}


void SX1278ALoRaInit( void )
{
    RFLRState = RFLR_STATE_IDLE;

    SX1278ALoRaSetDefaults( );
    
	// We first load the device register structure with the default value (read from the device)
    SX1278AReadBuffer( REG_LR_OPMODE, SX1278ARegs + 1, 0x70 - 1 );
    
	// Set the device in Sleep Mode
	// SX1278ALoRaSetOpMode( RFLR_OPMODE_SLEEP );
	
    SX1278ALR->RegLna = RFLR_LNA_GAIN_G1;	// LNA增益设置

    SX1278AWriteBuffer( REG_LR_OPMODE, SX1278ARegs + 1, 0x70 - 1 );

    // set the RF settings 
    SX1278ALoRaSetRFFrequency( LoRaSettings.RFFrequency );         		// set frequency，RF载波频率
    SX1278ALoRaSetSpreadingFactor( LoRaSettings.SpreadingFactor ); 		// SF6 only operates in implicit header mode.扩频因子
    SX1278ALoRaSetErrorCoding( LoRaSettings.ErrorCoding );				// 纠错编码率
    SX1278ALoRaSetPacketCrcOn( LoRaSettings.CrcOn );					// 打开CRC校验
    SX1278ALoRaSetSignalBandwidth( LoRaSettings.SignalBw );				// 信号带宽

	SX1278ALoRaSetImplicitHeaderOn( LoRaSettings.ImplicitHeaderOn );	// 显式报头模式
	SX1278ALoRaSetPreambleLength( LoRaSettings.PreambleLength );		// 前导码长度
	SX1278ALoRaSetSyncWord( LoRaSettings.SyncWord );					// 同步字
	
    SX1278ALoRaSetSymbTimeout( 0x3FF );									// 超时时间
    SX1278ALoRaSetPayloadLength( LoRaSettings.PayloadLength );			// 负载字节长度，隐式报文头模式下需要设置
    SX1278ALoRaSetLowDatarateOptimize( true );
	
    if( LoRaSettings.RFFrequency > 380000000 )  // 380000000
    {
        SX1278ALoRaSetPAOutput( RFLR_PACONFIG_PASELECT_PABOOST ); 		// 选择 PA_BOOST 管脚输出信号
        SX1278ALoRaSetPa20dBm( true );  								// 最大输出功率
        LoRaSettings.Power = 2;
        SX1278ALoRaSetRFPower( LoRaSettings.Power );
    }
    else
    {
        SX1278ALoRaSetPAOutput( RFLR_PACONFIG_PASELECT_RFO );
        SX1278ALoRaSetPa20dBm( false );
        LoRaSettings.Power = 2;
        SX1278ALoRaSetRFPower( LoRaSettings.Power );
    }
	
	// Set the device in Standby Mode
    SX1278ALoRaSetOpMode( RFLR_OPMODE_STANDBY );	// 设置为待机模式
}

void SX1278ALoRaSetDefaults( void )
{
    // REMARK: See SX1278A datasheet for modified default values.

    SX1278ARead( REG_LR_VERSION, &SX1278ALR->RegVersion );
}

void SX1278ALoRaReset( void )
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

void SX1278ALoRaSetOpMode( uint8_t opMode )
{
    static uint8_t opModePrev = RFLR_OPMODE_STANDBY;
    static bool antennaSwitchTxOnPrev = true;
    bool antennaSwitchTxOn = false;

    opModePrev = SX1278ALR->RegOpMode & ~RFLR_OPMODE_MASK;

    if( opMode != opModePrev )
    {
        if( opMode == RFLR_OPMODE_TRANSMITTER )
        {
            antennaSwitchTxOn = true;
        }
        else
        {
            antennaSwitchTxOn = false;
        }
        if( antennaSwitchTxOn != antennaSwitchTxOnPrev )
        {
            antennaSwitchTxOnPrev = antennaSwitchTxOn;
            RXTX( antennaSwitchTxOn ); // Antenna switch control
        }
        SX1278ALR->RegOpMode = ( SX1278ALR->RegOpMode & RFLR_OPMODE_MASK ) | opMode;

        SX1278AWrite( REG_LR_OPMODE, SX1278ALR->RegOpMode );        
    }
}

uint8_t SX1278ALoRaGetOpMode( void )
{
    SX1278ARead( REG_LR_OPMODE, &SX1278ALR->RegOpMode );
    
    return SX1278ALR->RegOpMode & ~RFLR_OPMODE_MASK;
}

uint8_t SX1278ALoRaReadRxGain( void )
{
    SX1278ARead( REG_LR_LNA, &SX1278ALR->RegLna );
    return( SX1278ALR->RegLna >> 5 ) & 0x07;
}

double SX1278ALoRaReadRssi( void )
{
    // Reads the RSSI value
    SX1278ARead( REG_LR_RSSIVALUE, &SX1278ALR->RegRssiValue );

    if( LoRaSettings.RFFrequency < 860000000 )  // LF
    {
        return A_RssiOffsetLF[LoRaSettings.SignalBw] + ( double )SX1278ALR->RegRssiValue;
    }
    else
    {
        return A_RssiOffsetHF[LoRaSettings.SignalBw] + ( double )SX1278ALR->RegRssiValue;
    }
}

uint8_t SX1278ALoRaGetPacketRxGain( void )
{
    return RxGain;
}

int8_t SX1278ALoRaGetPacketSnr( void )
{
    return RxPacketSnrEstimate;
}

double SX1278ALoRaGetPacketRssi( void )
{
    return RxPacketRssiValue;
}

void SX1278ALoRaStartRx( void )
{
    SX1278ALoRaSetRFState( RFLR_STATE_RX_INIT );
}

void SX1278ALoRaGetRxPacket( void *buffer, uint16_t *size )
{
    *size = RxPacketSize;
    RxPacketSize = 0;
    memcpy( ( void * )buffer, ( void * )RFBuffer, ( size_t )*size );
}

void SX1278ALoRaSetTxPacket( const void *buffer, uint16_t size )
{
    if( LoRaSettings.FreqHopOn == false )
    {
        TxPacketSize = size;
    }
    else
    {
        TxPacketSize = 255;
    }
    memcpy( ( void * )RFBuffer, buffer, ( size_t )TxPacketSize ); 

    RFLRState = RFLR_STATE_TX_INIT;
}

uint8_t SX1278ALoRaGetRFState( void )
{
    return RFLRState;
}

void SX1278ALoRaSetRFState( uint8_t state )
{
    RFLRState = state;
}

/*!
 * \brief Process the LoRa modem Rx and Tx state machines depending on the
 *        SX1278A operating mode.
 *
 * \retval rfState Current RF state [RF_IDLE, RF_BUSY, 
 *                                   RF_RX_DONE, RF_RX_TIMEOUT,
 *                                   RF_TX_DONE, RF_TX_TIMEOUT]
 */
uint32_t SX1278ALoRaProcess( void )
{
    uint32_t result = RF_BUSY;
    
    switch( RFLRState )
    {
    case RFLR_STATE_IDLE:
		 break;
		 
    case RFLR_STATE_RX_INIT:
        SX1278ALoRaSetOpMode( RFLR_OPMODE_STANDBY );

        SX1278ALR->RegIrqFlagsMask = RFLR_IRQFLAGS_RXTIMEOUT |
                                    //RFLR_IRQFLAGS_RXDONE |
                                    //RFLR_IRQFLAGS_PAYLOADCRCERROR |
                                    RFLR_IRQFLAGS_VALIDHEADER |
                                    RFLR_IRQFLAGS_TXDONE |
                                    RFLR_IRQFLAGS_CADDONE |
                                    //RFLR_IRQFLAGS_FHSSCHANGEDCHANNEL |
                                    RFLR_IRQFLAGS_CADDETECTED;
        SX1278AWrite( REG_LR_IRQFLAGSMASK, SX1278ALR->RegIrqFlagsMask );

        if( LoRaSettings.FreqHopOn == true )
        {
            SX1278ALR->RegHopPeriod = LoRaSettings.HopPeriod;

            SX1278ARead( REG_LR_HOPCHANNEL, &SX1278ALR->RegHopChannel );
            SX1278ALoRaSetRFFrequency( A_HoppingFrequencies[SX1278ALR->RegHopChannel & RFLR_HOPCHANNEL_CHANNEL_MASK] );
        }
        else
        {
            SX1278ALR->RegHopPeriod = 255;
        }
        
        SX1278AWrite( REG_LR_HOPPERIOD, SX1278ALR->RegHopPeriod );
                
                                    // RxDone                    RxTimeout                   FhssChangeChannel           CadDone
        SX1278ALR->RegDioMapping1 = RFLR_DIOMAPPING1_DIO0_00 | RFLR_DIOMAPPING1_DIO1_00 | RFLR_DIOMAPPING1_DIO2_00 | RFLR_DIOMAPPING1_DIO3_00;
                                    // CadDetected               ModeReady
        SX1278ALR->RegDioMapping2 = RFLR_DIOMAPPING2_DIO4_00 | RFLR_DIOMAPPING2_DIO5_00;
		
        SX1278AWriteBuffer( REG_LR_DIOMAPPING1, &SX1278ALR->RegDioMapping1, 2 );
    
        if( LoRaSettings.RxSingleOn == true ) // Rx single mode
        {
            SX1278ALoRaSetOpMode( RFLR_OPMODE_RECEIVER_SINGLE );
        }
        else // Rx continuous mode
        {
            SX1278ALR->RegFifoAddrPtr = SX1278ALR->RegFifoRxBaseAddr;
            SX1278AWrite( REG_LR_FIFOADDRPTR, SX1278ALR->RegFifoAddrPtr );
            
            SX1278ALoRaSetOpMode( RFLR_OPMODE_RECEIVER );
        }
        
        memset( RFBuffer, 0, ( size_t )RF_BUFFER_SIZE );  // 清空RFBuffer

        PacketTimeout = LoRaSettings.RxPacketTimeout;
        RxTimeoutTimer = GET_TICK_COUNT( );
        RFLRState = RFLR_STATE_RX_RUNNING;
        break;
		
    case RFLR_STATE_RX_RUNNING:
        if( DIO0 == 1 )	 // RxDone
        {
            RxTimeoutTimer = GET_TICK_COUNT( );
            if ( LoRaSettings.FreqHopOn == true )	// false
            {
                SX1278ARead( REG_LR_HOPCHANNEL, &SX1278ALR->RegHopChannel );
                SX1278ALoRaSetRFFrequency( A_HoppingFrequencies[SX1278ALR->RegHopChannel & RFLR_HOPCHANNEL_CHANNEL_MASK] );
            }
            // Clear Irq
            SX1278AWrite( REG_LR_IRQFLAGS, RFLR_IRQFLAGS_RXDONE  );
            RFLRState = RFLR_STATE_RX_DONE;
        }
//        if( DIO2 == 1 ) // FHSS Changed Channel
//        {
//            RxTimeoutTimer = GET_TICK_COUNT( );
//            if( LoRaSettings.FreqHopOn == true )
//            {
//                SX1278ARead( REG_LR_HOPCHANNEL, &SX1278ALR->RegHopChannel );
//                SX1278ALoRaSetRFFrequency( A_HoppingFrequencies[SX1278ALR->RegHopChannel & RFLR_HOPCHANNEL_CHANNEL_MASK] );
//            }
//            // Clear Irq
//            SX1278AWrite( REG_LR_IRQFLAGS, RFLR_IRQFLAGS_FHSSCHANGEDCHANNEL );
//            // Debug
//            RxGain = SX1278ALoRaReadRxGain( );
//        }

        if( LoRaSettings.RxSingleOn == true ) // Rx single mode
        {
            if( ( GET_TICK_COUNT() - RxTimeoutTimer ) > PacketTimeout )   //计算是否超时
            {
                RFLRState = RFLR_STATE_RX_TIMEOUT;
            }
        }
        break;
		
    case RFLR_STATE_RX_DONE:
        SX1278ARead( REG_LR_IRQFLAGS, &SX1278ALR->RegIrqFlags );
        if( ( SX1278ALR->RegIrqFlags & RFLR_IRQFLAGS_PAYLOADCRCERROR ) == RFLR_IRQFLAGS_PAYLOADCRCERROR )	// CRC校验
        {
            SX1278AWrite(REG_LR_IRQFLAGS, RFLR_IRQFLAGS_PAYLOADCRCERROR);	// Clear Irq
            
            if( LoRaSettings.RxSingleOn == true ) // Rx single mode
            {
                RFLRState = RFLR_STATE_RX_INIT;
            }
            else
            {
                RFLRState = RFLR_STATE_RX_RUNNING;
            }
            break;
        }
        
        {
            uint8_t rxSnrEstimate;
            SX1278ARead( REG_LR_PKTSNRVALUE, &rxSnrEstimate );
            if( rxSnrEstimate & 0x80 ) // The SNR sign bit is 1
            {
                // Invert and divide by 4
                RxPacketSnrEstimate = ( ( ~rxSnrEstimate + 1 ) & 0xFF ) >> 2;
                RxPacketSnrEstimate = -RxPacketSnrEstimate;
            }
            else
            {
                // Divide by 4
                RxPacketSnrEstimate = ( rxSnrEstimate & 0xFF ) >> 2;
            }
        }
        
        if( LoRaSettings.RFFrequency < 860000000 )  // LF
        {
            if( RxPacketSnrEstimate < 0 )
            {
                RxPacketRssiValue = NOISE_ABSOLUTE_ZERO + 10.0 * A_SignalBwLog[LoRaSettings.SignalBw] + NOISE_FIGURE_LF + ( double )RxPacketSnrEstimate;
            }
            else
            {    
                SX1278ARead( REG_LR_PKTRSSIVALUE, &SX1278ALR->RegPktRssiValue );
                RxPacketRssiValue = A_RssiOffsetLF[LoRaSettings.SignalBw] + ( double )SX1278ALR->RegPktRssiValue;
            }
        }
        else                                        // HF
        {    
            if( RxPacketSnrEstimate < 0 )
            {
                RxPacketRssiValue = NOISE_ABSOLUTE_ZERO + 10.0 * A_SignalBwLog[LoRaSettings.SignalBw] + NOISE_FIGURE_HF + ( double )RxPacketSnrEstimate;
            }
            else
            {    
                SX1278ARead( REG_LR_PKTRSSIVALUE, &SX1278ALR->RegPktRssiValue );
                RxPacketRssiValue = A_RssiOffsetHF[LoRaSettings.SignalBw] + ( double )SX1278ALR->RegPktRssiValue;
            }
        }

        if( LoRaSettings.RxSingleOn == true ) // Rx single mode
        {
            SX1278ALR->RegFifoAddrPtr = SX1278ALR->RegFifoRxBaseAddr;       
            SX1278AWrite( REG_LR_FIFOADDRPTR, SX1278ALR->RegFifoAddrPtr );	// 读取接收数据

            if( LoRaSettings.ImplicitHeaderOn == true )
            {
                RxPacketSize = SX1278ALR->RegPayloadLength;
                SX1278AReadFifo( RFBuffer, SX1278ALR->RegPayloadLength );
            }
            else
            {
                SX1278ARead( REG_LR_NBRXBYTES, &SX1278ALR->RegNbRxBytes );
                RxPacketSize = SX1278ALR->RegNbRxBytes;
                SX1278AReadFifo( RFBuffer, SX1278ALR->RegNbRxBytes );
            }
        }
        else // Rx continuous mode
        {
            SX1278ARead( REG_LR_FIFORXCURRENTADDR, &SX1278ALR->RegFifoRxCurrentAddr );

            if( LoRaSettings.ImplicitHeaderOn == true )
            {
                RxPacketSize = SX1278ALR->RegPayloadLength;
                SX1278ALR->RegFifoAddrPtr = SX1278ALR->RegFifoRxCurrentAddr;
                SX1278AWrite( REG_LR_FIFOADDRPTR, SX1278ALR->RegFifoAddrPtr );
                SX1278AReadFifo( RFBuffer, SX1278ALR->RegPayloadLength );
            }
            else
            {
                SX1278ARead( REG_LR_NBRXBYTES, &SX1278ALR->RegNbRxBytes );
                RxPacketSize = SX1278ALR->RegNbRxBytes;
                SX1278ALR->RegFifoAddrPtr = SX1278ALR->RegFifoRxCurrentAddr;
                SX1278AWrite( REG_LR_FIFOADDRPTR, SX1278ALR->RegFifoAddrPtr );
                SX1278AReadFifo( RFBuffer, SX1278ALR->RegNbRxBytes );
				DATA_PRINTF(RFBuffer, RxPacketSize);	// 侦测接收到的数据
			}
        }
        
        if( LoRaSettings.RxSingleOn == true ) // Rx single mode
        {
            RFLRState = RFLR_STATE_RX_INIT;
        }
        else // Rx continuous mode
        {
            RFLRState = RFLR_STATE_RX_RUNNING;
        }
        result = RF_RX_DONE;
        break;
		
    case RFLR_STATE_RX_TIMEOUT:
        RFLRState = RFLR_STATE_RX_INIT;
        result = RF_RX_TIMEOUT;
        break;
		
    case RFLR_STATE_TX_INIT:
        SX1278ALoRaSetOpMode( RFLR_OPMODE_STANDBY );
		
		A_RFPA0133_ON();

        if( LoRaSettings.FreqHopOn == true )
        {
            SX1278ALR->RegIrqFlagsMask = RFLR_IRQFLAGS_RXTIMEOUT |
                                        RFLR_IRQFLAGS_RXDONE |
                                        RFLR_IRQFLAGS_PAYLOADCRCERROR |
                                        RFLR_IRQFLAGS_VALIDHEADER |
                                        //RFLR_IRQFLAGS_TXDONE |
                                        RFLR_IRQFLAGS_CADDONE |
                                        //RFLR_IRQFLAGS_FHSSCHANGEDCHANNEL |
                                        RFLR_IRQFLAGS_CADDETECTED;
            SX1278ALR->RegHopPeriod = LoRaSettings.HopPeriod;

            SX1278ARead( REG_LR_HOPCHANNEL, &SX1278ALR->RegHopChannel );
            SX1278ALoRaSetRFFrequency( A_HoppingFrequencies[SX1278ALR->RegHopChannel & RFLR_HOPCHANNEL_CHANNEL_MASK] );
        }
        else
        {
            SX1278ALR->RegIrqFlagsMask = RFLR_IRQFLAGS_RXTIMEOUT |
                                        RFLR_IRQFLAGS_RXDONE |
                                        RFLR_IRQFLAGS_PAYLOADCRCERROR |
                                        RFLR_IRQFLAGS_VALIDHEADER |
                                        //RFLR_IRQFLAGS_TXDONE |
                                        RFLR_IRQFLAGS_CADDONE |
                                        RFLR_IRQFLAGS_FHSSCHANGEDCHANNEL |
                                        RFLR_IRQFLAGS_CADDETECTED;
            SX1278ALR->RegHopPeriod = 0;
        }
        SX1278AWrite( REG_LR_HOPPERIOD, SX1278ALR->RegHopPeriod );
        SX1278AWrite( REG_LR_IRQFLAGSMASK, SX1278ALR->RegIrqFlagsMask );

        // Initializes the payload size
        SX1278ALR->RegPayloadLength = TxPacketSize;
        SX1278AWrite( REG_LR_PAYLOADLENGTH, SX1278ALR->RegPayloadLength );
        
        SX1278ALR->RegFifoTxBaseAddr = 0x00; // Full buffer used for Tx
        SX1278AWrite( REG_LR_FIFOTXBASEADDR, SX1278ALR->RegFifoTxBaseAddr );

        SX1278ALR->RegFifoAddrPtr = SX1278ALR->RegFifoTxBaseAddr;
        SX1278AWrite( REG_LR_FIFOADDRPTR, SX1278ALR->RegFifoAddrPtr );
        
        // Write payload buffer to LORA modem
        SX1278AWriteFifo( RFBuffer, SX1278ALR->RegPayloadLength );
		
                                        // TxDone               RxTimeout                   FhssChangeChannel          ValidHeader         
        SX1278ALR->RegDioMapping1 = RFLR_DIOMAPPING1_DIO0_01 | RFLR_DIOMAPPING1_DIO1_00 | RFLR_DIOMAPPING1_DIO2_00 | RFLR_DIOMAPPING1_DIO3_01;
                                        // PllLock              Mode Ready
        SX1278ALR->RegDioMapping2 = RFLR_DIOMAPPING2_DIO4_01 | RFLR_DIOMAPPING2_DIO5_00;
		
        SX1278AWriteBuffer( REG_LR_DIOMAPPING1, &SX1278ALR->RegDioMapping1, 2 );

        SX1278ALoRaSetOpMode( RFLR_OPMODE_TRANSMITTER );

        RFLRState = RFLR_STATE_TX_RUNNING;
        break;
		
    case RFLR_STATE_TX_RUNNING:
        if( DIO0 == 1 ) // TxDone
        {
            // Clear Irq
            SX1278AWrite( REG_LR_IRQFLAGS, RFLR_IRQFLAGS_TXDONE  );
            RFLRState = RFLR_STATE_TX_DONE;
			
			A_RFPA0133_OFF();
        }
		
//         if( DIO2 == 1 ) // FHSS Changed Channel
//         {
//             if( LoRaSettings.FreqHopOn == true )
//             {
//                 SX1278ARead( REG_LR_HOPCHANNEL, &SX1278ALR->RegHopChannel );
//                 SX1278ALoRaSetRFFrequency( A_HoppingFrequencies[SX1278ALR->RegHopChannel & RFLR_HOPCHANNEL_CHANNEL_MASK] );
//             }
//             // Clear Irq
//             SX1278AWrite( REG_LR_IRQFLAGS, RFLR_IRQFLAGS_FHSSCHANGEDCHANNEL );
//         }
        break;
		
    case RFLR_STATE_TX_DONE:
        // optimize the power consumption by switching off the transmitter as soon as the packet has been sent
        SX1278ALoRaSetOpMode( RFLR_OPMODE_STANDBY );

        RFLRState = RFLR_STATE_IDLE;
        result = RF_TX_DONE;
        break;
		
    case RFLR_STATE_CAD_INIT:    
        SX1278ALoRaSetOpMode( RFLR_OPMODE_STANDBY );
    
        SX1278ALR->RegIrqFlagsMask = RFLR_IRQFLAGS_RXTIMEOUT |
                                    RFLR_IRQFLAGS_RXDONE |
                                    RFLR_IRQFLAGS_PAYLOADCRCERROR |
                                    RFLR_IRQFLAGS_VALIDHEADER |
                                    RFLR_IRQFLAGS_TXDONE |
                                    //RFLR_IRQFLAGS_CADDONE |
                                    RFLR_IRQFLAGS_FHSSCHANGEDCHANNEL; // |
                                    //RFLR_IRQFLAGS_CADDETECTED;
        SX1278AWrite( REG_LR_IRQFLAGSMASK, SX1278ALR->RegIrqFlagsMask );
           
                                    // RxDone                   RxTimeout                   FhssChangeChannel           CadDone
        SX1278ALR->RegDioMapping1 = RFLR_DIOMAPPING1_DIO0_00 | RFLR_DIOMAPPING1_DIO1_00 | RFLR_DIOMAPPING1_DIO2_00 | RFLR_DIOMAPPING1_DIO3_00;
                                    // CAD Detected              ModeReady
        SX1278ALR->RegDioMapping2 = RFLR_DIOMAPPING2_DIO4_00 | RFLR_DIOMAPPING2_DIO5_00;
        SX1278AWriteBuffer( REG_LR_DIOMAPPING1, &SX1278ALR->RegDioMapping1, 2 );
            
        SX1278ALoRaSetOpMode( RFLR_OPMODE_CAD );
        RFLRState = RFLR_STATE_CAD_RUNNING;
        break;
		
    case RFLR_STATE_CAD_RUNNING:
        if( DIO3 == 1 ) //CAD Done interrupt
        { 
            // Clear Irq
            SX1278AWrite( REG_LR_IRQFLAGS, RFLR_IRQFLAGS_CADDONE  );
            if( DIO4 == 1 ) // CAD Detected interrupt
            {
                // Clear Irq
                SX1278AWrite( REG_LR_IRQFLAGS, RFLR_IRQFLAGS_CADDETECTED  );
                // CAD detected, we have a LoRa preamble
                RFLRState = RFLR_STATE_RX_INIT;
                result = RF_CHANNEL_ACTIVITY_DETECTED;
            } 
            else
            {    
                // The device goes in Standby Mode automatically    
                RFLRState = RFLR_STATE_IDLE;
                result = RF_CHANNEL_EMPTY;
            }
        }   
        break;
    
    default:
        break;
    }
	
    return result;
}

