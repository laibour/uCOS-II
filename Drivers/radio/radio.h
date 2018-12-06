#ifndef __RADIO_H__
#define __RADIO_H__

#include "sys_config.h"

/*
 * SX1278 General parameters definition
 */
#define LORA                   		(1)    		// [0: OFF, 1: ON]
#define X_MODE_SIZE					(100)		// X mode send data size
#define SX1278_BUFFER_SIZE     		(256)		// Define the SX1278 send/receive size

/*!
 * Radio driver structure defining the different function pointers
 */
typedef struct RadioDriver
{
	char *name;
    int ( *Init )( void );
    void ( *Reset )( void );
    void ( *StartRx )( void );
    void ( *GetRxPacket )( void *buffer, uint16_t *size );
    void ( *SetTxPacket )( const void *buffer, uint16_t size );
    uint32_t ( *Process )( void );
	struct RadioDriver *ptNext;
}T_RadioDriver, *PT_RadioDriver;


typedef struct _RFLED
{
	uint8_t UpFlag;
	uint8_t DownFlag;
}T_RFLED, *PT_RFLED;


typedef struct _XmodeCfg
{
	uint8_t Enable;
	uint8_t Wakeup[9];
	uint8_t Data[X_MODE_SIZE];
}T_XmodeCfg, *PT_XmodeCfg;


typedef struct _SX1278BCongfig
{
	uint8_t Enable;
	uint8_t RFChannel;
	uint8_t RFChannelFlag;
}T_SX1278BCongfig, *PT_SX1278BCongfig;


typedef enum
{
	TASK_DATA_NULL,
    TASK_DATA_BUSY,
    TASK_DATA_DONE,
}tTaskReturnCodes;


typedef struct RadioSendData
{
	uint8_t Len;
	uint8_t Data[255];
}T_RadioSendData, *PT_RadioSendData;


typedef enum
{
	SX1278_STEP_IDLE,
	SX1278_STEP_WAIT,
    SX1278_STEP_WAKEUP,
	SX1278_STEP_SCAN,
	SX1278_STEP_INSTALL,
    SX1278_STEP_SETTING,
}tSX1278Steps;


typedef enum
{
    SX1278_STATE_IDLE,
    SX1278_STATE_RX_INIT,
    SX1278_STATE_RX_RUNNING,
    SX1278_STATE_RX_DONE,
    SX1278_STATE_RX_TIMEOUT,
    SX1278_STATE_TX_INIT,
    SX1278_STATE_TX_RUNNING,
    SX1278_STATE_TX_DONE,
    SX1278_STATE_TX_TIMEOUT,
}tSX1278States;


/*!
 * RF process function return codes
 */
typedef enum
{
    RF_IDLE,
    RF_BUSY,
    RF_RX_DONE,
    RF_RX_TIMEOUT,
    RF_TX_DONE,
    RF_TX_TIMEOUT,
    RF_LEN_ERROR,
    RF_CHANNEL_EMPTY,
    RF_CHANNEL_ACTIVITY_DETECTED,
}tRFProcessReturnCodes;

extern T_XmodeCfg XmodeCfg;
extern T_RadioState RadioState;
extern uint8_t BroadcastAddr[4];
extern T_SX1278BCongfig SX1278BCongfig;

void ScanDataConfig(void);
uint8_t GetRadioData(void);
void RadioPutProcess(void);
void RadioTaskProcess(void);
void SX1278ATaskProcess(void);
void SX1278BTaskProcess(void);
void SX1278ARssiBack(uint8_t *buffer);
bool SX1278AFeedback(uint8_t *buffer);
void InstallDataConfig(uint8_t *buffer);
uint8_t XmodeDataConfig(uint8_t *buffer);
void RadioGetProcess(uint8_t *buffer, uint8_t size);

void RF_LED_Blink(void);
void RadioRegister(void);
void RadioDeviceInit(void);
int RadioSetRxMode(char *pcName);
uint32_t RadioProcess(char *pcName);
int RegisterRadioDriver(PT_RadioDriver ptRadioDriver);
void RadioGetRxPacket(char *pcName, uint8_t *buffer, uint16_t* size);
void RadioSetTxPacket(char *pcName, const void *buffer, uint16_t size);


extern void SX1278BLoRaInit(void);
extern void SX1278ARead(uint8_t addr, uint8_t *data);
extern void USART2_SendData(uint8_t *chr, uint8_t num);
extern void NetComputeCRC(uint8_t *buffer, uint8_t length, uint8_t *pcrc);


#endif // __RADIO_H__
