
#include "stm32f1_temp_sensor.h"
#include "os_global.h"

#define TEMP_SENSOR_DMA_EN  0   // TEMP SENSOR DMA �Ƿ�ʹ��


/* Private define ------------------------------------------------------------*/
#define ADC1_DR_Address    ((uint32_t)0x4001244C)
__IO uint16_t ADCConvertedValue;


void ITempSensor_AdcInit(void)
{
   ADC_InitTypeDef   ADC_InitStructure;

   #if TEMP_SENSOR_DMA_EN
   DMA_InitTypeDef   DMA_InitStructure;
   #endif
   
   SET_REG_32_BIT(RCC->APB2ENR, RCC_APB2Periph_ADC1);     // ʹ��ADC1ʱ��
   SET_REG_32_BIT(RCC->APB2RSTR,   RCC_APB2Periph_ADC1);  // ADC1��λ
   CLEAR_REG_32_BIT(RCC->APB2RSTR, RCC_APB2Periph_ADC1);  // ��λ����	 

   /* DMA1 channel1 configuration ----------------------------------------------*/
  #if TEMP_SENSOR_DMA_EN
  DMA_DeInit(DMA1_Channel1);
  DMA_InitStructure.DMA_PeripheralBaseAddr = ADC1_DR_Address;
  DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t)&ADCConvertedValue;
  DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralSRC;
  DMA_InitStructure.DMA_BufferSize = 1;
  DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
  DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Disable;
  DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;
  DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;
  DMA_InitStructure.DMA_Mode = DMA_Mode_Circular;
  DMA_InitStructure.DMA_Priority = DMA_Priority_High;
  DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;
  DMA_Init(DMA1_Channel1, &DMA_InitStructure);
  
  /* Enable DMA1 channel 1 */
  DMA_Cmd(DMA1_Channel1, ENABLE);
  #endif
  
   /* ADC1 configuration ------------------------------------------------------*/
  
  ADC_InitStructure.ADC_Mode = ADC_Mode_Independent;  //����ģʽ

  #if TEMP_SENSOR_DMA_EN
  ADC_InitStructure.ADC_ScanConvMode = ENABLE;         //ʹ��ɨ��ģʽ 
  ADC_InitStructure.ADC_ContinuousConvMode = ENABLE;  //ʹ������ת��
  #else
  ADC_InitStructure.ADC_ScanConvMode = DISABLE;         // ��ɨ��ģʽ 
  ADC_InitStructure.ADC_ContinuousConvMode = DISABLE;  //�ر�����ת��
  #endif
  
  ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;  //��ֹ������⣬ʹ���������
  ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;   //�Ҷ���
  ADC_InitStructure.ADC_NbrOfChannel = 1;                    //1��ת���ڹ��������� Ҳ����ֻת����������1 
  ADC_Init(ADC1, &ADC_InitStructure);

  /* ADC1 regular channel 14 configuration */ 
  ADC_RegularChannelConfig(ADC1, ADC_Channel_16, 1, ADC_SampleTime_55Cycles5);

  /* Enable the temperature sensor and vref internal channel */
  #if 0
  ADC_TempSensorVrefintCmd(ENABLE);
  #else
  SET_REG_32_BIT(ADC1->CR2, ADC_CR2_TSVREFE);  
  #endif

  /* Enable ADC1 DMA */
#if  TEMP_SENSOR_DMA_EN
  #if 0
  ADC_DMACmd(ADC1, ENABLE);
  #else
  SET_REG_32_BIT(ADC1->CR2, ADC_CR2_DMA);  
  #endif
#endif
  
  /* Enable ADC1 */
  #if 0
  ADC_Cmd(ADC1, ENABLE);
  #else
  SET_REG_32_BIT(ADC1->CR2, ADC_CR2_ADON);  
  #endif

  /* Enable ADC1 reset calibaration register */   //ʹ��֮ǰһ��ҪУ׼
  ADC_ResetCalibration(ADC1);
  /* Check the end of ADC1 reset calibration register */
  while(ADC_GetResetCalibrationStatus(ADC1));

  /* Start ADC1 calibaration */
  ADC_StartCalibration(ADC1);
  /* Check the end of ADC1 calibration */
  while(ADC_GetCalibrationStatus(ADC1));

  #if TEMP_SENSOR_DMA_EN
  /* Start ADC1 Software Conversion */ 
  ADC_SoftwareStartConvCmd(ADC1, ENABLE);
  #endif
}

//���ADCֵ
//ch: @ref ADC_channels 
//ͨ��ֵ 0~16ȡֵ��ΧΪ��ADC_Channel_0~ADC_Channel_16
//����ֵ:ת�����
static uint16_t Get_TempSensorAdc(void)   
{
	SET_REG_32_BIT(ADC1->CR2, ADC_CR2_EXTTRIG | ADC_CR2_SWSTART);  //ʹ��ָ����ADC1�����ת����������	
	 
	while(!ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC ));//�ȴ�ת������

	return ADC_GetConversionValue(ADC1);	//�������һ��ADC1�������ת�����
}

//ȡtimes��,Ȼ��ƽ�� 
//times:��ȡ����
// �� Get_Adc_Average(ADC_Channel_4, 20);//��ȡͨ��5��ת��ֵ��20��ȡƽ��
//static 
uint16_t Get_Adc_Average(uint16_t times)
{
	uint32_t temp_val = 0;
	uint16_t single_val = 0;  // ����ֵ
	uint16_t t;
	
	for(t = 0; t < times; t++)
	{
	    single_val = Get_TempSensorAdc(); //Get_Adc(ch);
		temp_val += single_val;
		os_printf("adc val = %d, i = %d\n", single_val, t);
	}
	return temp_val / times;
} 

#include "os_timer.h"
static os_timer_t tTempSensorTimer;
#define MAX_TIMES   20  // 20 ��ƽ��
static uint32_t total_sum = 0;
static uint16_t get_adc_count = 0;
static uint16_t aver_temp, aver_adc = 0; // ƽ��ֵ
static void TempSensorTimer_CallBack(void * arg)
{
    uint32_t single_val = 0;

    //os_printf("temp sensor cb, tick = %ld\n", OS_GetSysTick());
	if(READ_REG_32_BIT(ADC1->SR, ADC_SR_EOC)) //ת������  
	{
	    single_val = (uint16_t)ADC1->DR;
		//os_printf("single_val = %d\n", single_val);
		get_adc_count++;
		total_sum += single_val;
		if(get_adc_count >= MAX_TIMES)
		{
		   aver_adc = total_sum / MAX_TIMES;

           // �¶ȼ��㹫ʽ: temp = (V25 - Vsense) / Avg_Slope + 25;
           // ����: V25 = 1.43V, Vsense = val * 3.3 / 4096, Avg_Slope = 4.3 mV = (4.3 / 1000) V
           aver_temp = (1.43 - (aver_adc * 3.3 / 4096.6)) * 1000 / 4.3 + 25;
		   os_printf("aver_adc = %d, aver_temp = %d 'C, tick = %ld\n", aver_adc, aver_temp, OS_GetSysTick());
		   
           // �� 0
		   total_sum = 0;
		   get_adc_count = 0;
		}
		SET_REG_32_BIT(ADC1->CR2, ADC_CR2_EXTTRIG | ADC_CR2_SWSTART);  // ������� AD ת��
	}
	else
	{
	  os_printf("adc conver not end\n");
	}
   	os_timer_arm(&tTempSensorTimer, 5, 0);  // 50 ms
}

void ITempSensor_Init(void)
{
    ITempSensor_AdcInit();
	SET_REG_32_BIT(ADC1->CR2, ADC_CR2_EXTTRIG | ADC_CR2_SWSTART);  // ������� AD ת��
	os_timer_setfn(&tTempSensorTimer, TempSensorTimer_CallBack, NULL);
	os_timer_arm(&tTempSensorTimer, 5, 0);  // 50 ms
}
