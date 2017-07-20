
#ifndef __FATFS_DEMO_H__
#define  __FATFS_DEMO_H__

#include <stdint.h>
#include "ff.h"
#include "GlobalDef.h"



#define SENSOR_TXT_FILE_NAME  "sensor_data.txt"

extern FATFS FlashDiskFatFs;

	
void FatFs_Demo(void);
E_RESULT FILE_Open(FIL * fileFIL, char * file_name);
E_RESULT FILE_Write(char * file_name, char * write_buf);
void FILE_FormatDisk(void);
uint32_t FILE_GetSensorTotalItems(void);




#endif  //end of file

