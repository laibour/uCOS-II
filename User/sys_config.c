#include "sys_config.h"
#include "debug.h"


uint32_t RELEASE_TIME = 0x20170809;

static void SysTickInit(void);
static void AD_Configuration(void);

void sys_Configuration(void)
{
    /* System Clocks Configuration */
	SystemInit();
	
	NVIC_Configuration();	 /* 设置NVIC中断分组2:2位抢占优先级，2位响应优先级 */
	
	/* 禁用JTAG功能 */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);      /* 开启AFIO时钟 */
//	GPIO_PinRemapConfig(GPIO_Remap_SWJ_JTAGDisable, ENABLE);  /* 使能SWD 禁用JTAG */
    
	AD_Configuration();		 /* 电压检测端口初始化 */
}

/* 独立看门狗，Time = (128/(40*10^3))*1250 = 4s */
void IWDG_Configuration(void)
{
	DBGMCU->CR |= 0x100;						   /* mask debug WDG */
	IWDG_WriteAccessCmd(IWDG_WriteAccess_Enable);  /* 允许写IWDG */
	IWDG_SetPrescaler(IWDG_Prescaler_128);		   /* 设置分频 */
	IWDG_SetReload(1250);						   /* 设置Reload */
	IWDG_ReloadCounter();						   /* 重载值 */
	IWDG_Enable();								   /* 使能IDWG */
}

/* AD采样配置 */
void AD_Configuration(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
    
	RCC_APB2PeriphClockCmd(AD_RCC, ENABLE);
    
	GPIO_InitStructure.GPIO_Pin   = AD_PIN;	 /* 管脚选择 */
	GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_IN_FLOATING;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(AD_GPIO, &GPIO_InitStructure);
}

// System tick (1ms)
volatile uint32_t TickCounter = 0;

void SysTickInit(void)
{
	/* Setup SysTick Timer for 1 us interrupts ( not too often to save power ) */
    if( SysTick_Config( SystemCoreClock / 1000 ) )
    {
        while (1);	/* Capture error */ 
    }
}

/******************NVIC_Configuration*****************************/
void NVIC_Configuration(void)
{ 
	/* Configure the NVIC Preemption Priority Bits */  
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);   //抢占优先级2，响应优先级2    
}

/*******************************************************************************
* Function Name  : GPIO_Configuration
* Description    : Configures the different GPIO ports. CAN TRX config； Test Pin；
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void MCU_LED_Configuration(void)
{
  	GPIO_InitTypeDef GPIO_InitStructure;
    
    RCC_APB2PeriphClockCmd(MCU_LED_RCC | RF1_LED_RCC | RF2_LED_RCC | DOWN_LED_RCC, ENABLE);   		// GPIO时钟使能
    
    GPIO_InitStructure.GPIO_Pin   = MCU_LED_PIN;		// 管脚选择
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;	// 管脚速率选择
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_Out_PP;	// 管脚功能配置
    GPIO_Init(MCU_LED_GPIO, &GPIO_InitStructure);		// 端口选择：如PA，PB口
    
	GPIO_InitStructure.GPIO_Pin   = RF1_LED_PIN;		// 管脚选择
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;	// 管脚速率选择
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_Out_PP;	// 管脚功能配置
    GPIO_Init(RF1_LED_GPIO, &GPIO_InitStructure);		// 端口选择：如PA，PB口
	
	GPIO_InitStructure.GPIO_Pin   = RF2_LED_PIN;		// 管脚选择
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;	// 管脚速率选择
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_Out_PP;	// 管脚功能配置
    GPIO_Init(RF2_LED_GPIO, &GPIO_InitStructure);		// 端口选择：如PA，PB口
	
	GPIO_InitStructure.GPIO_Pin   = DOWN_LED_PIN;		// 管脚选择
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;	// 管脚速率选择
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_Out_PP;	// 管脚功能配置
    GPIO_Init(DOWN_LED_GPIO, &GPIO_InitStructure);		// 端口选择：如PA，PB口
	
	GPIO_InitStructure.GPIO_Pin   = UP_LED_PIN;			// 管脚选择
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;	// 管脚速率选择
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_Out_PP;	// 管脚功能配置
    GPIO_Init(UP_LED_GPIO, &GPIO_InitStructure);		// 端口选择：如PA，PB口
	
	RF1_LED_OFF();
	MCU_LED_ON();
	DOWN_LED_OFF();
	RF2_LED_OFF();
	UP_LED_OFF();
}

void GPIO_PinReverse(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin)
{
  /* Check the parameters */
  assert_param(IS_GPIO_ALL_PERIPH(GPIOx));
  assert_param(IS_GPIO_PIN(GPIO_Pin));
  
  GPIOx->ODR ^=  GPIO_Pin;
}

// 软件延时函数，us级别
void Soft_delay_us(u16 time)
{    
   u32 i;
   
   while(time--)
   {
      i = 8;		// 自己定义
      while(i--){};
        __nop();
        __nop();
        __nop();
        __nop();
   }
}

// 软件延时函数，ms级别
void Soft_delay_ms(u16 time)
{
   u16 i = 0;
   
   while(time--)
   {
      i = 7950;		// 自定义
      while(i--);    
   }
}

/****************************************************************************************************************
** 函数名称:	void RCC_Configuration(void)
** 功能描述:	系统时钟初始化配置
** 输入参数:	无
** 返 回 值:	无
** 备    注:	RCC_HSICmd(ENABLE);	//使能内部高速晶振 ;
* 	       		RCC_SYSCLKConfig(RCC_SYSCLKSource_HSI);//选择内部高速时钟作为系统时钟SYSCLOCK=8MHZ	
*	       		RCC_HCLKConfig(RCC_SYSCLK_Div1);       //选择HCLK时钟源为系统时钟SYYSCLOCK
*  	       		RCC_PCLK1Config(RCC_HCLK_Div4);        //APB1时钟为2M 
*  	       		RCC_PCLK2Config(RCC_HCLK_Div4);        //APB2时钟为2M
*  	       		RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB , ENABLE);//使能APB2外设GPIOB时钟
****************************************************************************************************************/
void RCC_Configuration(void)
{
	/* RCC system reset(for debug purpose) */
  	RCC_DeInit();

  	/* Enable HSE */
  	RCC_HSEConfig(RCC_HSE_ON);

  	/* Wait till HSE is ready */
  	while (RCC_GetFlagStatus(RCC_FLAG_HSERDY) == RESET);

    /* Enable Prefetch Buffer */
    FLASH_PrefetchBufferCmd(FLASH_PrefetchBuffer_Enable);

    /* Flash 2 wait state */
    FLASH_SetLatency(FLASH_Latency_2);
 
    /* HCLK = SYSCLK */
    RCC_HCLKConfig(RCC_SYSCLK_Div1);
  
    /* PCLK2 = HCLK */
    RCC_PCLK2Config(RCC_HCLK_Div1);

    /* PCLK1 = HCLK/2 */
    RCC_PCLK1Config(RCC_HCLK_Div2);

    /* PLLCLK = 8MHz * 9 = 72 MHz */
    RCC_PLLConfig(RCC_PLLSource_HSE_Div1, RCC_PLLMul_9);

    /* Enable PLL */ 
    RCC_PLLCmd(ENABLE);

    /* Wait till PLL is ready */
    while(RCC_GetFlagStatus(RCC_FLAG_PLLRDY) == RESET);

    /* Select PLL as system clock source */
    RCC_SYSCLKConfig(RCC_SYSCLKSource_PLLCLK);

    /* Wait till PLL is used as system clock source */
    while(RCC_GetSYSCLKSource() != 0x08);
}
