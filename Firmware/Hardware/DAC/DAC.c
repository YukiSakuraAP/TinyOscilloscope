#include "DAC.h"

//int DACbuff[2][50]={0};

void DAC_Dual_Init(void)
{
	RCC->APB2ENR|=1<<2;    //使能PORTA时钟	  	
	RCC->APB1ENR|=1<<29;   //使能DAC时钟	  
	GPIOA->CRL&=0XFF00FFFF; 
	GPIOA->CRL|=0X00000000;//PA4 模拟输入    

	DAC->CR|=1<<0;	//使能DAC1
	DAC->CR|=0<<1;	//DAC1输出缓存使能 BOFF1=0  波形更稳定
	DAC->CR|=0<<2;	//不使用触发功能 TEN1=0
	DAC->CR|=0<<3;	//DAC TIM6 TRGO,不过要TEN1=1才行
	DAC->CR|=0<<6;	//不使用波形发生
	DAC->CR|=0<<8;	//屏蔽、幅值设置
	DAC->CR|=0<<12;	//DAC1 DMA不使能    
	DAC->CR|=1<<16;	//使能DAC2
	DAC->CR|=0<<17;	//DAC2输出缓存使能 BOFF1=0  波形更稳定
	DAC->CR|=0<<18;	//不使用触发功能 TEN1=0
	DAC->CR|=0<<19;	//DAC TIM6 TRGO,不过要TEN1=1才行
	DAC->CR|=0<<22;	//不使用波形发生
	DAC->CR|=0<<24;	//屏蔽、幅值设置
	DAC->CR|=0<<28;	//DAC2 DMA不使能

	DAC->DHR12RD=0;
}
void DAC_Dual_DeInit(void)
{
	RCC->APB2ENR|=1<<2;    //使能PORTA时钟	  	
	RCC->APB1ENR|=1<<29;   //使能DAC时钟	  
	GPIOA->CRL&=0XFF00FFFF; 
	GPIOA->CRL|=0X00000000;//PA4 模拟输入    

	DAC->CR=0;

	DAC->DHR12RD=0;
}
