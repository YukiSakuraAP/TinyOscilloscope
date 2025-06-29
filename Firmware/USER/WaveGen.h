#ifndef _WaveGen
#define _WaveGen

#include "sys.h"
#include "DAC.h"

#define MaxDots 2048
extern u32 WavData[MaxDots];
extern int Wavlen;

#define XYc_OutputInv(X,Y) (((4095-(u16)(X))<<16)|(4095-(u16)(Y)))
#define XYc_Output(X,Y) ((((u16)(X))<<16)|((u16)(Y)))
	
void WaveGen_Init(void);
void SetTimer5BaseFreq(u32 Freq);
void WaveGenSine(u32 FreqX,u32 FreqY,u32 BaseFreq);
void WaveGenTriA(u32 FreqX,u32 FreqY,u32 BaseFreq);
void WaveGenTASW(u32 FreqX,u32 FreqY,u32 BaseFreq);
void BadApplePlayer(void);


void LissajousPlot(void);

#endif
