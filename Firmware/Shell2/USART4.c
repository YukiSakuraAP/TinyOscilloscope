#include "USART4.h"
#include "ArrayOperation.h"

u16 Uart4Left=0,Uart4Len=0,busy4=0;
u8 Uart4TxBuff[512];

u8 USART4_RX_BUF[USART4_REC_LEN];//���ջ���,���USART_REC_LEN���ֽ�.
u16 USART4_RX_STA=0;//����״̬���
//����״̬
//bit15��	������ɱ�־
//bit14��	���յ�0x0d
//bit13~0��	���յ�����Ч�ֽ���Ŀ

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
	temp=(float)(36000000)/(bound*16);//�õ�USARTDIV
	mantissa=temp;//�õ���������
	fraction=(temp-mantissa)*16; //�õ�С������	 
	mantissa<<=4;
	mantissa+=fraction;
	RCC->APB2ENR|=1<<4;//ʹ��PORTC��ʱ��
	RCC->APB1ENR|=1<<19;//ʹ�ܴ���ʱ�� 
	GPIOC->CRH&=0XFFFF00FF;//IO״̬����
	GPIOC->CRH|=0X00008B00;//IO_C11 C10״̬���� 
	RCC->APB1RSTR|=1<<19;//��λ����4
	RCC->APB1RSTR&=~(1<<19);//ֹͣ��λ	   	   
	//����������
	UART4->BRR=mantissa;// ����������	 
	UART4->CR1|=0X200C;//1λֹͣ,��У��λ.
	//ʹ�ܽ����ж� 
	UART4->CR1|=3<<5;    //���ջ������ǿ��ж�ʹ��	    	
	MY_NVIC_Init(1,3,UART4_IRQn,2);//��2��������ȼ� 
}

void UART4_IRQHandler(void)
{
	static u8 res,sta;
	sta=UART4->SR;
	UART4->SR=0;
	if(sta&(1<<5))	//���յ�����
	{
		res=UART4->DR;
		UART4->SR=0;
		if((USART4_RX_STA&0x8000)==0)//����δ���
		{
			if(USART4_RX_STA&0x4000)//���յ���0x0d
			{
				if(res!=0x0a)USART4_RX_STA=0;//���մ���,���¿�ʼ
				else USART4_RX_STA|=0x8000;	//��������� 
			}else //��û�յ�0X0D
			{	
				if(res==0x0d)USART4_RX_STA|=0x4000;
				else
				{
					USART4_RX_BUF[USART4_RX_STA&0X3FFF]=res;
					USART4_RX_STA++;
					if(USART4_RX_STA>(USART4_REC_LEN-1))USART4_RX_STA=0;//�������ݴ���,���¿�ʼ����	  
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
