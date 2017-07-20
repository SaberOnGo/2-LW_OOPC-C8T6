
/******************** Sensor Data Record  Repository  ���������ݼ�¼�洢ͷ�ļ�   *********************** */
#ifndef __SDRR_H__
#define  __SDRR_H__

#include "GlobalDef.h"


// ����������
typedef enum
{
   SENSOR_HCHO = 0,
   SENSOR_PM25 = 1,
   SENSOR_TEMP = 2,
   SENSOR_HUMI = 3,  
   SENSOR_TIME = 4,
   SENSOR_END,
}E_SensorType;
	
// ���������ݼ�¼
typedef struct
{
   uint16_t no;  // ��� 0-65535
   T_RTC_TIME time;
   uint16_t  hcho;   // hcho Ũ��, unit: ug/m3
   uint16_t  pm2p5;  // PM2.5Ũ��, unit: ug/m3
   uint16_t  temp;   // �¶�, unit: 0.1 'C, ����ֵ / 10 ����ʵ���¶�ֵ
   uint16_t  RH;     // ���ʪ��, 0-100, �ٷ���

   // bit 0: HCHO ֵ; bit1: PM2.5; bit2: temp; bit3: humidity
   uint8_t   sensor_mask;   // ��������������  bitλΪ1: �ѻ�ȡ���ô���������
   
   uint16_t  sum;    // ��У����
}T_SDRR;


extern uint8_t  sector_buf[];


void SDRR_Init(void);
E_RESULT SDRR_SaveSensorPoint(E_SensorType type, void  * data);



#endif

