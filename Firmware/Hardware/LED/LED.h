#ifndef _LED
#define _LED

#include "stm32f10x.h"
#include "sys.h"

void LED_Init(void);

#define LED0 PAout(0)
#define LED1 PAout(1)

#endif
