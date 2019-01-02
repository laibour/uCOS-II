#ifndef __I2C_RTC_H
#define __I2C_RTC_H


#define PCF8563_ADDRESS		0xA2   /* PCF8563 ADDRESS */


#pragma pack(1)
typedef struct _RTCTime	// 定义系统时钟结构体
{
   uint8_t second;
   uint8_t minute;
   uint8_t hour;
   uint8_t day;
   uint8_t week;
   uint8_t month;
   uint8_t year;
} T_RTCTime, *PT_RTCTime;
#pragma pack()


void RTCInit(void);
void RTCRead(PT_SystemTime ptSystemTime);
void RTCWrite(PT_SystemTime ptSystemTime);
void I2C_PCF8563_BufferRead(uint8_t* pBuffer, uint8_t ReadAddr, uint8_t NumByteToRead);
void I2C_PCF8563_BufferWrite(uint8_t* pBuffer, uint8_t WriteAddr, uint8_t NumByteToWrite);


#endif

