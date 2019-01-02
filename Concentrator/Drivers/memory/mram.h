#ifndef __MRAM_H
#define __MRAM_H


#define STM32_EXT_SRAM_BEGIN	0x60000000 /* the begining address of external SRAM */
#define STM32_EXT_SRAM_END		0x60080000 /* the end address of external SRAM */


void MRAMRegister(void);
void MRAMWriteData(uint32_t WriteAddr, uint8_t data, uint16_t NumByteToWrite);


#endif
