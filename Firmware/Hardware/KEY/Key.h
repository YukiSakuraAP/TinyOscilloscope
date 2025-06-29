#ifndef _KEY
#define _KEY
//�����������������ֱ��ǣ�PA0 PE2 PE3 PE4

#include "stm32f10x.h"
void KEY_Init(void);
void KeyMagic(void);//�����Զ�������ӵ�1msʱ������
u8 getKey(u8 key,u16 delay);//��ȡ�������������������(���º궨��)������ʱ������
void resetKey(void);

#define Key0 1
#define Key1 2
#define Key2 4
#define Key3 8

struct key
{
	u8 Key;
	u16 Count;
	u8 lastKey;
};
extern struct key Keys;
#define KeyDelay 10;


#endif
