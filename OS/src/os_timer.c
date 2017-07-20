
#include "os_timer.h"



static T_OS_TIMER * head_handle = NULL;
static uint32_t timer_tick = 0;
	
void OS_TimerInit(T_OS_TIMER * timer, 
                                     timeout_callback timeout_func, 
                                     void * param,
                                     uint32_t time,
                                     uint8_t flag)
{
   timer->timeout_func = timeout_func;
   timer->param = param;
   timer->init_tick = time;
   timer->timeout_tick = Sys_GetRunTime() + time;
   timer->flag = flag;
}



OS_RESULT OS_TimerStart(T_OS_TIMER * timer)
{
   T_OS_TIMER * target = head_handle;
   while(target)
   {
      if(target == timer)
	  {
	     target->flag |= OS_TIMER_FLAG_ACTIVATED;
		 TIMER_DEBUG("timer 0x%lx existed\r\n", (uint32_t)(timer));
		 return OS_OK;
	  }
	  target = target->next;
   }
   
   timer->next = head_handle;
   head_handle = timer;  // pointe to the last timer
   timer->flag |= OS_TIMER_FLAG_ACTIVATED;
   	
   TIMER_DEBUG("handle = 0x%lx, flag = 0x%lx\r\n", (uint32_t)head_handle, (uint32_t)timer->flag);
   
   return OS_OK;
}

void OS_TimerStop(T_OS_TIMER * timer)
{
   T_OS_TIMER * cur;
   
   for(cur = head_handle; cur; cur = cur->next)
   {
      
      if(cur == timer)
      {
         cur->flag &= ~ OS_TIMER_FLAG_ACTIVATED;
         TIMER_DEBUG("stop timer: 0x%lx\r\n", (uint32_t)(cur));
      }
   }
}

// 判断定时器是否已停止: 1: 停止; 0: 已激活
uint8_t OS_TimerIsStop(T_OS_TIMER * timer)
{
   return ( !(timer->flag  & OS_TIMER_FLAG_ACTIVATED));
}

void OS_TimerCheck(void)
{
   T_OS_TIMER *cur = NULL;
   
   for(cur = head_handle; cur; cur = cur->next)
   {
      if(Sys_GetRunTime() >= cur->timeout_tick &&
	  	(cur->flag & OS_TIMER_FLAG_ACTIVATED))	
      {
		   TIMER_DEBUG("timer tick out: %ld ms\r\n", Sys_GetRunTime());
		   if(! (cur->flag & OS_TIMER_FLAG_PERIODIC))
		   {
		       cur->flag &= ~ OS_TIMER_FLAG_ACTIVATED;
		   }
		   else
		   {
		      cur->timeout_tick = Sys_GetRunTime() + cur->init_tick;
		   }
		   cur->timeout_func(cur->param);
      }
   }
}

void OS_TimerTickIncrease(uint32_t tick)
{
   timer_tick++;
}

#if 0
void OS_TimerTask(void)
{
   if(Sys_GetRunTime() > (timer_tick + 10))  // 10 ms
   {
      timer_tick = Sys_GetRunTime();
      OS_TimerCheck();
   }
}
#endif

#if 0
void os_timer_setfn(os_timer_t *timer, os_timer_func_t * func, void *parg)
{
   OS_TimerInit(timer, func, parg, 0, 0);
}
#endif

// tick 为系统滴答定时器定时间隔, 这里为10ms
void os_timer_arm(os_timer_t * timer, uint32_t tick, uint8_t is_repeat)
{
   timer->init_tick = tick;
   timer->timeout_tick = Sys_GetRunTime() + tick;
   if(is_repeat)
   {
      timer->flag |= OS_TIMER_FLAG_PERIODIC;  // timer repeat
   }
   OS_TimerStart(timer);
}



