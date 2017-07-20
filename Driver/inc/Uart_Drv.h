
#ifndef __UART_DRV__
#define __UART_DRV__

#include <stdint.h>
#include "stm32f10x.h"
//#include <intrinsics.h>
#include "GlobalDef.h"
//#include "Util_Drv.h"
#include <string.h>



#define UART_Q_RX_BUF_SIZE   64
#define UART_Q_TX_BUF_SIZE  512

#define PM25_RX_BUF_SIZE   64
#define PM25_TX_BUF_SIZE   32

#define HCHO_RX_BUF_SIZE   64
#define HCHO_TX_BUF_SIZE   32

// ѡ��Ĵ��ں�
#define PM25_SEL   1
#define QUEUE_SEL  2
#define HCHO_SEL   3

#define PM25_UART   USART1
#define QUEUE_UART  USART2   // �������ѡ���Ӳ������
#define HCHO_UART   USART3





// ʹ�ܽ���PM25����
#define EnableRxPM25Sensor()   USART_ITConfig(PM25_UART, USART_IT_RXNE, ENABLE)

// ��ͣ����PM25����
#define DisableRxPM25Sensor()  USART_ITConfig(PM25_UART, USART_IT_RXNE, DISABLE)

extern UartStatus Uart_SendByte(uint8_t data);

extern void Queue_UART_IRQHandler(void);
extern void PM25_UART_IRQHandler(void);
extern void HCHO_UART_IRQHandler(void);


void USART1_Init(uint32_t baudrate);
void USART2_Init(uint32_t baudrate);
void USART3_Init(uint32_t baudrate);


#endif

