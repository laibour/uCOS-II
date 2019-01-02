#ifndef __I2C_EE_H
#define __I2C_EE_H


#define I2C_Speed				200000
#define I2C_PageSize			128
#define I2C2_SLAVE_ADDRESS7		0xA0

#define EEPROM_BLOCK1_ADDRESS 	0xA0   /* BLOCK1 ADDRESS */
#define EEPROM_BLOCK2_ADDRESS 	0xA8   /* BLOCK2 ADDRESS */


#define EPROM_24AA1025_WP_ON()	GPIO_SetBits(GPIOC, GPIO_Pin_6)		// EEPROM写保护开
#define EPROM_24AA1025_WP_OFF()	GPIO_ResetBits(GPIOC, GPIO_Pin_6)	// EEPROM写保护关


extern T_MemoryDriver EEPROMDriver;


void EEPROMRegister(void);

#endif /* __I2C_EE_H */
