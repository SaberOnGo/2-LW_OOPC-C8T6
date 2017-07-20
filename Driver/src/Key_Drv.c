
#include "Key_Drv.h"

static uint8_t cur_display_mode = DISPLAY_CC;  // 当前显示模式

uint8_t is_rgb_on = 0;   // RGB 是否打开, 0: 默认不打开; 1: 打开
uint8_t is_debug_on = 0; // 调试打印开关

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
	uint16_t key_result = 0x0000;	//高8位表示是否S_key/L_key
	
	key_press = KEY_INPUT;	//读取按键IO电平
	
	switch(key_state)
	{
			case key_state_0:	   // 按键初始态 
				if(key_press != NO_KEY)	
				{	
					key_state = key_state_1;	//键被按下，状态转换到按键消抖和确认状态 
					key_time = 0;
				}
			break;
			case key_state_1:	   // 按键消抖与确认态 
			    if(++key_time >= 1)  // 改变判断条件可改变键盘灵敏度
				{
					key_value1 = key_press;	//记录是哪个按键被按下
					if(key_press != NO_KEY)	//按键仍然处于按下
					{
						key_time = 0;	//
						key_state = key_state_2; //消抖完成，状态转换到按下键时间的计时状态，但返回的还是无键事件
					}
					else
						key_state = key_state_0;// 按键已抬起,转换到按键初始态,此处完成和实现软件消抖，其实按键的按下和释放都在此消抖的
				}	
			break;
			case key_state_2:
				//key_value2 = key_press;	//记录是哪个按键被按下
				
					//key_time2 = 0;
			    	if(key_press == NO_KEY) //短按键操作
					{
						key_return = S_key;	//此时按键释放，说明是产生一次短操作，回送S_key
						key_result = (uint16_t)key_return;
						key_result = (key_result<<8)|(key_value1 & KEY_MASK);
						key_state = key_state_0;   // 转换到按键初始态 
					}
				
					else if(++key_time >= 200) // 继续按下，计时加10ms（10ms为本函数循环执行间隔
					{
						key_return = L_key;	//返回长按键状态
						key_time = 0;
						key_result = (uint16_t)key_return;
						key_result = (key_result<<8)|(key_value1 & KEY_MASK);
						key_state = key_state_3;   // 转换到等待按键释放状态
					}
					
					//key_return |= (key_value2 & KEY_MASK);	
				   // key_result = (key_result<<8)|(key_value2 & KEY_MASK);
					//低8位记录是哪个按键,如1101001
				
			break;
			case key_state_3:
				if(key_press == NO_KEY)
					key_state = key_state_0;
				//else if(key_press == 0x2E)   // key 4有问题
				//{
				//    key_state = key_state_0;
				//}
			break;
	}
	return key_result;	//返回按键结果
}

#include "PM25Sensor.h"
#include "LED_Drv.h"
void key_process(uint16_t key)
{ 
    uint8_t key_mode;
    key_mode = (uint8_t)((key & 0xFF00)>>8);	//高8位为按键模式


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

// 获取当前显示模式
uint8_t key_get_cur_display_mode(void)
{
   return cur_display_mode;
}

