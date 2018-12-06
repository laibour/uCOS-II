#ifndef __BLUETOOTH_H
#define __BLUETOOTH_H


#define	BLUE_BUFFER_SIZE		255		/* 蓝牙接收缓冲区大小 */


typedef enum
{
	BLUETOOTH_IDLE,
    BLUETOOTH_BUSY,
    BLUETOOTH_DONE,
	BLUETOOTH_ERROR,
}tBluetoothReturnCodes;


typedef enum
{
	BLUETOOTH_STATE_IDLE,
	BLUETOOTH_STATE_INIT,
	BLUETOOTH_STATE_POWER,
	BLUETOOTH_STATE_CFG,
	BLUETOOTH_STATE_RUNNING,
}tBluetoothStates;


extern uint8_t BlueRecvBuffer[BLUE_BUFFER_SIZE];


int BluetoothRegister(void);
void BlueMultiPacketProcess(void);
uint8_t BluetoothSendRec(uint8_t *str, uint8_t *find, uint16_t time, uint8_t num);


#endif

