#ifndef _TimeBase
#define _TimeBase
#include "sys.h"
#include "stm32f10x.h"
extern u32 sysTick;

void Timer2_Init(void);//定时器2初始化（1ms时基）

#endif
