#ifndef _DAC
#define _DAC

#include "sys.h"

//extern int DACbuff[2][50];
void DAC_Dual_Init(void);
void DAC_Dual_DeInit(void);
#define DAC_Output(X,Y) DAC->DHR12RD=(X)|(Y<<16)//PA4-A  PA5-B

#endif
