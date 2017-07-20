

/******************** Sensor Data Record  Repository  传感器数据记录处理文件   *********************** */

#include "SDRR.h"
#include "PM25Sensor.h"
#include "os_global.h"
#include "GlobalDef.h"
#include "FatFs_Demo.h"
#include <string.h>


// 此数组升级的时候也共用buf
uint8_t  sector_buf[4096];     // 以一个FLASH的物理扇区大小为容量, 减少对FLASH的擦写次数

static uint16_t cur_str_len    = 0;   // 当前字符串占用的长度
//static uint16_t cur_item_count = 0;   // 保存在临时buf中的传感器条目的总数
static uint32_t total_item_count = 0; // 总的传感器条目数

/**************************
每条数据的长度<= 70 B, 格式为:
                 编号      时间戳                    甲醛(mg/m3)        PM2.5(ug/m3)           温度('C)             相对湿度(%)
第0行:    No          TimeStamp                   HCHO(mg/m3)        PM2.5(ug/m3)           Temperature('C)    Humidity
第1行    [00000]    2017-06-01 13:13:59   9.001                        999                              25.7                          30%
第N行    [N-1   ]    2017-06-01 14:15:03   0.072                        008                              24.5                          47%

其中每 10 分钟 存一条记录, 每天记录 24 * 6 = 144 条
 2MB FLASH (W25X16 )多可以记录 :  1768 KB * 1024 /  (24 * 6 * 48 ) = 261 天
******************************/
//extern FATFS FlashDiskFatFs;

static T_SDRR tSDRR;



// 传感器数据记录模块初始化
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

// 将传感器数据转化为字符串
// 参数: T_SDRR : 传感器数据记录结构变量
//       uint8_t * buf: 输出字符串
//       uint16_t * size: 作输入时是buf的最大长度, 输出时是字符串的长度
//  一条占用 54 字节
//  "0-65535 Y-Mon-Day  HH:MM:SS  mg/m3  ug/m3  'C"
//  [00005]  17-05-01 23:59:51  0.063    031     23.1  49%
// "[00005]  17-05-01 23:59:51  0.063    031     23.1  49%"
E_RESULT SDRR_SensorPointToString(T_SDRR *sdrr, uint8_t * buf, uint16_t * size)
{
   //uint8_t mask = (1 << SENSOR_END) - 1;
   uint16_t left_size = *size;   // 最大可用长度
   uint16_t len = 0;
   
   if(NULL == sdrr || NULL == buf || NULL == size){ INSERT_ERROR_INFO(0);  return APP_FAILED; }

   // 检查所有传感器的数据是否保存
   #if 0 // 暂时屏蔽
   if(sdrr->sensor_mask < mask)
   {
      os_printf("sensor pointer not save all, sensor_mask = 0x%x\n", sdrr->sensor_mask);
	  return APP_FAILED;
   }
   #endif

   // 条目 日期 时间
   len += os_snprintf(&buf[len], left_size, "[%05d]  %02d-%02d-%02d %02d:%02d:%02d  ", total_item_count, 
                       sdrr->time.year, sdrr->time.month, sdrr->time.day, 
                       sdrr->time.hour, sdrr->time.min,   sdrr->time.sec);
   left_size = *size - len;
   len += os_snprintf(&buf[len], left_size, "%c.%03d  %3d    %3d.%c  %2d%%\r\n", (sdrr->hcho % 10000 / 1000) + 0x30,
   	                    sdrr->hcho % 1000, sdrr->pm2p5 % 1000, sdrr->temp % 1000 / 10, (sdrr->temp % 10) + 0x30, sdrr->RH % 100);
   total_item_count += 1;
   *size = len;  // 输出格式化后的字符串长度
   
   return APP_SUCCESS;
}

/***********************************
功能: 往指定文件中写入传感器记录
参数: char * file_name: 文件名
************************************/
E_RESULT SDRR_WriteRecordToFile(char * file_name)
{
    uint16_t item_len = 0; // 本次数据转化为字符串后的长度
	uint16_t left_len = 0;
	E_RESULT res = APP_FAILED;
	
	item_len = sizeof(sector_buf) - cur_str_len;  // 缓冲区剩余可用的长度 
	res = SDRR_SensorPointToString(&tSDRR, &sector_buf[cur_str_len], &item_len);
	cur_str_len += item_len;
	left_len = sizeof(sector_buf) - cur_str_len;

	#if 1
	if(left_len < item_len) // 剩下的不够一条数据长度
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
	   
	  //  将数据保存到FAT 文件中
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
功能: 保存一条传感器的数据
参数: E_SensorType type: 传感器数据点类型
             void  * data: 传感器数据
 返回值: 保存成功: APP_SUCCESS; 失败: APP_FAILED            
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
    if(tSDRR.sensor_mask == ( (1 << (SENSOR_END)) - 1))  // 各传感器节点已全部保存
	{
	   os_printf("ready to wirte data point to file, tick = %ld\n", OS_GetSysTick());
	   SDRR_WriteRecordToFile(SENSOR_TXT_FILE_NAME);
	}

	return APP_SUCCESS;
}





