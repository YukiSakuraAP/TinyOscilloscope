#include "DAC.h"

//int DACbuff[2][50]={0};

void DAC_Dual_Init(void)
{
	RCC->APB2ENR|=1<<2;    //ʹ��PORTAʱ��	  	
	RCC->APB1ENR|=1<<29;   //ʹ��DACʱ��	  
	GPIOA->CRL&=0XFF00FFFF; 
	GPIOA->CRL|=0X00000000;//PA4 ģ������    

	DAC->CR|=1<<0;	//ʹ��DAC1
	DAC->CR|=0<<1;	//DAC1�������ʹ�� BOFF1=0  ���θ��ȶ�
	DAC->CR|=0<<2;	//��ʹ�ô������� TEN1=0
	DAC->CR|=0<<3;	//DAC TIM6 TRGO,����ҪTEN1=1����
	DAC->CR|=0<<6;	//��ʹ�ò��η���
	DAC->CR|=0<<8;	//���Ρ���ֵ����
	DAC->CR|=0<<12;	//DAC1 DMA��ʹ��    
	DAC->CR|=1<<16;	//ʹ��DAC2
	DAC->CR|=0<<17;	//DAC2�������ʹ�� BOFF1=0  ���θ��ȶ�
	DAC->CR|=0<<18;	//��ʹ�ô������� TEN1=0
	DAC->CR|=0<<19;	//DAC TIM6 TRGO,����ҪTEN1=1����
	DAC->CR|=0<<22;	//��ʹ�ò��η���
	DAC->CR|=0<<24;	//���Ρ���ֵ����
	DAC->CR|=0<<28;	//DAC2 DMA��ʹ��

	DAC->DHR12RD=0;
}
void DAC_Dual_DeInit(void)
{
	RCC->APB2ENR|=1<<2;    //ʹ��PORTAʱ��	  	
	RCC->APB1ENR|=1<<29;   //ʹ��DACʱ��	  
	GPIOA->CRL&=0XFF00FFFF; 
	GPIOA->CRL|=0X00000000;//PA4 ģ������    

	DAC->CR=0;

	DAC->DHR12RD=0;
}
