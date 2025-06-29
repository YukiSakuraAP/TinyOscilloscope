#ifndef _USART4_
#define _USART4_

#include "sys.h"
#include "delay.h"

#define USART4_REC_LEN 200 //�����������ֽ��� 200
extern u8  USART4_RX_BUF[USART4_REC_LEN]; //���ջ���,���USART_REC_LEN���ֽ�.ĩ�ֽ�Ϊ���з� 
extern u16 USART4_RX_STA; //����״̬���	

void uart4_init(u32 bound);
unsigned char isUart4Left(void);
void SendBuff4(unsigned char *s,unsigned char Len);
void SendString4(unsigned char *s);

#endif
