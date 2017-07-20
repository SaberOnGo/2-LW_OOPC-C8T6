
#include "Key_Drv.h"

static uint8_t cur_display_mode = DISPLAY_CC;  // ��ǰ��ʾģʽ

uint8_t is_rgb_on = 0;   // RGB �Ƿ��, 0: Ĭ�ϲ���; 1: ��
uint8_t is_debug_on = 0; // ���Դ�ӡ����

void key_gpio_init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
	
	GPIO_InitStructure.GPIO_Pin = KEY1_GPIO_Pin;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_Init(KEY1_PORT, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = KEY1_GPIO_Pin;
	GPIO_Init(KEY2_PORT, &GPIO_InitStructure);

	//GPIO_InitStructure.GPIO_Pin = KEY1_GPIO_Pin;
	//GPIO_Init(KEY3_PORT, &GPIO_InitStructure);
}

uint16_t key_scan(void)
{
	static uint8_t key_state = key_state_0;
	static uint8_t key_time = 0;
	//static uint8 key_time2 = 0;
	static uint8_t key_value1 = 0;
	//static uint8_t key_value2 = 0;
	uint8_t key_press = 0;
	uint8_t key_return = N_key;
	uint16_t key_result = 0x0000;	//��8λ��ʾ�Ƿ�S_key/L_key
	
	key_press = KEY_INPUT;	//��ȡ����IO��ƽ
	
	switch(key_state)
	{
			case key_state_0:	   // ������ʼ̬ 
				if(key_press != NO_KEY)	
				{	
					key_state = key_state_1;	//�������£�״̬ת��������������ȷ��״̬ 
					key_time = 0;
				}
			break;
			case key_state_1:	   // ����������ȷ��̬ 
			    if(++key_time >= 1)  // �ı��ж������ɸı����������
				{
					key_value1 = key_press;	//��¼���ĸ�����������
					if(key_press != NO_KEY)	//������Ȼ���ڰ���
					{
						key_time = 0;	//
						key_state = key_state_2; //������ɣ�״̬ת�������¼�ʱ��ļ�ʱ״̬�������صĻ����޼��¼�
					}
					else
						key_state = key_state_0;// ������̧��,ת����������ʼ̬,�˴���ɺ�ʵ�������������ʵ�����İ��º��ͷŶ��ڴ�������
				}	
			break;
			case key_state_2:
				//key_value2 = key_press;	//��¼���ĸ�����������
				
					//key_time2 = 0;
			    	if(key_press == NO_KEY) //�̰�������
					{
						key_return = S_key;	//��ʱ�����ͷţ�˵���ǲ���һ�ζ̲���������S_key
						key_result = (uint16_t)key_return;
						key_result = (key_result<<8)|(key_value1 & KEY_MASK);
						key_state = key_state_0;   // ת����������ʼ̬ 
					}
				
					else if(++key_time >= 200) // �������£���ʱ��10ms��10msΪ������ѭ��ִ�м��
					{
						key_return = L_key;	//���س�����״̬
						key_time = 0;
						key_result = (uint16_t)key_return;
						key_result = (key_result<<8)|(key_value1 & KEY_MASK);
						key_state = key_state_3;   // ת�����ȴ������ͷ�״̬
					}
					
					//key_return |= (key_value2 & KEY_MASK);	
				   // key_result = (key_result<<8)|(key_value2 & KEY_MASK);
					//��8λ��¼���ĸ�����,��1101001
				
			break;
			case key_state_3:
				if(key_press == NO_KEY)
					key_state = key_state_0;
				//else if(key_press == 0x2E)   // key 4������
				//{
				//    key_state = key_state_0;
				//}
			break;
	}
	return key_result;	//���ذ������
}

#include "PM25Sensor.h"
#include "LED_Drv.h"
void key_process(uint16_t key)
{ 
    uint8_t key_mode;
    key_mode = (uint8_t)((key & 0xFF00)>>8);	//��8λΪ����ģʽ


	if(key_mode != N_key)
	{
	   os_printf("key = 0x%x\n", key);
	   switch(key)
	   {
	       case FUNC_KEY_S:
		   {
              
			  is_rgb_on ^= 1;
			  if(is_rgb_on)LED_IndicateColorOfHCHO();
			  else{ LED_AllClose();  }
			  os_printf("FUNC KEY S, rgb = %d\n", is_rgb_on);
		   }break;
		   case FUNC_KEY_L:
		   {
              os_printf("FUNC KEY L\n");
			  is_debug_on ^= 1;
			  if(is_debug_on)
			  {
                 os_printf("-------debug on------\n");
			  }
			  os_printf("debug = %d\n", is_debug_on);
		   }break;
	       case NEXT_KEY_S:
		   {
		   	  os_printf("NEXT KEY S\n"); 
	       }break;
		   case NEXT_KEY_L:
		   {
		      os_printf("NEXT KEY L\n");
		   }break;
		  
		   default:
		   {
		   	   
		   }break;  	
	   }
	}
}

// ��ȡ��ǰ��ʾģʽ
uint8_t key_get_cur_display_mode(void)
{
   return cur_display_mode;
}

