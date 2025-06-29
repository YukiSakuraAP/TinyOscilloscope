#include "TimeBase.h"
#include "Key.h"
#include "DAC.h"

u32 sysTick=0;

//此处只初始化单独的Timer2
void Timer2_Init(void)//定时器2初始化（1ms时基）
{
//	RCC->APB1ENR|=0x00000001;
//	RCC->APB1RSTR|=0x00000001;
//	RCC->APB1RSTR&=0xFFFFFFFE;
//	
	TIM_TimeBaseInitTypeDef timeInit;
	NVIC_InitTypeDef nvicInit;
	
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2,ENABLE);
	
//	TIM2->PSC=9-1;
//	TIM2->ARR=8000-1;
//	TIM2->DIER|=0x0001;
//	TIM2->SR=0x0000;
//	
	timeInit.TIM_ClockDivision=TIM_CKD_DIV1;
	timeInit.TIM_CounterMode=TIM_CounterMode_Down;
	timeInit.TIM_Period=8000-1;
	timeInit.TIM_Prescaler=7-1;//适配56MHz模式
	
	TIM_TimeBaseInit(TIM2 ,&timeInit);

//	TIM2->CR1=0x0080;
//	TIM2->SR=0x0000;
//	TIM2->CR1|=0x0001;
	
	TIM_ITConfig(TIM2,TIM_IT_Update,ENABLE);
	
	//MY_NVIC_Init(1,3,TIM2_IRQn,2);
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
	nvicInit.NVIC_IRQChannel=TIM2_IRQn;
	nvicInit.NVIC_IRQChannelCmd=ENABLE;
	nvicInit.NVIC_IRQChannelPreemptionPriority=2;
	nvicInit.NVIC_IRQChannelSubPriority=2;
	NVIC_Init(&nvicInit);
	
	TIM_Cmd(TIM2,ENABLE);
}

extern u32 Msg_500msFlash;
extern u32 CNT_ShowInfo,CNT_Clock;
void TIM2_IRQHandler(void)//1ms定时函数
{
//	u8 DACpoi;
	//TIM2->SR=0;
	if(TIM_GetITStatus(TIM2,TIM_IT_Update))
	{
		TIM_ClearITPendingBit(TIM2,TIM_IT_Update);
		sysTick++;
		if(sysTick%500==0)Msg_500msFlash++;//每隔500ms发送一条灯闪烁(main.c)
		if(CNT_ShowInfo!=0)CNT_ShowInfo++;//菜单显示控制变量，当其不为0时，每隔1模式自增（主函数5s清零）(main.c)
		if(CNT_Clock)CNT_Clock--;
//		DACpoi=sysTick%50;
//		DAC_Output(DACbuff[0][DACpoi],DACbuff[1][DACpoi]);
		KeyMagic();//按键自动机(Key.h)
	}
}
