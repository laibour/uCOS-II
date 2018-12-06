#ifndef __SX1278A_H__
#define __SX1278A_H__

#include <stdint.h>
#include <stdbool.h>
#include "radio.h"
#include "sx1278a-LoRa.h"


/* RF电源控制端口 */
#define RF_POWER_ON()		GPIO_SetBits(GPIOF, GPIO_Pin_11)	// RF POWER ON
#define RF_POWER_OFF()		GPIO_ResetBits(GPIOF, GPIO_Pin_11)	// RF POWER OFF


int SX1278ARegister(void);
uint8_t SX1278AInit( void );

/*!
 * \brief SX1278AB registers array
 */
extern uint8_t SX1278ARegs[0x70];

/*!
 * \brief Enables LoRa modem or FSK modem
 *
 * \param [IN] opMode New operating mode
 */
void SX1278ASetLoRaOn( bool enable );

/*!
 * \brief Gets the LoRa modem state
 *
 * \retval LoraOn Current LoRa modem mode
 */
bool SX1278AGetLoRaOn( void );

/*!
 * \brief Initializes the SX1278A
 */
uint8_t SX1278AInit( void );

/*!
 * \brief Resets the SX1278A
 */
void SX1278AReset( void );

/*!
 * \brief Sets the SX1278A operating mode
 *
 * \param [IN] opMode New operating mode
 */
void SX1278ASetOpMode( uint8_t opMode );

/*!
 * \brief Gets the SX1278A operating mode
 *
 * \retval opMode Current operating mode
 */
uint8_t SX1278AGetOpMode( void );

/*!
 * \brief Reads the current Rx gain setting
 *
 * \retval rxGain Current gain setting
 */
uint8_t SX1278AReadRxGain( void );

/*!
 * \brief Trigs and reads the current RSSI value
 *
 * \retval rssiValue Current RSSI value in [dBm]
 */
double SX1278AReadRssi( void );

/*!
 * \brief Gets the Rx gain value measured while receiving the packet
 *
 * \retval rxGainValue Current Rx gain value
 */
uint8_t SX1278AGetPacketRxGain( void );

/*!
 * \brief Gets the SNR value measured while receiving the packet
 *
 * \retval snrValue Current SNR value in [dB]
 */
int8_t SX1278AGetPacketSnr( void );

/*!
 * \brief Gets the RSSI value measured while receiving the packet
 *
 * \retval rssiValue Current RSSI value in [dBm]
 */
double SX1278AGetPacketRssi( void );

/*!
 * \brief Gets the AFC value measured while receiving the packet
 *
 * \retval afcValue Current AFC value in [Hz]
 */
uint32_t SX1278AGetPacketAfc( void );

/*!
 * \brief Sets the radio in Rx mode. Waiting for a packet
 */
void SX1278AStartRx( void );

/*!
 * \brief Gets a copy of the current received buffer
 *
 * \param [IN]: buffer     Buffer pointer
 * \param [IN]: size       Buffer size
 */
void SX1278AGetRxPacket( void *buffer, uint16_t *size );

/*!
 * \brief Sets a copy of the buffer to be transmitted and starts the
 *        transmission
 *
 * \param [IN]: buffer     Buffer pointer
 * \param [IN]: size       Buffer size
 */
void SX1278ASetTxPacket( const void *buffer, uint16_t size );

/*!
 * \brief Gets the current RFState
 *
 * \retval rfState Current RF state [RF_IDLE, RF_BUSY, 
 *                                   RF_RX_DONE, RF_RX_TIMEOUT,
 *                                   RF_TX_DONE, RF_TX_TIMEOUT]
 */
uint8_t SX1278AGetRFState( void );

/*!
 * \brief Sets the new state of the RF state machine
 *
 * \param [IN]: state New RF state machine state
 */
void SX1278ASetRFState( uint8_t state );

/*!
 * \brief Process the Rx and Tx state machines depending on the
 *       SX1278A operating mode.
 *
 * \retval rfState Current RF state [RF_IDLE, RF_BUSY, 
 *                                   RF_RX_DONE, RF_RX_TIMEOUT,
 *                                   RF_TX_DONE, RF_TX_TIMEOUT]
 */
uint32_t SX1278AProcess( void );

#endif //__SX1278A_H__
