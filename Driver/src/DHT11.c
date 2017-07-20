
#include "DHT11.h"
#include "delay.h"
#include "SDRR.h"
#include "os_timer.h"
#include "os_global.h"
#include "GlobalDef.h"

// �������ݿ�Ϊ��������
static void DHT_SetIODir(E_IO_DIR dir)
{
    GPIO_InitTypeDef GPIO_InitStructure;

	GPIO_InitStructure.GPIO_Pin = DHT_IO_Pin;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_10MHz;
	if(dir == INPUT)GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
    else GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
		
	GPIO_Init(DHT_IO_PORT, &GPIO_InitStructure);
}

static os_timer_t tTimerDHT;  
static os_timer_t tTimerGetTempHumi;  // ��ʱ��ȡһ����ʪ�ȵĶ�ʱ��
	
static uint8_t timing_count = 0;  // ��ʱ����
static uint8_t dht_sta = DHT_NOT_INIT; 
static E_BOOL dht_is_busy = E_FALSE;  // E_FALSE: ����; E_TRUE: æ
T_DHT tDHT;

static uint8_t DHT_RxByte(uint8_t * out)//����һ���ֽ�
{     
    uint8_t i;    
	uint8_t data = 0;  
	uint16_t timeout;
	uint16_t count = 0;  
	
    for (i = 0; i < 8; i++)    
	{   	 
        timeout = 0xFFFF;
		while((! DHT_IO_READ()) && timeout--) //�ȴ�50us�͵�ƽ��ʼ�����ź�
		{ 
           NOP();
		}
        if(! timeout){os_printf("line = %d, PD err\n", __LINE__);  return DHT_PD_TIMEOUT;  }
		
		timeout = 1000;
		count   = 0;
		while( DHT_IO_READ() && timeout--)   // ���ݸߵ�ƽʱ���ж��� bit 1 or bit0
		{
		   delay_us(10);
		   count++;   //����ߵ�ƽʱ��
		}
		if(! timeout){os_printf("line = %d, PU err\n", __LINE__);  return DHT_PU_TIMEOUT;  }
		data <<= 1;	//  �������ݸ�λ��ǰ   
		if(4 <= count && count < 9)  // 70 us Ϊ bit 1, 26 - 28 us Ϊ bit0
		{
		   data |= 1;
		}
		else if(count >= 8)
		{
		   os_printf("dht bit err count = %d\n", count);
		   return DHT_BIT_ERR;
		}
	}
	*out = data;
	return DHT_OK;
}   

#define DHT_LEAVE(sta)  { dht_sta = sta; dht_is_busy = E_FALSE; return;}

static uint32_t dht_time_count = 0;

extern uint8_t is_debug_on; // ���Դ�ӡ����

// �ص�����, ���� DHT11 ��������ms����ʱ�Ļص�
static void TimerDHT_CallBack(void * arg)
{
   if(timing_count == 0)
   {
      timing_count++;
      DHT_IO_L();
	  os_timer_arm(&tTimerDHT, 2, 0);  // DH11��ʼ�ź���Ҫ����18ms   
   }
   else if(timing_count == 1)
   {
      uint32_t timeout = 1000;  // 1 ms ��ʱ����
	  
      timing_count++;
	  DHT_IO_H();
	  delay_us(39);  // ���� ���� 20-40us, �ȴ� DHT ��Ӧ
      DHT_SetIODir(INPUT);

	  timeout = 0xFFFF;
      while( ( DHT_IO_READ()) && timeout--);  
	  if(! timeout){ DHT_LEAVE(DHT_PU_TIMEOUT); }
      if(! DHT_IO_READ())  // ��� DHT ������, ��ȴ�������
      {
         timeout = 0xFFFF;
         while( (! DHT_IO_READ()) && timeout--)  // DHT ����80 us ����
         {
            NOP();
         }
		 if(! timeout){ DHT_LEAVE(DHT_PD_TIMEOUT); }
		 dht_sta = DHT_INIT_OK;
		 do
		 {
		    uint8_t Temp_H, Temp_L, Humi_H, Humi_L, newSum = 0, Sum = 0;
            uint8_t res = 0;
			
			DHT_IO_H();
			timeout = 0xFFFF;
			while((DHT_IO_READ()) && timeout--)  //�ȴ�DHT80us����������
			{
			   NOP();
			}
			if(! timeout){ DHT_LEAVE(DHT_PU_TIMEOUT);}
			res = DHT_RxByte(&Humi_H);
			if(res){ INSERT_ERROR_INFO(0); DHT_LEAVE(DHT_ERR); }
			res = DHT_RxByte(&Humi_L);
			if(res){ INSERT_ERROR_INFO(0); DHT_LEAVE(DHT_ERR); }
			res = DHT_RxByte(&Temp_H);
			if(res){ INSERT_ERROR_INFO(0); DHT_LEAVE(DHT_ERR); }
			res = DHT_RxByte(&Temp_L);
			if(res){ INSERT_ERROR_INFO(0); DHT_LEAVE(DHT_ERR); }
			res = DHT_RxByte(&Sum);
			if(res){ INSERT_ERROR_INFO(0); DHT_LEAVE(DHT_ERR); }
			newSum = Humi_H + Humi_L + Temp_H + Temp_L;
			if(newSum != Sum)
			{
			   os_printf("---dht ck err----: new = 0x%x, sum = 0x%x, H_H = %d\tH_L = %d\tT_H = %d\tT_L = %d\n", 
			   	           newSum, Sum, Humi_H, Humi_L, Temp_H, Temp_L);
			   DHT_LEAVE(DHT_CHECK_ERR);
			}
			// ������ȷ
			if(OS_IsTimeout(dht_time_count))
			{
			    dht_time_count = OS_SetTimeout(SEC(10));
				if(is_debug_on)
				{	
				    os_printf("tick = %ld, dht ok: H_H = %d, H_L = %d, T_H = %d, T_L = %d\n", 
			            	    OS_GetSysTick(), Humi_H, Humi_L, Temp_H, Temp_L);
				}
		 	}
			// ��������
			tDHT.humi_H = Humi_H;
			tDHT.humi_L = Humi_L;
			tDHT.temp_H = Temp_H;
			tDHT.temp_L = Temp_L;
			tDHT.sum    = Sum;
			DHT_LEAVE(DHT_OK);
		 }while(0);
      }
	  else
	  {
		 DHT_LEAVE(DHT_NO_RESP);
	  }
   }
}

static uint32_t save_tick = 0;  // ����ʱ��
static void TimerGetTempHumi_CallBack(void * arg)
{
	if(dht_sta == DHT_OK)  // ��ȡ��ʪ�ȳɹ�
    {
       uint16_t val;

	   if(OS_IsTimeout(save_tick))
	   {
	      save_tick = OS_SetTimeout(SEC(60 * 1));
		  
		  // ������ʪ��
          val = tDHT.temp_H * 10;
          SDRR_SaveSensorPoint(SENSOR_TEMP, &val);
	      SDRR_SaveSensorPoint(SENSOR_HUMI, &tDHT.humi_H);
	   }
    }
	DHT_Start();
	os_timer_arm(&tTimerGetTempHumi, 50, 0);  // 500 ms ��ʱ
}

void DHT_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
  
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);  // PCLK2 = HCLK = 48MHz

    GPIO_InitStructure.GPIO_Pin   = DHT_IO_Pin;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_10MHz;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_Out_PP;
    GPIO_Init(DHT_IO_PORT, &GPIO_InitStructure);

	os_timer_setfn(&tTimerDHT, TimerDHT_CallBack, NULL);
	os_timer_setfn(&tTimerGetTempHumi, TimerGetTempHumi_CallBack, NULL);
	os_timer_arm(&tTimerGetTempHumi, 500, 0);  // 5 s ��

	DHT_Start();
}

// ���� DHT
void DHT_Start(void)
{
   if(dht_is_busy)return;
   timing_count = 0;         // ��ʱ���� �� 0
   dht_sta = DHT_NOT_INIT;
   DHT_SetIODir(OUTPUT);  // IO �ܽ� Ϊ���
   DHT_IO_H();
   os_timer_disarm(&tTimerDHT);
   os_timer_arm(&tTimerDHT, 1, 0);  // 10 ms
   dht_is_busy = E_TRUE;
}



