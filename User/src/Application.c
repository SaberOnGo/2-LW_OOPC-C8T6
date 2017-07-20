
#include "stm32f10x.h"
#include "TimerManager.h"
#include "os_timer.h"
#include "os_global.h"
#include "Uart_drv.h"
#include "uart_queue.h"
#include "LCD1602_Drv.h"
#include "delay.h"
#include "PM25Sensor.h"
#include "Key_Drv.h"
#include "LED_Drv.h"
#include "bsp.h"
#include "SDRR.h"


/*******************************************************************************
* Function Name  : RCC_Configuration
* Description    : ���ò�ͬ��ϵͳʱ��
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
static void SysClockConfig(void)
{
        /* RCC system reset(for debug purpose) */
        RCC_DeInit();

#ifdef HSIENABLE                //��ʹ��HSI�����ڲ�ʱ����Ϊϵͳʱ��ʱ
        RCC_HSICmd(ENABLE);        //���ڲ�����ʱ��
        //�ȴ�HSI׼����
        while(RCC_GetFlagStatus(RCC_FLAG_HSIRDY) == RESET);

        FLASH_PrefetchBufferCmd(FLASH_PrefetchBuffer_Enable);        //����FLASHԤȡָ����
        //FLASHʱ�����
        //�Ƽ�ֵ:SYSCLK = 0~24MHz   Latency=0
        //       SYSCLK = 24~48MHz  Latency=1
        //       SYSCLK = 48~72MHz  Latency=2
        FLASH_SetLatency(FLASH_Latency_2);
        RCC_HCLKConfig(RCC_SYSCLK_Div1);        //����HCLK(AHBʱ��)=SYSCLK
        RCC_PCLK2Config(RCC_HCLK_Div1);                //PCLK2(APB2) = HCLK
        RCC_PCLK1Config(RCC_HCLK_Div2);                //PCLK1(APB1) = HCLK = 24 MHz

        //PLL���� SYSCLK: HSI / 2 * 12 = 4*12 = 48MHz
        RCC_PLLConfig(RCC_PLLSource_HSI_Div2, RCC_PLLMul_12);
        //����PLL
        RCC_PLLCmd(ENABLE);//���PLL������ϵͳʱ��,���ܱ�DISABLE
        //�ȴ�PLL�ȶ�
        while(RCC_GetFlagStatus(RCC_FLAG_PLLRDY) == RESET){;}

        //����ϵͳʱ��SYSCLK = PLL���
        RCC_SYSCLKConfig(RCC_SYSCLKSource_PLLCLK);

        //�ȴ�PLL�ɹ�������ϵͳʱ�ӵ�ʱ��Դ,���ȴ��ȶ�
        // 0x00:HSI��Ϊϵͳʱ��
        // 0x04:HSE��Ϊϵͳʱ��
        // 0x08:PLL��Ϊϵͳʱ��
        while(RCC_GetSYSCLKSource() != 0x08);
		
		SystemCoreClock = CPU_CLOCK;
#else
        /* Enable HSE */
        RCC_HSEConfig(RCC_HSE_ON);

        /* Wait till HSE is ready */
        HSEStartUpStatus = RCC_WaitForHSEStartUp();

        if(HSEStartUpStatus == SUCCESS)
        {
                /* HCLK = SYSCLK */
                RCC_HCLKConfig(RCC_SYSCLK_Div1);
                /* PCLK2 = HCLK */
                RCC_PCLK2Config(RCC_HCLK_Div1);
                /* PCLK1 = HCLK/2 */
                RCC_PCLK1Config(RCC_HCLK_Div2);
                /* Flash 2 wait state */
                FLASH_SetLatency(FLASH_Latency_2);
                /* Enable Prefetch Buffer */
                FLASH_PrefetchBufferCmd(FLASH_PrefetchBuffer_Enable);
                /* PLLCLK = 8MHz * 6 = 48 MHz */
                RCC_PLLConfig(RCC_PLLSource_HSE_Div1, RCC_PLLMul_6);
                /* Enable PLL */
                RCC_PLLCmd(ENABLE);
                /* Wait till PLL is ready */
                while(RCC_GetFlagStatus(RCC_FLAG_PLLRDY) == RESET)
                {
                }
                /* Select PLL as system clock source */
                RCC_SYSCLKConfig(RCC_SYSCLKSource_PLLCLK);
                /* Wait till PLL is used as system clock source */
                while(RCC_GetSYSCLKSource() != 0x08);
        }
#endif
        /* Enable DMA1 clock */
       // RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);
        /* Enable ADC1E clock */
        //RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);        
        /* TIM4 clock source enable */
       // RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4, ENABLE);
        /* Enable GPIOA, GPIOB, GPIOC, GPIOD and AFIO clocks */
       // RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB | RCC_APB2Periph_GPIOC | RCC_APB2Periph_GPIOD | RCC_APB2Periph_AFIO, ENABLE);
}

/*******************************************************************************
* Function Name  : Set_USBClock
* Description    : Configures USB Clock input (48MHz)
* Input          : None.
* Return         : None.
*******************************************************************************/
//static 
void Set_USBClock(void)
{
    /* USBCLK = PLLCLK  = 48 MHz */
	RCC_USBCLKConfig(RCC_USBCLKSource_PLLCLK_Div1);

    /* Enable USB clock */
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USB, ENABLE);
}

// ����ʱ�ӳ�ʼ��
static void RCC_PeriphInit(void)
{
    RCC_APB2PeriphClockCmd(  RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB | RCC_APB2Periph_GPIOC
		                      | RCC_APB2Periph_GPIOD | RCC_APB2Periph_AFIO, ENABLE);

	#if 0
    GPIO_PinRemapConfig(GPIO_Remap_SWJ_JTAGDisable, ENABLE);  // ��ֹJTAG
    GPIO_PinRemapConfig(GPIO_Remap_PD01, ENABLE);  // ʹ��PD0 PD1
    #else
    SET_REG_32_BIT(AFIO->MAPR, AFIO_MAPR_PD01_REMAP | AFIO_MAPR_SWJ_CFG_JTAGDISABLE); 
	#endif
}

/**
 * NVIC Configuration
 */
void NVIC_Configuration(void)
{
#if  defined(VECT_TAB_RAM)
    // Set the Vector Table base location at 0x20000000
    NVIC_SetVectorTable(NVIC_VectTab_RAM, 0x00);
#else
    #if SYS_USING_BOOTLOADER
    NVIC_SetVectorTable(NVIC_VectTab_FLASH, APP_VECTOR_OFFSET);
    #else
	 /* Set the Vector Table base location at 0x08000000 */
    NVIC_SetVectorTable(NVIC_VectTab_FLASH, 0x0);
	#endif
#endif
}

#include "sfud_demo.h"
#include "UsbApp.h"
#include "FatFs_Demo.h"
#include "ff.h"
#include "DHT11.h"

void AppInit(void)
{
   RCC_ClocksTypeDef rcc_clocks;

   GLOBAL_DISABLE_IRQ();
   NVIC_Configuration();
   
   SysClockConfig();  
   SysTick_Init();
   RCC_PeriphInit();


   Uart_Q_Init();
  // USART2_Init(115200);
   
   //USART1_Init(9600);
   USART3_Init(9600);  // HCHO ���մ���
  
  
   GLOBAL_ENABLE_IRQ();
   
   BSP_Init(); 
  

   #if 1
   RCC_GetClocksFreq(&rcc_clocks);  // ��ȡϵͳʱ��
   os_printf("SysClk = %d, HCLK = %d, PCLK1 = %d, PCLK2 = %d, ADCLK = %d MHz\r\n", rcc_clocks.SYSCLK_Frequency / FREQ_1MHz,
   	               rcc_clocks.HCLK_Frequency / FREQ_1MHz, rcc_clocks.PCLK1_Frequency / FREQ_1MHz, 
   	               rcc_clocks.PCLK2_Frequency / FREQ_1MHz, rcc_clocks.ADCCLK_Frequency / FREQ_1MHz);
   os_printf("app version: %s %s %s\r\n", SOFT_VERSION, __DATE__, __TIME__);
   #endif

   


   
   
   
   //usb_main();

  
   //FatFs_Demo();
   //ITempSensor_Init();
   //PM25_Init();
   //SDRR_Init();
   //DHT_Init();
   //LCD1602_Init();
   //key_gpio_init();
   //LED_Init();
   //LED_Flash(8);
   
   //LCD1602_WriteString(0, 0, "sensor init...");
   //LCD1602_WriteString(1, 0, "please wait...");
}

