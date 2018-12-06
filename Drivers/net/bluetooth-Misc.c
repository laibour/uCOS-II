#include "sys_config.h"
#include "bluetooth.h"
#include "bluetooth-Misc.h"

uint8_t BluetoothCfg(void)
{
	uint8_t ret;
	uint8_t result = BLUETOOTH_BUSY;
	static uint8_t BluetoothNum = 0;
	uint8_t BluetoothInitRely[] = "OK";
	uint8_t BluetoothInitData[][13] =
	{
		"AT",
        "AT+BAUD4",		// ²¨ÌØÂÊ
        "AT+NAMEST-06",	// name
		"AT+PIN1234",	// pin
	};
	
	BluetoothInitData[2][10] = ((ConParm.ID[0]>>4)&0x0F) + 0x30;
	BluetoothInitData[2][11] = (ConParm.ID[0]&0x0F) + 0x30;
	ret = BluetoothSendRec(BluetoothInitData[BluetoothNum], BluetoothInitRely, 1000, 3);
	if (ret == BLUETOOTH_DONE)
	{
		BluetoothNum++;
		if (BluetoothNum == 4)
		{
			BluetoothNum = 0;
			result = BLUETOOTH_DONE;
		}
	}
	else if (ret == BLUETOOTH_ERROR)
	{
		BluetoothNum = 0;
		result = BLUETOOTH_ERROR;
	}
	
	return result;
}
