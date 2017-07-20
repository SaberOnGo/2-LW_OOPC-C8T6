
#include "PM25Sensor.h"
#include "LCD1602_Drv.h"
#include "os_timer.h"
#include "uart_queue.h"
#include "LED_Drv.h"
#include "Key_Drv.h"
#include "delay.h"

#include "DHT11.h"


static uint8_t volatile rx_pm25 = 0;   // 是否接收到PM25数据
static uint8_t volatile is_using = 1;  // 信号量

//#if (SENSOR_SELECT != HCHO_SENSOR)
static volatile T_PM25_Sensor tPM25;

#if 1
static const 
#else
static __eeprom	
#endif

uint8_t AqiLevelString[][14] = {
{"Good          " },
{"Moderate      "},
{"Unhealthy     "},
{"Unhealthy     "},
{"Very Unhealthy"},
{"Just In Hell  "},
{"Heavy In Hell "},
};

//对于 E_AQI_STD
static const uint8_t AqiStdString[][7] = {
{"AQI US"},
{"AQI CN"},
};

// AQI 等级, 共 7 级
static const T_AQI AqiLevel[] = 
{
// C_low_us C_high_us, C_low_cn, C_high_cn, aqi_low,  aqi_high 
  	{0,       15.4,       0,         35,          0,          50  }, 
	{15.5,    40.4,      35.1,      75,          51,         100 }, 
  	{40.5,    65.4,      75.1,      115,         101,       150 },
  	{65.5,    150.4,     115.1,     150,        151,        200 },
    {150.5,   250.4,     150.1,     250,        201,        300},
    {250.5,   350.4,     250.1,     350,        301,        400},
    {350.5,   500.4,     350.1,     500,        401,        500},
};

#define AQI_LEVEL_STR_SIZE  ( sizeof(AqiLevelString) / sizeof(AqiLevelString[0]) )  
#define AQI_LEVEL_SIZE      (sizeof(AqiLevel) / sizeof(AqiLevel[0]))

//#else
#define DISPLAY_INTERNAL_SEC     3    // 3 sec 显示一次
static volatile uint16_t hcho_ppb[18];   // HCHO 气体体积分数, ppb
#define HCHO_PPB_ARRAY_SIZE   (sizeof(hcho_ppb) / sizeof(hcho_ppb[0]))
static volatile uint16_t mass_fraction;       // HCHO 质量分数, ug/m3
static volatile uint16_t new_ppb = 0;         // 最新的甲醛值
static volatile uint16_t aver_ppb = 0;        // 平均 ppb 值 = 0.001 ppm
static volatile uint8_t  rx_count = 0;        // 接收计数
static volatile uint8_t  is_power_first = E_TRUE;   // 开机后第一次运行
static volatile uint8_t rx_hcho = 0;            // 是否有hcho 传感器的数据过来
static volatile uint8_t is_disp_hcho = 0;     // hcho 数据是否计算完成, 可以显示数据
//#endif

#define ON_USING    0
#define ON_IDLE     1

#define Take_PM25_Sem()    if(is_using)is_using--
#define Release_PM25_Sem() is_using++
#define TryTakePM25_Sem()  is_using


//#if (SENSOR_SELECT != HCHO_SENSOR)
    //uint32_t total_count = 0;

	
	   
	   






static void LCD_Display_CC(uint16_t pm25, uint16_t pm10)
{
	
	LCD1602_WriteString(0,  0, "PM2.5 ");
    LCD1602_WriteInteger(0, 6, pm25, 3);
    LCD1602_WriteString(0, 10, "ug/m3");

	LCD1602_WriteString(1,  0, "PM10  ");
    LCD1602_WriteInteger(1, 6, pm10, 3);
    LCD1602_WriteString(1, 10, "ug/m3");
}

static void LCD_Display_PC(uint16_t pc0p3um, uint16_t pc2p5um)
{
	
	LCD1602_WriteString(0,  0, "> 0.3um ");
    LCD1602_WriteInteger(0, 8, pc0p3um, 5);

	LCD1602_WriteString(1,  0, "> 2.5um ");
    LCD1602_WriteInteger(1, 8, pc2p5um, 5);
}

// AQI 计算
// 参数: uint16_t pm25: PM25 浓度, 
// E_AQI_STD aqi_std: 标准选择
// uint8_t * level: 输出的aqi等级: 0-6 : 表示AQI 等级: 1-7
// 返回值: AQI
static uint16_t PM25_AqiCalculate(uint16_t pm25, E_AQI_STD aqi_std, uint8_t *level)
{
    float c_low = 0, c_high = 0;
	uint8_t i;
    float deltaC;     // 浓度差值
    uint16_t deltaI;  // AQI 差值
	uint16_t aqi = 0;
	
	if(pm25 > 500)pm25 = 500;
	
	if(aqi_std == AQI_CN)
	{
	    for(i = 0; i < AQI_LEVEL_SIZE; i++)
		{
		    if(AqiLevel[i].C_low_cn <= pm25 && pm25 <= AqiLevel[i].C_high_cn)
		    {
		        c_low  = AqiLevel[i].C_low_cn;
				c_high = AqiLevel[i].C_high_cn;
				break;
		    }
		}
	}
	else
	{
	    for(i = 0; i < AQI_LEVEL_SIZE; i++)
		{
		    if(AqiLevel[i].C_low_us <= pm25 && pm25 <= AqiLevel[i].C_high_us)
		    {
		       c_low  = AqiLevel[i].C_low_us;
			   c_high = AqiLevel[i].C_high_us;
			   break;
		    }
		}
	}

    if(i < AQI_LEVEL_SIZE)	
    {
        deltaC = c_high - c_low;
		deltaI = AqiLevel[i].I_high - AqiLevel[i].I_low;
		aqi = (uint16_t)(((double)deltaI) * (pm25 - c_low) / deltaC + AqiLevel[i].I_low);
    }
	else{ i = 0; }

	*level = i;
	return aqi;
}

// 显示AQI 等级
static void LCD_Display_AQI(uint16_t pm25, E_AQI_STD aqi_std)
{
    uint16_t aqi;
    uint8_t level = 0;
   
    aqi = PM25_AqiCalculate(pm25, aqi_std, &level);

	if(is_rgb_on)LED_AqiIndicate((E_AQI_LEVEL)level);
	LCD1602_WriteString(0,  0, (const uint8_t *)(&AqiStdString[(uint8_t)aqi_std][0]) );
    LCD1602_WriteInteger(0, 7, aqi, 3);
	LCD1602_WriteString(1,  0, (const uint8_t *)(&AqiLevelString[level][0]) );
}

#if 0
static void LCD_Display_HCHO(uint16_t hcho)
{
   uint16_t temp, left;

   LCD1602_ClearScreen();
   
   temp = hcho / 1000;  // mg
   left = hcho % 1000 / 10;  // 保留2位
   LCD1602_WriteString (0,  0, "HCHO ");
   LCD1602_WriteInteger(0,  5, temp, 3);  // 整数部分
   LCD1602_WriteString (0,  8, ".");
   LCD1602_SetXY(0, 9);
   LCD1602_WriteData(left / 10 + 0x30);
   LCD1602_WriteData(left % 10 + 0x30);
   LCD1602_WriteString (0,  11, "mg/m3");
}
#endif

// 显示模式初始化
void PM25_Display_Init(uint8_t display_mode)
{
    uint16_t temp;
	LCD1602_ClearScreen();
    switch(display_mode)
    {
        case DISPLAY_CC:  // 显示浓度
        {
			LED_StopAqiIndicate();
			temp = tPM25.pm25_air;  // 消除 volatile 声明引起的警告
			LCD_Display_CC(temp, tPM25.pm10_air);
        }break;
		case DISPLAY_PC:  // 显示粒子计数
        {
			LED_StopAqiIndicate();

			temp = 0;
			temp += tPM25.PtCnt_2p5um;  
			temp += tPM25.PtCnt_5p0um;
			temp += tPM25.PtCnt_10p0um;
			LCD_Display_PC(tPM25.PtCnt_0p3um, temp);
        }break;
		case DISPLAY_AQI_US:
        {
			LCD_Display_AQI(tPM25.pm25_air, AQI_US);
        }break;
		case DISPLAY_AQI_CN:
        {
			LCD_Display_AQI(tPM25.pm25_air, AQI_CN);
        }break;
		
		#if (SENSOR_SELECT == PMS5003S)
		case DISPLAY_HCHO:  // 显示甲醛, 无甲醛则显示版本号
        {
			LED_StopAqiIndicate();
			//LCD_Display_HCHO(tPM25.extra.hcho); 
        }break;
        #endif
    }
}
// 显示值, 只改变显示的数值
static void PM25_Display_Value(uint8_t display_mode)
{
     uint16_t temp;
     switch(display_mode)
     {
        case DISPLAY_CC:  // 显示浓度
        {
			LED_StopAqiIndicate();
			LCD1602_WriteInteger(0, 6, tPM25.pm25_air, 3);
			LCD1602_WriteInteger(1, 6, tPM25.pm10_air, 3);
        }break;
		case DISPLAY_PC:
		{
			LED_StopAqiIndicate();
			// 消除 volatile 声明引起的警告
			temp = 0;
			temp += tPM25.PtCnt_2p5um;  
			temp += tPM25.PtCnt_5p0um;
			temp += tPM25.PtCnt_10p0um;
			LCD1602_WriteInteger(0, 8, tPM25.PtCnt_0p3um, 5);
			LCD1602_WriteInteger(1, 8, temp, 5);
		}break;
		case DISPLAY_AQI_US:
		{
			LCD_Display_AQI(tPM25.pm25_air, AQI_US); 
		}break;
		case DISPLAY_AQI_CN:
		{
			LCD_Display_AQI(tPM25.pm25_air, AQI_CN);
		}break;
		#if (SENSOR_SELECT == PMS5003S)
		case DISPLAY_HCHO:
		{
			LED_StopAqiIndicate();
        }break;
        #endif
    }
}



// 数据接收处理
void PM25_Receive(uint8_t * buf, uint16_t  len)
{
    uint16_t *p;   // 传感器数据起始指针
	uint8_t i;
	uint16_t new_sum = 0;  // 待计算的校验和
	uint16_t sum_len;      // 校验长度
	
    if(buf[0] != 0x42 && buf[1] != 0x4d)
    {
		return;
    }
	//if(TryTakePM25_Sem() == ON_USING)return;
	
	Take_PM25_Sem();
	tPM25.len = (((uint16_t)buf[2]) << 8) + buf[3];

	p = (uint16_t *)&tPM25.pm1_cf1;
	for(i = 0; i < 14; i++)  // 包括校验和
	{
	   p[i] = (((uint16_t)buf[4 + i * 2]) << 8) + buf[5 + i * 2];
	}
    
    sum_len = 4 + len - 2;  // 校验的数据长度, 除校验码外
	for(i = 0; i < sum_len; i++)
	{
	   new_sum += buf[i];
	}
    if(new_sum == tPM25.sum)
    {
       rx_pm25 = 1;  // 接收到PM25数据
    }
	Release_PM25_Sem();  // 释放信号量
}
//#else  // end #if (SENSOR_SELECT != HCHO_SENSOR)
#include "Uart_Drv.h"

static uint8_t Cmd_Buf[9] = {0xFF, 0x01, 0x78, 0x41, 0x00, 0x00, 0x00, 0x00, 0x46};
static void HCHO_SendCmd(uint8_t buf_2, uint8_t buf_3)
{
    Cmd_Buf[2] = buf_2;
	Cmd_Buf[3] = buf_3;
	Cmd_Buf[8] = HCHO_CheckSum(Cmd_Buf, 9);
	Uart_SendDatas(Cmd_Buf, 9);
}
// 切换到查询模式
void HCHO_SwitchToQueryMode(void)
{
    HCHO_SendCmd(0x78, 0x41);
}
// 切换到主动模式
void HCHO_SwitchToActiveMode(void)
{
    HCHO_SendCmd(0x78, 0x40);
}
// 查询气体浓度
void HCHO_QueryHCHOConcentration(void)
{
    HCHO_SendCmd(0x86, 0x00);
}
static uint16_t CalculateHchoAver(uint8_t start, uint8_t div)
{
    uint32_t sum = 0;
    uint8_t i;		   
	for(i = start; i < HCHO_PPB_ARRAY_SIZE; i++)
	{
	  sum += hcho_ppb[i];
	}
	return (uint16_t)((double)sum / div);
}
// uint8_t div: 除数
static uint16_t HCHO_CalculateAverageVal(uint8_t div)
{
	return CalculateHchoAver(0, div);
	// 公式: X = C * M / 22.4
	// X: ug/m3, C: ppb, M: 气体相对分子质量, 22.4: 标准大气压下空气相对分子质量
	//mass_fraction =	(uint16_t)((double)aver_ppb * 30 / 22.4); 
}
static os_timer_t tTimerCalculator;  // 计算数据的定时器, 把计算数据的逻辑部分放在中断外面处理
//static volatile uint8_t  rx_complete = 0;  // 接收完成

// 甲醛浓度曲线变化平滑处理
// 变化趋势不要太快剧烈变化, 平滑变化曲线, 抑制传感器测量噪音
#if 0
static
uint16_t TrendCurveSmooth(uint16_t new_val)
{
      double delta = 0.0;	         // 最新的传感器数据与之前平均值的差值, 正值
	  uint8_t is_negative = 0;	 // 是否为负值, 1: 负值; 0: 非负值
	  uint16_t average = 0;
   	
	  average = HCHO_CalculateAverageVal(HCHO_PPB_ARRAY_SIZE);
	  if(new_val >= average)  // 甲醛浓度呈上升趋势
   {
		 delta = new_val - average;
	  }
	  else	// 最新的浓度比平均值浓度小, 甲醛浓度呈下降趋势
	  {
		 delta = average - new_val;
		 is_negative = 1;
	  }
      // 10 秒内变化不能超过 1 ug
      // (M + 1)* M * delta / (2N) <= 1ug, 其中 M 为 显示间隔内获取的传感器数据, 这里假设 1 秒 获取一次数据, N 为采样次数
      //max_change_val = ((double)2 * HCHO_PPB_ARRAY_SIZE) / ((DISPLAY_INTERNAL_SEC + 1) * DISPLAY_INTERNAL_SEC);
	  if(delta >= 1.5)
	  {
	     delta = 1.5;
	  }
      if(is_negative)
      {
          if(average >= delta)average = (uint16_t)(average - delta);
		  else { average = 0;  }
      }
	  else
	  {
	      average = (uint16_t)(average + delta);
	  }
	  return average;
}
#endif

// 计算传感器数据的定时回调
static void TimerCalculator_CallBack(void * arg)
{
    uint8_t i;
    uint16_t smoothed_ppb = 0;  // 平滑后的最新的测量数据
    uint32_t sec = OS_GetSysTick() / 100;  // 当前系统运行秒值
    
	#if 0
	if( sec > 120)  // 系统开机后120 s后
	{
	     smoothed_ppb = TrendCurveSmooth(new_ppb);
	}
    else
    {
         smoothed_ppb = new_ppb;
    }
	#else
    smoothed_ppb = new_ppb;
	#endif

	 if(is_debug_on)os_printf("tick = %ld, new_ppb = %d ug/m3\n", OS_GetSysTick(), new_ppb);
	
	for(i = 1; i < HCHO_PPB_ARRAY_SIZE; i++)
	{
	  hcho_ppb[i - 1] = hcho_ppb[i];
	}
	hcho_ppb[HCHO_PPB_ARRAY_SIZE - 1] = smoothed_ppb; // 最新的值放在最后, FIFO
    rx_count++;
	
	if(is_power_first)
	{
	    if(rx_count > 8)  // 超过 8 次即开始显示
	    {
	       // 公式: X = C * M / 22.4
	       // X: ug/m3, C: ppb, M: 气体相对分子质量, 22.4: 标准大气压下空气相对分子质量
	       aver_ppb = CalculateHchoAver(HCHO_PPB_ARRAY_SIZE - 3, 3);  // 只计算最后3个
		   mass_fraction =	(uint16_t)((double)aver_ppb * 30 / 22.4); 
		   is_disp_hcho = E_TRUE;  // 显示甲醛值
	    }
		if(rx_count >= HCHO_PPB_ARRAY_SIZE)
	    {
			is_power_first = 0;
			rx_count = 0;
		}
	}
	else
	{
	    if(rx_count >= DISPLAY_INTERNAL_SEC)  // 间隔几秒显示一次最新计算出的甲醛值
	    {
	        // 公式: X = C * M / 22.4
	        // X: ug/m3, C: ppb, M: 气体相对分子质量, 22.4: 标准大气压下空气相对分子质量
	        if(sec < 120)
	        {
	           aver_ppb = CalculateHchoAver(HCHO_PPB_ARRAY_SIZE - 3, 3);
	        }
			else
			{
			   aver_ppb = HCHO_CalculateAverageVal(HCHO_PPB_ARRAY_SIZE);
			}
			if(65 < aver_ppb && aver_ppb <= 79)
			{
			    aver_ppb -= 15;  //校正
			}
			else if(aver_ppb > 79) aver_ppb -= 25;  //校正
			
	        mass_fraction =	(uint16_t)((double)aver_ppb * 30 / 22.4); 
			
            rx_count = 0;
	        is_disp_hcho = E_TRUE;  // 显示 甲醛值
		}
	}

	if(is_debug_on)os_printf("aver_ppb = %d, aver_mass = %d ug/m3\n", aver_ppb, mass_fraction);
		
    rx_hcho = 0;
	new_ppb = 0;  // 数据清0

    //if(is_disp_hcho)
	   //os_printf("tick = %ld, aver_ppb = %d ppm, aver_mass = %d ug/m3\n", Sys_GetRunTime(), aver_ppb, mass_fraction);
}

void HCHO_Receive(uint8_t * buf, uint16_t len)
{
   if(buf[0] == 0xFF)
	  {
      if(buf[1] == 0x17)  // 为HCHO Sensor 主动发送的传感器数据
      {
		new_ppb = ((uint16_t)(buf[4] << 8)) + buf[5];  
		rx_hcho = 1;
		os_timer_arm(&tTimerCalculator, 0, 0);  // 立即启动回调
      }
	  else if(buf[1] == 0x86)
	  {
	     rx_hcho  = 2;
		 mass_fraction = ((uint16_t)buf[2] << 8) + buf[3]; // 气体浓度, 单位: ug / m3
	  }
   }
}
// 显示质量分数, ug/m3
#if 0
#define HCHO_DisplayMassFract(val)  LCD1602_WriteInteger(0,  5, val, 5)
#else
static void HCHO_DisplayMassFract(uint16_t val)
{
   LCD1602_SetXY(0, 5);
   LCD1602_WriteData(val % 10000 / 1000 + 0x30);  // 整数部分
   LCD1602_WriteData('.');
   LCD1602_WriteData(val % 1000 / 100 + 0x30);
   LCD1602_WriteData(val % 100 / 10 + 0x30);
   LCD1602_WriteData(val % 10 + 0x30);
}

#endif
// 显示甲醛质量分数的初始化
static void HCHO_DisplayMassFractInit(uint16_t val)
{
   LCD1602_ClearScreen();
   LCD1602_WriteString (0,  0, "HCHO ");
   HCHO_DisplayMassFract(val);     
   LCD1602_WriteString (0,  10, " mg/m3");
}
// 显示甲醛的PPM数值
static void HCHO_DisplayPPM(uint16_t val)
{
   LCD1602_SetXY(0, 5);
   LCD1602_WriteData(val % 10000 / 1000 + 0x30);  // 整数部分
   LCD1602_WriteData('.');
   LCD1602_WriteData(val % 1000 / 100 + 0x30);
   LCD1602_WriteData(val % 100 / 10 + 0x30);
   LCD1602_WriteData(val % 10 + 0x30);
}

// 显示甲醛的体积分数初始化, ppm
// val: ppb 值
static void HCHO_DisplayPPMInit(uint16_t val)
{
   LCD1602_ClearScreen();
   LCD1602_WriteString (0,  0, "HCHO ");
   HCHO_DisplayPPM(val);
   LCD1602_WriteString (0,  10, " ppm  ");
}



// 温度带一位小数
static void TempHumi_Display(uint16_t temp, uint16_t humi)
{
   LCD1602_SetXY(1, 0);
   //LCD1602_WriteData(temp % 1000 / 100 + 0x30);
   LCD1602_WriteData(temp % 100 / 10 + 0x30);
   LCD1602_WriteData(temp % 10 + 0x30);
   //LCD1602_WriteData('.');
   
   LCD1602_SetXY(1, 10);
   LCD1602_WriteData(humi % 100 / 10 + 0x30);
   LCD1602_WriteData(humi % 10 + 0x30);
}

// 温度显示初始化
static void TEMP_DisplayInit(void)
{
   TempHumi_Display(tDHT.temp_H, tDHT.humi_H);
   LCD1602_WriteString (1,  3, "'C");
   LCD1602_WriteString (1,  7, "RH ");
   LCD1602_WriteString (1,  12, "%");
}

void HCHO_DisplayInit(uint8_t display_mode)
{
   switch(display_mode)
   {
      case HCHO_DISPLAY_MASSFRACT:
	  {
	  	 HCHO_DisplayMassFractInit(mass_fraction);
	  }break;
	  case HCHO_DISPLAY_PPM:
	  {
	  	 HCHO_DisplayPPMInit(aver_ppb);
	  }break;
   }
   TEMP_DisplayInit();
   if(is_rgb_on)LED_IndicateColorOfHCHO();
   //HCHO_Indicate(mass_fraction);
}
static void HCHO_DisplayValue(uint8_t display_mode)
{
   switch(display_mode)
   {
      case HCHO_DISPLAY_MASSFRACT:
	  {
	  	 HCHO_DisplayMassFract(mass_fraction);
	  }break;
	  case HCHO_DISPLAY_PPM:
	  {
	  	 HCHO_DisplayPPM(aver_ppb);
	  }break;
   }
   TempHumi_Display(tDHT.temp_H, tDHT.humi_H);
   if(is_rgb_on)LED_IndicateColorOfHCHO();
   //HCHO_Indicate(mass_fraction);
}

// 根据甲醛值提示
// < 0.08 mg/m3 : Good
// 0.08 - 0.2 mg /m3: Unhealthy
// 0.2 - 0.5 mg /m3: Very Unhealthy
// > 0.5 mg/m3: Dangerous
void HCHO_Indicate(uint16_t val)
{
    #if 1
    if(val < 80)
    {
       LCD1602_WriteString(1,  0, "Good          ");
	   if(is_rgb_on)LED_AqiIndicate(AQI_GOOD);             // 绿灯
    }
	else if(val < 500)
	{
	   LCD1602_WriteString(1,  0, "Unhealthy     ");
	   if(is_rgb_on)LED_AqiIndicate(AQI_Moderate);        // 黄灯
	}
	else
	{
	   LCD1602_WriteString(1,  0, "Very Unhealthy");
	   if(is_rgb_on)LED_AqiIndicate(AQI_VeryUnhealthy);  // 显示红灯
	}
	#endif
	//else
	//{
	   //LCD1602_WriteString(1,  0, "Dangerous     ");
	   //LED_AqiIndicate(AQI_HeavyInHell);
	//}
}

// 计算校验和, 去掉头尾 2 个字节
uint8_t HCHO_CheckSum(uint8_t * buf, uint16_t len)
{
    uint16_t j, newSum = 0;
	if(len < 2)return 0;
	buf += 1;
    len -= 2;
	for(j = 0; j < len; j++)
	{
	   newSum += buf[j];
	}
	newSum = (~newSum) + 1;
	return newSum;
}

//#endif
#include "SDRR.h"

static os_timer_t tTimerCheckPM25;
static void TimerCheckPM25_CallBack(void * arg)
{
    static uint8_t is_rx_hcho_first_time = E_TRUE; // 是否第一次显示
    static uint8_t is_rx_pm25_first_time = E_TRUE;    // 是否第一次显示
    
	#if 0
    if(TryTakePM25_Sem() == ON_USING)  // 正在被占用
    {
       os_timer_arm(&tTimerCheckPM25, 1, 0);
	   return;
    }
	#endif
	
    if(rx_pm25)
    {
       rx_pm25 = 0;
#if 0
	   os_printf("time = %ld ms\r\n\r\n", Sys_GetRunTime() * 10);
	   os_printf("PM1.0[CF] = %d ug/m3\r\n", tPM25.pm1_cf1);
	   os_printf("PM2.5[CF] = %d ug/m3\r\n", tPM25.pm25_cf1);
	   os_printf("PM10[CF]  = %d ug/m3\r\n\r\n", tPM25.pm10_cf1);
	   os_printf("PM1.0[air]  = %d ug/m3\r\n", tPM25.pm1_air);
	   os_printf("PM2.5[air]  = %d ug/m3\r\n", tPM25.pm25_air);
	   os_printf("PM10[air]   = %d ug/m3\r\n\r\n", tPM25.pm10_air);
	   os_printf("PC0.3um  = %ld\r\n", tPM25.PtCnt_0p3um);
	   os_printf("PC0.5um  = %ld\r\n", tPM25.PtCnt_0p5um);
	   os_printf("PC1.0um  = %ld\r\n", tPM25.PtCnt_1p0um);
	   os_printf("PC2.5um  = %ld\r\n", tPM25.PtCnt_2p5um);
	   os_printf("PC5.0um  = %ld\r\n", tPM25.PtCnt_5p0um);
	   os_printf("PC10 um  = %ld\r\n\r\n", tPM25.PtCnt_10p0um);
	   os_printf("hcho = %d mg/m3 \r\n", tPM25.extra.hcho / 1000);
	   os_printf("hcho = %d, ver[0] = 0x%x, ver[1] = 0x%x\r\n", tPM25.extra.hcho, tPM25.extra.ver[0], tPM25.extra.ver[1]);
	   os_printf("sum = 0x%x\r\n\r\n", tPM25.sum);
#endif

	   if(is_rx_pm25_first_time == E_TRUE)
	   {
	      PM25_Display_Init(key_get_cur_display_mode());
	      is_rx_pm25_first_time = E_FALSE;
	   }
       else
       {
          PM25_Display_Value(key_get_cur_display_mode());
       }
    }
	
	if(is_disp_hcho)
	{
		   is_disp_hcho = E_FALSE;
		   if(is_rx_hcho_first_time == E_TRUE)
		   {
		      HCHO_DisplayInit(key_get_cur_display_mode());  // 第一次初始化清屏
			  is_rx_hcho_first_time = E_FALSE;
		   }
		   else
		   {
		      HCHO_DisplayValue(key_get_cur_display_mode());
		   }

           do
           {
               static uint32_t sec = 0;

			   if(OS_SetTimeout(sec) && (OS_GetSysTick() > SEC(15 * 60)))  // 开机后 15分钟开始记录
			   {
			       os_printf("save pm2.5 = %d ug/m3, tick = %ld s\n", mass_fraction, OS_GetSysTick() / 100);
			       sec = OS_SetTimeout(SEC(5 * 60));  // 5 分钟记录一次
			       SDRR_SaveSensorPoint(SENSOR_HCHO, (void *)&mass_fraction);
			   }
           }
		   while(0);
	}
	
    os_timer_arm(&tTimerCheckPM25, 35, 0);  //500 ms 后
}

void LED_IndicateColorOfHCHO(void)
{
   E_AQI_LEVEL level;

   if(mass_fraction < 80)
   {
      level = AQI_GOOD;
   }
   else if(mass_fraction < 200)
   {
      level = AQI_LightUnhealthy;
   }
   else if(mass_fraction < 500)
   {
      level = AQI_VeryUnhealthy;
   }
   else
   {
      level = AQI_HeavyInHell;
   }
   LED_AqiIndicate(level);
}

void PM25_Init(void)
{
   #if (SENSOR_SELECT == HCHO_SENSOR)
   //HCHO_SwitchToActiveMode();
   //HCHO_SwitchToActiveMode();
   #endif
   
   os_timer_setfn(&tTimerCheckPM25, TimerCheckPM25_CallBack, NULL);
   os_timer_arm(&tTimerCheckPM25, 200, 0);  // 2 s后
   
   //#if (SENSOR_SELECT == HCHO_SENSOR)
   os_timer_setfn(&tTimerCalculator, TimerCalculator_CallBack, NULL);  // 定时器初始化, 设置回调环境
   //#endif
}


static uint8_t Pm25_Rx_Buf[PM25_RX_BUF_SIZE];   // 接收缓冲区
static volatile uint16_t pm25_rx_buf_count = 0;	// 接收计数
static volatile uint8_t  last_pm25_val = 0;


static uint8_t Hcho_Rx_Buf[HCHO_RX_BUF_SIZE];   // 接收缓冲区
static volatile uint16_t hcho_rx_buf_count = 0;	// 接收计数
static volatile uint8_t  last_hcho_val = 0;



//注意,读取USARTx->SR能避免莫名其妙的错误 
void PM25_UART_IRQHandler(void)	// PM25中断服务程序
{
   uint8_t data = 0;
   //uint16_t len;
   	
   if(USART_GetITStatus(PM25_UART, USART_IT_RXNE) != RESET)  // 接收非空标志置1
   {
        data = USART_ReceiveData(PM25_UART);
	  
        if(last_pm25_val == 0x42 && data == 0x4d)  // PM25 Sensor 的起始头
		{
			pm25_rx_buf_count = 0;
			Pm25_Rx_Buf[pm25_rx_buf_count] = 0x42;
			pm25_rx_buf_count++;
		}
		else if(pm25_rx_buf_count >= sizeof(Pm25_Rx_Buf))
		{
			pm25_rx_buf_count = 0;
			// 或者这里可以禁止接收中断
		}
		Pm25_Rx_Buf[pm25_rx_buf_count] = data;
		pm25_rx_buf_count++;
		last_pm25_val = data;
		if(pm25_rx_buf_count > 3)
		{
		   uint16_t len;
		   
		   len = (((uint16_t)Pm25_Rx_Buf[2]) << 8) + Pm25_Rx_Buf[3];
		   if((len + 4) == pm25_rx_buf_count)
		   {
			 // rx_complete = 1;  // 接收完成
			 //DisableRxPM25Sensor();  // 暂停接收中断
			 PM25_Receive(Pm25_Rx_Buf, len);
			 pm25_rx_buf_count = 0;
		   }
	   }
   }
}

//注意,读取USARTx->SR能避免莫名其妙的错误 
void HCHO_UART_IRQHandler(void)	// HCHO中断服务程序
{
   uint8_t data = 0;
  // uint16_t len;
   	
   if(USART_GetITStatus(HCHO_UART, USART_IT_RXNE) != RESET)  // 接收非空标志置1
   {
        data = USART_ReceiveData(HCHO_UART);
		
        if(last_hcho_val == 0xFF && (data == 0x17 || data == 0x86) )  // hcho Sensor 的起始头
		{
			hcho_rx_buf_count = 0;
			Hcho_Rx_Buf[hcho_rx_buf_count] = 0xFF;
			hcho_rx_buf_count++;
		}
		else if(hcho_rx_buf_count >= sizeof(Hcho_Rx_Buf))
		{
			hcho_rx_buf_count = 0;
			// 或者这里可以禁止接收中断
		}
		Hcho_Rx_Buf[hcho_rx_buf_count] = data;
		last_hcho_val = data;
		hcho_rx_buf_count++;
		if(hcho_rx_buf_count == 9)  // 一共 9 个字节
		{
		     if(HCHO_CheckSum(Hcho_Rx_Buf, 9) == Hcho_Rx_Buf[8])  // 校验和正确
		     {
		        HCHO_Receive(Hcho_Rx_Buf, 9);
		     }
			 hcho_rx_buf_count = 0;
	    }
   }
}


