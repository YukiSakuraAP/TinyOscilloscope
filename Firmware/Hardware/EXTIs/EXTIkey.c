#include "EXTIkey.h"
#include "usart.h"
void EXTIkey_Init(void)
{
	EXTI_InitTypeDef extInit;
	NVIC_InitTypeDef nvicInit;
	
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO,ENABLE);
	GPIO_EXTILineConfig(GPIO_PortSourceGPIOE,GPIO_PinSource2);
	
	extInit.EXTI_Line=EXTI_Line2;
	extInit.EXTI_LineCmd=ENABLE;
	extInit.EXTI_Mode=EXTI_Mode_Interrupt;
	extInit.EXTI_Trigger=EXTI_Trigger_Falling;
	EXTI_Init(&extInit);
	
	nvicInit.NVIC_IRQChannel=EXTI2_IRQn;
	nvicInit.NVIC_IRQChannelCmd=ENABLE;
	nvicInit.NVIC_IRQChannelPreemptionPriority=2;
	nvicInit.NVIC_IRQChannelSubPriority=2;
	NVIC_Init(&nvicInit);
	
}

u32 CNT_EXTI2falling=0;

void EXTI2_IRQHandler(void)
{
	CNT_EXTI2falling=1;
	EXTI_ClearITPendingBit(EXTI_Line2);
}
