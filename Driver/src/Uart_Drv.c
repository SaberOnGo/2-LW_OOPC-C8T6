
#include "Uart_Drv.h"
#include "stm32f10x.h"
#include "GlobalDef.h"
#include "os_global.h"
#include "PM25Sensor.h"

void USART1_Init(uint32_t baudrate)
{
   
    USART_InitTypeDef USART_InitStructure;
    GPIO_InitTypeDef GPIO_InitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;
  
	//RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);
	
     //USART1_TX   PA.9
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
   
    //USART1_RX	  PA.10
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOA, &GPIO_InitStructure);  

    //Usart1 NVIC 配置
    /* Configure the NVIC Preemption Priority Bits */  
    NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 3;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;		

	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			//IRQ通道使能
	NVIC_Init(&NVIC_InitStructure);	

    // 串口外设初始化
    USART_InitStructure.USART_BaudRate = baudrate;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_Parity = USART_Parity_No;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
    USART_Init(USART1, &USART_InitStructure);
	
    USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);  // 接收中断使能
    //USART_ITConfig(USART1, USART_IT_TXE, ENABLE);
    
    USART_Cmd(USART1, ENABLE);                    
}


#if 1
#include <stdio.h>
#include "uart_queue.h"
//FILE __stdout;  

int fputc(int ch, FILE *f)
{
  /* Place your implementation of fputc here */
  /* e.g. write a character to the USART */
  #if 1
  USART_SendData(QUEUE_UART, (uint8_t)ch);

  /* Loop until the end of transmission */
  //while (USART_GetFlagStatus(QUEUE_UART, USART_FLAG_TXE) == RESET);
  while (!(QUEUE_UART->SR & USART_FLAG_TXE));
  #else
  Uart_SendByte(ch);
  #endif
  
  return ch;
}
#endif

void USART2_Init(uint32_t baudrate)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;
	
	/* config USART2 clock */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);
	
	/* USART2 GPIO config */
    /* Configure USART2 Tx (PA.02) as alternate function push-pull */
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
	    
    /* Configure USART2 Rx (PA.03) as input floating */
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

	//Usart NVIC 配置
    /* Configure the NVIC Preemption Priority Bits */  
    NVIC_InitStructure.NVIC_IRQChannel = USART2_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 3;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 2;		

	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			//IRQ通道使能
	NVIC_Init(&NVIC_InitStructure);	
	
	/* USART2 mode config */
	USART_InitStructure.USART_BaudRate = baudrate;
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;
	USART_InitStructure.USART_StopBits = USART_StopBits_1;
	USART_InitStructure.USART_Parity = USART_Parity_No ;
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;	
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;

	USART_Init(USART2, &USART_InitStructure); 

	
	//USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);  
	//USART_ITConfig(USART2, USART_IT_TXE, ENABLE); 
    
		
	USART_Cmd(USART2, ENABLE);
}

void USART3_Init(uint32_t baudrate)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;
	
    //RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3, ENABLE);
	
	/* USART  GPIO config */
    /* Configure USART3  Tx  as alternate function push-pull */
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &GPIO_InitStructure);
	    
    /* Configure USART3 Rx  as input floating */
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

	//Usart NVIC 配置
    /* Configure the NVIC Preemption Priority Bits */  
    NVIC_InitStructure.NVIC_IRQChannel = USART3_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 3;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;		

	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			//IRQ通道使能
	NVIC_Init(&NVIC_InitStructure);	

	
	/* USART mode config */
	USART_InitStructure.USART_BaudRate = baudrate;
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;
	USART_InitStructure.USART_StopBits = USART_StopBits_1;
	USART_InitStructure.USART_Parity = USART_Parity_No ;
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;	
	USART_InitStructure.USART_Mode = USART_Mode_Rx; //| USART_Mode_Tx;

	USART_Init(USART3, &USART_InitStructure); 
	
	USART_ITConfig(USART3, USART_IT_RXNE, ENABLE);  
	//USART_ITConfig(USART3, USART_IT_TXE, ENABLE); 

	USART_Cmd(USART3, ENABLE);
}

#if 1
void USART1_IRQHandler(void)
{ 
   #if  (PM25_SEL == 1)
   PM25_UART_IRQHandler();
   #elif (HCHO_SEL == 1)
   HCHO_UART_IRQHandler();
   #elif (QUEUE_SEL == 1)
   Queue_UART_IRQHandler();
   #endif
}

void USART2_IRQHandler(void)
{
   #if  (PM25_SEL == 2)
   PM25_UART_IRQHandler();
   #elif (HCHO_SEL == 2)
   HCHO_UART_IRQHandler();
   #elif (QUEUE_SEL == 2)
   Queue_UART_IRQHandler();
   #endif

}

void USART3_IRQHandler(void)
{
   #if  (PM25_SEL == 3)
   PM25_UART_IRQHandler();
   #elif (HCHO_SEL == 3)
   HCHO_UART_IRQHandler();
   #elif (QUEUE_SEL == 3)
   Queue_UART_IRQHandler();
   #endif
}
#endif





