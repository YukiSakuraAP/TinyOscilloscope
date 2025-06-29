#ifndef _KEY
#define _KEY
//开发板上三个按键分别是：PA0 PE2 PE3 PE4

#include "stm32f10x.h"
void KEY_Init(void);
void KeyMagic(void);//按键自动机，添加到1ms时基函数
u8 getKey(u8 key,u16 delay);//获取按键按下情况（按键号(如下宏定义)，长按时间间隔）
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
