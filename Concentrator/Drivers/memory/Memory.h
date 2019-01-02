#ifndef __MEMORY_H
#define __MEMORY_H

/* memory driver structure defining the different function pointers */
typedef struct MemoryDriver
{
	char *name;
    void (*Init)(void);
	void (*BufferRead)(uint8_t* pBuffer, uint32_t ReadAddr, uint16_t NumByteToRead);
    void (*BufferWrite)(uint8_t* pBuffer, uint32_t WriteAddr, uint16_t NumByteToWrite);
	void (*AllBufferReset)(void);
	struct MemoryDriver *ptNext;
}T_MemoryDriver, *PT_MemoryDriver;


extern volatile uint8_t I2C_DEVICE_ADDR;

void MemoryRegister(void);
void MemoryDeviceInit(void);
void AllMemoryBufferReset(char *pcName);
void RegisterMemoryDriver(PT_MemoryDriver ptMemoryDriver);
void MemoryBufferRead(char *pcName, uint8_t* pBuffer, uint32_t ReadAddr, uint16_t NumByteToRead);
void MemoryBufferWrite(char *pcName, uint8_t* pBuffer, uint32_t WriteAddr, uint16_t NumByteToWrite);


#endif
