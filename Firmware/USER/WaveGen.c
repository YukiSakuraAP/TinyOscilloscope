#include "math.h"
#include "LED.h"
#include "Key.h"
#include "TimeBase.h"
#include "oled.h"
#include "DAC.h"
#include "delay.h"
#include "usart.h"
#include "stdio.h"
#include "WaveGen.h"

u32 WavData[MaxDots];
int LDcnt=0,LDdir=1;
int Wavlen=MaxDots;

//�˴�ֻ��ʼ��������Timer5
void Timer5_Init(void)//��ʱ��5��ʼ��
{
	TIM_TimeBaseInitTypeDef timeInit;
	NVIC_InitTypeDef nvicInit;
	
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM5,ENABLE);
	
	timeInit.TIM_ClockDivision=TIM_CKD_DIV1;
	timeInit.TIM_CounterMode=TIM_CounterMode_Down;
	timeInit.TIM_Period=8000-1;
	timeInit.TIM_Prescaler=7-1;//����56MHzģʽ
	
	TIM_TimeBaseInit(TIM5 ,&timeInit);
	TIM_ITConfig(TIM5,TIM_IT_Update,ENABLE);
	
	//MY_NVIC_Init(1,3,TIM2_IRQn,2);
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
	nvicInit.NVIC_IRQChannel=TIM5_IRQn;
	nvicInit.NVIC_IRQChannelCmd=ENABLE;
	nvicInit.NVIC_IRQChannelPreemptionPriority=2;
	nvicInit.NVIC_IRQChannelSubPriority=2;
	NVIC_Init(&nvicInit);
	
	TIM_Cmd(TIM5,ENABLE);
}

void WaveGen_Init(void)
{
	Timer5_Init();
}

void SetTimer5BaseFreq(u32 Freq)
{
	if(Freq==0)Freq=1;
	if(Freq<100){TIM5->PSC=1000-1;TIM5->ARR=56000/Freq-1;}
	else if(Freq<   10000){TIM5->PSC=10-1;TIM5->ARR=5600000/Freq-1;}
	else if(Freq<=1000000){TIM5->PSC=1-1;TIM5->ARR=56000000/Freq-1;}
	else {TIM5->PSC=1-1;TIM5->ARR=56-1;}//��Ƶ��1MHz
}

void WaveGenSine(u32 FreqX,u32 FreqY,u32 BaseFreq)
{
	u32 GenDiv=2,i,FreqXYbase;
	if(BaseFreq<(FreqX*2))return;//������
	if(BaseFreq<(FreqY*2))return;//������
	FreqXYbase=((FreqX<FreqY)?FreqX:FreqY);
	GenDiv=BaseFreq/FreqXYbase;//ѡȡ���Ƶ����Ϊ��׼
	if(GenDiv>MaxDots)GenDiv=MaxDots;//����̫�಻����
	SetTimer5BaseFreq(BaseFreq);
	Wavlen=3;
	for(i=0;i<=GenDiv;i++)
	{
		if(FreqX<FreqY)
			WavData[i]=XYc_Output(2048+2000*sin(3.1415926*2*i/GenDiv),2048+2000*sin(3.1415926*2*i*FreqY/FreqX/GenDiv));
		else
			WavData[i]=XYc_Output(2048+2000*sin(3.1415926*2*i*FreqX/FreqY/GenDiv),2048+2000*sin(3.1415926*2*i/GenDiv));
	}
	Wavlen=GenDiv;
}

#define Abs(x) ((x)>0?(x):-(x))
void WaveGenTriA(u32 FreqX,u32 FreqY,u32 BaseFreq)
{
	s32 GenDiv=2,i,FreqXYbase,HalfDiv=0;
	float XY=FreqX/FreqY;
	float YX=FreqY/FreqX;
	if(BaseFreq<(FreqX*2))return;//������
	if(BaseFreq<(FreqY*2))return;//������
	FreqXYbase=((FreqX<FreqY)?FreqX:FreqY);
	GenDiv=BaseFreq/FreqXYbase;//ѡȡ���Ƶ����Ϊ��׼
	if(GenDiv>MaxDots)GenDiv=MaxDots;//����̫�಻����
	SetTimer5BaseFreq(BaseFreq);
	Wavlen=3;
	HalfDiv=GenDiv/2;
	for(i=0;i<=GenDiv;i++)
	{
		if(FreqX<FreqY)
			WavData[i]=XYc_Output(24+4000*Abs(i-HalfDiv)/HalfDiv,24+4000*YX*Abs((int)(i*YX)%GenDiv-HalfDiv)/GenDiv);
		else
			WavData[i]=XYc_Output(24+4000*XY*Abs((int)(i*XY)%GenDiv-HalfDiv)/GenDiv,24+4000*Abs(i-HalfDiv)/HalfDiv);
	}
	Wavlen=GenDiv;
}

void WaveGenTASW(u32 FreqX,u32 FreqY,u32 BaseFreq)
{
	s32 GenDiv=2,i,FreqXYbase,HalfDiv=0;
	float XY=FreqX/FreqY;
	float YX=FreqY/FreqX;
	if(BaseFreq<(FreqX*2))return;//������
	if(BaseFreq<(FreqY*2))return;//������
	FreqXYbase=((FreqX<FreqY)?FreqX:FreqY);
	GenDiv=BaseFreq/FreqXYbase;//ѡȡ���Ƶ����Ϊ��׼
	if(GenDiv>MaxDots)GenDiv=MaxDots;//����̫�಻����
	SetTimer5BaseFreq(BaseFreq);
	Wavlen=3;
	HalfDiv=GenDiv/2;
	for(i=0;i<=GenDiv;i++)
	{
		if(FreqX<FreqY)
			WavData[i]=XYc_Output(24+4000*(i)/GenDiv,2048+2000*sin(3.1415926*2*i*YX/GenDiv));
		else
			WavData[i]=XYc_Output(2048+2000*sin(3.1415926*2*i*XY/GenDiv),24+4000*(i)/GenDiv);
	}
	Wavlen=GenDiv;
}

//void OscDispAutoMagic(unsigned char *Data,u32 SizeX,u32 SizeY,u32 OffsetX,u32 OffsetY)
void OscDispAutoMagic()
{
	static u32 pos=0;
	static s32 x=0,y=0,z=0;
	//ʱ�䷨
//	while(1)
//	{
//		x=pos%SizeX;y=pos/SizeY;//ת��Ϊ��ģ����
//		z=Data[y*(SizeX/8)+x/8]&(1<<(x%8));
//		PAout(8)=z;
//		if(z)//ʡ�Կ�λ������
//		{
//			PAout(8)=1;
//			delay_us(15);
//			PAout(8)=0;
//			DAC_Output((4000-y)*24+OffsetY,0+(4000-x)*24+OffsetX);//�Ŵ����壬ת������
//			pos++;pos%=SizeX*SizeY;
//			break;
//		}
//		else PAout(8)=0;
//		pos++;pos%=SizeX*SizeY;
//	}
//	//ʱ�䷨�Ľ�
//	while(1)
//	{
//		x+=xDir;
//		if((x<0)||(x>=SizeX))
//		{
//			xDir=-xDir;
//			x+=xDir;
//			y+=yDir;
//			if((y<0)||(y>=SizeY))
//			{
//				yDir=-yDir;
//				y+=yDir;
//			}
//		}
//		z=Data[y*(SizeX/8)+x/8]&(1<<(x%8));
//		PAout(8)=z;
//		if(z)//ʡ�Կ�λ������
//		{
//			PAout(8)=1;
//			delay_us(15);
//			PAout(8)=0;
//			DAC_Output((4000-y)*24+OffsetY,0+(4000-x)*24+OffsetX);//�Ŵ����壬ת������
//			break;
//		}
//		else PAout(8)=0;
//	}
	//XY-Z��
//	x=pos%SizeX;y=pos/SizeY;//ת��Ϊ��ģ����
//	PAout(8)=Data[y*(SizeX/8)+x/8]&(1<<(x%8));
//	DAC_Output((y)*24+OffsetY,0+(x)*24+OffsetX);//�Ŵ����壬ת������
//	pos++;pos%=SizeX*SizeY;
	
	//��Բ
//	DAC_Output(2048+1800*sin(pos*3.14/30),2048+1800*cos(pos*3.14/30));
//	pos++;
//	PAout(8)=((pos%60)<50);
	//��Sin(2����)
//	DAC_Output(2048+1800*sin(pos*3.14/30),(pos%120)*30);
//	pos++;
//	PAout(8)=((pos%120)>5);

	//�߶η�
	DAC->DHR12RD=WavData[LDcnt];
	LDcnt++;if(LDcnt>=Wavlen)LDcnt=0;
	
//	//��ת�߶η�
//	if(LDcnt<=0){LDcnt=1;LDdir=1;}
//	else if(LDcnt>=(Wavlen-1)){LDcnt=(Wavlen-2);LDdir=-1;}
//	else LDcnt+=LDdir;
//	DAC->DHR12RD=WavData[LDcnt];

}

void TIM5_IRQHandler(void)//1ms��ʱ����
{
	if(TIM5->SR)
	{
		TIM5->SR=0;
		OscDispAutoMagic();
	}
}

extern int Msg_PathFin;
#define DPMax2 (MaxDots*2)
extern u8 DotPath[DPMax2];
static u8 DispBuff[32];//��ʾ�����õĻ���

void BadApplePlayer(void)
{
	s32 i=0;
	u8 Msg_Refresh=1;
	u8 OE=1,Zoom=6;
	u16 X,Y;
	
	reInit:
	i=0;
	Msg_Refresh=1;
	OE=1;Zoom=6;
	OLED_Clear();
	OLED_DrawLine(64,0,64,64);
	OLED_Refresh_Gram();
	
	for(i=0;i<100;i++)
	{
		WavData[i]=XYc_OutputInv(40.95*i,40.95*i);
	}
	Wavlen=i;
	for(;i<200;i++)
	{
		WavData[i]=XYc_OutputInv(8190-40.95*i,4095);
	}
	Wavlen=i;
	for(;i<300;i++)
	{
		WavData[i]=XYc_OutputInv(40.95*i-8190,4095+8190-40.95*i);
	}
	Wavlen=i;
	
	Mod_Recv=Recv_Pack;
	
	while(1)
	{
		LED1=(sysTick%1000)<500;
		if((sysTick%100)<5){USART1->DR='A';delay_ms(6);}
		if(Msg_PathFin)
		{
			LED0=!LED0;
			Wavlen=3;
			for(i=0;i<Msg_PathFin;i++)
			{
				X=DotPath[2*i]<<Zoom;
				Y=DotPath[2*i+1]<<Zoom;
				WavData[i]=XYc_OutputInv(X,Y);
			}
			if(Msg_PathFin>=MaxDots)Wavlen=MaxDots;
			else if(Msg_PathFin>=3)Wavlen=Msg_PathFin;
			else Wavlen=Msg_PathFin+2;
//			WaveGen=3;
//			WavRecvLen=Msg_PathFin;
			SetTimer5BaseFreq(12500);
			Msg_PathFin=0;
//			Msg_OscSetting++;
			Msg_Refresh++;
		}
		if(getKey(Key0,500))
		{
			OE=!OE;
			Msg_Refresh++;
		}
		if(getKey(Key1,500))
		{
			Zoom++;
			Zoom%=8;
			Msg_Refresh++;
		}
		if(getKey(Key2,500))
		{
			goto reInit;
		}
		if(getKey(Key3,500))
		{
			return;
		}
		
		if(Msg_Refresh)
		{
			OLED_ClearG();
			for(i=0;i<Wavlen;i++)
			{
	//			X=WavData[i]>>(22-Zoom);
	//			Y=(WavData[i]>>(6-Zoom))&0x3F;
				X=63-(WavData[i]>>22);//�����񾵷�ת��������Ҫ��ת��ʾ
				Y=(WavData[i]>>6)&0x3F;//Y����Ϊ���½�
				OLED_DrawPoint(X,Y,1);
			}
			for(i=0;i<5;i++)OLED_DrawLine(64,i*16,66-i%2,i*16);
			OLED_DrawLine(64,63,65,63);
			OLED_DrawLine(64,0,64,64);
			OLED_ShowStringG(80,0,(OE?(u8*)"DAC On":(u8*)"DACoff"),OE?0xff:0);
			sprintf((char*)DispBuff,"L:%4d",(int)Wavlen);
			OLED_ShowStringG(80,2,DispBuff,0);
			sprintf((char*)DispBuff,"Z:%4d",(int)1<<Zoom);
			OLED_ShowStringG(80,4,DispBuff,0);
			OLED_Refresh_Gram();
			if(OE)DAC_Dual_Init();
			else DAC_Dual_DeInit();
			Msg_Refresh=0;
		}
	}
}

//�˴�ֻ�ڲ���ʼ��Timer4��������˫·DACģʽ
static void Timer4_Init(u32 Rate)
{
	TIM_TimeBaseInitTypeDef timeInit;
	TIM_OCInitTypeDef timeOCinit;
	
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4,ENABLE);
	
	TIM_Cmd(TIM4,DISABLE);//��ʼ����ʱ��
	timeInit.TIM_ClockDivision=TIM_CKD_DIV1;
	timeInit.TIM_CounterMode=TIM_CounterMode_Up;
	//���ղ���Ƶ�ʲ�ͬ���ú��ʵļ���ֵ
	if(Rate>=10000)//10k���ϸ߾��ȼ���
	{
		timeInit.TIM_Period=8000000/Rate-1;
		timeInit.TIM_Prescaler=7-1;//����56MHzģʽ
	}
	else if(Rate<=20)//20Hz���µ�Ƶ����
	{
		timeInit.TIM_Period=10000/Rate-1;
		timeInit.TIM_Prescaler=5600-1;//����56MHzģʽ
	}
	else
	{
		timeInit.TIM_Period=1000000/Rate-1;
		timeInit.TIM_Prescaler=56-1;//����56MHzģʽ
	}
	TIM_TimeBaseInit(TIM4 ,&timeInit);
	
	//��ʼ������Ƚ�ͨ��4����ΪADC����Դ
	timeOCinit.TIM_OCMode=TIM_OCMode_PWM1;
	timeOCinit.TIM_OutputState=TIM_OutputState_Enable;
	timeOCinit.TIM_OCPolarity=TIM_OCPolarity_High;
	timeOCinit.TIM_Pulse=1;
	TIM_OC4Init(TIM4,&timeOCinit);
	
	TIM_CtrlPWMOutputs(TIM4,ENABLE);
	
	TIM_Cmd(TIM4,ENABLE);//��ʱ����������
}

//�ڲ���ʼ��ADC1ΪTimer4-CH4����ģʽ��������˫·ADCģʽ
static void ADC1_Init(u32 Rate)
{
	ADC_InitTypeDef ADCinit;
	NVIC_InitTypeDef nvicInit;
	GPIO_InitTypeDef GPIO_InitStructure;
	
	RCC_APB1PeriphClockCmd(RCC_APB2Periph_GPIOA|RCC_APB2ENR_ADC1EN|RCC_APB2ENR_ADC2EN,ENABLE);

	RCC_ADCCLKConfig(RCC_PCLK2_Div4);   //����ADC��Ƶ����4 56M/4=14,ADC���ʱ�䲻�ܳ���14M
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1	, ENABLE );	  //ʹ��ADC1ͨ��ʱ��
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC2	, ENABLE );	  //ʹ��ADC1ͨ��ʱ��

	//PA6 ��Ϊģ��ͨ����������                         
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6|GPIO_Pin_7;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;		//ģ����������
	GPIO_Init(GPIOA, &GPIO_InitStructure);	

	ADC_DeInit(ADC1);  //��λADC1,������ ADC1 ��ȫ���Ĵ�������Ϊȱʡֵ

	#define MaxRate 1000000 //�������ʣ����ڱ߽���������
	if(Rate<MaxRate)
	{
		ADCinit.ADC_ContinuousConvMode=DISABLE;
		ADCinit.ADC_ExternalTrigConv=ADC_ExternalTrigConv_T4_CC4;//TIM4.CCR4��Ϊ����Դ
	}
	else
	{//Ƶ�ʹ��ߣ�ʹ������ת��ģʽ
		ADCinit.ADC_ContinuousConvMode=ENABLE;
		ADCinit.ADC_ExternalTrigConv=ADC_ExternalTrigConv_None;//����ת����Ϊ����Դ
	}
	ADCinit.ADC_DataAlign=ADC_DataAlign_Right;
	ADCinit.ADC_Mode=ADC_Mode_RegSimult;
	ADCinit.ADC_NbrOfChannel=1;
	ADCinit.ADC_ScanConvMode=DISABLE;
	ADC_Init(ADC1 ,&ADCinit);
	ADC_ExternalTrigConvCmd(ADC1,ENABLE);//��ADC
  	//���ղ���Ƶ������ָ��ADC�Ĺ�����ͨ����һ�����У�����ʱ��
	if(Rate>=500000)ADC_RegularChannelConfig(ADC1, ADC_Channel_6, 1, ADC_SampleTime_1Cycles5 );	//ADC1,ADCͨ��,����ʱ��Ϊ1.5+12.5=14����	  			    
	else if(Rate>=200000)ADC_RegularChannelConfig(ADC1, ADC_Channel_6, 1, ADC_SampleTime_7Cycles5 );	//ADC1,ADCͨ��,����ʱ��Ϊ7.5+12.5=20����	  			    
	else if(Rate>=100000)ADC_RegularChannelConfig(ADC1, ADC_Channel_6, 1, ADC_SampleTime_13Cycles5 );	//ADC1,ADCͨ��,����ʱ��Ϊ13.5+12.5=26����	  			    
	else if(Rate>=40000)ADC_RegularChannelConfig(ADC1, ADC_Channel_6, 1, ADC_SampleTime_55Cycles5 );	//ADC1,ADCͨ��,����ʱ��Ϊ55.5+12.5=68����	  			    
	else ADC_RegularChannelConfig(ADC1, ADC_Channel_6, 1, ADC_SampleTime_239Cycles5 );	//ADC1,ADCͨ��,����ʱ��Ϊ239.5+12.5=252����	  			    
	
	if(Rate<MaxRate)
	{
		ADCinit.ADC_ContinuousConvMode=DISABLE;
		ADCinit.ADC_ExternalTrigConv=ADC_ExternalTrigConv_None;//TIM4.CCR4��Ϊ����Դ����ADC2����Ϊ
	}
	else
	{//Ƶ�ʹ��ߣ�ʹ������ת��ģʽ
		ADCinit.ADC_ContinuousConvMode=ENABLE;
		ADCinit.ADC_ExternalTrigConv=ADC_ExternalTrigConv_None;//����ת����Ϊ����Դ
	}
	ADC_Init(ADC2 ,&ADCinit);
	ADC_ExternalTrigConvCmd(ADC2,ENABLE);//��ADC
	
	
	/* ��˫ADCģʽ���ת�����ó����ⲿ�¼�����ʱ���û����뽫�����óɽ�������ADC����ADC���ó�����������������Է�ֹ����Ĵ�����ת�������ǣ����ʹ�ADC���ⲿ��������ͬʱ�����*/
  	//���ղ���Ƶ������ָ��ADC�Ĺ�����ͨ����һ�����У�����ʱ��
	if(Rate>=500000)ADC_RegularChannelConfig(ADC2, ADC_Channel_7, 1, ADC_SampleTime_1Cycles5 );	//ADC1,ADCͨ��,����ʱ��Ϊ1.5+12.5=14����	  			    
	else if(Rate>=200000)ADC_RegularChannelConfig(ADC2, ADC_Channel_7, 1, ADC_SampleTime_7Cycles5 );	//ADC1,ADCͨ��,����ʱ��Ϊ7.5+12.5=20����	  			    
	else if(Rate>=100000)ADC_RegularChannelConfig(ADC2, ADC_Channel_7, 1, ADC_SampleTime_13Cycles5 );	//ADC1,ADCͨ��,����ʱ��Ϊ13.5+12.5=26����	  			    
	else if(Rate>=40000)ADC_RegularChannelConfig(ADC2, ADC_Channel_7, 1, ADC_SampleTime_55Cycles5 );	//ADC1,ADCͨ��,����ʱ��Ϊ55.5+12.5=68����	  			    
	else ADC_RegularChannelConfig(ADC2, ADC_Channel_7, 1, ADC_SampleTime_239Cycles5 );	//ADC1,ADCͨ��,����ʱ��Ϊ239.5+12.5=252����	  			    
	ADC_ITConfig(ADC2,ADC_IT_EOC,ENABLE);

    ADC_DMACmd(ADC1, ENABLE);//ADC��DMA����ʹ��
	ADC_Cmd(ADC1, ENABLE);	//ʹ��ָ����ADC1
	ADC_Cmd(ADC2, ENABLE);	//ʹ��ָ����ADC2
	
	ADC_ResetCalibration(ADC1);	//ʹ�ܸ�λУ׼  
	while(ADC_GetResetCalibrationStatus(ADC1));	//�ȴ���λУ׼����
	ADC_StartCalibration(ADC1);	 //����ADУ׼
	while(ADC_GetCalibrationStatus(ADC1));	 //�ȴ�У׼����
	ADC_ResetCalibration(ADC2);	//ʹ�ܸ�λУ׼  
	while(ADC_GetResetCalibrationStatus(ADC2));	//�ȴ���λУ׼����
	ADC_StartCalibration(ADC2);	 //����ADУ׼
	while(ADC_GetCalibrationStatus(ADC2));	 //�ȴ�У׼����
 
if(Rate>=MaxRate)ADC_SoftwareStartConvCmd(ADC1,ENABLE);//���ڳ���1Mʱ��������ת�������ڱ߽���������
	
	//MY_NVIC_Init(1,3,TIM2_IRQn,2);
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
	nvicInit.NVIC_IRQChannel=ADC1_2_IRQn;
	nvicInit.NVIC_IRQChannelCmd=DISABLE;//˫ͨ��������ͼģʽ�������ж�
	nvicInit.NVIC_IRQChannelPreemptionPriority=1;
	nvicInit.NVIC_IRQChannelSubPriority=1;
	NVIC_Init(&nvicInit);
}

static DMA_InitTypeDef ADC_DMA_InitStructure;//���̼��٣�ע�ⲻ������

static void ADC_DMA_Init(u32 buffLenth)
{
	NVIC_InitTypeDef nvicInit;

	if(buffLenth>MaxDots)buffLenth=MaxDots;
	Wavlen=buffLenth;
	
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);//ʹ��ʱ��

	DMA_DeInit(DMA1_Channel1);    //��ͨ��һ�Ĵ�����ΪĬ��ֵ
	ADC_DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)&(ADC1->DR);//�ò������Զ���DMA�������ַ
	ADC_DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t)&WavData;//�ò������Զ���DMA�ڴ����ַ(ת���������ĵ�ַ)
	ADC_DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralSRC;//�ò����涨����������Ϊ���ݴ����Ŀ�ĵػ�����Դ���˴�����Ϊ��Դ
	ADC_DMA_InitStructure.DMA_BufferSize = buffLenth;//����ָ��DMAͨ����DMA����Ĵ�С,��λΪ���ݵ�λ������Ҳ����ADCConvertedValue�Ĵ�С
	ADC_DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;//�趨�����ַ�Ĵ����������,�˴���Ϊ���� Disable
	ADC_DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;//�����趨�ڴ��ַ�Ĵ����������,�˴���Ϊ������Enable
	ADC_DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Word;//���ݿ��Ϊ32λ
	ADC_DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Word;//���ݿ��Ϊ32λ
	ADC_DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;//DMA_Mode_Circular; //������ѭ������ģʽ
	ADC_DMA_InitStructure.DMA_Priority = DMA_Priority_High;//DMAͨ��ӵ�и����ȼ� �ֱ�4���ȼ� �͡��С��ߡ��ǳ���
	ADC_DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;//ʹ��DMAͨ�����ڴ浽�ڴ洫��
	DMA_Init(DMA1_Channel1, &ADC_DMA_InitStructure);//����DMA_InitStruct��ָ���Ĳ�����ʼ��DMA��ͨ��

	DMA_ITConfig(DMA1_Channel1,DMA_IT_TC,ENABLE);//����DMA�ж�
	
	nvicInit.NVIC_IRQChannel=DMA1_Channel1_IRQn;
	nvicInit.NVIC_IRQChannelCmd=ENABLE;
	nvicInit.NVIC_IRQChannelPreemptionPriority=2;
	nvicInit.NVIC_IRQChannelSubPriority=2;
	NVIC_Init(&nvicInit);
	
	DMA_Cmd(DMA1_Channel1, ENABLE);//����DMAͨ��һ
}

static void ADC_DMA_Restart(void)//����DMA���䣨����������
{
	DMA_DeInit(DMA1_Channel1);
	DMA_Init(DMA1_Channel1, &ADC_DMA_InitStructure);
	DMA_ITConfig(DMA1_Channel1, DMA_IT_TC, ENABLE);
	DMA_Cmd(DMA1_Channel1, ENABLE);
}

extern u8 Msg_ADC_DMA;

static void DualOscInit(u32 Rate,u32 Lenth)
{
	Timer4_Init(Rate);//��ʼ��������ʱ��Tim4��Ƶ�ʣ�ADC��TIM4.CCR4������
	ADC1_Init(Rate);//�������ʳ�ʼ��ADC����ҪӰ�����ʱ�䣩
	ADC_DMA_Init(Lenth);//DMA��ʼ������ʵ�ʲ���������
}

void DualOscDeInit(void)
{
	TIM_Cmd(TIM4,DISABLE);//��ʱ�����رգ�
	TIM_DeInit(TIM4);
	ADC_ExternalTrigConvCmd(ADC1,DISABLE);//��ADC
	ADC_ITConfig(ADC2,ADC_IT_EOC,DISABLE);
    ADC_DMACmd(ADC1, DISABLE);//ADC��DMA����ʹ��
	ADC_Cmd(ADC1, DISABLE);	//ʧ��ָ����ADC1
	ADC_Cmd(ADC2, DISABLE);	//ʧ��ָ����ADC2
	ADC_DeInit(ADC1);
	ADC_DeInit(ADC2);
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, DISABLE);//ʧ��ʱ��
	DMA_ITConfig(DMA1_Channel1,DMA_IT_TC,DISABLE);//�ر�DMA�ж�
	DMA_Cmd(DMA1_Channel1, DISABLE);//�ر�DMAͨ��һ
	DMA_DeInit(DMA1_Channel1);
}


static const int  SamplingRateMap[] = { 10,50,100, 500,1000,2500, 5000,10000,25000, 50000,100000,250000, 500000,1000000 };//������Ӧ�Ĳ�����
static const int        WindowMap[] = { 16,32,64,128,256,512,768,1024,1536,2048 };//�������봰��(��Ч������)��Ӧ

void LissajousPlot(void)
{
	s32 i=0;
	u8 Msg_Refresh=1;
	u8 OE=1,Zoom=0,Sel=0,Window=7,Rate=7;
	s32 X,Y;
	
	reInit:
	i=0;
	Msg_Refresh=1;
	OE=1;Zoom=0;Sel=0;Window=7;Rate=7;
	OLED_Clear();
	OLED_Clear();
	OLED_ShowString(0,0,(u8*)"XY Osc Initing...",0);
	OLED_DrawLine(64,0,64,64);
	OLED_Refresh_Gram();
	DualOscInit(SamplingRateMap[Rate],WindowMap[Window]);
	ADC_DMA_Restart();
	Mod_Recv=Recv_None;
	OLED_ShowString(0,2,(u8*)"XY Osc Inited...   ",0);
	while(1)
	{
		LED1=(sysTick%1000)<500;
		if(Msg_ADC_DMA)
		{
			LED0=!LED0;
			DAC_Dual_Init();
			Wavlen=Wavlen;
			SetTimer5BaseFreq(12500);
			Msg_ADC_DMA=0;
			Msg_Refresh++;
		}
		if(getKey(Key0,500))
		{
			Sel++;Sel%=5;
			Msg_Refresh++;
		}
		if(getKey(Key1,500))
		{
			switch(Sel)
			{
				case 0:break;
				case 1:OE=0;break;
				case 2:Rate--;if(Rate==255)Rate=13;break;
				case 3:Window--;if(Window==255)Window=10;break;
				case 4:Zoom--;Zoom%=8;break;
			}
			DualOscInit(SamplingRateMap[Rate],WindowMap[Window]);
			Msg_Refresh++;
		}
		if(getKey(Key2,500))
		{
			switch(Sel)
			{
				case 0:break;
				case 1:OE=1;break;
				case 2:Rate++;Rate%=14;break;
				case 3:Window++;Window%=10;break;
				case 4:Zoom++;Zoom%=8;break;
			}
			DualOscInit(SamplingRateMap[Rate],WindowMap[Window]);
			Msg_Refresh++;
		}
		if(getKey(Key3,500))
		{
			if(Sel)Sel=0;
			else break;
		}
		
		if(Msg_Refresh)
		{
			OLED_ClearG();
			for(i=0;i<Wavlen;i++)
			{
				X=(WavData[i]&0xFFFF0000)>>(22-Zoom);
				if(X<0)X=0;if(X>64)X=64;
				Y=((WavData[i]&0x0000FFFF)>>(6-Zoom));
				if(Y<0)Y=0;if(Y>64)Y=64;
				OLED_DrawPoint(X,Y,1);
			}
			for(i=0;i<5;i++)OLED_DrawLine(64,i*16,66-i%2,i*16);
			OLED_DrawLine(64,63,65,63);
			OLED_DrawLine(64,0,64,64);
			OLED_ShowStringG(80,0,(u8*)"DAC",Sel==1?0xff:0);
			OLED_ShowStringG(OE?112:104,0,(OE?(u8*)"On":(u8*)"off"),OE?0xff:0);
			i=SamplingRateMap[Rate];
			if(i>=1000000)sprintf((char*)DispBuff,"R:%3dM",(int)i/1000000);
			else if(i>=10000)sprintf((char*)DispBuff,"R:%3dK",(int)i/1000);
			else if(i>=1000)sprintf((char*)DispBuff,"R:%.1fK",i/1000.0);
			else sprintf((char*)DispBuff,"R:%4d",(int)i);
			OLED_ShowStringG(80,2,DispBuff,Sel==2?0xff:0);
			sprintf((char*)DispBuff,"L:%4d",(int)Wavlen);
			OLED_ShowStringG(80,4,DispBuff,Sel==3?0xff:0);
			sprintf((char*)DispBuff,"Z:%4d",(int)1<<Zoom);
			OLED_ShowStringG(80,6,DispBuff,Sel==4?0xff:0);
			OLED_Refresh_Gram();
			if(OE)DAC_Dual_Init();
			else DAC_Dual_DeInit();
			Msg_Refresh=0;
			ADC_DMA_Restart();
		}
	}
	DualOscDeInit();
}
