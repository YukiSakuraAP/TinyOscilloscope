#ifndef _Osc
#define _Osc

#include "sys.h"

//#define ADCbuffLenth 15000//ADC�洢����󳤶ȣ�v2�汾
#define ADCbuffLenth 8000//ADC�洢����󳤶ȣ�v3Ϊ�˽�Լ�ڴ棬���ڴ�����DAC���������

extern u32 Osc_TrigLevel,Osc_SmitLevel,Osc_SamplingRate,Osc_WindowLenth,Osc_Sync;//������ƽ��ʩ���ش���ƫ������ʣ�������ͬ��ģʽ
extern volatile u16 ADCbuff[ADCbuffLenth];//ADC�洢��

extern u8 Msg_ADC_DMA,Msg_OscSetting;//��DMA���������Ϣ����������ʾ����������Ϣ��

void OscInit(u32 Rate,u32 Lenth);//ʾ������ʼ���������ʣ�ʵ�ʲ������ȣ�
void ADC_DMA_Restart(void);//����DMA���䣨����������

#endif
