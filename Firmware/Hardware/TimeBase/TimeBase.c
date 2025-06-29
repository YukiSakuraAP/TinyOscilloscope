#include "TimeBase.h"
#include "Key.h"
#include "DAC.h"

u32 sysTick=0;

//�˴�ֻ��ʼ��������Timer2
void Timer2_Init(void)//��ʱ��2��ʼ����1msʱ����
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
	timeInit.TIM_Prescaler=7-1;//����56MHzģʽ
	
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
void TIM2_IRQHandler(void)//1ms��ʱ����
{
//	u8 DACpoi;
	//TIM2->SR=0;
	if(TIM_GetITStatus(TIM2,TIM_IT_Update))
	{
		TIM_ClearITPendingBit(TIM2,TIM_IT_Update);
		sysTick++;
		if(sysTick%500==0)Msg_500msFlash++;//ÿ��500ms����һ������˸(main.c)
		if(CNT_ShowInfo!=0)CNT_ShowInfo++;//�˵���ʾ���Ʊ��������䲻Ϊ0ʱ��ÿ��1ģʽ������������5s���㣩(main.c)
		if(CNT_Clock)CNT_Clock--;
//		DACpoi=sysTick%50;
//		DAC_Output(DACbuff[0][DACpoi],DACbuff[1][DACpoi]);
		KeyMagic();//�����Զ���(Key.h)
	}
}
