#include "platform.h"
#include "sx1278b.h"
#include "sx1278b-Hal.h"
#include "sx1278b-LoRa.h"
#include "sx1278b-LoRaMisc.h"

/*!
 * SX1278B definitions
 */
#define XTAL_FREQ                                   32000000
#define FREQ_STEP                                   61.03515625


void SX1278BLoRaSetRFFrequency( uint32_t freq )
{
    BLoRaSettings.RFFrequency = freq;

    freq = ( uint32_t )( ( double )freq / ( double )FREQ_STEP );
    SX1278BLR->RegFrfMsb = ( uint8_t )( ( freq >> 16 ) & 0xFF );
    SX1278BLR->RegFrfMid = ( uint8_t )( ( freq >> 8 ) & 0xFF );
    SX1278BLR->RegFrfLsb = ( uint8_t )( freq & 0xFF );
    SX1278BWriteBuffer( REG_LR_FRFMSB, &SX1278BLR->RegFrfMsb, 3 );
}

uint32_t SX1278BLoRaGetRFFrequency( void )
{
    SX1278BReadBuffer( REG_LR_FRFMSB, &SX1278BLR->RegFrfMsb, 3 );
    BLoRaSettings.RFFrequency = ( ( uint32_t )SX1278BLR->RegFrfMsb << 16 ) | ( ( uint32_t )SX1278BLR->RegFrfMid << 8 ) | ( ( uint32_t )SX1278BLR->RegFrfLsb );
    BLoRaSettings.RFFrequency = ( uint32_t )( ( double )BLoRaSettings.RFFrequency * ( double )FREQ_STEP );

    return BLoRaSettings.RFFrequency;
}

void SX1278BLoRaSetRFPower( int8_t power )
{
    SX1278BRead( REG_LR_PACONFIG, &SX1278BLR->RegPaConfig );
    SX1278BRead( REG_LR_PADAC, &SX1278BLR->RegPaDac );
    
    if( ( SX1278BLR->RegPaConfig & RFLR_PACONFIG_PASELECT_PABOOST ) == RFLR_PACONFIG_PASELECT_PABOOST )
    {
        if( ( SX1278BLR->RegPaDac & 0x87 ) == 0x87 )
        {
            if( power < 5 )
            {
                power = 5;
            }
            if( power > 20 )
            {
                power = 20;
            }
            SX1278BLR->RegPaConfig = ( SX1278BLR->RegPaConfig & RFLR_PACONFIG_MAX_POWER_MASK ) | 0x70;
            SX1278BLR->RegPaConfig = ( SX1278BLR->RegPaConfig & RFLR_PACONFIG_OUTPUTPOWER_MASK ) | ( uint8_t )( ( uint16_t )( power - 5 ) & 0x0F );
        }
        else
        {
            if( power < 2 )
            {
                power = 2;
            }
            if( power > 17 )
            {
                power = 17;
            }
            SX1278BLR->RegPaConfig = ( SX1278BLR->RegPaConfig & RFLR_PACONFIG_MAX_POWER_MASK ) | 0x70;
            SX1278BLR->RegPaConfig = ( SX1278BLR->RegPaConfig & RFLR_PACONFIG_OUTPUTPOWER_MASK ) | ( uint8_t )( ( uint16_t )( power - 2 ) & 0x0F );
        }
    }
    else
    {
        if( power < -1 )
        {
            power = -1;
        }
        if( power > 14 )
        {
            power = 14;
        }
        SX1278BLR->RegPaConfig = ( SX1278BLR->RegPaConfig & RFLR_PACONFIG_MAX_POWER_MASK ) | 0x70;
        SX1278BLR->RegPaConfig = ( SX1278BLR->RegPaConfig & RFLR_PACONFIG_OUTPUTPOWER_MASK ) | ( uint8_t )( ( uint16_t )( power + 1 ) & 0x0F );
    }
    SX1278BWrite( REG_LR_PACONFIG, SX1278BLR->RegPaConfig );
    BLoRaSettings.Power = power;
}

int8_t SX1278BLoRaGetRFPower( void )
{
    SX1278BRead( REG_LR_PACONFIG, &SX1278BLR->RegPaConfig );
    SX1278BRead( REG_LR_PADAC, &SX1278BLR->RegPaDac );

    if( ( SX1278BLR->RegPaConfig & RFLR_PACONFIG_PASELECT_PABOOST ) == RFLR_PACONFIG_PASELECT_PABOOST )
    {
        if( ( SX1278BLR->RegPaDac & 0x07 ) == 0x07 )
        {
            BLoRaSettings.Power = 5 + ( SX1278BLR->RegPaConfig & ~RFLR_PACONFIG_OUTPUTPOWER_MASK );
        }
        else
        {
            BLoRaSettings.Power = 2 + ( SX1278BLR->RegPaConfig & ~RFLR_PACONFIG_OUTPUTPOWER_MASK );
        }
    }
    else
    {
        BLoRaSettings.Power = -1 + ( SX1278BLR->RegPaConfig & ~RFLR_PACONFIG_OUTPUTPOWER_MASK );
    }
    return BLoRaSettings.Power;
}

void SX1278BLoRaSetSignalBandwidth( uint8_t bw )
{
    SX1278BRead( REG_LR_MODEMCONFIG1, &SX1278BLR->RegModemConfig1 );
    SX1278BLR->RegModemConfig1 = ( SX1278BLR->RegModemConfig1 & RFLR_MODEMCONFIG1_BW_MASK ) | ( bw << 4 );
    SX1278BWrite( REG_LR_MODEMCONFIG1, SX1278BLR->RegModemConfig1 );
    BLoRaSettings.SignalBw = bw;
}

uint8_t SX1278BLoRaGetSignalBandwidth( void )
{
    SX1278BRead( REG_LR_MODEMCONFIG1, &SX1278BLR->RegModemConfig1 );
    BLoRaSettings.SignalBw = ( SX1278BLR->RegModemConfig1 & ~RFLR_MODEMCONFIG1_BW_MASK ) >> 4;
    return BLoRaSettings.SignalBw;
}

void SX1278BLoRaSetSpreadingFactor( uint8_t factor )
{

    if( factor > 12 )
    {
        factor = 12;
    }
    else if( factor < 6 )
    {
        factor = 6;
    }

    if( factor == 6 )
    {
        SX1278BLoRaSetNbTrigPeaks( 5 );
    }
    else
    {
        SX1278BLoRaSetNbTrigPeaks( 3 );
    }

    SX1278BRead( REG_LR_MODEMCONFIG2, &SX1278BLR->RegModemConfig2 );    
    SX1278BLR->RegModemConfig2 = ( SX1278BLR->RegModemConfig2 & RFLR_MODEMCONFIG2_SF_MASK ) | ( factor << 4 );
    SX1278BWrite( REG_LR_MODEMCONFIG2, SX1278BLR->RegModemConfig2 );    
    BLoRaSettings.SpreadingFactor = factor;
}

uint8_t SX1278BLoRaGetSpreadingFactor( void )
{
    SX1278BRead( REG_LR_MODEMCONFIG2, &SX1278BLR->RegModemConfig2 );   
    BLoRaSettings.SpreadingFactor = ( SX1278BLR->RegModemConfig2 & ~RFLR_MODEMCONFIG2_SF_MASK ) >> 4;
    return BLoRaSettings.SpreadingFactor;
}

void SX1278BLoRaSetErrorCoding( uint8_t value )
{
    SX1278BRead( REG_LR_MODEMCONFIG1, &SX1278BLR->RegModemConfig1 );
    SX1278BLR->RegModemConfig1 = ( SX1278BLR->RegModemConfig1 & RFLR_MODEMCONFIG1_CODINGRATE_MASK ) | ( value << 1 );
    SX1278BWrite( REG_LR_MODEMCONFIG1, SX1278BLR->RegModemConfig1 );
    BLoRaSettings.ErrorCoding = value;
}

uint8_t SX1278BLoRaGetErrorCoding( void )
{
    SX1278BRead( REG_LR_MODEMCONFIG1, &SX1278BLR->RegModemConfig1 );
    BLoRaSettings.ErrorCoding = ( SX1278BLR->RegModemConfig1 & ~RFLR_MODEMCONFIG1_CODINGRATE_MASK ) >> 1;
    return BLoRaSettings.ErrorCoding;
}

void SX1278BLoRaSetPacketCrcOn( bool enable )
{
    SX1278BRead( REG_LR_MODEMCONFIG2, &SX1278BLR->RegModemConfig2 );
    SX1278BLR->RegModemConfig2 = ( SX1278BLR->RegModemConfig2 & RFLR_MODEMCONFIG2_RXPAYLOADCRC_MASK ) | ( enable << 2 );
    SX1278BWrite( REG_LR_MODEMCONFIG2, SX1278BLR->RegModemConfig2 );
    BLoRaSettings.CrcOn = enable;
}

void SX1278BLoRaSetPreambleLength( uint16_t value )
{
    SX1278BReadBuffer( REG_LR_PREAMBLEMSB, &SX1278BLR->RegPreambleMsb, 2 );

    SX1278BLR->RegPreambleMsb = ( value >> 8 ) & 0x00FF;
    SX1278BLR->RegPreambleLsb = value & 0xFF;
    SX1278BWriteBuffer( REG_LR_PREAMBLEMSB, &SX1278BLR->RegPreambleMsb, 2 );
}

uint16_t SX1278BLoRaGetPreambleLength( void )
{
    SX1278BReadBuffer( REG_LR_PREAMBLEMSB, &SX1278BLR->RegPreambleMsb, 2 );
    return ( ( SX1278BLR->RegPreambleMsb & 0x00FF ) << 8 ) | SX1278BLR->RegPreambleLsb;
}

bool SX1278BLoRaGetPacketCrcOn( void )
{
    SX1278BRead( REG_LR_MODEMCONFIG2, &SX1278BLR->RegModemConfig2 );
    BLoRaSettings.CrcOn = ( SX1278BLR->RegModemConfig2 & RFLR_MODEMCONFIG2_RXPAYLOADCRC_ON ) >> 1;
    return BLoRaSettings.CrcOn;
}

void SX1278BLoRaSetImplicitHeaderOn( bool enable )
{
    SX1278BRead( REG_LR_MODEMCONFIG1, &SX1278BLR->RegModemConfig1 );
    SX1278BLR->RegModemConfig1 = ( SX1278BLR->RegModemConfig1 & RFLR_MODEMCONFIG1_IMPLICITHEADER_MASK ) | ( enable );
    SX1278BWrite( REG_LR_MODEMCONFIG1, SX1278BLR->RegModemConfig1 );
    BLoRaSettings.ImplicitHeaderOn = enable;
}

bool SX1278BLoRaGetImplicitHeaderOn( void )
{
    SX1278BRead( REG_LR_MODEMCONFIG1, &SX1278BLR->RegModemConfig1 );
    BLoRaSettings.ImplicitHeaderOn = ( SX1278BLR->RegModemConfig1 & RFLR_MODEMCONFIG1_IMPLICITHEADER_ON );
    return BLoRaSettings.ImplicitHeaderOn;
}

void SX1278BLoRaSetRxSingleOn( bool enable )
{
    BLoRaSettings.RxSingleOn = enable;
}

bool SX1278BLoRaGetRxSingleOn( void )
{
    return BLoRaSettings.RxSingleOn;
}

void SX1278BLoRaSetFreqHopOn( bool enable )
{
    BLoRaSettings.FreqHopOn = enable;
}

bool SX1278BLoRaGetFreqHopOn( void )
{
    return BLoRaSettings.FreqHopOn;
}

void SX1278BLoRaSetHopPeriod( uint8_t value )
{
    SX1278BLR->RegHopPeriod = value;
    SX1278BWrite( REG_LR_HOPPERIOD, SX1278BLR->RegHopPeriod );
    BLoRaSettings.HopPeriod = value;
}

uint8_t SX1278BLoRaGetHopPeriod( void )
{
    SX1278BRead( REG_LR_HOPPERIOD, &SX1278BLR->RegHopPeriod );
    BLoRaSettings.HopPeriod = SX1278BLR->RegHopPeriod;
    return BLoRaSettings.HopPeriod;
}

void SX1278BLoRaSetTxPacketTimeout( uint32_t value )
{
    BLoRaSettings.TxPacketTimeout = value;
}

uint32_t SX1278BLoRaGetTxPacketTimeout( void )
{
    return BLoRaSettings.TxPacketTimeout;
}

void SX1278BLoRaSetRxPacketTimeout( uint32_t value )
{
    BLoRaSettings.RxPacketTimeout = value;
}

uint32_t SX1278BLoRaGetRxPacketTimeout( void )
{
    return BLoRaSettings.RxPacketTimeout;
}

void SX1278BLoRaSetPayloadLength( uint8_t value )
{
    SX1278BLR->RegPayloadLength = value;
    SX1278BWrite( REG_LR_PAYLOADLENGTH, SX1278BLR->RegPayloadLength );
    BLoRaSettings.PayloadLength = value;
}

uint8_t SX1278BLoRaGetPayloadLength( void )
{
    SX1278BRead( REG_LR_PAYLOADLENGTH, &SX1278BLR->RegPayloadLength );
    BLoRaSettings.PayloadLength = SX1278BLR->RegPayloadLength;
    return BLoRaSettings.PayloadLength;
}

void SX1278BLoRaSetPa20dBm( bool enale )
{
    SX1278BRead( REG_LR_PADAC, &SX1278BLR->RegPaDac );
    SX1278BRead( REG_LR_PACONFIG, &SX1278BLR->RegPaConfig );

    if( ( SX1278BLR->RegPaConfig & RFLR_PACONFIG_PASELECT_PABOOST ) == RFLR_PACONFIG_PASELECT_PABOOST )
    {    
        if( enale == true )
        {
            SX1278BLR->RegPaDac = 0x87;
        }
    }
    else
    {
        SX1278BLR->RegPaDac = 0x84;
    }
    SX1278BWrite( REG_LR_PADAC, SX1278BLR->RegPaDac );
}

bool SX1278BLoRaGetPa20dBm( void )
{
    SX1278BRead( REG_LR_PADAC, &SX1278BLR->RegPaDac );
    
    return ( ( SX1278BLR->RegPaDac & 0x07 ) == 0x07 ) ? true : false;
}

void SX1278BLoRaSetPAOutput( uint8_t outputPin )
{
    SX1278BRead( REG_LR_PACONFIG, &SX1278BLR->RegPaConfig );
    SX1278BLR->RegPaConfig = (SX1278BLR->RegPaConfig & RFLR_PACONFIG_PASELECT_MASK ) | outputPin;
    SX1278BWrite( REG_LR_PACONFIG, SX1278BLR->RegPaConfig );
}

uint8_t SX1278BLoRaGetPAOutput( void )
{
    SX1278BRead( REG_LR_PACONFIG, &SX1278BLR->RegPaConfig );
    return SX1278BLR->RegPaConfig & ~RFLR_PACONFIG_PASELECT_MASK;
}

void SX1278BLoRaSetPaRamp( uint8_t value )
{
    SX1278BRead( REG_LR_PARAMP, &SX1278BLR->RegPaRamp );
    SX1278BLR->RegPaRamp = ( SX1278BLR->RegPaRamp & RFLR_PARAMP_MASK ) | ( value & ~RFLR_PARAMP_MASK );
    SX1278BWrite( REG_LR_PARAMP, SX1278BLR->RegPaRamp );
}

uint8_t SX1278BLoRaGetPaRamp( void )
{
    SX1278BRead( REG_LR_PARAMP, &SX1278BLR->RegPaRamp );
    return SX1278BLR->RegPaRamp & ~RFLR_PARAMP_MASK;
}

void SX1278BLoRaSetSymbTimeout( uint16_t value )
{
    SX1278BReadBuffer( REG_LR_MODEMCONFIG2, &SX1278BLR->RegModemConfig2, 2 );

    SX1278BLR->RegModemConfig2 = ( SX1278BLR->RegModemConfig2 & RFLR_MODEMCONFIG2_SYMBTIMEOUTMSB_MASK ) | ( ( value >> 8 ) & ~RFLR_MODEMCONFIG2_SYMBTIMEOUTMSB_MASK );
    SX1278BLR->RegSymbTimeoutLsb = value & 0xFF;
    SX1278BWriteBuffer( REG_LR_MODEMCONFIG2, &SX1278BLR->RegModemConfig2, 2 );
}

uint16_t SX1278BLoRaGetSymbTimeout( void )
{
    SX1278BReadBuffer( REG_LR_MODEMCONFIG2, &SX1278BLR->RegModemConfig2, 2 );
    return ( ( SX1278BLR->RegModemConfig2 & ~RFLR_MODEMCONFIG2_SYMBTIMEOUTMSB_MASK ) << 8 ) | SX1278BLR->RegSymbTimeoutLsb;
}

void SX1278BLoRaSetLowDatarateOptimize( bool enable )
{
    SX1278BRead( REG_LR_MODEMCONFIG3, &SX1278BLR->RegModemConfig3 );
    SX1278BLR->RegModemConfig3 = ( SX1278BLR->RegModemConfig3 & RFLR_MODEMCONFIG3_LOWDATARATEOPTIMIZE_MASK ) | ( enable << 3 );
    SX1278BWrite( REG_LR_MODEMCONFIG3, SX1278BLR->RegModemConfig3 );
}

bool SX1278BLoRaGetLowDatarateOptimize( void )
{
    SX1278BRead( REG_LR_MODEMCONFIG3, &SX1278BLR->RegModemConfig3 );
    return ( ( SX1278BLR->RegModemConfig3 & RFLR_MODEMCONFIG3_LOWDATARATEOPTIMIZE_ON ) >> 3 );
}

void SX1278BLoRaSetNbTrigPeaks( uint8_t value )
{
    SX1278BRead( 0x31, &SX1278BLR->RegTestReserved31 );
    SX1278BLR->RegTestReserved31 = ( SX1278BLR->RegTestReserved31 & 0xF8 ) | value;
    SX1278BWrite( 0x31, SX1278BLR->RegTestReserved31 );
}

uint8_t SX1278BLoRaGetNbTrigPeaks( void )
{
    SX1278BRead( 0x31, &SX1278BLR->RegTestReserved31 );
    return ( SX1278BLR->RegTestReserved31 & 0x07 );
}

