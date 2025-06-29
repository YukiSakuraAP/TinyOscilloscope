#include "LED.h"

void LED_Init(void)
{
//	GPIO_InitTypeDef dat;
//	dat.GPIO_Mode=GPIO_Mode_Out_PP;
//	dat.GPIO_Pin=GPIO_Pin_5;
//	dat.GPIO_Speed=GPIO_Speed_50MHz;
//	
//	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB|RCC_APB2Periph_GPIOE,ENABLE);
//	GPIO_Init(GPIOB,&dat);
//	GPIO_Init(GPIOE,&dat);
	
	RCC->APB2ENR |= RCC_APB2Periph_GPIOA;
	
	GPIOA->CRL &=0xFFFFFF00;
	GPIOA->CRL |=0x00000033;
}

