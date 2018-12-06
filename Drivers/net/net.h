#ifndef __NET_H
#define __NET_H


#define	FETCH_PACKET_NUM		(6)
#define	NODE_PACKET_NUM			(30)
#define GPRS_REPORT_TIME		(15000)  /* 等待GPRS报告回复时间15s */

#define CRC_TYPE_IBM 			1
#define CRC_TYPE_CCITT 			0
// Polynomial = X^16 + X^15 + X^2 + 1
#define POLYNOMIAL_IBM 			0x8005
// Polynomial = X^16 + X^12 + X^5 + 1
#define POLYNOMIAL_CCITT 		0x1021
// Seeds
#define CRC_IBM_SEED 			0xFFFF
#define CRC_CCITT_SEED			0x1D0F

/* net send data */
typedef struct NetSendData
{
	uint8_t Len;
	uint8_t Data[255];
}T_NetSendData, *PT_NetSendData;


/*!
 * Network driver structure defining the different function pointers
 */
typedef struct NetDriver
{
	char *name;
    void (*Init)(void);
    int (*Connect)(void);
    void (*Disconnect)(void);
    void (*SendData)(uint8_t *buffer, uint8_t size);
    void (*RecvData)(uint8_t *buffer, uint8_t *size);
    void (*Heartbeat)(void);
	void (*Process)(void);
	struct NetDriver *ptNext;
}T_NetDriver, *PT_NetDriver;

extern uint8_t BroadcastAddr[4];

void NetRegister(void);
void NetDeviceInit(void);
void NetTaskProcess(void);
int NetConnect(char *pcName);
void NetProcess(char *pcName);
bool NetDataCheck(uint8_t *buffer, uint8_t len);
uint8_t NetRecvData(char *pcName, uint8_t *data, uint8_t *size);
bool NetCheckCRC(uint8_t *buffer, uint8_t length, uint16_t *pcrc);
void NetComputeCRC(uint8_t *buffer, uint8_t length, uint8_t *pcrc);

int RegisterNetDriver(PT_NetDriver ptNetDriver);
void NetSendPacket(char *pcName, uint8_t *buffer, uint8_t size);
void GPRSPutData(uint8_t *data, uint8_t size);


#endif

