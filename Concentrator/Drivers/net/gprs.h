#ifndef __GPRS_H
#define __GPRS_H


#define	GPRS_BUFFER_SIZE		(255)		/* GPRS buffer size */

/* GPRS¶Ë¿Ú³õÊ¼»¯ */
// GPRS POWER PORT
#define	GPRS_POWER_RCC			RCC_APB2Periph_GPIOG
#define	GPRS_POWER_PIN			GPIO_Pin_6
#define	GPRS_POWER_GPIO			GPIOG
#define GPRS_POWER_ON()			GPIO_SetBits(GPRS_POWER_GPIO, GPRS_POWER_PIN)
#define GPRS_POWER_OFF()		GPIO_ResetBits(GPRS_POWER_GPIO, GPRS_POWER_PIN)

// GPRS PWRKEY PORT
#define	GPRS_PWRKEY_RCC			RCC_APB2Periph_GPIOA
#define	GPRS_PWRKEY_PIN			GPIO_Pin_1
#define	GPRS_PWRKEY_GPIO		GPIOA
#define GPRS_PWRKEY_ON()		GPIO_SetBits(GPRS_PWRKEY_GPIO, GPRS_PWRKEY_PIN)
#define GPRS_PWRKEY_OFF()		GPIO_ResetBits(GPRS_PWRKEY_GPIO, GPRS_PWRKEY_PIN)

// GPRS DCD PORT
#define	GPRS_DCD_RCC			RCC_APB2Periph_GPIOC
#define	GPRS_DCD_PIN			GPIO_Pin_1
#define	GPRS_DCD_GPIO			GPIOC

// GPRS DTR PORT
#define	GPRS_DTR_RCC			RCC_APB2Periph_GPIOC
#define	GPRS_DTR_PIN			GPIO_Pin_2
#define	GPRS_DTR_GPIO			GPIOC
#define GPRS_DTR_ON()			GPIO_ResetBits(GPRS_DTR_GPIO, GPRS_DTR_PIN)
#define GPRS_DTR_OFF()			GPIO_SetBits(GPRS_DTR_GPIO, GPRS_DTR_PIN)


typedef enum
{
	GPRS_STATE_IDLE,
	GPRS_STATE_POWERON,
	GPRS_STATE_CFG,
	GRPS_STATE_OPEN,
	GPRS_STATE_RUNNING,
	GPRS_STATE_REGISTER,
	GPRS_STATE_EXIT,
	GPRS_STATE_FAULT,
	GPRS_STATE_SHIFT,
	GPRS_STATE_POWERDOWM,
}tGPRSStates;

typedef enum
{
	GPRS_POWER_IDLE,
	GPRS_POWER_INIT,
	GPRS_POWER_ON,
	GPRS_POWER_KEY,
	GPRS_POWER_AT,
	GPRS_POWER_WAIT,
	GPRS_POWER_DONE,
}tGPRSPowerStates;

typedef enum
{
	GPRS_IDLE,
    GPRS_BUSY,
    GPRS_DONE,
	GPRS_ERROR,
}tGPRSReturnCodes;


extern uint8_t GPRSOnline;
extern uint8_t GPRSPutFlag;
extern uint8_t GPRSGetFlag;
extern uint8_t GPRSRecvFlag;
extern uint8_t GPRSRecvSize;
extern uint8_t GPRSRecvBuffer[GPRS_BUFFER_SIZE];


int GPRSRegister(void);
void GPRSMultiPacketProcess(void);
uint8_t GPRSSendRec(uint8_t *str, uint8_t *find, uint16_t time, uint8_t num);


#endif
