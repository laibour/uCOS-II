#include "sys_config.h"
#include "i2c_rtc.h"
#include "memory.h"


void RTCInit(void)
{
	RTCWrite(&SystemTime);
	
//	RTCRead(&SystemTime);
}


void RTCWrite(PT_SystemTime ptSystemTime)
{
	T_RTCTime RTCTime;
	
	RTCTime.second = ptSystemTime->second;
	RTCTime.minute = ptSystemTime->minute;
	RTCTime.hour   = ptSystemTime->hour;
	RTCTime.day    = ptSystemTime->day;
	RTCTime.week   = 0x05;
	RTCTime.month  = ptSystemTime->month;
	RTCTime.year   = ptSystemTime->yearL;
	
	I2C_DEVICE_ADDR = PCF8563_ADDRESS;
	I2C_PCF8563_BufferWrite((uint8_t *)&RTCTime, 0x02, 7);
}


void RTCRead(PT_SystemTime ptSystemTime)
{
	T_RTCTime RTCTime;
	
	I2C_DEVICE_ADDR = PCF8563_ADDRESS;
	I2C_PCF8563_BufferRead((uint8_t *)&RTCTime, 0x02, 7);
	
	ptSystemTime->second = (RTCTime.second & 0x7F);
	ptSystemTime->minute = (RTCTime.minute & 0x7F);
	ptSystemTime->hour   = (RTCTime.hour   & 0x3F);
	ptSystemTime->day    = (RTCTime.day    & 0x3F);
	ptSystemTime->month  = (RTCTime.month  & 0x1F);
	ptSystemTime->yearL  = (RTCTime.year   & 0x7F);
	ptSystemTime->yearH  = 0x20;
}


void I2C_PCF8563_BufferWrite(uint8_t* pBuffer, uint8_t WriteAddr, uint8_t NumByteToWrite)
{
  /* While the bus is busy */
  while(I2C_GetFlagStatus(I2C2, I2C_FLAG_BUSY));
  
  /* Send START condition */
  I2C_GenerateSTART(I2C2, ENABLE);
  
  /* Test on EV5 and clear it */
  while(!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_MODE_SELECT)); 
  
  /* Send EEPROM address for write */
  I2C_Send7bitAddress(I2C2, I2C_DEVICE_ADDR, I2C_Direction_Transmitter);

  /* Test on EV6 and clear it */
  while(!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED));

  /* Send the EEPROM's internal address to write to */    
  I2C_SendData(I2C2, (uint8_t)WriteAddr);  

  /* Test on EV8 and clear it */
  while(!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_BYTE_TRANSMITTED));

  /* While there is data to be written */
  while(NumByteToWrite--)
  {
    /* Send the current byte */
    I2C_SendData(I2C2, *pBuffer);
    
    /* Point to the next byte to be written */
    pBuffer++; 
    
    /* Test on EV8 and clear it */
    while (!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_BYTE_TRANSMITTED));
  }
  
  /* Send STOP condition */
  I2C_GenerateSTOP(I2C2, ENABLE);
}


void I2C_PCF8563_BufferRead(uint8_t* pBuffer, uint8_t ReadAddr, uint8_t NumByteToRead)
{  
    /* While the bus is busy */
  while(I2C_GetFlagStatus(I2C2, I2C_FLAG_BUSY));
  
  /* Send START condition */
  I2C_GenerateSTART(I2C2, ENABLE);
  
  /* Test on EV5 and clear it */
  while(!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_MODE_SELECT));
   
  /* Send EEPROM address for write */
  I2C_Send7bitAddress(I2C2, I2C_DEVICE_ADDR, I2C_Direction_Transmitter);

  /* Test on EV6 and clear it */
  while(!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED));
  
  /* Clear EV6 by setting again the PE bit */
  I2C_Cmd(I2C2, ENABLE);
  
  /* Send the EEPROM's internal address to write to */
  I2C_SendData(I2C2, (uint8_t)ReadAddr);  
  
  /* Test on EV8 and clear it */
  while(!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_BYTE_TRANSMITTED));
  
  /* Send STRAT condition a second time */  
  I2C_GenerateSTART(I2C2, ENABLE);
  
  /* Test on EV5 and clear it */
  while(!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_MODE_SELECT));
  
  /* Send EEPROM address for read */
  I2C_Send7bitAddress(I2C2, I2C_DEVICE_ADDR, I2C_Direction_Receiver);
  
  /* Test on EV6 and clear it */
  while(!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED));
  
  /* While there is data to be read */
  while(NumByteToRead)  
  {
    if(NumByteToRead == 1)
    {
      /* Disable Acknowledgement */
      I2C_AcknowledgeConfig(I2C2, DISABLE);
      
      /* Send STOP Condition */
      I2C_GenerateSTOP(I2C2, ENABLE);
    }

    /* Test on EV7 and clear it */
    if(I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_BYTE_RECEIVED))  
    {      
      /* Read a byte from the EEPROM */
      *pBuffer = I2C_ReceiveData(I2C2);

      /* Point to the next location where the byte read will be saved */
      pBuffer++;
      
      /* Decrement the read bytes counter */
      NumByteToRead--;        
    }   
  }

  /* Enable Acknowledgement to be ready for another reception */
  I2C_AcknowledgeConfig(I2C2, ENABLE);
}
