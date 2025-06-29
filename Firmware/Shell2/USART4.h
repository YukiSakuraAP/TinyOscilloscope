#ifndef _USART4_
#define _USART4_

#include "sys.h"
#include "delay.h"

#define USART4_REC_LEN 200 //定义最大接收字节数 200
extern u8  USART4_RX_BUF[USART4_REC_LEN]; //接收缓冲,最大USART_REC_LEN个字节.末字节为换行符 
extern u16 USART4_RX_STA; //接收状态标记	

void uart4_init(u32 bound);
unsigned char isUart4Left(void);
void SendBuff4(unsigned char *s,unsigned char Len);
void SendString4(unsigned char *s);

#endif
