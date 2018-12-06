#include "sys_config.h"
#include "memory.h"
#include "i2c_ee.h"


/* Private functions */
static void EEPROMInit(void);
static void I2C_Configuration(void);
static void GPIO_Configuration(void);
static void I2C_EE_WaitEepromStandbyState(void);
//static void I2C_EE_ByteWrite(uint8_t* pBuffer, uint16_t WriteAddr);
static void I2C_EE_PageWrite(uint8_t* pBuffer, uint16_t WriteAddr, uint8_t NumByteToWrite);
static void I2C_EE_BufferWrite(uint8_t* pBuffer, uint16_t WriteAddr, uint16_t NumByteToWrite);
static void I2C_EE_BufferRead(uint8_t* pBuffer, uint16_t ReadAddr, uint16_t NumByteToRead);
static void EEPROMBufferRead(uint8_t* pBuffer, uint32_t ReadAddr, uint16_t NumByteToRead);
static void EEPROMBufferWrite(uint8_t* pBuffer, uint32_t WriteAddr, uint16_t NumByteToWrite);

T_MemoryDriver EEPROMDriver = {
	.name	     = "EEPROM",
	.Init		 = EEPROMInit,
	.BufferRead  = EEPROMBufferRead,
    .BufferWrite = EEPROMBufferWrite,
};

void EEPROMInit(void)
{
	/* GPIO configuration */
	GPIO_Configuration();
	
	/* I2C configuration */
	I2C_Configuration();
	
	/* Select the EEPROM Block1 */
	I2C_DEVICE_ADDR = EEPROM_BLOCK1_ADDRESS;
	
//	EEPROMTest();
}


#if 0
void EEPROMTest(void)
{
	uint8_t EEPROMBuffer[255];
	
	memset(EEPROMBuffer, 0xAA, sizeof(EEPROMBuffer));
	
	EEPROMBufferWrite(EEPROMBuffer, 0x16FF, sizeof(EEPROMBuffer));
	
	memset(EEPROMBuffer, 0, sizeof(EEPROMBuffer));
	
	Soft_delay_ms(10);
	
	EEPROMBufferRead(EEPROMBuffer, 0x16FF, sizeof(EEPROMBuffer));
}
#endif


void EEPROMRegister(void)
{
	RegisterMemoryDriver(&EEPROMDriver);
}


void EEPROMBufferRead(uint8_t* pBuffer, uint32_t ReadAddr, uint16_t NumByteToRead)
{
	if (ReadAddr < 0x10000)
	{
		I2C_DEVICE_ADDR = EEPROM_BLOCK1_ADDRESS;
	}
	else if (ReadAddr>=0x10000 && ReadAddr<0x20000)
	{
		I2C_DEVICE_ADDR = EEPROM_BLOCK2_ADDRESS;
	}
	else	// invalid addr, return
	{
		DBG_PRINTF("EEPROM write address is invalid!\n");
		return;
	}
	
	I2C_EE_BufferRead(pBuffer, (uint16_t)ReadAddr, NumByteToRead);
}


void EEPROMBufferWrite(uint8_t* pBuffer, uint32_t WriteAddr, uint16_t NumByteToWrite)
{
	if (WriteAddr < 0x10000)
	{
		I2C_DEVICE_ADDR = EEPROM_BLOCK1_ADDRESS;
	}
	else if (WriteAddr>=0x10000 && WriteAddr<0x20000)
	{
		I2C_DEVICE_ADDR = EEPROM_BLOCK2_ADDRESS;
	}
	else	// invalid addr, return
	{
		DBG_PRINTF("EEPROM write address is invalid!\n");
		return;
	}
	
	I2C_EE_BufferWrite(pBuffer, (uint16_t)WriteAddr, NumByteToWrite);
}


/**
  * @brief  Configure the used I/O ports pin
  * @param  None
  * @retval : None
  */
void GPIO_Configuration(void)
{
	GPIO_InitTypeDef  GPIO_InitStructure; 
	
	/* I2C2 and GPIOB and GPIOC Periph clock enable */
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C2, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB | RCC_APB2Periph_GPIOC, ENABLE);
	
	/* Configure I2C1 pins: SCL and SDA */
	GPIO_InitStructure.GPIO_Pin =  GPIO_Pin_10 | GPIO_Pin_11;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_OD;
	GPIO_Init(GPIOB, &GPIO_InitStructure);
}

/**
  * @brief  I2C Configuration
  * @param  None
  * @retval : None
  */
void I2C_Configuration(void)
{
	I2C_InitTypeDef  I2C_InitStructure; 
	
	/* I2C configuration */
	I2C_InitStructure.I2C_Mode = I2C_Mode_I2C;
	I2C_InitStructure.I2C_DutyCycle = I2C_DutyCycle_2;
	I2C_InitStructure.I2C_OwnAddress1 = I2C2_SLAVE_ADDRESS7;
	I2C_InitStructure.I2C_Ack = I2C_Ack_Enable;
	I2C_InitStructure.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;
	I2C_InitStructure.I2C_ClockSpeed = I2C_Speed;
  
	/* I2C Peripheral Enable */
	I2C_Cmd(I2C2, ENABLE);
	
	/* Apply I2C configuration after enabling it */
	I2C_Init(I2C2, &I2C_InitStructure);
}


/**
  * @brief  Writes buffer of data to the I2C EEPROM.
  * @param pBuffer : pointer to the buffer  containing the data to be 
  *   written to the EEPROM.
  * @param WriteAddr : EEPROM's internal address to write to.
  * @param NumByteToWrite : number of bytes to write to the EEPROM.
  * @retval : None
  */
void I2C_EE_BufferWrite(uint8_t* pBuffer, uint16_t WriteAddr, uint16_t NumByteToWrite)
{
  uint16_t NumOfPage = 0;
  uint8_t  NumOfSingle = 0, Addr = 0, count = 0;

  if (NumByteToWrite == 0)
  {
     return;	//增加写入个数为0个时的返回
  }
  Addr = WriteAddr % I2C_PageSize;
  count = I2C_PageSize - Addr;
  NumOfPage = NumByteToWrite / I2C_PageSize;
  NumOfSingle = NumByteToWrite % I2C_PageSize;
 
  /* If WriteAddr is I2C_PageSize aligned  */
  if (Addr == 0)
  {
    if (NumOfPage == 0)	/* If NumByteToWrite < I2C_PageSize */
    {
      I2C_EE_PageWrite(pBuffer, WriteAddr, NumOfSingle);
      I2C_EE_WaitEepromStandbyState();
    }
    else  /* If NumByteToWrite > I2C_PageSize */
    {
      while (NumOfPage--)
      {
        I2C_EE_PageWrite(pBuffer, WriteAddr, I2C_PageSize); 
    	I2C_EE_WaitEepromStandbyState();
        WriteAddr += I2C_PageSize;
        pBuffer   += I2C_PageSize;
      }

      if (NumOfSingle != 0)
      {
        I2C_EE_PageWrite(pBuffer, WriteAddr, NumOfSingle);
        I2C_EE_WaitEepromStandbyState();
      }
    }
  }
  else	/* If WriteAddr is not I2C_PageSize aligned  */
  {
    if (NumOfPage == 0)	/* If NumByteToWrite < I2C_PageSize */
    {
		if(NumOfSingle > count)
		{
			I2C_EE_PageWrite(pBuffer, WriteAddr, count);
			I2C_EE_WaitEepromStandbyState();
			WriteAddr += count;
			pBuffer += count;
			I2C_EE_PageWrite(pBuffer, WriteAddr, NumOfSingle-count);
			I2C_EE_WaitEepromStandbyState();
		}
		else
		{
			I2C_EE_PageWrite(pBuffer, WriteAddr, NumOfSingle);
			I2C_EE_WaitEepromStandbyState();
		}
		
		#if 0
		I2C_EE_PageWrite(pBuffer, WriteAddr, NumOfSingle);
		I2C_EE_WaitEepromStandbyState();
		#endif
    }
    else  /* If NumByteToWrite > I2C_PageSize */
    {
      NumByteToWrite -= count;
      NumOfPage   = NumByteToWrite/I2C_PageSize;
      NumOfSingle = NumByteToWrite%I2C_PageSize;	
      
      if (count != 0)
      {  
        I2C_EE_PageWrite(pBuffer, WriteAddr, count);
        I2C_EE_WaitEepromStandbyState();
        WriteAddr += count;
        pBuffer += count;
      }
      
      while(NumOfPage--)
      {
        I2C_EE_PageWrite(pBuffer, WriteAddr, I2C_PageSize);
        I2C_EE_WaitEepromStandbyState();
        WriteAddr +=  I2C_PageSize;
        pBuffer += I2C_PageSize;  
      }
	  
      if(NumOfSingle != 0)
      {
        I2C_EE_PageWrite(pBuffer, WriteAddr, NumOfSingle); 
        I2C_EE_WaitEepromStandbyState();
      }
    }
  }  
}

/**
  * @brief  Writes one byte to the I2C EEPROM.
  * @param pBuffer : pointer to the buffer  containing the data to be 
  *   written to the EEPROM.
  * @param WriteAddr : EEPROM's internal address to write to.
  * @retval : None
  */
void I2C_EE_ByteWrite(uint8_t* pBuffer, uint16_t WriteAddr)
{
  /* Send STRAT condition */
  I2C_GenerateSTART(I2C2, ENABLE);

  /* Test on EV5 and clear it */
  while(!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_MODE_SELECT));  

  /* Send EEPROM address for write */
  I2C_Send7bitAddress(I2C2, I2C_DEVICE_ADDR, I2C_Direction_Transmitter);
  
  /* Test on EV6 and clear it */
  while(!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED));
      
  /* Send the EEPROM's internal MSB to write to */
  I2C_SendData(I2C2, (uint8_t)(WriteAddr>>8));
  
  /* Test on EV8 and clear it */
  while(!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_BYTE_TRANSMITTED));
  
  /* Send the EEPROM's internal LSB to write to */
  I2C_SendData(I2C2, (uint8_t)WriteAddr);
  
  /* Test on EV8 and clear it */
  while(!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_BYTE_TRANSMITTED));
  
  /* Send the byte to be written */
  I2C_SendData(I2C2, *pBuffer); 
   
  /* Test on EV8 and clear it */
  while(!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_BYTE_TRANSMITTED));
  
  /* Send STOP condition */
  I2C_GenerateSTOP(I2C2, ENABLE);
}

/**
  * @brief  Writes more than one byte to the EEPROM with a single WRITE
  *   cycle. The number of byte can't exceed the EEPROM page size.
  * @param pBuffer : pointer to the buffer containing the data to be 
  *   written to the EEPROM.
  * @param WriteAddr : EEPROM's internal address to write to.
  * @param NumByteToWrite : number of bytes to write to the EEPROM.
  * @retval : None
  */
void I2C_EE_PageWrite(uint8_t* pBuffer, uint16_t WriteAddr, uint8_t NumByteToWrite)
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
  I2C_SendData(I2C2, (uint8_t)(WriteAddr>>8));  

  /* Test on EV8 and clear it */
  while(!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_BYTE_TRANSMITTED));  

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

/**
  * @brief  Reads a block of data from the EEPROM.
  * @param pBuffer : pointer to the buffer that receives the data read 
  *   from the EEPROM.
  * @param ReadAddr : EEPROM's internal address to read from.
  * @param NumByteToRead : number of bytes to read from the EEPROM.
  * @retval : None
  */
void I2C_EE_BufferRead(uint8_t* pBuffer, uint16_t ReadAddr, uint16_t NumByteToRead)
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
  I2C_SendData(I2C2, (uint8_t)(ReadAddr>>8));  

  /* Test on EV8 and clear it */
  while(!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_BYTE_TRANSMITTED));
  
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

/**
  * @brief  Wait for EEPROM Standby state
  * @param  None
  * @retval : None
  */
void I2C_EE_WaitEepromStandbyState(void)
{
  __IO uint16_t SR1_Tmp = 0;

  do
  {
    /* Send START condition */
    I2C_GenerateSTART(I2C2, ENABLE);
    /* Read I2C1 SR1 register */
    SR1_Tmp = I2C_ReadRegister(I2C2, I2C_Register_SR1);
    /* Send EEPROM address for write */
    I2C_Send7bitAddress(I2C2, I2C_DEVICE_ADDR, I2C_Direction_Transmitter);
  }while(!(I2C_ReadRegister(I2C2, I2C_Register_SR1) & 0x0002));
  
  /* Clear AF flag */
  I2C_ClearFlag(I2C2, I2C_FLAG_AF);
  
  /* STOP condition */    
  I2C_GenerateSTOP(I2C2, ENABLE);  
}
