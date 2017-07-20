
#ifndef __LED_DRV_H__
#define  __LED_DRV_H__

#include "stm32f10x.h"
#include "GlobalDef.h"


typedef enum
{
    LED_GREEN = 0,
	LED_YELLOW,
	LED_RED,
}E_LED_SELECT;

typedef enum
{
    LED_CLOSE = 0,
    LED_OPEN = 1,
}E_LED_STATE;

#define LED_GREEN_PORT         GPIOC
#define LED_GREEN_GPIO_Pin     GPIO_Pin_15
#define LED_GREEN_Set(val)    GPIO_WriteBit(LED_GREEN_PORT, LED_GREEN_GPIO_Pin,\
	(val == LED_OPEN)? Bit_SET: Bit_RESET)


#define LED_YELLOW_PORT        GPIOD
#define LED_YELLOW_GPIO_Pin    GPIO_Pin_0
#define LED_YELLOW_Set(val)       GPIO_WriteBit(LED_YELLOW_PORT, LED_YELLOW_GPIO_Pin,\
	(val == LED_OPEN)? Bit_SET: Bit_RESET)


#define LED_RED_PORT           GPIOD
#define LED_RED_GPIO_Pin       GPIO_Pin_1
#define LED_RED_Set(val)       GPIO_WriteBit(LED_RED_PORT, LED_RED_GPIO_Pin,\
	(val == LED_OPEN)? Bit_SET: Bit_RESET)

// ��Ӧ PM25 ��LED�ȼ�, 7��
typedef enum
{
   AQI_GOOD = 0,           // ��,      �̵�
   AQI_Moderate = 1,       // �е�,    �̻Ƶ�
   AQI_LightUnhealthy = 2, // �����Ⱦ, �Ƶ�
   AQI_MidUnhealthy = 3,   // �ж���Ⱦ, �Ƶ�
   AQI_VeryUnhealthy = 4,  // �ض���Ⱦ, ���
   AQI_JustInHell    = 5,  // �ս������,   �� �������
   AQI_HeavyInHell   = 6,  // ��ȵ���ģʽ, 3�ƿ���
   AQI_LEVEL_END,
}E_AQI_LEVEL;

void LED_AllSet(E_LED_STATE state);



void LED_Init(void);
void LED_Config(E_LED_SELECT led, E_LED_STATE state);

void LED_AqiIndicate(E_AQI_LEVEL level);
void LED_StopAqiIndicate(void);
void LED_Flash(uint16_t times);
void LEDTest(void);

#define LED_AllClose() LED_AllSet(LED_CLOSE)
#define LED_AllOpen()  LED_AllSet(LED_OPEN)


#endif

