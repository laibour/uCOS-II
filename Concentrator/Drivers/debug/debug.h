#ifndef __DEBUG_H
#define __DEBUG_H


#define DBG_PRINTF	printf			/* 开启打印宏 */
//#define DBG_PRINTF(...)			/* 关闭打印宏 */

#define DATA_PRINTF	DataPrint		/* 开启打印宏 */
//#define DATA_PRINTF(...)			/* 关闭打印宏 */

//#define DBG_PRINTF(format, ...)	printf(format, ##__VA_ARGS__)	/* 想打印时开启宏 */

#define	COM_BUFFER_SIZE			(80)
#define	GPRS_CONFIG_LINK_NUM	(30)  /* GPRS配置申请连接发送次数 */

extern uint8_t ComSendFlag;
extern uint8_t ComSendSize;
extern uint8_t ComSendBuffer[COM_BUFFER_SIZE];

void DebugInit(void);
void SX1278ARssiTest(void);
void DataPrint(uint8_t *data, uint8_t len);
void DebugSendData(uint8_t *chr, uint8_t num);

#endif
