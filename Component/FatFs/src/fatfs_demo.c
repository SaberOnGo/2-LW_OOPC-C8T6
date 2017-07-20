
#include "fatfs_demo.h"
#include "stm32f10x.h"
#include "GlobalDef.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "ff.h"
#include "spi_flash_interface.h"
#include "os_global.h"

/* Private variables ---------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
FATFS FlashDiskFatFs;         /* Work area (file system object) for logical drive */
char FlashDiskPath[8] = "0:";
char UpdateDirPath[32] = "0:/update";   // �����ļ�·��
static char FileOutPath[64];

#if _USE_LFN
char Lfname[_MAX_LFN + 1];
#endif

FIL   fileFIL;


#include <string.h>


//�õ�����ʣ������
//drv:���̱��("0:"/"1:")
//total:������	 ����λKB��
//free:ʣ������	 ����λKB��
//����ֵ:0,����.����,�������
FRESULT exf_getfree(uint8_t * drv, uint32_t * total, uint32_t *free)
{
	FATFS * fs1;
	FRESULT res;
    uint32_t fre_clust = 0, fre_sect = 0, tot_sect = 0;
	
    //�õ�������Ϣ�����д�����
    res = f_getfree((const TCHAR*)drv, (DWORD*)&fre_clust, &fs1);
    if(res == FR_OK)
	{											   
	    tot_sect = (fs1->n_fatent - 2) * fs1->csize;	//�õ���������
	    fre_sect = fre_clust * fs1->csize;			//�õ�����������	   
//#if _MAX_SS != 512				  			  
//		tot_sect* = fs1->ssize / 512;
//		fre_sect* = fs1->ssize / 512;
//#endif	  
        #if (SPI_FLASH_SECTOR_SIZE == 4096L)
		*total = tot_sect  << 2;	// x4, ��λΪKB, ������СΪ 4096 B
		*free  = fre_sect  << 2;	// x4, ��λΪKB 
		#elif (SPI_FLASH_SECTOR_SIZE == 512L)
        *total = tot_sect  / 2;	//��λΪKB, �����FatFs������СΪ 512 B
		*free  = fre_sect  / 2;	//��λΪKB 
        #else
		#error "ext_getfree error"
		#endif
 	}
	else
	{
	    os_printf("FatFs error = %d\n", res);
	}
	return res;
}	

// �ļ��ĵ�0����ʾ�����������ַ���
const char SensorNameItem[] = "Number	     Timestamp	     HCHO	  PM2.5  Temp  Humidity\r\n";

// �ļ��ĵ�1����ʾ���������ݵ�λ
const char SensorUnitItem[] = "0-65535 Y-Mon-Day  HH:MM:SS  mg/m3  ug/m3  'C\r\n";

// ��ָ���ļ�
E_RESULT FILE_Open(FIL * pFileFIL, char * file_name)
{
   FRESULT res;
   char sensor_dir[64] = "0:/sensor";        // ��������Ŀ¼
   int len = 0;
   DIR dirs;
   uint8_t is_first_create = 0;  // �Ƿ��һ�δ���
   	
   // �ж�Ŀ¼�Ƿ����, �������򴴽�
   res = f_opendir(&dirs, (const TCHAR *)sensor_dir);  // ������Ŀ¼
   os_printf("open dir %s %s\n", sensor_dir, (res == FR_OK ? "ok" : "failed"));
   if(res == FR_NO_PATH)  
   {
       // ������Ŀ¼
       res = f_mkdir((const TCHAR *)sensor_dir);
	   if(res != FR_OK){ os_printf("create dir %s failed, res = %d\n", sensor_dir, res); return APP_FAILED; }
	   else { os_printf("create dir %s success\n", sensor_dir); }
	   is_first_create = E_TRUE;
   }
   if(res){ INSERT_ERROR_INFO(res); return APP_FAILED; }
   f_closedir(&dirs);
   
   // ���ļ���ȡ�ļ�ָ��
   len = os_strlen(sensor_dir);
   sensor_dir[len] = '/';
   os_strncpy(&sensor_dir[len + 1], file_name, sizeof(sensor_dir));

   res = f_open(pFileFIL, (const TCHAR * )sensor_dir, FA_OPEN_ALWAYS | FA_WRITE | FA_READ);  // ���ļ�, ��д
   if(is_first_create)
   {
      UINT bw = 0;
	  
      // ͷ������ʾ��λ
      res = f_write(pFileFIL, SensorNameItem, os_strlen(SensorNameItem), &bw);
      res = f_write(pFileFIL, SensorUnitItem, os_strlen(SensorUnitItem), &bw);
   }
   f_sync(pFileFIL);  // ��С���ļ�������, ͬ���ļ�����, ��ֹ���ƻ�
   os_printf("f_open %s, fsize = %d\n", (res == FR_OK ? "ok" : "failed"), pFileFIL->fsize);
   if(res){ INSERT_ERROR_INFO(res); return APP_FAILED;  }

   return APP_SUCCESS;
}

// �����ļ��в�д�봫��������
E_RESULT FILE_Write(char * file_name, char * write_buf)
{
	FRESULT res;
	int  len = 0;
	UINT bw;  // �������, д��󳤶� 

   if(FILE_Open(&fileFIL, file_name) == APP_FAILED) return APP_FAILED;
   
   res = f_lseek(&fileFIL, fileFIL.fsize); // �ļ�ָ���Ƶ����
   os_printf("f_lseek res = %d, fptr = %ld, fsize = %ld\n", res, fileFIL.fptr, fileFIL.fsize);
   
   len = os_strlen(write_buf);
   res = f_write(&fileFIL, write_buf, len, &bw);
   f_close(&fileFIL);
   os_printf("f_write res = %d, len = %d, bw = %d\n", res, len, bw);
   
   return APP_SUCCESS;
}



#define RECURSIVE_EN   0  // �ݹ�����ʹ��: 1; ��ֹ: 0

FRESULT FILE_Scan(
	char * path,		   /* Pointer to the working buffer with start path */
	int   pathMaxLen,    /* the max length of the working buffer  */
	char * fileInName,   /*   �����ҵ��ļ��� */
	char * filePath,     // ������ҳɹ�, �򿽱��ļ�·������
	int filePathMaxLen // filePath buf ����󳤶�
)
{
	FILINFO  Finfo;
	DIR dirs;
	FRESULT res;
	FRESULT result;
	char *fn;
    
    WORD AccFiles = 0;  /* AccFiles ��file ��AccDirs ��folders */

	#if RECURSIVE_EN
    WORD AccDirs = 0;		
    int len;                
    #endif
	
	DWORD AccSize = 0;				/* �ļ��е��ֽ��� */
	char * p = NULL;

	
	
#if _USE_LFN
	Finfo.lfname = Lfname;
	Finfo.lfsize = sizeof(Lfname);
#endif
	res = f_opendir(&dirs, path);
	if (res == FR_OK) 
	{
	    #if RECURSIVE_EN
		len = os_strlen(path);  // ����Ҫ�ݹ�����
		#endif
		
		while ((f_readdir(&dirs, &Finfo) == FR_OK) && Finfo.fname[0]) 
		{
			if (_FS_RPATH && Finfo.fname[0] == '.') 
				continue;
#if _USE_LFN
			fn = *Finfo.lfname ? Finfo.lfname : Finfo.fname;
#else
			fn = Finfo.fname;
#endif
			if (Finfo.fattrib & AM_DIR)   // ��Ŀ¼
			{
			    #if RECURSIVE_EN // ����ݹ��������ⲿ�ִ��� , ����FileOutPath Ϊ�ⲿ��̬����
				AccDirs++;
				path[len] = '/'; 
				os_strncpy(path + len + 1, fn, pathMaxLen);
				//��¼�ļ�Ŀ¼

				res = FILE_Scan(path, fileInName, FileOutPath);
				path[len] = '\0';
				if (res != FR_OK)
				{
				   os_printf("scan file() error  = %d\n", res);
				   break;
				}	
				#else
				
				os_printf("scan file failed, dirs\n");
				res = FR_NO_FILE;
				break;
				
				#endif
			} 
			else   // ���ļ�
			{
				AccFiles++;
				AccSize += Finfo.fsize;

				//��¼�ļ�
				os_snprintf(filePath, filePathMaxLen, "%s/%s\r\n", path, fn);  // �ļ�·���Ϊ256 B
				p = strstr(filePath, fileInName);
				if (p != NULL)
				{
				    
					os_printf("search file success: %s\n", filePath);
					res = FR_OK;
				}
				else
				{
				    os_printf("not find file\n");
				    res = FR_NO_FILE;
				}
			}
		}
	}
	result = f_closedir(&dirs);
	os_printf("closedir %s\n", (result == FR_OK ? "OK" : "Failed"));
	
	return res;
}


#include "sfud.h"
#include "F10X_Flash_If.h"
#include "delay.h"
void FILE_FormatDisk(void)
{
	FRESULT res;
    uint32_t total = 0, free = 0;


	// ���ش���
	res = f_mount(&FlashDiskFatFs, (TCHAR const*)FlashDiskPath, 1);
	os_printf("f_mount = %d\n", res);
	
	if (res == FR_NO_FILESYSTEM)  // FAT�ļ�ϵͳ����,���¸�ʽ��FLASH
	{
	    os_printf("FatFs Error, do Format...\r\n");
		res = f_mkfs((TCHAR const*)FlashDiskPath, 1, 4096);  //��ʽ��FLASH,0,�̷�; 1,����Ҫ������,8������Ϊ1����
		if(res == FR_OK)
		{
		    os_printf("Flash Disk Format Finish !\n");  
		}
		else
		{
		    os_printf("Flash Disk Format Failed = %d\n", res);
		}
	}
	else
	{
	    if(res != FR_OK)
	    {
	        os_printf("FatFs mount error = %d\r\n", res);
	    }
		else
		{
		    os_printf("FatFs mount success\r\n");

			#if 1
			res = FILE_Scan(UpdateDirPath, sizeof(UpdateDirPath), "Update.bin", FileOutPath, sizeof(FileOutPath));  // ���������ļ�
			if(res == FR_OK)
			{
			    os_printf("finded file path = %s\n", FileOutPath);
			    res = f_open(&fileFIL, FileOutPath, FA_OPEN_EXISTING | FA_READ);	  //���ļ�
		        if(res == FR_OK)
		        {
		            uint32_t bytes_to_read = 0;
                    uint16_t sector_index = 0;   // �������
					const sfud_flash *flash = sfud_get_device_table() + 0;
					
					while(1)
					{
					    res = f_read(&fileFIL, spi_flash_buf, sizeof(spi_flash_buf), (UINT *)&bytes_to_read);  // ÿ�ζ�ȡ 4KB
				        if(res || bytes_to_read == 0)  /* �ļ��������� */
				        {
				            f_close(&fileFIL);
							
				            os_printf("read file: res = %d, br = %d\r\n", res, bytes_to_read);
				            // �ļ�ȫ����ȡ���, �� res = 0, ͬʱ bytes_to_read Ϊ 0
                            if(FR_OK == res && 0 == bytes_to_read)
							{
							    os_printf("read file all done\n"); 
	                            do
								{
								   T_APP_FLASH_ATTR appAttr;

								   // ɾ�������ļ�
								   res = f_unlink(FileOutPath);
                                   os_printf("delete file in %s %s\n", FileOutPath, (res == FR_OK ? "ok" : "failed"));
								   
								   // �ļ�����, ��������־λ
								   os_memset(&appAttr, 0, sizeof(T_APP_FLASH_ATTR));
	                               appAttr.fileLen = fileFIL.fsize;
								   appAttr.upgrade = 1;
								   appAttr.upgrade_inverse = ~(appAttr.upgrade);
								   Sys_WriteAppAttr(&appAttr);

								   os_printf("read file success, len = %d, file_size = %ld Bytes\r\n", bytes_to_read, fileFIL.fsize);
								   // ������λ
								   os_printf("write spi flash done, ready to jump to the bootloader...\n");
								   delay_ms(3000);
								   JumpToBootloader();
								}while(0);
							}
							break;
				        }
						else
						{
							// ��flash д��bin�ļ�, ��λ�ò������ļ�ϵͳ��, ��������
						    sfud_erase(flash, (FLASH_APP1_START_SECTOR + sector_index) << 12, 4096);     //�����������

							if(bytes_to_read < sizeof(spi_flash_buf))  // �ļ�����
							{
							   // ���߽�
							   os_memset(&spi_flash_buf[bytes_to_read], 0xCC, sizeof(spi_flash_buf) - bytes_to_read);
							}
		                    sfud_write(flash, (FLASH_APP1_START_SECTOR + sector_index) << 12, 4096, spi_flash_buf);
		                    sector_index++;
							
						    os_printf("read file success, len = %d, file_size = %ld Bytes, index = %d\r\n", 
								        bytes_to_read, fileFIL.fsize, sector_index);
						}
					}  
		        }
				else
				{
				    os_printf("can't open file, error = %d\n", res);
				}
			}
			#endif

			if(exf_getfree("0", &total, &free) == FR_OK)
		    {
		       os_printf("read fatfs file size success\r\n");
			   os_printf("total = %ld KB, free = %ld KB\n", total, free);
		    }
		}
	}	
}

extern long strtol(const char *str, char **endptr, int base);

// �õ��������ļ��а�������������Ŀ��
uint32_t FILE_GetSensorTotalItems(void)
{
   uint8_t res;
   uint32_t index = 0;
   
   if(FILE_Open(&fileFIL, SENSOR_TXT_FILE_NAME) == APP_FAILED) return APP_FAILED;
   if(fileFIL.fsize > 256)
   {
       res = f_lseek(&fileFIL, fileFIL.fsize - 150); // �ļ�ָ���Ƶ����һ������֮ǰ
       os_printf("1, res = %d\n", res);
   }
   else
   {
       res = f_lseek(&fileFIL, fileFIL.fsize); // �ļ�ָ���Ƶ����һ������֮ǰ
       os_printf("1, res = %d\n", res);
   }
   os_printf("f_lseek res = %d, fptr = %ld, fsize = %ld\n", res, fileFIL.fptr, fileFIL.fsize);
   if(res == FR_OK)
   {
      char buf[256];
	  uint32_t bytes_to_read = 0;
	  char * p1 = NULL;
	  char * p2 = NULL;
	  char * pEnd = NULL;
	  
	  	
      os_memset(buf, 0, sizeof(buf));
	  res = f_read(&fileFIL, buf, sizeof(buf), (UINT *)&bytes_to_read);  
	  if(res == FR_OK)
	  {
	     os_printf("buf = %s\n", buf);
	     p1 = os_strrchr((const char *)buf, '[');  // �������һ�γ��� '[' �ַ���λ��
	     if(p1)
	     {
	        p2 = os_strrchr(p1, ']');
			if(p2)
			{
			    // ����ŵ��ַ���ת��Ϊ����
			    index = strtol(&p1[1], &pEnd, 10);
				if(0 == index && pEnd == (char *)&p1[1])
				{
				   os_printf("convert failed, index = %d, pEnd = 0x%x, &p1[1] = 0x%x\n", index, (uint32_t)pEnd, (uint32_t)&p1[1]);
				   f_close(&fileFIL);
				   return 0;
				}
				index += 1;
				os_printf("had %ld items of sensor data\n", index);
			}
			else { os_printf("p2 null\n"); }
	     }
		 else { os_printf("p1 null\n"); }
	  }
	  else
	  {
         os_printf("line = %d read failed = %d\n", __LINE__, res);
	  }
   }
   f_close(&fileFIL);
   return index;
}






void FatFs_Demo(void)
{

   
   FILE_FormatDisk();

   
}


