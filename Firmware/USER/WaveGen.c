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

//此处只初始化单独的Timer5
void Timer5_Init(void)//定时器5初始化
{
	TIM_TimeBaseInitTypeDef timeInit;
	NVIC_InitTypeDef nvicInit;
	
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM5,ENABLE);
	
	timeInit.TIM_ClockDivision=TIM_CKD_DIV1;
	timeInit.TIM_CounterMode=TIM_CounterMode_Down;
	timeInit.TIM_Period=8000-1;
	timeInit.TIM_Prescaler=7-1;//适配56MHz模式
	
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
	else {TIM5->PSC=1-1;TIM5->ARR=56-1;}//限频率1MHz
}

void WaveGenSine(u32 FreqX,u32 FreqY,u32 BaseFreq)
{
	u32 GenDiv=2,i,FreqXYbase;
	if(BaseFreq<(FreqX*2))return;//不成立
	if(BaseFreq<(FreqY*2))return;//不成立
	FreqXYbase=((FreqX<FreqY)?FreqX:FreqY);
	GenDiv=BaseFreq/FreqXYbase;//选取最低频率作为基准
	if(GenDiv>MaxDots)GenDiv=MaxDots;//点数太多不成立
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
	if(BaseFreq<(FreqX*2))return;//不成立
	if(BaseFreq<(FreqY*2))return;//不成立
	FreqXYbase=((FreqX<FreqY)?FreqX:FreqY);
	GenDiv=BaseFreq/FreqXYbase;//选取最低频率作为基准
	if(GenDiv>MaxDots)GenDiv=MaxDots;//点数太多不成立
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
	if(BaseFreq<(FreqX*2))return;//不成立
	if(BaseFreq<(FreqY*2))return;//不成立
	FreqXYbase=((FreqX<FreqY)?FreqX:FreqY);
	GenDiv=BaseFreq/FreqXYbase;//选取最低频率作为基准
	if(GenDiv>MaxDots)GenDiv=MaxDots;//点数太多不成立
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
	//时间法
//	while(1)
//	{
//		x=pos%SizeX;y=pos/SizeY;//转换为字模坐标
//		z=Data[y*(SizeX/8)+x/8]&(1<<(x%8));
//		PAout(8)=z;
//		if(z)//省略空位，更快
//		{
//			PAout(8)=1;
//			delay_us(15);
//			PAout(8)=0;
//			DAC_Output((4000-y)*24+OffsetY,0+(4000-x)*24+OffsetX);//放大字体，转换方向
//			pos++;pos%=SizeX*SizeY;
//			break;
//		}
//		else PAout(8)=0;
//		pos++;pos%=SizeX*SizeY;
//	}
//	//时间法改进
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
//		if(z)//省略空位，更快
//		{
//			PAout(8)=1;
//			delay_us(15);
//			PAout(8)=0;
//			DAC_Output((4000-y)*24+OffsetY,0+(4000-x)*24+OffsetX);//放大字体，转换方向
//			break;
//		}
//		else PAout(8)=0;
//	}
	//XY-Z法
//	x=pos%SizeX;y=pos/SizeY;//转换为字模坐标
//	PAout(8)=Data[y*(SizeX/8)+x/8]&(1<<(x%8));
//	DAC_Output((y)*24+OffsetY,0+(x)*24+OffsetX);//放大字体，转换方向
//	pos++;pos%=SizeX*SizeY;
	
	//画圆
//	DAC_Output(2048+1800*sin(pos*3.14/30),2048+1800*cos(pos*3.14/30));
//	pos++;
//	PAout(8)=((pos%60)<50);
	//画Sin(2周期)
//	DAC_Output(2048+1800*sin(pos*3.14/30),(pos%120)*30);
//	pos++;
//	PAout(8)=((pos%120)>5);

	//线段法
	DAC->DHR12RD=WavData[LDcnt];
	LDcnt++;if(LDcnt>=Wavlen)LDcnt=0;
	
//	//回转线段法
//	if(LDcnt<=0){LDcnt=1;LDdir=1;}
//	else if(LDcnt>=(Wavlen-1)){LDcnt=(Wavlen-2);LDdir=-1;}
//	else LDcnt+=LDdir;
//	DAC->DHR12RD=WavData[LDcnt];

}

void TIM5_IRQHandler(void)//1ms定时函数
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
static u8 DispBuff[32];//显示内容用的缓存

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
				X=63-(WavData[i]>>22);//由于振镜反转，所以需要反转显示
				Y=(WavData[i]>>6)&0x3F;//Y坐标为左下角
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

//此处只内部初始化Timer4，仅限于双路DAC模式
static void Timer4_Init(u32 Rate)
{
	TIM_TimeBaseInitTypeDef timeInit;
	TIM_OCInitTypeDef timeOCinit;
	
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4,ENABLE);
	
	TIM_Cmd(TIM4,DISABLE);//初始化定时器
	timeInit.TIM_ClockDivision=TIM_CKD_DIV1;
	timeInit.TIM_CounterMode=TIM_CounterMode_Up;
	//按照采样频率不同设置合适的计数值
	if(Rate>=10000)//10k以上高精度计数
	{
		timeInit.TIM_Period=8000000/Rate-1;
		timeInit.TIM_Prescaler=7-1;//适配56MHz模式
	}
	else if(Rate<=20)//20Hz以下低频计数
	{
		timeInit.TIM_Period=10000/Rate-1;
		timeInit.TIM_Prescaler=5600-1;//适配56MHz模式
	}
	else
	{
		timeInit.TIM_Period=1000000/Rate-1;
		timeInit.TIM_Prescaler=56-1;//适配56MHz模式
	}
	TIM_TimeBaseInit(TIM4 ,&timeInit);
	
	//初始化输出比较通道4，作为ADC触发源
	timeOCinit.TIM_OCMode=TIM_OCMode_PWM1;
	timeOCinit.TIM_OutputState=TIM_OutputState_Enable;
	timeOCinit.TIM_OCPolarity=TIM_OCPolarity_High;
	timeOCinit.TIM_Pulse=1;
	TIM_OC4Init(TIM4,&timeOCinit);
	
	TIM_CtrlPWMOutputs(TIM4,ENABLE);
	
	TIM_Cmd(TIM4,ENABLE);//定时器，启动！
}

//内部初始化ADC1为Timer4-CH4触发模式，仅限于双路ADC模式
static void ADC1_Init(u32 Rate)
{
	ADC_InitTypeDef ADCinit;
	NVIC_InitTypeDef nvicInit;
	GPIO_InitTypeDef GPIO_InitStructure;
	
	RCC_APB1PeriphClockCmd(RCC_APB2Periph_GPIOA|RCC_APB2ENR_ADC1EN|RCC_APB2ENR_ADC2EN,ENABLE);

	RCC_ADCCLKConfig(RCC_PCLK2_Div4);   //设置ADC分频因子4 56M/4=14,ADC最大时间不能超过14M
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1	, ENABLE );	  //使能ADC1通道时钟
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC2	, ENABLE );	  //使能ADC1通道时钟

	//PA6 作为模拟通道输入引脚                         
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6|GPIO_Pin_7;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;		//模拟输入引脚
	GPIO_Init(GPIOA, &GPIO_InitStructure);	

	ADC_DeInit(ADC1);  //复位ADC1,将外设 ADC1 的全部寄存器重设为缺省值

	#define MaxRate 1000000 //最大采样率，存在边界条件问题
	if(Rate<MaxRate)
	{
		ADCinit.ADC_ContinuousConvMode=DISABLE;
		ADCinit.ADC_ExternalTrigConv=ADC_ExternalTrigConv_T4_CC4;//TIM4.CCR4作为触发源
	}
	else
	{//频率过高，使用连续转换模式
		ADCinit.ADC_ContinuousConvMode=ENABLE;
		ADCinit.ADC_ExternalTrigConv=ADC_ExternalTrigConv_None;//连续转换作为触发源
	}
	ADCinit.ADC_DataAlign=ADC_DataAlign_Right;
	ADCinit.ADC_Mode=ADC_Mode_RegSimult;
	ADCinit.ADC_NbrOfChannel=1;
	ADCinit.ADC_ScanConvMode=DISABLE;
	ADC_Init(ADC1 ,&ADCinit);
	ADC_ExternalTrigConvCmd(ADC1,ENABLE);//主ADC
  	//按照采样频率设置指定ADC的规则组通道，一个序列，采样时间
	if(Rate>=500000)ADC_RegularChannelConfig(ADC1, ADC_Channel_6, 1, ADC_SampleTime_1Cycles5 );	//ADC1,ADC通道,采样时间为1.5+12.5=14周期	  			    
	else if(Rate>=200000)ADC_RegularChannelConfig(ADC1, ADC_Channel_6, 1, ADC_SampleTime_7Cycles5 );	//ADC1,ADC通道,采样时间为7.5+12.5=20周期	  			    
	else if(Rate>=100000)ADC_RegularChannelConfig(ADC1, ADC_Channel_6, 1, ADC_SampleTime_13Cycles5 );	//ADC1,ADC通道,采样时间为13.5+12.5=26周期	  			    
	else if(Rate>=40000)ADC_RegularChannelConfig(ADC1, ADC_Channel_6, 1, ADC_SampleTime_55Cycles5 );	//ADC1,ADC通道,采样时间为55.5+12.5=68周期	  			    
	else ADC_RegularChannelConfig(ADC1, ADC_Channel_6, 1, ADC_SampleTime_239Cycles5 );	//ADC1,ADC通道,采样时间为239.5+12.5=252周期	  			    
	
	if(Rate<MaxRate)
	{
		ADCinit.ADC_ContinuousConvMode=DISABLE;
		ADCinit.ADC_ExternalTrigConv=ADC_ExternalTrigConv_None;//TIM4.CCR4作为触发源，但ADC2不作为
	}
	else
	{//频率过高，使用连续转换模式
		ADCinit.ADC_ContinuousConvMode=ENABLE;
		ADCinit.ADC_ExternalTrigConv=ADC_ExternalTrigConv_None;//连续转换作为触发源
	}
	ADC_Init(ADC2 ,&ADCinit);
	ADC_ExternalTrigConvCmd(ADC2,ENABLE);//从ADC
	
	
	/* 在双ADC模式里，当转换配置成由外部事件触发时，用户必须将其设置成仅触发主ADC，从ADC设置成软件触发，这样可以防止意外的触发从转换。但是，主和从ADC的外部触发必须同时被激活。*/
  	//按照采样频率设置指定ADC的规则组通道，一个序列，采样时间
	if(Rate>=500000)ADC_RegularChannelConfig(ADC2, ADC_Channel_7, 1, ADC_SampleTime_1Cycles5 );	//ADC1,ADC通道,采样时间为1.5+12.5=14周期	  			    
	else if(Rate>=200000)ADC_RegularChannelConfig(ADC2, ADC_Channel_7, 1, ADC_SampleTime_7Cycles5 );	//ADC1,ADC通道,采样时间为7.5+12.5=20周期	  			    
	else if(Rate>=100000)ADC_RegularChannelConfig(ADC2, ADC_Channel_7, 1, ADC_SampleTime_13Cycles5 );	//ADC1,ADC通道,采样时间为13.5+12.5=26周期	  			    
	else if(Rate>=40000)ADC_RegularChannelConfig(ADC2, ADC_Channel_7, 1, ADC_SampleTime_55Cycles5 );	//ADC1,ADC通道,采样时间为55.5+12.5=68周期	  			    
	else ADC_RegularChannelConfig(ADC2, ADC_Channel_7, 1, ADC_SampleTime_239Cycles5 );	//ADC1,ADC通道,采样时间为239.5+12.5=252周期	  			    
	ADC_ITConfig(ADC2,ADC_IT_EOC,ENABLE);

    ADC_DMACmd(ADC1, ENABLE);//ADC的DMA传输使用
	ADC_Cmd(ADC1, ENABLE);	//使能指定的ADC1
	ADC_Cmd(ADC2, ENABLE);	//使能指定的ADC2
	
	ADC_ResetCalibration(ADC1);	//使能复位校准  
	while(ADC_GetResetCalibrationStatus(ADC1));	//等待复位校准结束
	ADC_StartCalibration(ADC1);	 //开启AD校准
	while(ADC_GetCalibrationStatus(ADC1));	 //等待校准结束
	ADC_ResetCalibration(ADC2);	//使能复位校准  
	while(ADC_GetResetCalibrationStatus(ADC2));	//等待复位校准结束
	ADC_StartCalibration(ADC2);	 //开启AD校准
	while(ADC_GetCalibrationStatus(ADC2));	 //等待校准结束
 
if(Rate>=MaxRate)ADC_SoftwareStartConvCmd(ADC1,ENABLE);//仅在超过1M时开启连续转换，存在边界条件问题
	
	//MY_NVIC_Init(1,3,TIM2_IRQn,2);
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
	nvicInit.NVIC_IRQChannel=ADC1_2_IRQn;
	nvicInit.NVIC_IRQChannelCmd=DISABLE;//双通道李萨如图模式不开启中断
	nvicInit.NVIC_IRQChannelPreemptionPriority=1;
	nvicInit.NVIC_IRQChannelSubPriority=1;
	NVIC_Init(&nvicInit);
}

static DMA_InitTypeDef ADC_DMA_InitStructure;//过程加速，注意不能重名

static void ADC_DMA_Init(u32 buffLenth)
{
	NVIC_InitTypeDef nvicInit;

	if(buffLenth>MaxDots)buffLenth=MaxDots;
	Wavlen=buffLenth;
	
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);//使能时钟

	DMA_DeInit(DMA1_Channel1);    //将通道一寄存器设为默认值
	ADC_DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)&(ADC1->DR);//该参数用以定义DMA外设基地址
	ADC_DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t)&WavData;//该参数用以定义DMA内存基地址(转换结果保存的地址)
	ADC_DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralSRC;//该参数规定了外设是作为数据传输的目的地还是来源，此处是作为来源
	ADC_DMA_InitStructure.DMA_BufferSize = buffLenth;//定义指定DMA通道的DMA缓存的大小,单位为数据单位。这里也就是ADCConvertedValue的大小
	ADC_DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;//设定外设地址寄存器递增与否,此处设为不变 Disable
	ADC_DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;//用来设定内存地址寄存器递增与否,此处设为递增，Enable
	ADC_DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Word;//数据宽度为32位
	ADC_DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Word;//数据宽度为32位
	ADC_DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;//DMA_Mode_Circular; //工作在循环缓存模式
	ADC_DMA_InitStructure.DMA_Priority = DMA_Priority_High;//DMA通道拥有高优先级 分别4个等级 低、中、高、非常高
	ADC_DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;//使能DMA通道的内存到内存传输
	DMA_Init(DMA1_Channel1, &ADC_DMA_InitStructure);//根据DMA_InitStruct中指定的参数初始化DMA的通道

	DMA_ITConfig(DMA1_Channel1,DMA_IT_TC,ENABLE);//开启DMA中断
	
	nvicInit.NVIC_IRQChannel=DMA1_Channel1_IRQn;
	nvicInit.NVIC_IRQChannelCmd=ENABLE;
	nvicInit.NVIC_IRQChannelPreemptionPriority=2;
	nvicInit.NVIC_IRQChannelSubPriority=2;
	NVIC_Init(&nvicInit);
	
	DMA_Cmd(DMA1_Channel1, ENABLE);//启动DMA通道一
}

static void ADC_DMA_Restart(void)//重启DMA传输（重启采样）
{
	DMA_DeInit(DMA1_Channel1);
	DMA_Init(DMA1_Channel1, &ADC_DMA_InitStructure);
	DMA_ITConfig(DMA1_Channel1, DMA_IT_TC, ENABLE);
	DMA_Cmd(DMA1_Channel1, ENABLE);
}

extern u8 Msg_ADC_DMA;

static void DualOscInit(u32 Rate,u32 Lenth)
{
	Timer4_Init(Rate);//初始化触发定时器Tim4的频率（ADC由TIM4.CCR4触发）
	ADC1_Init(Rate);//按采样率初始化ADC（主要影响采样时间）
	ADC_DMA_Init(Lenth);//DMA初始化（按实际采样点数）
}

void DualOscDeInit(void)
{
	TIM_Cmd(TIM4,DISABLE);//定时器，关闭！
	TIM_DeInit(TIM4);
	ADC_ExternalTrigConvCmd(ADC1,DISABLE);//主ADC
	ADC_ITConfig(ADC2,ADC_IT_EOC,DISABLE);
    ADC_DMACmd(ADC1, DISABLE);//ADC的DMA传输使用
	ADC_Cmd(ADC1, DISABLE);	//失能指定的ADC1
	ADC_Cmd(ADC2, DISABLE);	//失能指定的ADC2
	ADC_DeInit(ADC1);
	ADC_DeInit(ADC2);
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, DISABLE);//失能时钟
	DMA_ITConfig(DMA1_Channel1,DMA_IT_TC,DISABLE);//关闭DMA中断
	DMA_Cmd(DMA1_Channel1, DISABLE);//关闭DMA通道一
	DMA_DeInit(DMA1_Channel1);
}


static const int  SamplingRateMap[] = { 10,50,100, 500,1000,2500, 5000,10000,25000, 50000,100000,250000, 500000,1000000 };//按键对应的采样率
static const int        WindowMap[] = { 16,32,64,128,256,512,768,1024,1536,2048 };//采样率与窗长(有效数据数)对应

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
