#include "platform.h"
#include "sx1278a-Hal.h"
#include "sx1278a.h"
#include "sx1278a-LoRa.h"
#include "sx1278a-LoRaMisc.h"

/*!
 * SX1278A definitions
 */
#define XTAL_FREQ                                   32000000
#define FREQ_STEP                                   61.03515625


void SX1278ALoRaSetRFFrequency( uint32_t freq )
{
    LoRaSettings.RFFrequency = freq;

    freq = ( uint32_t )( ( double )freq / ( double )FREQ_STEP );
    SX1278ALR->RegFrfMsb = ( uint8_t )( ( freq >> 16 ) & 0xFF );
    SX1278ALR->RegFrfMid = ( uint8_t )( ( freq >> 8 ) & 0xFF );
    SX1278ALR->RegFrfLsb = ( uint8_t )( freq & 0xFF );
    SX1278AWriteBuffer( REG_LR_FRFMSB, &SX1278ALR->RegFrfMsb, 3 );
}

uint32_t SX1278ALoRaGetRFFrequency( void )
{
    SX1278AReadBuffer( REG_LR_FRFMSB, &SX1278ALR->RegFrfMsb, 3 );
    LoRaSettings.RFFrequency = ( ( uint32_t )SX1278ALR->RegFrfMsb << 16 ) | ( ( uint32_t )SX1278ALR->RegFrfMid << 8 ) | ( ( uint32_t )SX1278ALR->RegFrfLsb );
    LoRaSettings.RFFrequency = ( uint32_t )( ( double )LoRaSettings.RFFrequency * ( double )FREQ_STEP );

    return LoRaSettings.RFFrequency;
}

void SX1278ALoRaSetRFPower( int8_t power )
{
    SX1278ARead( REG_LR_PACONFIG, &SX1278ALR->RegPaConfig );
    SX1278ARead( REG_LR_PADAC, &SX1278ALR->RegPaDac );
    
    if( ( SX1278ALR->RegPaConfig & RFLR_PACONFIG_PASELECT_PABOOST ) == RFLR_PACONFIG_PASELECT_PABOOST )
    {
        if( ( SX1278ALR->RegPaDac & 0x87 ) == 0x87 )
        {
            if( power < 5 )
            {
                power = 5;
            }
            if( power > 20 )
            {
                power = 20;
            }
            SX1278ALR->RegPaConfig = ( SX1278ALR->RegPaConfig & RFLR_PACONFIG_MAX_POWER_MASK ) | 0x70;
            SX1278ALR->RegPaConfig = ( SX1278ALR->RegPaConfig & RFLR_PACONFIG_OUTPUTPOWER_MASK ) | ( uint8_t )( ( uint16_t )( power - 5 ) & 0x0F );
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
            SX1278ALR->RegPaConfig = ( SX1278ALR->RegPaConfig & RFLR_PACONFIG_MAX_POWER_MASK ) | 0x70;
            SX1278ALR->RegPaConfig = ( SX1278ALR->RegPaConfig & RFLR_PACONFIG_OUTPUTPOWER_MASK ) | ( uint8_t )( ( uint16_t )( power - 2 ) & 0x0F );
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
        SX1278ALR->RegPaConfig = ( SX1278ALR->RegPaConfig & RFLR_PACONFIG_MAX_POWER_MASK ) | 0x70;
        SX1278ALR->RegPaConfig = ( SX1278ALR->RegPaConfig & RFLR_PACONFIG_OUTPUTPOWER_MASK ) | ( uint8_t )( ( uint16_t )( power + 1 ) & 0x0F );
    }
    SX1278AWrite( REG_LR_PACONFIG, SX1278ALR->RegPaConfig );
    LoRaSettings.Power = power;
}

int8_t SX1278ALoRaGetRFPower( void )
{
    SX1278ARead( REG_LR_PACONFIG, &SX1278ALR->RegPaConfig );
    SX1278ARead( REG_LR_PADAC, &SX1278ALR->RegPaDac );

    if( ( SX1278ALR->RegPaConfig & RFLR_PACONFIG_PASELECT_PABOOST ) == RFLR_PACONFIG_PASELECT_PABOOST )
    {
        if( ( SX1278ALR->RegPaDac & 0x07 ) == 0x07 )
        {
            LoRaSettings.Power = 5 + ( SX1278ALR->RegPaConfig & ~RFLR_PACONFIG_OUTPUTPOWER_MASK );
        }
        else
        {
            LoRaSettings.Power = 2 + ( SX1278ALR->RegPaConfig & ~RFLR_PACONFIG_OUTPUTPOWER_MASK );
        }
    }
    else
    {
        LoRaSettings.Power = -1 + ( SX1278ALR->RegPaConfig & ~RFLR_PACONFIG_OUTPUTPOWER_MASK );
    }
    return LoRaSettings.Power;
}

void SX1278ALoRaSetSignalBandwidth( uint8_t bw )
{
    SX1278ARead( REG_LR_MODEMCONFIG1, &SX1278ALR->RegModemConfig1 );
    SX1278ALR->RegModemConfig1 = ( SX1278ALR->RegModemConfig1 & RFLR_MODEMCONFIG1_BW_MASK ) | ( bw << 4 );
    SX1278AWrite( REG_LR_MODEMCONFIG1, SX1278ALR->RegModemConfig1 );
    LoRaSettings.SignalBw = bw;
}

uint8_t SX1278ALoRaGetSignalBandwidth( void )
{
    SX1278ARead( REG_LR_MODEMCONFIG1, &SX1278ALR->RegModemConfig1 );
    LoRaSettings.SignalBw = ( SX1278ALR->RegModemConfig1 & ~RFLR_MODEMCONFIG1_BW_MASK ) >> 4;
    return LoRaSettings.SignalBw;
}

void SX1278ALoRaSetSpreadingFactor( uint8_t factor )
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
        SX1278ALoRaSetNbTrigPeaks( 5 );
    }
    else
    {
        SX1278ALoRaSetNbTrigPeaks( 3 );
    }

    SX1278ARead( REG_LR_MODEMCONFIG2, &SX1278ALR->RegModemConfig2 );    
    SX1278ALR->RegModemConfig2 = ( SX1278ALR->RegModemConfig2 & RFLR_MODEMCONFIG2_SF_MASK ) | ( factor << 4 );
    SX1278AWrite( REG_LR_MODEMCONFIG2, SX1278ALR->RegModemConfig2 );    
    LoRaSettings.SpreadingFactor = factor;
}

uint8_t SX1278ALoRaGetSpreadingFactor( void )
{
    SX1278ARead( REG_LR_MODEMCONFIG2, &SX1278ALR->RegModemConfig2 );   
    LoRaSettings.SpreadingFactor = ( SX1278ALR->RegModemConfig2 & ~RFLR_MODEMCONFIG2_SF_MASK ) >> 4;
    return LoRaSettings.SpreadingFactor;
}

void SX1278ALoRaSetErrorCoding( uint8_t value )
{
    SX1278ARead( REG_LR_MODEMCONFIG1, &SX1278ALR->RegModemConfig1 );
    SX1278ALR->RegModemConfig1 = ( SX1278ALR->RegModemConfig1 & RFLR_MODEMCONFIG1_CODINGRATE_MASK ) | ( value << 1 );
    SX1278AWrite( REG_LR_MODEMCONFIG1, SX1278ALR->RegModemConfig1 );
    LoRaSettings.ErrorCoding = value;
}

uint8_t SX1278ALoRaGetErrorCoding( void )
{
    SX1278ARead( REG_LR_MODEMCONFIG1, &SX1278ALR->RegModemConfig1 );
    LoRaSettings.ErrorCoding = ( SX1278ALR->RegModemConfig1 & ~RFLR_MODEMCONFIG1_CODINGRATE_MASK ) >> 1;
    return LoRaSettings.ErrorCoding;
}

void SX1278ALoRaSetPacketCrcOn( bool enable )
{
    SX1278ARead( REG_LR_MODEMCONFIG2, &SX1278ALR->RegModemConfig2 );
    SX1278ALR->RegModemConfig2 = ( SX1278ALR->RegModemConfig2 & RFLR_MODEMCONFIG2_RXPAYLOADCRC_MASK ) | ( enable << 2 );
    SX1278AWrite( REG_LR_MODEMCONFIG2, SX1278ALR->RegModemConfig2 );
    LoRaSettings.CrcOn = enable;
}

void SX1278ALoRaSetPreambleLength( uint16_t value )
{
    SX1278AReadBuffer( REG_LR_PREAMBLEMSB, &SX1278ALR->RegPreambleMsb, 2 );

    SX1278ALR->RegPreambleMsb = ( value >> 8 ) & 0x00FF;
    SX1278ALR->RegPreambleLsb = value & 0xFF;
    SX1278AWriteBuffer( REG_LR_PREAMBLEMSB, &SX1278ALR->RegPreambleMsb, 2 );
}

uint16_t SX1278ALoRaGetPreambleLength( void )
{
    SX1278AReadBuffer( REG_LR_PREAMBLEMSB, &SX1278ALR->RegPreambleMsb, 2 );
    return ( ( SX1278ALR->RegPreambleMsb & 0x00FF ) << 8 ) | SX1278ALR->RegPreambleLsb;
}

bool SX1278ALoRaGetPacketCrcOn( void )
{
    SX1278ARead( REG_LR_MODEMCONFIG2, &SX1278ALR->RegModemConfig2 );
    LoRaSettings.CrcOn = ( SX1278ALR->RegModemConfig2 & RFLR_MODEMCONFIG2_RXPAYLOADCRC_ON ) >> 1;
    return LoRaSettings.CrcOn;
}

void SX1278ALoRaSetImplicitHeaderOn( bool enable )
{
    SX1278ARead( REG_LR_MODEMCONFIG1, &SX1278ALR->RegModemConfig1 );
    SX1278ALR->RegModemConfig1 = ( SX1278ALR->RegModemConfig1 & RFLR_MODEMCONFIG1_IMPLICITHEADER_MASK ) | ( enable );
    SX1278AWrite( REG_LR_MODEMCONFIG1, SX1278ALR->RegModemConfig1 );
    LoRaSettings.ImplicitHeaderOn = enable;
}

bool SX1278ALoRaGetImplicitHeaderOn( void )
{
    SX1278ARead( REG_LR_MODEMCONFIG1, &SX1278ALR->RegModemConfig1 );
    LoRaSettings.ImplicitHeaderOn = ( SX1278ALR->RegModemConfig1 & RFLR_MODEMCONFIG1_IMPLICITHEADER_ON );
    return LoRaSettings.ImplicitHeaderOn;
}

void SX1278ALoRaSetRxSingleOn( bool enable )
{
    LoRaSettings.RxSingleOn = enable;
}

bool SX1278ALoRaGetRxSingleOn( void )
{
    return LoRaSettings.RxSingleOn;
}

void SX1278ALoRaSetFreqHopOn( bool enable )
{
    LoRaSettings.FreqHopOn = enable;
}

bool SX1278ALoRaGetFreqHopOn( void )
{
    return LoRaSettings.FreqHopOn;
}

void SX1278ALoRaSetHopPeriod( uint8_t value )
{
    SX1278ALR->RegHopPeriod = value;
    SX1278AWrite( REG_LR_HOPPERIOD, SX1278ALR->RegHopPeriod );
    LoRaSettings.HopPeriod = value;
}

uint8_t SX1278ALoRaGetHopPeriod( void )
{
    SX1278ARead( REG_LR_HOPPERIOD, &SX1278ALR->RegHopPeriod );
    LoRaSettings.HopPeriod = SX1278ALR->RegHopPeriod;
    return LoRaSettings.HopPeriod;
}

void SX1278ALoRaSetTxPacketTimeout( uint32_t value )
{
    LoRaSettings.TxPacketTimeout = value;
}

uint32_t SX1278ALoRaGetTxPacketTimeout( void )
{
    return LoRaSettings.TxPacketTimeout;
}

void SX1278ALoRaSetRxPacketTimeout( uint32_t value )
{
    LoRaSettings.RxPacketTimeout = value;
}

uint32_t SX1278ALoRaGetRxPacketTimeout( void )
{
    return LoRaSettings.RxPacketTimeout;
}

void SX1278ALoRaSetPayloadLength( uint8_t value )
{
    SX1278ALR->RegPayloadLength = value;
    SX1278AWrite( REG_LR_PAYLOADLENGTH, SX1278ALR->RegPayloadLength );
    LoRaSettings.PayloadLength = value;
}

uint8_t SX1278ALoRaGetPayloadLength( void )
{
    SX1278ARead( REG_LR_PAYLOADLENGTH, &SX1278ALR->RegPayloadLength );
    LoRaSettings.PayloadLength = SX1278ALR->RegPayloadLength;
    return LoRaSettings.PayloadLength;
}

void SX1278ALoRaSetPa20dBm( bool enale )
{
    SX1278ARead( REG_LR_PADAC, &SX1278ALR->RegPaDac );
    SX1278ARead( REG_LR_PACONFIG, &SX1278ALR->RegPaConfig );

    if( ( SX1278ALR->RegPaConfig & RFLR_PACONFIG_PASELECT_PABOOST ) == RFLR_PACONFIG_PASELECT_PABOOST )
    {    
        if( enale == true )
        {
            SX1278ALR->RegPaDac = 0x87;
        }
    }
    else
    {
        SX1278ALR->RegPaDac = 0x84;
    }
    SX1278AWrite( REG_LR_PADAC, SX1278ALR->RegPaDac );
}

bool SX1278ALoRaGetPa20dBm( void )
{
    SX1278ARead( REG_LR_PADAC, &SX1278ALR->RegPaDac );
    
    return ( ( SX1278ALR->RegPaDac & 0x07 ) == 0x07 ) ? true : false;
}

void SX1278ALoRaSetPAOutput( uint8_t outputPin )
{
    SX1278ARead( REG_LR_PACONFIG, &SX1278ALR->RegPaConfig );
    SX1278ALR->RegPaConfig = (SX1278ALR->RegPaConfig & RFLR_PACONFIG_PASELECT_MASK ) | outputPin;
    SX1278AWrite( REG_LR_PACONFIG, SX1278ALR->RegPaConfig );
}

uint8_t SX1278ALoRaGetPAOutput( void )
{
    SX1278ARead( REG_LR_PACONFIG, &SX1278ALR->RegPaConfig );
    return SX1278ALR->RegPaConfig & ~RFLR_PACONFIG_PASELECT_MASK;
}

void SX1278ALoRaSetPaRamp( uint8_t value )
{
    SX1278ARead( REG_LR_PARAMP, &SX1278ALR->RegPaRamp );
    SX1278ALR->RegPaRamp = ( SX1278ALR->RegPaRamp & RFLR_PARAMP_MASK ) | ( value & ~RFLR_PARAMP_MASK );
    SX1278AWrite( REG_LR_PARAMP, SX1278ALR->RegPaRamp );
}

uint8_t SX1278ALoRaGetPaRamp( void )
{
    SX1278ARead( REG_LR_PARAMP, &SX1278ALR->RegPaRamp );
    return SX1278ALR->RegPaRamp & ~RFLR_PARAMP_MASK;
}

void SX1278ALoRaSetSymbTimeout( uint16_t value )
{
    SX1278AReadBuffer( REG_LR_MODEMCONFIG2, &SX1278ALR->RegModemConfig2, 2 );

    SX1278ALR->RegModemConfig2 = ( SX1278ALR->RegModemConfig2 & RFLR_MODEMCONFIG2_SYMBTIMEOUTMSB_MASK ) | ( ( value >> 8 ) & ~RFLR_MODEMCONFIG2_SYMBTIMEOUTMSB_MASK );
    SX1278ALR->RegSymbTimeoutLsb = value & 0xFF;
    SX1278AWriteBuffer( REG_LR_MODEMCONFIG2, &SX1278ALR->RegModemConfig2, 2 );
}

uint16_t SX1278ALoRaGetSymbTimeout( void )
{
    SX1278AReadBuffer( REG_LR_MODEMCONFIG2, &SX1278ALR->RegModemConfig2, 2 );
    return ( ( SX1278ALR->RegModemConfig2 & ~RFLR_MODEMCONFIG2_SYMBTIMEOUTMSB_MASK ) << 8 ) | SX1278ALR->RegSymbTimeoutLsb;
}

void SX1278ALoRaSetLowDatarateOptimize( bool enable )
{
    SX1278ARead( REG_LR_MODEMCONFIG3, &SX1278ALR->RegModemConfig3 );
    SX1278ALR->RegModemConfig3 = ( SX1278ALR->RegModemConfig3 & RFLR_MODEMCONFIG3_LOWDATARATEOPTIMIZE_MASK ) | ( enable << 3 );
    SX1278AWrite( REG_LR_MODEMCONFIG3, SX1278ALR->RegModemConfig3 );
}

bool SX1278ALoRaGetLowDatarateOptimize( void )
{
    SX1278ARead( REG_LR_MODEMCONFIG3, &SX1278ALR->RegModemConfig3 );
    return ( ( SX1278ALR->RegModemConfig3 & RFLR_MODEMCONFIG3_LOWDATARATEOPTIMIZE_ON ) >> 3 );
}

void SX1278ALoRaSetNbTrigPeaks( uint8_t value )
{
    SX1278ARead( 0x31, &SX1278ALR->RegTestReserved31 );
    SX1278ALR->RegTestReserved31 = ( SX1278ALR->RegTestReserved31 & 0xF8 ) | value;
    SX1278AWrite( 0x31, SX1278ALR->RegTestReserved31 );
}

uint8_t SX1278ALoRaGetNbTrigPeaks( void )
{
    SX1278ARead( 0x31, &SX1278ALR->RegTestReserved31 );
    return ( SX1278ALR->RegTestReserved31 & 0x07 );
}

