#include "USART4.h"
#include "ArrayOperation.h"

u16 Uart4Left=0,Uart4Len=0,busy4=0;
u8 Uart4TxBuff[512];

u8 USART4_RX_BUF[USART4_REC_LEN];//接收缓冲,最大USART_REC_LEN个字节.
u16 USART4_RX_STA=0;//接收状态标记
//接收状态
//bit15，	接收完成标志
//bit14，	接收到0x0d
//bit13~0，	接收到的有效字节数目

unsigned char isUart4Left(void)
{
	return Uart4Len-Uart4Left+busy4;
}

void SendString4(unsigned char *s)
{
	unsigned char i=Uart4Len;
    while (*s)//Check the end of the string
    {
		Uart4TxBuff[i]=*s;
		i++;
		s++;
    }
	if(!isUart4Left())
	{
		UART4->DR=Uart4TxBuff[0];
		Uart4Left++;
	}
	Uart4Len=i;
    busy4 = 1;           //Clear transmit busy4 flag
}

void SendBuff4(unsigned char *s,unsigned char Len)
{
	unsigned char i=Uart4Len;
    while (Len)              //Check the end of the string
    {
		Uart4TxBuff[i]=*s;
		i++;
		s++;
        Len--;
    }
	if(!isUart4Left())
	{
		UART4->DR=Uart4TxBuff[0];
		Uart4Left++;
	}
	Uart4Len=i;
    busy4 = 1;           //Clear transmit busy4 flag
}

void uart4_init(u32 bound)
{//USART4_INIT
	float temp;
	u16 mantissa;
	u16 fraction;
	temp=(float)(36000000)/(bound*16);//得到USARTDIV
	mantissa=temp;//得到整数部分
	fraction=(temp-mantissa)*16; //得到小数部分	 
	mantissa<<=4;
	mantissa+=fraction;
	RCC->APB2ENR|=1<<4;//使能PORTC口时钟
	RCC->APB1ENR|=1<<19;//使能串口时钟 
	GPIOC->CRH&=0XFFFF00FF;//IO状态设置
	GPIOC->CRH|=0X00008B00;//IO_C11 C10状态设置 
	RCC->APB1RSTR|=1<<19;//复位串口4
	RCC->APB1RSTR&=~(1<<19);//停止复位	   	   
	//波特率设置
	UART4->BRR=mantissa;// 波特率设置	 
	UART4->CR1|=0X200C;//1位停止,无校验位.
	//使能接收中断 
	UART4->CR1|=3<<5;    //接收缓冲区非空中断使能	    	
	MY_NVIC_Init(1,3,UART4_IRQn,2);//组2，最低优先级 
}

void UART4_IRQHandler(void)
{
	static u8 res,sta;
	sta=UART4->SR;
	UART4->SR=0;
	if(sta&(1<<5))	//接收到数据
	{
		res=UART4->DR;
		UART4->SR=0;
		if((USART4_RX_STA&0x8000)==0)//接收未完成
		{
			if(USART4_RX_STA&0x4000)//接收到了0x0d
			{
				if(res!=0x0a)USART4_RX_STA=0;//接收错误,重新开始
				else USART4_RX_STA|=0x8000;	//接收完成了 
			}else //还没收到0X0D
			{	
				if(res==0x0d)USART4_RX_STA|=0x4000;
				else
				{
					USART4_RX_BUF[USART4_RX_STA&0X3FFF]=res;
					USART4_RX_STA++;
					if(USART4_RX_STA>(USART4_REC_LEN-1))USART4_RX_STA=0;//接收数据错误,重新开始接收	  
				}		 
			}
		}
	}
	if(sta&(1<<6))
	{
		if(Uart4Left<Uart4Len)
		{
			UART4->DR=Uart4TxBuff[Uart4Left];
			Uart4Left++;
		}
		else busy4=Uart4Len=Uart4Left=0;
	}
	sta=0;
}
