
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
char UpdateDirPath[32] = "0:/update";   // 升级文件路径
static char FileOutPath[64];

#if _USE_LFN
char Lfname[_MAX_LFN + 1];
#endif

FIL   fileFIL;


#include <string.h>


//得到磁盘剩余容量
//drv:磁盘编号("0:"/"1:")
//total:总容量	 （单位KB）
//free:剩余容量	 （单位KB）
//返回值:0,正常.其他,错误代码
FRESULT exf_getfree(uint8_t * drv, uint32_t * total, uint32_t *free)
{
	FATFS * fs1;
	FRESULT res;
    uint32_t fre_clust = 0, fre_sect = 0, tot_sect = 0;
	
    //得到磁盘信息及空闲簇数量
    res = f_getfree((const TCHAR*)drv, (DWORD*)&fre_clust, &fs1);
    if(res == FR_OK)
	{											   
	    tot_sect = (fs1->n_fatent - 2) * fs1->csize;	//得到总扇区数
	    fre_sect = fre_clust * fs1->csize;			//得到空闲扇区数	   
//#if _MAX_SS != 512				  			  
//		tot_sect* = fs1->ssize / 512;
//		fre_sect* = fs1->ssize / 512;
//#endif	  
        #if (SPI_FLASH_SECTOR_SIZE == 4096L)
		*total = tot_sect  << 2;	// x4, 单位为KB, 扇区大小为 4096 B
		*free  = fre_sect  << 2;	// x4, 单位为KB 
		#elif (SPI_FLASH_SECTOR_SIZE == 512L)
        *total = tot_sect  / 2;	//单位为KB, 这里的FatFs扇区大小为 512 B
		*free  = fre_sect  / 2;	//单位为KB 
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

// 文件的第0行显示传感器名称字符串
const char SensorNameItem[] = "Number	     Timestamp	     HCHO	  PM2.5  Temp  Humidity\r\n";

// 文件的第1行显示传感器数据单位
const char SensorUnitItem[] = "0-65535 Y-Mon-Day  HH:MM:SS  mg/m3  ug/m3  'C\r\n";

// 打开指定文件
E_RESULT FILE_Open(FIL * pFileFIL, char * file_name)
{
   FRESULT res;
   char sensor_dir[64] = "0:/sensor";        // 传感器的目录
   int len = 0;
   DIR dirs;
   uint8_t is_first_create = 0;  // 是否第一次创建
   	
   // 判断目录是否存在, 不存在则创建
   res = f_opendir(&dirs, (const TCHAR *)sensor_dir);  // 打开所在目录
   os_printf("open dir %s %s\n", sensor_dir, (res == FR_OK ? "ok" : "failed"));
   if(res == FR_NO_PATH)  
   {
       // 创建该目录
       res = f_mkdir((const TCHAR *)sensor_dir);
	   if(res != FR_OK){ os_printf("create dir %s failed, res = %d\n", sensor_dir, res); return APP_FAILED; }
	   else { os_printf("create dir %s success\n", sensor_dir); }
	   is_first_create = E_TRUE;
   }
   if(res){ INSERT_ERROR_INFO(res); return APP_FAILED; }
   f_closedir(&dirs);
   
   // 打开文件获取文件指针
   len = os_strlen(sensor_dir);
   sensor_dir[len] = '/';
   os_strncpy(&sensor_dir[len + 1], file_name, sizeof(sensor_dir));

   res = f_open(pFileFIL, (const TCHAR * )sensor_dir, FA_OPEN_ALWAYS | FA_WRITE | FA_READ);  // 打开文件, 可写
   if(is_first_create)
   {
      UINT bw = 0;
	  
      // 头两行提示单位
      res = f_write(pFileFIL, SensorNameItem, os_strlen(SensorNameItem), &bw);
      res = f_write(pFileFIL, SensorUnitItem, os_strlen(SensorUnitItem), &bw);
   }
   f_sync(pFileFIL);  // 最小化文件作用域, 同步文件数据, 防止被破坏
   os_printf("f_open %s, fsize = %d\n", (res == FR_OK ? "ok" : "failed"), pFileFIL->fsize);
   if(res){ INSERT_ERROR_INFO(res); return APP_FAILED;  }

   return APP_SUCCESS;
}

// 创建文件夹并写入传感器数据
E_RESULT FILE_Write(char * file_name, char * write_buf)
{
	FRESULT res;
	int  len = 0;
	UINT bw;  // 输出参数, 写入后长度 

   if(FILE_Open(&fileFIL, file_name) == APP_FAILED) return APP_FAILED;
   
   res = f_lseek(&fileFIL, fileFIL.fsize); // 文件指针移到最后
   os_printf("f_lseek res = %d, fptr = %ld, fsize = %ld\n", res, fileFIL.fptr, fileFIL.fsize);
   
   len = os_strlen(write_buf);
   res = f_write(&fileFIL, write_buf, len, &bw);
   f_close(&fileFIL);
   os_printf("f_write res = %d, len = %d, bw = %d\n", res, len, bw);
   
   return APP_SUCCESS;
}



#define RECURSIVE_EN   0  // 递归搜索使能: 1; 禁止: 0

FRESULT FILE_Scan(
	char * path,		   /* Pointer to the working buffer with start path */
	int   pathMaxLen,    /* the max length of the working buffer  */
	char * fileInName,   /*   待查找的文件名 */
	char * filePath,     // 如果查找成功, 则拷贝文件路径到此
	int filePathMaxLen // filePath buf 的最大长度
)
{
	FILINFO  Finfo;
	DIR dirs;
	FRESULT res;
	FRESULT result;
	char *fn;
    
    WORD AccFiles = 0;  /* AccFiles 个file 及AccDirs 个folders */

	#if RECURSIVE_EN
    WORD AccDirs = 0;		
    int len;                
    #endif
	
	DWORD AccSize = 0;				/* 文件中的字节数 */
	char * p = NULL;

	
	
#if _USE_LFN
	Finfo.lfname = Lfname;
	Finfo.lfsize = sizeof(Lfname);
#endif
	res = f_opendir(&dirs, path);
	if (res == FR_OK) 
	{
	    #if RECURSIVE_EN
		len = os_strlen(path);  // 不需要递归搜索
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
			if (Finfo.fattrib & AM_DIR)   // 是目录
			{
			    #if RECURSIVE_EN // 如需递归搜索打开这部分代码 , 并且FileOutPath 为外部静态数组
				AccDirs++;
				path[len] = '/'; 
				os_strncpy(path + len + 1, fn, pathMaxLen);
				//记录文件目录

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
			else   // 是文件
			{
				AccFiles++;
				AccSize += Finfo.fsize;

				//记录文件
				os_snprintf(filePath, filePathMaxLen, "%s/%s\r\n", path, fn);  // 文件路径最长为256 B
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


	// 挂载磁盘
	res = f_mount(&FlashDiskFatFs, (TCHAR const*)FlashDiskPath, 1);
	os_printf("f_mount = %d\n", res);
	
	if (res == FR_NO_FILESYSTEM)  // FAT文件系统错误,重新格式化FLASH
	{
	    os_printf("FatFs Error, do Format...\r\n");
		res = f_mkfs((TCHAR const*)FlashDiskPath, 1, 4096);  //格式化FLASH,0,盘符; 1,不需要引导区,8个扇区为1个簇
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
			res = FILE_Scan(UpdateDirPath, sizeof(UpdateDirPath), "Update.bin", FileOutPath, sizeof(FileOutPath));  // 查找升级文件
			if(res == FR_OK)
			{
			    os_printf("finded file path = %s\n", FileOutPath);
			    res = f_open(&fileFIL, FileOutPath, FA_OPEN_EXISTING | FA_READ);	  //打开文件
		        if(res == FR_OK)
		        {
		            uint32_t bytes_to_read = 0;
                    uint16_t sector_index = 0;   // 扇区序号
					const sfud_flash *flash = sfud_get_device_table() + 0;
					
					while(1)
					{
					    res = f_read(&fileFIL, spi_flash_buf, sizeof(spi_flash_buf), (UINT *)&bytes_to_read);  // 每次读取 4KB
				        if(res || bytes_to_read == 0)  /* 文件结束错误 */
				        {
				            f_close(&fileFIL);
							
				            os_printf("read file: res = %d, br = %d\r\n", res, bytes_to_read);
				            // 文件全部读取完成, 则 res = 0, 同时 bytes_to_read 为 0
                            if(FR_OK == res && 0 == bytes_to_read)
							{
							    os_printf("read file all done\n"); 
	                            do
								{
								   T_APP_FLASH_ATTR appAttr;

								   // 删除升级文件
								   res = f_unlink(FileOutPath);
                                   os_printf("delete file in %s %s\n", FileOutPath, (res == FR_OK ? "ok" : "failed"));
								   
								   // 文件结束, 置升级标志位
								   os_memset(&appAttr, 0, sizeof(T_APP_FLASH_ATTR));
	                               appAttr.fileLen = fileFIL.fsize;
								   appAttr.upgrade = 1;
								   appAttr.upgrade_inverse = ~(appAttr.upgrade);
								   Sys_WriteAppAttr(&appAttr);

								   os_printf("read file success, len = %d, file_size = %ld Bytes\r\n", bytes_to_read, fileFIL.fsize);
								   // 启动复位
								   os_printf("write spi flash done, ready to jump to the bootloader...\n");
								   delay_ms(3000);
								   JumpToBootloader();
								}while(0);
							}
							break;
				        }
						else
						{
							// 往flash 写入bin文件, 该位置并不在文件系统上, 对外隐藏
						    sfud_erase(flash, (FLASH_APP1_START_SECTOR + sector_index) << 12, 4096);     //擦除这个扇区

							if(bytes_to_read < sizeof(spi_flash_buf))  // 文件结束
							{
							   // 填充边界
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

// 得到传感器文件中包含的数据总条目数
uint32_t FILE_GetSensorTotalItems(void)
{
   uint8_t res;
   uint32_t index = 0;
   
   if(FILE_Open(&fileFIL, SENSOR_TXT_FILE_NAME) == APP_FAILED) return APP_FAILED;
   if(fileFIL.fsize > 256)
   {
       res = f_lseek(&fileFIL, fileFIL.fsize - 150); // 文件指针移到最后一条数据之前
       os_printf("1, res = %d\n", res);
   }
   else
   {
       res = f_lseek(&fileFIL, fileFIL.fsize); // 文件指针移到最后一条数据之前
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
	     p1 = os_strrchr((const char *)buf, '[');  // 查找最后一次出现 '[' 字符的位置
	     if(p1)
	     {
	        p2 = os_strrchr(p1, ']');
			if(p2)
			{
			    // 将序号的字符串转化为数字
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


