#ifndef __SYS_CONFIG_H
#define __SYS_CONFIG_H

/********************************************************************************/
// stm32头文件
#include "stm32f10x.h"
#include "stm32f10x_it.h"

// C库头文件
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <string.h>
#include "debug.h"


#define	FULL						(0xFF)
#define TICK_RATE_MS( ms )     		( ms )
#define GET_TICK_COUNT( )			(TickCounter)
#define SOFTWARE_VERSION			(0x20)
#define	HARDWARE_VERSION			(0x20)

/********************************************************************************/

// AD PORT
#define	AD_RCC				RCC_APB2Periph_GPIOC
#define	AD_PIN				GPIO_Pin_15
#define	AD_GPIO				GPIOC

// MCU LED
#define	MCU_LED_RCC			RCC_APB2Periph_GPIOA
#define	MCU_LED_PIN			GPIO_Pin_12
#define	MCU_LED_GPIO		GPIOA
#define MCU_LED_ON()		GPIO_ResetBits(MCU_LED_GPIO, MCU_LED_PIN)
#define MCU_LED_OFF()		GPIO_SetBits(MCU_LED_GPIO, MCU_LED_PIN)
#define MCU_LED_TOGGLE()	GPIO_PinReverse(MCU_LED_GPIO, MCU_LED_PIN)

// GPRS LED
#define	GPRS_LED_RCC		RCC_APB2Periph_GPIOA
#define	GPRS_LED_PIN		GPIO_Pin_8
#define	GPRS_LED_GPIO		GPIOA
#define GPRS_LED_ON()		GPIO_ResetBits(GPRS_LED_GPIO, GPRS_LED_PIN)
#define GPRS_LED_OFF()		GPIO_SetBits(GPRS_LED_GPIO, GPRS_LED_PIN)

// RF1 LED
#define	RF1_LED_RCC			RCC_APB2Periph_GPIOA
#define	RF1_LED_PIN			GPIO_Pin_11
#define	RF1_LED_GPIO		GPIOA
#define RF1_LED_ON()		GPIO_ResetBits(RF1_LED_GPIO, RF1_LED_PIN)
#define RF1_LED_OFF()		GPIO_SetBits(RF1_LED_GPIO, RF1_LED_PIN)

// RF2 LED
#define	RF2_LED_RCC			RCC_APB2Periph_GPIOC
#define	RF2_LED_PIN			GPIO_Pin_9
#define	RF2_LED_GPIO		GPIOC
#define RF2_LED_ON()		GPIO_ResetBits(RF2_LED_GPIO, RF2_LED_PIN)
#define RF2_LED_OFF()		GPIO_SetBits(RF2_LED_GPIO, RF2_LED_PIN)

// RF DOWN LED
#define	DOWN_LED_RCC		RCC_APB2Periph_GPIOF
#define	DOWN_LED_PIN		GPIO_Pin_9
#define	DOWN_LED_GPIO		GPIOF
#define DOWN_LED_ON()		GPIO_ResetBits(DOWN_LED_GPIO, DOWN_LED_PIN)
#define DOWN_LED_OFF()		GPIO_SetBits(DOWN_LED_GPIO, DOWN_LED_PIN)

// RF UP LED
#define	UP_LED_RCC			RCC_APB2Periph_GPIOF
#define	UP_LED_PIN			GPIO_Pin_10
#define	UP_LED_GPIO			GPIOF
#define UP_LED_ON()			GPIO_ResetBits(UP_LED_GPIO, UP_LED_PIN)
#define UP_LED_OFF()		GPIO_SetBits(UP_LED_GPIO, UP_LED_PIN)


extern uint32_t RELEASE_TIME;
extern volatile uint32_t TickCounter;

// 函数声明
void Soft_delay_us(u16 time);
void Soft_delay_ms(u16 time);
void sys_Configuration(void);
void RCC_Configuration(void);
void NVIC_Configuration(void);
void IWDG_Configuration(void);
void MCU_LED_Configuration(void);
void GPIO_PinReverse(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin);

#endif
