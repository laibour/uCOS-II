#ifndef __GPRS_HAL_H
#define __GPRS_HAL_H


void GPRSIOInit(void);
void USART2_Init(void);
void USART2_SendString(uint8_t *ch);
void USART2_SendData(uint8_t *chr, uint8_t num);


#endif
