
/***************************** SD CARD �洢�ӿڷ���ͷ�ļ� ****************************************************/
#ifndef __SD_CARD_INTERFACE_H__
#define  __SD_CARD_INTERFACE_H__

#include <stdint.h>

#include "diskio.h"
#include "Integer.h"

#define SD_CARD_DISK_SIZE            ((uint64_t)((uint64_t)2 * 1024 * 1024 * 1024))   // 2GB SD ��
#define SD_CARD_SECTOR_SIZE          512                           // ������С
#define SD_CARD_BLOCK_SIZE           512           // ���С

int32_t SD_initialize(void);
DRESULT SD_ioctl(BYTE lun, BYTE cmd, void *buff);

#endif

