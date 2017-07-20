
#ifndef __LCD1602_DRV_H__
#define  __LCD1602_DRV_H__

#include "stm32f10x.h"

typedef  unsigned int uint;
typedef  unsigned char uchar;

#define LCD_PWR_SW_PIN   GPIO_Pin_3
#define LCD_PWR_SW_PORT  GPIOB
#define LCD_PWR_SW_H()   GPIO_SetBits(LCD_PWR_SW_PORT,   LCD_PWR_SW_PIN)
#define LCD_PWR_SW_L()   GPIO_ResetBits(LCD_PWR_SW_PORT, LCD_PWR_SW_PIN)


#define LCD_RS_PIN   GPIO_Pin_13   // PC13
#define LCD_RS_PORT  GPIOC
#define LCD_RS_H()   GPIO_SetBits(LCD_RS_PORT, LCD_RS_PIN)
#define LCD_RS_L()   GPIO_ResetBits(LCD_RS_PORT, LCD_RS_PIN)

#define LCD_RW_PIN   GPIO_Pin_9   // PB9
#define LCD_RW_PORT  GPIOB
#define LCD_RW_H()   GPIO_SetBits(LCD_RW_PORT, LCD_RW_PIN)
#define LCD_RW_L()   GPIO_ResetBits(LCD_RW_PORT, LCD_RW_PIN)


#define LCD_EN_PIN   GPIO_Pin_8   // PB8
#define LCD_EN_PORT  GPIOB
#define LCD_EN_H()   GPIO_SetBits(LCD_EN_PORT, LCD_EN_PIN)
#define LCD_EN_L()   GPIO_ResetBits(LCD_EN_PORT, LCD_EN_PIN)

// PB4 - PB7: LCD D7-D4
#define LCD_DATA_PIN   (GPIO_Pin_7 | GPIO_Pin_6 | GPIO_Pin_5 | GPIO_Pin_4)   
#define LCD_DATA_PORT  GPIOB
#define LCD_D7_WRITE(bitVal) GPIO_WriteBit(LCD_DATA_PORT, GPIO_Pin_4, ((bitVal) ? Bit_SET : Bit_RESET) )
#define LCD_D6_WRITE(bitVal) GPIO_WriteBit(LCD_DATA_PORT, GPIO_Pin_5, ((bitVal) ? Bit_SET : Bit_RESET) )
#define LCD_D5_WRITE(bitVal) GPIO_WriteBit(LCD_DATA_PORT, GPIO_Pin_6, ((bitVal) ? Bit_SET : Bit_RESET) )
#define LCD_D4_WRITE(bitVal) GPIO_WriteBit(LCD_DATA_PORT, GPIO_Pin_7, ((bitVal) ? Bit_SET : Bit_RESET) )

#define LCD_D7_READ()  GPIO_ReadInputDataBit(LCD_DATA_PORT, GPIO_Pin_4)
#define LCD_D6_READ()  GPIO_ReadInputDataBit(LCD_DATA_PORT, GPIO_Pin_5)
#define LCD_D5_READ()  GPIO_ReadInputDataBit(LCD_DATA_PORT, GPIO_Pin_6)
#define LCD_D4_READ()  GPIO_ReadInputDataBit(LCD_DATA_PORT, GPIO_Pin_7)
#define LCD_DATA_4BIT_READ()  (GPIO_ReadInputData(LCD_DATA_PORT) & 0x00F0)


#define LCD_DATA_4BIT_H()   GPIO_SetBits(LCD_DATA_PORT, LCD_DATA_PIN)    // 数据位 4bit 都置为 1
#define LCD_DATA_4BIT_L()   GPIO_ResetBits(LCD_DATA_PORT, LCD_DATA_PIN)

// 发送val的高4位值
#define LCD_DATA_4BIT_OUT(val)   {\
	LCD_D7_WRITE(val & 0x80);\
	LCD_D6_WRITE(val & 0x40);\
	LCD_D5_WRITE(val & 0x20);\
	LCD_D4_WRITE(val & 0x10);\
	}

#define RCC_APB2Periph_LCD_GPIO  RCC_APB2Periph_GPIOB


/***********************函数声明***********************************/
void LCD1602_HardwareInit(void);
void LCD1602_Init(void);
void LCD1602_SetXY(uint8_t x, uint8_t y );
void LCD1602_WriteCmd(uint8_t ins );
void LCD1602_WriteData(uint8_t data );
void LCD1602_WriteString(uint8_t x, uint8_t y, const uint8_t *s );
void LCD1602_WriteInteger(uint8_t x, uint8_t y, uint32_t val, uint8_t placeholder_size);
void LCD1602_ClearScreen(void);





#endif

