#include "sys_config.h"
#include "ucos_ii.h"

/* Global var */
static OS_EVENT *App_UserIFMbox;
#define APP_TASK_START_PRIO         3
#define APP_TASK_UART_PRIO          4
#define APP_TASK_LED_PRIO       	12

#define APP_TASK_START_STK_SIZE     512
#define APP_TASK_UART_STK_SIZE   	256
#define APP_TASK_LED_STK_SIZE       128

//#define OS_TASK_TMR_PRIO            (OS_LOWEST_PRIO - 2)

static OS_STK App_TaskStartStk[APP_TASK_START_STK_SIZE];
static OS_STK App_TaskUartStk[APP_TASK_UART_STK_SIZE];
static OS_STK App_TaskLedStk[APP_TASK_LED_STK_SIZE];

static void App_TaskCreate(void);
static void App_EventCreate(void);
static void App_TaskStart(void *p_arg);

int main(void)
{
    uint8_t os_err;
    
    OSInit();
	
    /* Create the start task */
	os_err = OSTaskCreateExt((void (*)(void *)) App_TaskStart,
             (void *)0,
             (OS_STK *)&App_TaskStartStk[APP_TASK_START_STK_SIZE - 1],
             (INT8U)APP_TASK_START_PRIO,
             (INT16U)APP_TASK_START_PRIO,
             (OS_STK *)&App_TaskStartStk[0],
             (INT32U)APP_TASK_START_STK_SIZE,
             (void *)0,
             (INT16U)(OS_TASK_OPT_STK_CLR | OS_TASK_OPT_STK_CHK));
             
    #if (OS_TASK_NAME_SIZE >= 11)
		OSTaskNameSet(APP_TASK_START_PRIO, (uint8_t *)"Start Task", &os_err);
    #endif
	
	OSStart(); /* Start multitasking (i.e. give control to uC/OS-II). */
}

/****************************************************************************************************************
** 函数名称:	main
** 功能描述:	主函数
** 输入参数:	无
** 返 回 值:	无
** 备    注:	无
****************************************************************************************************************/
static void App_TaskStart(void *p_arg)
{
	sys_Configuration();
	
	OS_CPU_SysTickInit();
	
	//App_EventCreate();
	
	App_TaskCreate();
    
	while(1)
	{
		OSTimeDlyHMSM(0, 0, 0, 50);
	}
}

void App_TaskUart(void)
{
	DebugInit();
	
	while(1)
	{
		DBG_PRINTF("The led is blink. \n");
		
		OSTimeDlyHMSM(0, 0, 0, 500);
	}
}

void App_TaskLed(void)
{
	MCU_LED_Configuration();
	
	while(1)
	{
		MCU_LED_TOGGLE();
		
		OSTimeDly(500);
	}
}

void App_TaskCreate(void)
{
	uint8_t os_err;
	
	/* Create the start task */
	os_err = OSTaskCreateExt((void (*)(void *)) App_TaskUart,
             (void *)0,
             (OS_STK *)&App_TaskUartStk[APP_TASK_UART_STK_SIZE - 1],
             (INT8U)APP_TASK_UART_PRIO,
             (INT16U)APP_TASK_UART_PRIO,
             (OS_STK *)&App_TaskStartStk[0],
             (INT32U)APP_TASK_UART_STK_SIZE,
             (void *)0,
             (INT16U)(OS_TASK_OPT_STK_CLR | OS_TASK_OPT_STK_CHK));
			 
	#if (OS_TASK_NAME_SIZE >= 9)
		OSTaskNameSet(APP_TASK_UART_PRIO, (uint8_t *)"User Uart", &os_err);
	#endif

    os_err = OSTaskCreateExt((void (*)(void *)) App_TaskLed,
             (void *)0,
             (OS_STK *)&App_TaskLedStk[APP_TASK_LED_STK_SIZE - 1],
             (INT8U)APP_TASK_LED_PRIO,
             (INT16U)APP_TASK_LED_PRIO,
             (OS_STK *)&App_TaskLedStk[0],
             (INT32U)APP_TASK_LED_STK_SIZE,
             (void *)0,
             (INT16U)(OS_TASK_OPT_STK_CLR | OS_TASK_OPT_STK_CHK));
             
    #if (OS_TASK_NAME_SIZE >= 9)
		OSTaskNameSet(APP_TASK_LED_PRIO, (uint8_t *)"User Led", &os_err);
	#endif
}

int fputc(int ch, FILE *f)
{
	UART4->SR;
	
	USART_SendData(UART4, (uint8_t)ch);							// 发送一个字符
	
	while(USART_GetFlagStatus(UART4, USART_FLAG_TC) == RESET)	// 等待发送完成
	{}
	
	return ch;
}
