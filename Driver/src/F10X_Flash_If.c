
#include "F10X_Flash_If.h"
#include "os_global.h"
#include "sfud.h"

extern uint8_t spi_flash_buf[];

// 写入APP 属性
// 成功: 返回0;  失败； 1
uint8_t F10X_FLASH_WriteAppAttr(uint32_t flashAddress, T_APP_FLASH_ATTR * newAttr)
{
   const sfud_flash *flash = sfud_get_device_table() + 0;
   T_APP_FLASH_ATTR * flashAttr = (T_APP_FLASH_ATTR *)spi_flash_buf;
   
   if(NULL == newAttr){ INSERT_ERROR_INFO(0); return FLASH_FAILED; }
   os_printf("write flashAddr = 0x%x, sector = %d\n", flashAddress, flashAddress >> 12);
   
   sfud_read(flash, flashAddress, 4096, spi_flash_buf);
   os_memcpy(spi_flash_buf, newAttr, sizeof(T_APP_FLASH_ATTR));
   
   flashAttr->checkSum		  =  Sys_GenSum32(flashAttr, sizeof(T_APP_FLASH_ATTR) - 8);
   flashAttr->checkSumInverse	=  ~(flashAttr->checkSum);
   os_printf("w: check = 0x%x, checkInverse = 0x%x\n", flashAttr->checkSum, flashAttr->checkSumInverse);
   
   sfud_erase(flash, flashAddress, 4096);
   sfud_write(flash, flashAddress, 4096, spi_flash_buf);

   sfud_read(flash, flashAddress, 4096, spi_flash_buf);
   os_printf("r: check = 0x%x, checkInverse = 0x%x\n", flashAttr->checkSum, flashAttr->checkSumInverse);
   
   return FLASH_SUCCESS;
}

// 读取APP 属性
// 成功: 返回0;  失败； 1
uint8_t F10X_FLASH_ReadAppAttr(uint32_t flashAddress, T_APP_FLASH_ATTR * outAttr)
{
    uint8_t buf[sizeof(T_APP_FLASH_ATTR)];
    uint32_t sumResult = 0;
    const sfud_flash *flash = sfud_get_device_table() + 0;

	if(NULL == outAttr){ INSERT_ERROR_INFO(0); return FLASH_FAILED; }
	os_printf("read flashAddr = 0x%x, sector = %d\n", flashAddress, flashAddress >> 12);
	
    sfud_read(flash, flashAddress, sizeof(buf), buf); 
	sumResult = Sys_GenSum32(buf, sizeof(T_APP_FLASH_ATTR) - 8);  // 除8个字节的校验码外, 其它都需校验
	os_memcpy(outAttr, buf, sizeof(T_APP_FLASH_ATTR));
	
	if(sumResult != outAttr->checkSum || (outAttr->checkSum != (~outAttr->checkSumInverse)))
	{
	   os_printf("read APP attr failed, newSum = 0x%x, checkSum = 0x%x, checkSumInverse = 0x%x\r\n", 
	   	           sumResult, outAttr->checkSum, outAttr->checkSumInverse);
	   return FLASH_FAILED;  // failed
	}

	return FLASH_SUCCESS;  // success
}

