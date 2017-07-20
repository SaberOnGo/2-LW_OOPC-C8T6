
#ifndef __DHT11_H__
#define  __DHT11_H__

#include "GlobalDef.h"

// DHT11 IO �ܽ� -->   PB1
#define DHT_IO_Pin         GPIO_Pin_1
#define DHT_IO_PORT        GPIOB
#define DHT_IO_READ()      READ_REG_32_BIT(DHT_IO_PORT->IDR, DHT_IO_Pin)   
#define DHT_IO_H()         SET_REG_32_BIT(DHT_IO_PORT->BSRR, DHT_IO_Pin)  // �����, GPIOx->BSRR = GPIO_Pin;
#define DHT_IO_L()         SET_REG_32_BIT(DHT_IO_PORT->BRR,  DHT_IO_Pin)  // �����  GPIOx->BRR = GPIO_Pin;

// DHT ��Ӧ������
typedef enum
{
   DHT_OK = 0,
   DHT_INIT_OK,      // 1 ��ʼ���ɹ�
   DHT_NO_RESP,     // ������ ��DHT ��Ӧ
   DHT_PD_TIMEOUT,  // DHT ���ͳ�ʱ
   DHT_PU_TIMEOUT,  // DHT ���߳�ʱ
   DHT_BIT_ERR,     // DHT ����λ����
   DHT_CHECK_ERR,  // У�����
   DHT_NOT_INIT,  // ��δ��ʼ��
   DHT_ERR,
}DHT_RESULT;

typedef struct
{
   uint8_t humi_H;
   uint8_t humi_L;
   uint8_t temp_H;
   uint8_t temp_L;
   uint8_t sum;
}T_DHT;

extern T_DHT tDHT;

void DHT_Init(void);
void DHT_Start(void);

#endif

