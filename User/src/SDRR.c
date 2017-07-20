

/******************** Sensor Data Record  Repository  ���������ݼ�¼�����ļ�   *********************** */

#include "SDRR.h"
#include "PM25Sensor.h"
#include "os_global.h"
#include "GlobalDef.h"
#include "FatFs_Demo.h"
#include <string.h>


// ������������ʱ��Ҳ����buf
uint8_t  sector_buf[4096];     // ��һ��FLASH������������СΪ����, ���ٶ�FLASH�Ĳ�д����

static uint16_t cur_str_len    = 0;   // ��ǰ�ַ���ռ�õĳ���
//static uint16_t cur_item_count = 0;   // ��������ʱbuf�еĴ�������Ŀ������
static uint32_t total_item_count = 0; // �ܵĴ�������Ŀ��

/**************************
ÿ�����ݵĳ���<= 70 B, ��ʽΪ:
                 ���      ʱ���                    ��ȩ(mg/m3)        PM2.5(ug/m3)           �¶�('C)             ���ʪ��(%)
��0��:    No          TimeStamp                   HCHO(mg/m3)        PM2.5(ug/m3)           Temperature('C)    Humidity
��1��    [00000]    2017-06-01 13:13:59   9.001                        999                              25.7                          30%
��N��    [N-1   ]    2017-06-01 14:15:03   0.072                        008                              24.5                          47%

����ÿ 10 ���� ��һ����¼, ÿ���¼ 24 * 6 = 144 ��
 2MB FLASH (W25X16 )����Լ�¼ :  1768 KB * 1024 /  (24 * 6 * 48 ) = 261 ��
******************************/
//extern FATFS FlashDiskFatFs;

static T_SDRR tSDRR;



// ���������ݼ�¼ģ���ʼ��
void SDRR_Init(void)
{
   //uint16_t len = 0;
   
   os_memset(&tSDRR, 0, sizeof(tSDRR));

   #if 0
   tSDRR.time.year = 17;
   tSDRR.time.month = 6;
   tSDRR.time.day = 13;
   tSDRR.time.hour = 11;
   tSDRR.time.min = 10;
   tSDRR.time.sec = 58;
   tSDRR.hcho = 76;
   tSDRR.pm2p5 = 22;
   tSDRR.RH = 53;
   tSDRR.temp = 27;
   
   os_memset(sector_buf, 0, sizeof(sector_buf));

   len += os_snprintf(&sector_buf[len], sizeof(sector_buf) - cur_str_len, "[%05d]  %02d-%02d-%02d %02d:%02d:%02d  ", total_item_count, 
                       tSDRR.time.year, tSDRR.time.month, tSDRR.time.day, 
                       tSDRR.time.hour, tSDRR.time.min,   tSDRR.time.sec);
   len += os_snprintf(&sector_buf[len], sizeof(sector_buf) - cur_str_len, "%c.%03d  %3d    %3d.%c  %2d%%\r\n", (tSDRR.hcho % 10000 / 1000) + 0x30,
   	                    tSDRR.hcho % 1000, tSDRR.pm2p5 % 1000, tSDRR.temp % 1000 / 10, (tSDRR.temp % 10) + 0x30, tSDRR.RH % 100);
   
   os_printf("%s, len = %d\n", (const char *)sector_buf, len);
   #endif

   total_item_count = FILE_GetSensorTotalItems();
   os_printf("init: total_items = %ld\n", total_item_count);
}

// ������������ת��Ϊ�ַ���
// ����: T_SDRR : ���������ݼ�¼�ṹ����
//       uint8_t * buf: ����ַ���
//       uint16_t * size: ������ʱ��buf����󳤶�, ���ʱ���ַ����ĳ���
//  һ��ռ�� 54 �ֽ�
//  "0-65535 Y-Mon-Day  HH:MM:SS  mg/m3  ug/m3  'C"
//  [00005]  17-05-01 23:59:51  0.063    031     23.1  49%
// "[00005]  17-05-01 23:59:51  0.063    031     23.1  49%"
E_RESULT SDRR_SensorPointToString(T_SDRR *sdrr, uint8_t * buf, uint16_t * size)
{
   //uint8_t mask = (1 << SENSOR_END) - 1;
   uint16_t left_size = *size;   // �����ó���
   uint16_t len = 0;
   
   if(NULL == sdrr || NULL == buf || NULL == size){ INSERT_ERROR_INFO(0);  return APP_FAILED; }

   // ������д������������Ƿ񱣴�
   #if 0 // ��ʱ����
   if(sdrr->sensor_mask < mask)
   {
      os_printf("sensor pointer not save all, sensor_mask = 0x%x\n", sdrr->sensor_mask);
	  return APP_FAILED;
   }
   #endif

   // ��Ŀ ���� ʱ��
   len += os_snprintf(&buf[len], left_size, "[%05d]  %02d-%02d-%02d %02d:%02d:%02d  ", total_item_count, 
                       sdrr->time.year, sdrr->time.month, sdrr->time.day, 
                       sdrr->time.hour, sdrr->time.min,   sdrr->time.sec);
   left_size = *size - len;
   len += os_snprintf(&buf[len], left_size, "%c.%03d  %3d    %3d.%c  %2d%%\r\n", (sdrr->hcho % 10000 / 1000) + 0x30,
   	                    sdrr->hcho % 1000, sdrr->pm2p5 % 1000, sdrr->temp % 1000 / 10, (sdrr->temp % 10) + 0x30, sdrr->RH % 100);
   total_item_count += 1;
   *size = len;  // �����ʽ������ַ�������
   
   return APP_SUCCESS;
}

/***********************************
����: ��ָ���ļ���д�봫������¼
����: char * file_name: �ļ���
************************************/
E_RESULT SDRR_WriteRecordToFile(char * file_name)
{
    uint16_t item_len = 0; // ��������ת��Ϊ�ַ�����ĳ���
	uint16_t left_len = 0;
	E_RESULT res = APP_FAILED;
	
	item_len = sizeof(sector_buf) - cur_str_len;  // ������ʣ����õĳ��� 
	res = SDRR_SensorPointToString(&tSDRR, &sector_buf[cur_str_len], &item_len);
	cur_str_len += item_len;
	left_len = sizeof(sector_buf) - cur_str_len;

	#if 1
	if(left_len < item_len) // ʣ�µĲ���һ�����ݳ���
	{
	   if(left_len > 2)
	   {
	      os_memset(&sector_buf[cur_str_len], ' ', left_len - 2);
		  os_strncpy(&sector_buf[sizeof(sector_buf) - 2], "\r\n", 2);
	   }
	   else if(left_len == 2)
	   {
	      os_strncpy(&sector_buf[sizeof(sector_buf) - 2], "\r\n", 2);
	   }
	   
	  //  �����ݱ��浽FAT �ļ���
	   res = FILE_Write(file_name, (char *)sector_buf);
	   os_memset(sector_buf, 0, sizeof(sector_buf));
	   cur_str_len  = 0;
	}
	#else
    res = FILE_Write(file_name, (char *)sector_buf);
	   os_memset(sector_buf, 0, sizeof(sector_buf));
	   cur_str_len  = 0;
	#endif
	
	return res;
}
	

/***************************
����: ����һ��������������
����: E_SensorType type: ���������ݵ�����
             void  * data: ����������
 ����ֵ: ����ɹ�: APP_SUCCESS; ʧ��: APP_FAILED            
******************************/
E_RESULT SDRR_SaveSensorPoint(E_SensorType type, void  * data)
{
	
	switch(type)
	{
	    case SENSOR_HCHO:
	   	{
			tSDRR.hcho = *((uint16_t *)data);
		}break;
		case SENSOR_PM25:
		{
		   tSDRR.pm2p5 = *((uint16_t *)data);
		}break;
		case SENSOR_TEMP:
		{
		   tSDRR.temp = *((uint16_t *)data);
		}break;
		case SENSOR_HUMI:
		{
			tSDRR.RH  = *((uint16_t *)data);
		}break;
		case SENSOR_TIME:
		{
			os_memcpy(&tSDRR.time, data, sizeof(T_RTC_TIME));
		}break;
		default:
		  break;
	}

    if(type < ((uint8_t)SENSOR_END) )
    {
       tSDRR.sensor_mask |= (1 << type);
    }

    tSDRR.sensor_mask |= 1 << SENSOR_PM25;
	tSDRR.sensor_mask |= 1 << SENSOR_TIME;
    if(tSDRR.sensor_mask == ( (1 << (SENSOR_END)) - 1))  // ���������ڵ���ȫ������
	{
	   os_printf("ready to wirte data point to file, tick = %ld\n", OS_GetSysTick());
	   SDRR_WriteRecordToFile(SENSOR_TXT_FILE_NAME);
	}

	return APP_SUCCESS;
}





