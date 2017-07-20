
/******************** Sensor Data Record  Repository  传感器数据记录存储头文件   *********************** */
#ifndef __SDRR_H__
#define  __SDRR_H__

#include "GlobalDef.h"


// 传感器类型
typedef enum
{
   SENSOR_HCHO = 0,
   SENSOR_PM25 = 1,
   SENSOR_TEMP = 2,
   SENSOR_HUMI = 3,  
   SENSOR_TIME = 4,
   SENSOR_END,
}E_SensorType;
	
// 传感器数据记录
typedef struct
{
   uint16_t no;  // 编号 0-65535
   T_RTC_TIME time;
   uint16_t  hcho;   // hcho 浓度, unit: ug/m3
   uint16_t  pm2p5;  // PM2.5浓度, unit: ug/m3
   uint16_t  temp;   // 温度, unit: 0.1 'C, 即此值 / 10 才是实际温度值
   uint16_t  RH;     // 相对湿度, 0-100, 百分数

   // bit 0: HCHO 值; bit1: PM2.5; bit2: temp; bit3: humidity
   uint8_t   sensor_mask;   // 传感器数据掩码  bit位为1: 已获取到该传感器数据
   
   uint16_t  sum;    // 和校验结果
}T_SDRR;


extern uint8_t  sector_buf[];


void SDRR_Init(void);
E_RESULT SDRR_SaveSensorPoint(E_SensorType type, void  * data);



#endif

