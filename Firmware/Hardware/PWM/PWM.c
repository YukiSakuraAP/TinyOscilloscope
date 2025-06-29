#include "PWM.h"
#include "math.h"
//此处只初始化单独的Timer4
void Timer3_Init(void)
{
	TIM_TimeBaseInitTypeDef timeInit;
	TIM_OCInitTypeDef timeOCinit;
	NVIC_InitTypeDef nvicInit;
	
	GPIO_InitTypeDef dat;
	dat.GPIO_Mode=GPIO_Mode_AF_PP;
	dat.GPIO_Mode=GPIO_Mode_AIN;
	dat.GPIO_Pin=GPIO_Pin_7;
	dat.GPIO_Speed=GPIO_Speed_50MHz;
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA,ENABLE);
	GPIO_Init(GPIOA,&dat);
	
//	GPIO_InitTypeDef dat;
//	dat.GPIO_Mode=GPIO_Mode_AF_PP;
//	dat.GPIO_Pin=GPIO_Pin_5;
//	dat.GPIO_Speed=GPIO_Speed_50MHz;
//	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB|RCC_APB2Periph_AFIO,ENABLE);
//	GPIO_Init(GPIOB,&dat);
//	
	//GPIO_PinRemapConfig(GPIO_PartialRemap_TIM3, ENABLE); //Timer3?????  TIM3_CH2->PB5    
 
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3,ENABLE);
	
	timeInit.TIM_ClockDivision=TIM_CKD_DIV1;
	timeInit.TIM_CounterMode=TIM_CounterMode_Up;
	timeInit.TIM_Period=10000-1;
	timeInit.TIM_Prescaler=28-1;
	TIM_TimeBaseInit(TIM3 ,&timeInit);
	
	timeOCinit.TIM_OCMode=TIM_OCMode_PWM1;
	timeOCinit.TIM_OutputState=TIM_OutputState_Enable;
	timeOCinit.TIM_OCPolarity=TIM_OCPolarity_High;
	timeOCinit.TIM_Pulse=50;
	TIM_OC2Init(TIM3,&timeOCinit);
	
	TIM_CtrlPWMOutputs(TIM3,ENABLE);
	
	TIM_ITConfig(TIM3,TIM_IT_Update,ENABLE);
	
	//MY_NVIC_Init(1,3,TIM3_IRQn,2);
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
	nvicInit.NVIC_IRQChannel=TIM3_IRQn;
	nvicInit.NVIC_IRQChannelCmd=ENABLE;
	nvicInit.NVIC_IRQChannelPreemptionPriority=2;
	nvicInit.NVIC_IRQChannelSubPriority=2;
	NVIC_Init(&nvicInit);
	
	TIM_Cmd(TIM3,ENABLE);
}

void setPWM(u32 val)
{
	TIM_OCInitTypeDef timeOCinit;
	timeOCinit.TIM_OCMode=TIM_OCMode_PWM1;
	timeOCinit.TIM_OutputState=TIM_OutputState_Enable;
	timeOCinit.TIM_OCPolarity=TIM_OCPolarity_High;
	timeOCinit.TIM_Pulse=val;
	TIM_OC2Init(TIM3,&timeOCinit);
}

void TIM3_IRQHandler(void)
{
	static int i=0;
	if(TIM_GetITStatus(TIM3,TIM_IT_Update))
	{
		TIM_ClearITPendingBit(TIM3,TIM_IT_Update);
		i++;i%=3000;
		TIM_SetCompare2(TIM3,5000+5000*sin(6.28*i/3000));
	}
}
