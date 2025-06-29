#ifndef _Osc
#define _Osc

#include "sys.h"

//#define ADCbuffLenth 15000//ADC存储区最大长度，v2版本
#define ADCbuffLenth 8000//ADC存储区最大长度；v3为了节约内存，将内存分配给DAC，大幅缩减

extern u32 Osc_TrigLevel,Osc_SmitLevel,Osc_SamplingRate,Osc_WindowLenth,Osc_Sync;//触发电平，施密特触发偏差，采样率，窗长，同步模式
extern volatile u16 ADCbuff[ADCbuffLenth];//ADC存储区

extern u8 Msg_ADC_DMA,Msg_OscSetting;//“DMA传输完成消息”，“更新示波器设置消息”

void OscInit(u32 Rate,u32 Lenth);//示波器初始化（采样率，实际采样长度）
void ADC_DMA_Restart(void);//重启DMA传输（重启采样）

#endif
