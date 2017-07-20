
#ifndef __KEY_DRV_H__
#define  __KEY_DRV_H__

#include "stm32f10x.h"
#include "GlobalDef.h"



/****************************Ӳ������ begin  ****************************************/
#define KEY1_PORT      GPIOB
#define KEY1_GPIO_Pin  GPIO_Pin_0

#define KEY2_PORT      GPIOA
#define KEY2_GPIO_Pin  GPIO_Pin_6

#define KEY3_PORT      
#define KEY3_GPIO_Pin  



#define KEY1_INPUT   ((GPIO_ReadInputDataBit(KEY1_PORT, KEY1_GPIO_Pin)) << 1)
#define KEY2_INPUT   ((GPIO_ReadInputDataBit(KEY2_PORT, KEY2_GPIO_Pin)) << 2)
//#define KEY3_INPUT   ((GPIO_ReadInputDataBit(KEY3_PORT, KEY3_GPIO_Pin)) << 3)
/****************************Ӳ������ end ****************************************/

#define key_state_0 	0
#define key_state_1		1
#define key_state_2		2
#define key_state_3	    3
#define key_state_4		4	//�������˳����


//����״̬
#define N_key       0             //�޼� 
#define S_key       1             //����
#define L_key       2             //���� 

#define KEY1_S   0x0104   //0x0C: 0b0000 0100 (bit1 Ϊ 0)
#define KEY1_L   0x0204

#define KEY2_S   0x0102   //0x0A: 0b0000 0010 (bit2 Ϊ 0)
#define KEY2_L   0x0202



#define KEY_MASK    0x06  // ֻ��2������, bit2-bit1
#define NO_KEY      0x06  // 


#define KEY_INPUT  ((uint8_t)((KEY1_INPUT) |(KEY2_INPUT) ))


// ���ڰ�����
#define FUNC_KEY_S     KEY1_S
#define FUNC_KEY_L     KEY1_L

#define NEXT_KEY_S     KEY2_S
#define NEXT_KEY_L     KEY2_L




#define DISPLAY_CC      0   // ��ʾ PM2.5 PM10 Ũ��
#define DISPLAY_PC      1   // ��ʾ PM2.5 PM10 ������
#define DISPLAY_AQI_US  2   // ��ʾ AQI US ��׼
#define DISPLAY_AQI_CN  3   // ��ʾ AQI CN ��׼
#define DISPLAY_HCHO    4   // ��ʾ ��ȩŨ��

#define HCHO_DISPLAY_MASSFRACT   0  // ��ʾ��������, ug/m3
#define HCHO_DISPLAY_PPM         1  // ��ʾ�������, ppm


extern uint8_t is_rgb_on;
extern uint8_t is_debug_on; // ���Դ�ӡ����

//#if  (SENSOR_SELECT == HCHO_SENSOR)
extern void HCHO_DisplayInit(uint8_t display_mode);
//#else
extern void PM25_Display_Init(uint8_t display_mode);
//#endif

void key_gpio_init(void);
uint16_t key_scan(void);
void key_process(uint16_t keyval);
uint8_t key_get_cur_display_mode(void);

#endif

