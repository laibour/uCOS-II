#include <string.h>
#include "platform.h"
#include "radio.h"
#include "sx1278b.h"
#include "sx1278b-Hal.h"
#include "sx1278b-LoRa.h"
#include "sx1278b-LoRaMisc.h"


/*!
 * Constant values need to compute the RSSI value
 */
#define RSSI_OFFSET_LF                              -155.0
#define RSSI_OFFSET_HF                              -150.0

#define NOISE_ABSOLUTE_ZERO                         -174.0

#define NOISE_FIGURE_LF                                4.0
#define NOISE_FIGURE_HF                                6.0





/*!
 * Precomputed signal bandwidth log values
 * Used to compute the Packet RSSI value.
 */
const double B_SignalBwLog[] =
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

const double B_RssiOffsetLF[] =
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

const double B_RssiOffsetHF[] =
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
const int32_t B_HoppingFrequencies[] =
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
 * SX1278B LoRa registers variable
 */
tSX1278BLR* SX1278BLR;

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


void SX1278BLoRaSetSyncWord( uint8_t value )
{
    SX1278BWrite( 0x39, value );
}


void SX1278BLoRaInit( void )
{
    RFLRState = RFLR_STATE_IDLE;

    SX1278BLoRaSetDefaults( );
    
	// We first load the device register structure with the default value (read from the device)
    SX1278BReadBuffer( REG_LR_OPMODE, SX1278BRegs + 1, 0x70 - 1 );
    
//	// Set the device in Sleep Mode
//    SX1278BLoRaSetOpMode( RFLR_OPMODE_SLEEP );
	
    SX1278BLR->RegLna = RFLR_LNA_GAIN_G1;	// LNA增益设置

    SX1278BWriteBuffer( REG_LR_OPMODE, SX1278BRegs + 1, 0x70 - 1 );

    // set the RF settings 
    SX1278BLoRaSetRFFrequency( BLoRaSettings.RFFrequency );         	// set frequency，RF载波频率
    SX1278BLoRaSetSpreadingFactor( BLoRaSettings.SpreadingFactor ); 	// SF6 only operates in implicit header mode.扩频因子
    SX1278BLoRaSetErrorCoding( BLoRaSettings.ErrorCoding );			// 纠错编码率
    SX1278BLoRaSetPacketCrcOn( BLoRaSettings.CrcOn );					// 打开CRC校验
    SX1278BLoRaSetSignalBandwidth( BLoRaSettings.SignalBw );			// 信号带宽

	SX1278BLoRaSetImplicitHeaderOn( BLoRaSettings.ImplicitHeaderOn );	// 显式报头模式
	SX1278BLoRaSetPreambleLength( BLoRaSettings.PreambleLength );		// 前导码长度
	SX1278BLoRaSetSyncWord( BLoRaSettings.SyncWord );					// 同步字
	
    SX1278BLoRaSetSymbTimeout( 0x3FF );									// 超时时间
    SX1278BLoRaSetPayloadLength( BLoRaSettings.PayloadLength );		// 负载字节长度，隐式报文头模式下需要设置
    SX1278BLoRaSetLowDatarateOptimize( true );
	
    if( BLoRaSettings.RFFrequency > 380000000 )  // 380000000
    {
        SX1278BLoRaSetPAOutput( RFLR_PACONFIG_PASELECT_PABOOST ); 		// 选择 PA_BOOST 管脚输出信号
        SX1278BLoRaSetPa20dBm( true );  									// 最大输出功率
        BLoRaSettings.Power = 2;
        SX1278BLoRaSetRFPower( BLoRaSettings.Power );
    }
    else
    {
        SX1278BLoRaSetPAOutput( RFLR_PACONFIG_PASELECT_RFO );
        SX1278BLoRaSetPa20dBm( false );
        BLoRaSettings.Power = 2;
        SX1278BLoRaSetRFPower( BLoRaSettings.Power );
    }
	
	// Set the device in Standby Mode
    SX1278BLoRaSetOpMode( RFLR_OPMODE_STANDBY );	// 设置为待机模式
}

void SX1278BLoRaSetDefaults( void )
{
    // REMARK: See SX1278B datasheet for modified default values.

    SX1278BRead( REG_LR_VERSION, &SX1278BLR->RegVersion );
}

void SX1278BLoRaReset( void )
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

void SX1278BLoRaSetOpMode( uint8_t opMode )
{
    static uint8_t opModePrev = RFLR_OPMODE_STANDBY;
    static bool antennaSwitchTxOnPrev = true;
    bool antennaSwitchTxOn = false;

    opModePrev = SX1278BLR->RegOpMode & ~RFLR_OPMODE_MASK;

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
        SX1278BLR->RegOpMode = ( SX1278BLR->RegOpMode & RFLR_OPMODE_MASK ) | opMode;

        SX1278BWrite( REG_LR_OPMODE, SX1278BLR->RegOpMode );        
    }
}

uint8_t SX1278BLoRaGetOpMode( void )
{
    SX1278BRead( REG_LR_OPMODE, &SX1278BLR->RegOpMode );
    
    return SX1278BLR->RegOpMode & ~RFLR_OPMODE_MASK;
}

uint8_t SX1278BLoRaReadRxGain( void )
{
    SX1278BRead( REG_LR_LNA, &SX1278BLR->RegLna );
    return( SX1278BLR->RegLna >> 5 ) & 0x07;
}

double SX1278BLoRaReadRssi( void )
{
    // Reads the RSSI value
    SX1278BRead( REG_LR_RSSIVALUE, &SX1278BLR->RegRssiValue );

    if( BLoRaSettings.RFFrequency < 860000000 )  // LF
    {
        return B_RssiOffsetLF[BLoRaSettings.SignalBw] + ( double )SX1278BLR->RegRssiValue;
    }
    else
    {
        return B_RssiOffsetHF[BLoRaSettings.SignalBw] + ( double )SX1278BLR->RegRssiValue;
    }
}

uint8_t SX1278BLoRaGetPacketRxGain( void )
{
    return RxGain;
}

int8_t SX1278BLoRaGetPacketSnr( void )
{
    return RxPacketSnrEstimate;
}

double SX1278BLoRaGetPacketRssi( void )
{
    return RxPacketRssiValue;
}

void SX1278BLoRaStartRx( void )
{
    SX1278BLoRaSetRFState( RFLR_STATE_RX_INIT );
}

void SX1278BLoRaGetRxPacket( void *buffer, uint16_t *size )
{
    *size = RxPacketSize;
    RxPacketSize = 0;
    memcpy( ( void * )buffer, ( void * )RFBuffer, ( size_t )*size );
}

void SX1278BLoRaSetTxPacket( const void *buffer, uint16_t size )
{
    if( BLoRaSettings.FreqHopOn == false )
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

uint8_t SX1278BLoRaGetRFState( void )
{
    return RFLRState;
}

void SX1278BLoRaSetRFState( uint8_t state )
{
    RFLRState = state;
}

/*!
 * \brief Process the LoRa modem Rx and Tx state machines depending on the
 *        SX1278B operating mode.
 *
 * \retval rfState Current RF state [RF_IDLE, RF_BUSY, 
 *                                   RF_RX_DONE, RF_RX_TIMEOUT,
 *                                   RF_TX_DONE, RF_TX_TIMEOUT]
 */
uint32_t SX1278BLoRaProcess( void )
{
    uint32_t result = RF_BUSY;
    
    switch( RFLRState )
    {
    case RFLR_STATE_IDLE:
		 break;
		 
    case RFLR_STATE_RX_INIT:
        SX1278BLoRaSetOpMode( RFLR_OPMODE_STANDBY );

        SX1278BLR->RegIrqFlagsMask = RFLR_IRQFLAGS_RXTIMEOUT |
                                    //RFLR_IRQFLAGS_RXDONE |
                                    //RFLR_IRQFLAGS_PAYLOADCRCERROR |
                                    RFLR_IRQFLAGS_VALIDHEADER |
                                    RFLR_IRQFLAGS_TXDONE |
                                    RFLR_IRQFLAGS_CADDONE |
                                    //RFLR_IRQFLAGS_FHSSCHANGEDCHANNEL |
                                    RFLR_IRQFLAGS_CADDETECTED;
        SX1278BWrite( REG_LR_IRQFLAGSMASK, SX1278BLR->RegIrqFlagsMask );

        if( BLoRaSettings.FreqHopOn == true )
        {
            SX1278BLR->RegHopPeriod = BLoRaSettings.HopPeriod;

            SX1278BRead( REG_LR_HOPCHANNEL, &SX1278BLR->RegHopChannel );
            SX1278BLoRaSetRFFrequency( B_HoppingFrequencies[SX1278BLR->RegHopChannel & RFLR_HOPCHANNEL_CHANNEL_MASK] );
        }
        else
        {
            SX1278BLR->RegHopPeriod = 255;
        }
        
        SX1278BWrite( REG_LR_HOPPERIOD, SX1278BLR->RegHopPeriod );
                
                                    // RxDone                    RxTimeout                   FhssChangeChannel           CadDone
        SX1278BLR->RegDioMapping1 = RFLR_DIOMAPPING1_DIO0_00 | RFLR_DIOMAPPING1_DIO1_00 | RFLR_DIOMAPPING1_DIO2_00 | RFLR_DIOMAPPING1_DIO3_00;
                                    // CadDetected               ModeReady
        SX1278BLR->RegDioMapping2 = RFLR_DIOMAPPING2_DIO4_00 | RFLR_DIOMAPPING2_DIO5_00;
		
        SX1278BWriteBuffer( REG_LR_DIOMAPPING1, &SX1278BLR->RegDioMapping1, 2 );
    
        if( BLoRaSettings.RxSingleOn == true ) // Rx single mode
        {
            SX1278BLoRaSetOpMode( RFLR_OPMODE_RECEIVER_SINGLE );
        }
        else // Rx continuous mode
        {
            SX1278BLR->RegFifoAddrPtr = SX1278BLR->RegFifoRxBaseAddr;
            SX1278BWrite( REG_LR_FIFOADDRPTR, SX1278BLR->RegFifoAddrPtr );
            
            SX1278BLoRaSetOpMode( RFLR_OPMODE_RECEIVER );
        }
        
        memset( RFBuffer, 0, ( size_t )RF_BUFFER_SIZE );	// 清空RFBuffer

        PacketTimeout = BLoRaSettings.RxPacketTimeout;
        RxTimeoutTimer = GET_TICK_COUNT( );
        RFLRState = RFLR_STATE_RX_RUNNING;
        break;
		
    case RFLR_STATE_RX_RUNNING:
        if( DIO0 == 1 )	// RxDone
        {
            RxTimeoutTimer = GET_TICK_COUNT( );
            if( BLoRaSettings.FreqHopOn == true )	// false
            {
                SX1278BRead( REG_LR_HOPCHANNEL, &SX1278BLR->RegHopChannel );
                SX1278BLoRaSetRFFrequency( B_HoppingFrequencies[SX1278BLR->RegHopChannel & RFLR_HOPCHANNEL_CHANNEL_MASK] );
            }
            // Clear Irq
            SX1278BWrite( REG_LR_IRQFLAGS, RFLR_IRQFLAGS_RXDONE  );
            RFLRState = RFLR_STATE_RX_DONE;
        }
        if( DIO2 == 1 ) // FHSS Changed Channel
        {
            RxTimeoutTimer = GET_TICK_COUNT( );
            if( BLoRaSettings.FreqHopOn == true )
            {
                SX1278BRead( REG_LR_HOPCHANNEL, &SX1278BLR->RegHopChannel );
                SX1278BLoRaSetRFFrequency( B_HoppingFrequencies[SX1278BLR->RegHopChannel & RFLR_HOPCHANNEL_CHANNEL_MASK] );
            }
            // Clear Irq
            SX1278BWrite( REG_LR_IRQFLAGS, RFLR_IRQFLAGS_FHSSCHANGEDCHANNEL );
            // Debug
            RxGain = SX1278BLoRaReadRxGain( );
        }

        if( BLoRaSettings.RxSingleOn == true ) // Rx single mode
        {
            if( ( GET_TICK_COUNT() - RxTimeoutTimer ) > PacketTimeout )   //计算是否超时
            {
                RFLRState = RFLR_STATE_RX_TIMEOUT;
            }
        }
        break;
		
    case RFLR_STATE_RX_DONE:
        SX1278BRead( REG_LR_IRQFLAGS, &SX1278BLR->RegIrqFlags );
        if( ( SX1278BLR->RegIrqFlags & RFLR_IRQFLAGS_PAYLOADCRCERROR ) == RFLR_IRQFLAGS_PAYLOADCRCERROR )	// CRC校验
        {
            SX1278BWrite(REG_LR_IRQFLAGS, RFLR_IRQFLAGS_PAYLOADCRCERROR);	// Clear Irq
            
            if( BLoRaSettings.RxSingleOn == true ) // Rx single mode
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
            SX1278BRead( REG_LR_PKTSNRVALUE, &rxSnrEstimate );
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
        
        if( BLoRaSettings.RFFrequency < 860000000 )  // LF
        {    
            if( RxPacketSnrEstimate < 0 )
            {
                RxPacketRssiValue = NOISE_ABSOLUTE_ZERO + 10.0 * B_SignalBwLog[BLoRaSettings.SignalBw] + NOISE_FIGURE_LF + ( double )RxPacketSnrEstimate;
            }
            else
            {    
                SX1278BRead( REG_LR_PKTRSSIVALUE, &SX1278BLR->RegPktRssiValue );
                RxPacketRssiValue = B_RssiOffsetLF[BLoRaSettings.SignalBw] + ( double )SX1278BLR->RegPktRssiValue;
            }
        }
        else                                        // HF
        {    
            if( RxPacketSnrEstimate < 0 )
            {
                RxPacketRssiValue = NOISE_ABSOLUTE_ZERO + 10.0 * B_SignalBwLog[BLoRaSettings.SignalBw] + NOISE_FIGURE_HF + ( double )RxPacketSnrEstimate;
            }
            else
            {    
                SX1278BRead( REG_LR_PKTRSSIVALUE, &SX1278BLR->RegPktRssiValue );
                RxPacketRssiValue = B_RssiOffsetHF[BLoRaSettings.SignalBw] + ( double )SX1278BLR->RegPktRssiValue;
            }
        }

        if( BLoRaSettings.RxSingleOn == true ) // Rx single mode
        {
            SX1278BLR->RegFifoAddrPtr = SX1278BLR->RegFifoRxBaseAddr;       
            SX1278BWrite( REG_LR_FIFOADDRPTR, SX1278BLR->RegFifoAddrPtr );	// 读取接收数据

            if( BLoRaSettings.ImplicitHeaderOn == true )
            {
                RxPacketSize = SX1278BLR->RegPayloadLength;
                SX1278BReadFifo( RFBuffer, SX1278BLR->RegPayloadLength );
            }
            else
            {
                SX1278BRead( REG_LR_NBRXBYTES, &SX1278BLR->RegNbRxBytes );
                RxPacketSize = SX1278BLR->RegNbRxBytes;
                SX1278BReadFifo( RFBuffer, SX1278BLR->RegNbRxBytes );
            }
        }
        else // Rx continuous mode
        {
            SX1278BRead( REG_LR_FIFORXCURRENTADDR, &SX1278BLR->RegFifoRxCurrentAddr );

            if( BLoRaSettings.ImplicitHeaderOn == true )
            {
                RxPacketSize = SX1278BLR->RegPayloadLength;
                SX1278BLR->RegFifoAddrPtr = SX1278BLR->RegFifoRxCurrentAddr;
                SX1278BWrite( REG_LR_FIFOADDRPTR, SX1278BLR->RegFifoAddrPtr );
                SX1278BReadFifo( RFBuffer, SX1278BLR->RegPayloadLength );
            }
            else
            {
                SX1278BRead( REG_LR_NBRXBYTES, &SX1278BLR->RegNbRxBytes );
                RxPacketSize = SX1278BLR->RegNbRxBytes;
                SX1278BLR->RegFifoAddrPtr = SX1278BLR->RegFifoRxCurrentAddr;
                SX1278BWrite( REG_LR_FIFOADDRPTR, SX1278BLR->RegFifoAddrPtr );
                SX1278BReadFifo( RFBuffer, SX1278BLR->RegNbRxBytes );
				DATA_PRINTF(RFBuffer, RxPacketSize);	// 侦测接收到的数据
			}
        }
        
        if( BLoRaSettings.RxSingleOn == true ) // Rx single mode
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
        SX1278BLoRaSetOpMode( RFLR_OPMODE_STANDBY );
		
		B_RFPA0133_ON();

        if( BLoRaSettings.FreqHopOn == true )
        {
            SX1278BLR->RegIrqFlagsMask = RFLR_IRQFLAGS_RXTIMEOUT |
                                        RFLR_IRQFLAGS_RXDONE |
                                        RFLR_IRQFLAGS_PAYLOADCRCERROR |
                                        RFLR_IRQFLAGS_VALIDHEADER |
                                        //RFLR_IRQFLAGS_TXDONE |
                                        RFLR_IRQFLAGS_CADDONE |
                                        //RFLR_IRQFLAGS_FHSSCHANGEDCHANNEL |
                                        RFLR_IRQFLAGS_CADDETECTED;
            SX1278BLR->RegHopPeriod = BLoRaSettings.HopPeriod;

            SX1278BRead( REG_LR_HOPCHANNEL, &SX1278BLR->RegHopChannel );
            SX1278BLoRaSetRFFrequency( B_HoppingFrequencies[SX1278BLR->RegHopChannel & RFLR_HOPCHANNEL_CHANNEL_MASK] );
        }
        else
        {
            SX1278BLR->RegIrqFlagsMask = RFLR_IRQFLAGS_RXTIMEOUT |
                                        RFLR_IRQFLAGS_RXDONE |
                                        RFLR_IRQFLAGS_PAYLOADCRCERROR |
                                        RFLR_IRQFLAGS_VALIDHEADER |
                                        //RFLR_IRQFLAGS_TXDONE |
                                        RFLR_IRQFLAGS_CADDONE |
                                        RFLR_IRQFLAGS_FHSSCHANGEDCHANNEL |
                                        RFLR_IRQFLAGS_CADDETECTED;
            SX1278BLR->RegHopPeriod = 0;
        }
        SX1278BWrite( REG_LR_HOPPERIOD, SX1278BLR->RegHopPeriod );
        SX1278BWrite( REG_LR_IRQFLAGSMASK, SX1278BLR->RegIrqFlagsMask );

        // Initializes the payload size
        SX1278BLR->RegPayloadLength = TxPacketSize;
        SX1278BWrite( REG_LR_PAYLOADLENGTH, SX1278BLR->RegPayloadLength );
        
        SX1278BLR->RegFifoTxBaseAddr = 0x00; // Full buffer used for Tx
        SX1278BWrite( REG_LR_FIFOTXBASEADDR, SX1278BLR->RegFifoTxBaseAddr );

        SX1278BLR->RegFifoAddrPtr = SX1278BLR->RegFifoTxBaseAddr;
        SX1278BWrite( REG_LR_FIFOADDRPTR, SX1278BLR->RegFifoAddrPtr );
        
        // Write payload buffer to LORA modem
        SX1278BWriteFifo( RFBuffer, SX1278BLR->RegPayloadLength );
		
                                        // TxDone               RxTimeout                   FhssChangeChannel          ValidHeader         
        SX1278BLR->RegDioMapping1 = RFLR_DIOMAPPING1_DIO0_01 | RFLR_DIOMAPPING1_DIO1_00 | RFLR_DIOMAPPING1_DIO2_00 | RFLR_DIOMAPPING1_DIO3_01;
                                        // PllLock              Mode Ready
        SX1278BLR->RegDioMapping2 = RFLR_DIOMAPPING2_DIO4_01 | RFLR_DIOMAPPING2_DIO5_00;
		
        SX1278BWriteBuffer( REG_LR_DIOMAPPING1, &SX1278BLR->RegDioMapping1, 2 );

        SX1278BLoRaSetOpMode( RFLR_OPMODE_TRANSMITTER );

        RFLRState = RFLR_STATE_TX_RUNNING;
        break;
		
    case RFLR_STATE_TX_RUNNING:
        if( DIO0 == 1 ) // TxDone
        {
            // Clear Irq
            SX1278BWrite( REG_LR_IRQFLAGS, RFLR_IRQFLAGS_TXDONE  );
            RFLRState = RFLR_STATE_TX_DONE;
			
			B_RFPA0133_OFF();
        }
		
//         if( DIO2 == 1 ) // FHSS Changed Channel
//         {
//             if( BLoRaSettings.FreqHopOn == true )
//             {
//                 SX1278BRead( REG_LR_HOPCHANNEL, &SX1278BLR->RegHopChannel );
//                 SX1278BLoRaSetRFFrequency( B_HoppingFrequencies[SX1278BLR->RegHopChannel & RFLR_HOPCHANNEL_CHANNEL_MASK] );
//             }
//             // Clear Irq
//             SX1278BWrite( REG_LR_IRQFLAGS, RFLR_IRQFLAGS_FHSSCHANGEDCHANNEL );
//         }
        break;
		
    case RFLR_STATE_TX_DONE:
        // optimize the power consumption by switching off the transmitter as soon as the packet has been sent
        SX1278BLoRaSetOpMode( RFLR_OPMODE_STANDBY );

        RFLRState = RFLR_STATE_IDLE;
        result = RF_TX_DONE;
        break;
		
    case RFLR_STATE_CAD_INIT:    
        SX1278BLoRaSetOpMode( RFLR_OPMODE_STANDBY );
    
        SX1278BLR->RegIrqFlagsMask = RFLR_IRQFLAGS_RXTIMEOUT |
                                    RFLR_IRQFLAGS_RXDONE |
                                    RFLR_IRQFLAGS_PAYLOADCRCERROR |
                                    RFLR_IRQFLAGS_VALIDHEADER |
                                    RFLR_IRQFLAGS_TXDONE |
                                    //RFLR_IRQFLAGS_CADDONE |
                                    RFLR_IRQFLAGS_FHSSCHANGEDCHANNEL; // |
                                    //RFLR_IRQFLAGS_CADDETECTED;
        SX1278BWrite( REG_LR_IRQFLAGSMASK, SX1278BLR->RegIrqFlagsMask );
           
                                    // RxDone                   RxTimeout                   FhssChangeChannel           CadDone
        SX1278BLR->RegDioMapping1 = RFLR_DIOMAPPING1_DIO0_00 | RFLR_DIOMAPPING1_DIO1_00 | RFLR_DIOMAPPING1_DIO2_00 | RFLR_DIOMAPPING1_DIO3_00;
                                    // CAD Detected              ModeReady
        SX1278BLR->RegDioMapping2 = RFLR_DIOMAPPING2_DIO4_00 | RFLR_DIOMAPPING2_DIO5_00;
        SX1278BWriteBuffer( REG_LR_DIOMAPPING1, &SX1278BLR->RegDioMapping1, 2 );
            
        SX1278BLoRaSetOpMode( RFLR_OPMODE_CAD );
        RFLRState = RFLR_STATE_CAD_RUNNING;
        break;
		
    case RFLR_STATE_CAD_RUNNING:
        if( DIO3 == 1 ) //CAD Done interrupt
        { 
            // Clear Irq
            SX1278BWrite( REG_LR_IRQFLAGS, RFLR_IRQFLAGS_CADDONE  );
            if( DIO4 == 1 ) // CAD Detected interrupt
            {
                // Clear Irq
                SX1278BWrite( REG_LR_IRQFLAGS, RFLR_IRQFLAGS_CADDETECTED  );
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

