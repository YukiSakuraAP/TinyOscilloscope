#include "Osc.h"

/*
虚拟示波器下位机 for SSv4.2E @ Yuki Sakura

使用时请将此文件插入至工程文件夹中，并将 OscInit() 加入至初始化队列
资源占用：
72MHz主频
GPIOB.9 PWM输出，与ADC采样同步
GPIOA.6 ADC输入，示波器测量引脚
定时器4 100us，可共享
ADC1 CH6 规则模式 240周期模式，资源独占
串口 >=115200bps，资源独占
*/

u32 Osc_TrigLevel=2048,Osc_SmitLevel=200,Osc_SamplingRate=10000,Osc_WindowLenth=1000,Osc_Sync=1;

//此处只初始化单独的Timer4
void Timer4_Init(u32 Rate)
{
	TIM_TimeBaseInitTypeDef timeInit;
	TIM_OCInitTypeDef timeOCinit;
	NVIC_InitTypeDef nvicInit;
	
	GPIO_InitTypeDef dat;//初始化GPIOB.9(PWM输出，与采样同步)
	dat.GPIO_Mode=GPIO_Mode_AF_PP;
	dat.GPIO_Pin=GPIO_Pin_9;
	dat.GPIO_Speed=GPIO_Speed_50MHz;
	
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB,ENABLE);
	GPIO_Init(GPIOB,&dat);
	
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
	
	//定时器中断（没开）
	//TIM_ITConfig(TIM4,TIM_IT_Update|TIM_IT_CC4,ENABLE);
	
	//MY_NVIC_Init(1,3,TIM2_IRQn,2);
//	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
//	nvicInit.NVIC_IRQChannel=TIM4_IRQn;
//	nvicInit.NVIC_IRQChannelCmd=ENABLE;
//	nvicInit.NVIC_IRQChannelPreemptionPriority=2;
//	nvicInit.NVIC_IRQChannelSubPriority=2;
//	NVIC_Init(&nvicInit);
	
	TIM_Cmd(TIM4,ENABLE);//定时器，启动！
}

//定时器4中断，自动触发ADC1的6通道，由于没配置中断，这里进不来
void TIM4_IRQHandler(void)
{
	if(TIM_GetITStatus(TIM4,TIM_IT_Update))
	{
		TIM_ClearITPendingBit(TIM4,TIM_IT_Update);
	}
	if(TIM_GetITStatus(TIM4,TIM_IT_CC4))
	{
		TIM_ClearITPendingBit(TIM4,TIM_IT_CC4);
		//ADC_SoftwareStartConvCmd(ADC1, ENABLE);		//使能指定的ADC1的软件转换启动功能	
	}
}

//初始化ADC1为Timer4-CH4触发模式
void ADC1_Init(u32 Rate)
{
	ADC_InitTypeDef ADCinit;
	NVIC_InitTypeDef nvicInit;
	GPIO_InitTypeDef GPIO_InitStructure;
	
	RCC_APB1PeriphClockCmd(RCC_APB2Periph_GPIOA|RCC_APB2ENR_ADC1EN,ENABLE);

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1	, ENABLE );	  //使能ADC1通道时钟
	RCC_ADCCLKConfig(RCC_PCLK2_Div4);   //设置ADC分频因子4 56M/4=14,ADC最大时间不能超过14M

	//PA6 作为模拟通道输入引脚                         
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;
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
	ADCinit.ADC_Mode=ADC_Mode_Independent;
	ADCinit.ADC_NbrOfChannel=1;
	ADCinit.ADC_ScanConvMode=DISABLE;
	ADC_Init(ADC1 ,&ADCinit);
	ADC_ExternalTrigConvCmd(ADC1,ENABLE);
  	//按照采样频率设置指定ADC的规则组通道，一个序列，采样时间
	if(Rate>=500000)ADC_RegularChannelConfig(ADC1, ADC_Channel_6, 1, ADC_SampleTime_1Cycles5 );	//ADC1,ADC通道,采样时间为1.5+12.5=14周期	  			    
	else if(Rate>=200000)ADC_RegularChannelConfig(ADC1, ADC_Channel_6, 1, ADC_SampleTime_7Cycles5 );	//ADC1,ADC通道,采样时间为7.5+12.5=20周期	  			    
	else if(Rate>=100000)ADC_RegularChannelConfig(ADC1, ADC_Channel_6, 1, ADC_SampleTime_13Cycles5 );	//ADC1,ADC通道,采样时间为13.5+12.5=26周期	  			    
	else if(Rate>=40000)ADC_RegularChannelConfig(ADC1, ADC_Channel_6, 1, ADC_SampleTime_55Cycles5 );	//ADC1,ADC通道,采样时间为55.5+12.5=68周期	  			    
	else ADC_RegularChannelConfig(ADC1, ADC_Channel_6, 1, ADC_SampleTime_239Cycles5 );	//ADC1,ADC通道,采样时间为239.5+12.5=252周期	  			    
	ADC_ITConfig(ADC1,ADC_IT_EOC,ENABLE);

    ADC_DMACmd(ADC1, ENABLE);//ADC的DMA传输使用
	ADC_Cmd(ADC1, ENABLE);	//使能指定的ADC1
	
	ADC_ResetCalibration(ADC1);	//使能复位校准  
	while(ADC_GetResetCalibrationStatus(ADC1));	//等待复位校准结束
	ADC_StartCalibration(ADC1);	 //开启AD校准
	while(ADC_GetCalibrationStatus(ADC1));	 //等待校准结束
 
if(Rate>=MaxRate)ADC_SoftwareStartConvCmd(ADC1,ENABLE);//仅在超过1M时开启连续转换，存在边界条件问题
	
	//MY_NVIC_Init(1,3,TIM2_IRQn,2);
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
	nvicInit.NVIC_IRQChannel=ADC1_2_IRQn;
	nvicInit.NVIC_IRQChannelCmd=(Rate<=20000)?ENABLE:DISABLE;//20K以下才开启中断
	nvicInit.NVIC_IRQChannelPreemptionPriority=1;
	nvicInit.NVIC_IRQChannelSubPriority=1;
	NVIC_Init(&nvicInit);
}
//收到数据后立刻通过串口上传，要求波特率>=115200，采样率高于20K会自动关断
void ADC1_2_IRQHandler(void)
{
	ADC_ClearITPendingBit(ADC1,ADC_IT_EOC);
	if(Osc_Sync==0)USART1->DR=ADC1->DR>>4;
}

volatile u16 ADCbuff[ADCbuffLenth];//ADC存储区

DMA_InitTypeDef ADC_DMA_InitStructure;//过程加速，注意不能重名

void ADC_DMA_Init(u32 buffLenth)
{
	NVIC_InitTypeDef nvicInit;

	if(buffLenth>ADCbuffLenth)buffLenth=ADCbuffLenth;

	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);//使能时钟

	DMA_DeInit(DMA1_Channel1);    //将通道一寄存器设为默认值
	ADC_DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)&(ADC1->DR);//该参数用以定义DMA外设基地址
	ADC_DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t)&ADCbuff;//该参数用以定义DMA内存基地址(转换结果保存的地址)
	ADC_DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralSRC;//该参数规定了外设是作为数据传输的目的地还是来源，此处是作为来源
	ADC_DMA_InitStructure.DMA_BufferSize = buffLenth;//定义指定DMA通道的DMA缓存的大小,单位为数据单位。这里也就是ADCConvertedValue的大小
	ADC_DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;//设定外设地址寄存器递增与否,此处设为不变 Disable
	ADC_DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;//用来设定内存地址寄存器递增与否,此处设为递增，Enable
	ADC_DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;//数据宽度为16位
	ADC_DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;//数据宽度为16位
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

void ADC_DMA_Restart(void)//重启DMA传输（重启采样）
{
	DMA_DeInit(DMA1_Channel1);
	DMA_Init(DMA1_Channel1, &ADC_DMA_InitStructure);
	DMA_ITConfig(DMA1_Channel1, DMA_IT_TC, ENABLE);
	DMA_Cmd(DMA1_Channel1, ENABLE);
}

u8 Msg_ADC_DMA=0,Msg_OscSetting=0;

void DMA1_Channel1_IRQHandler(void)//DMA中断
{
	if(DMA_GetITStatus(DMA1_IT_TC1))//传输完成
	{
		DMA_ClearITPendingBit(DMA1_IT_GL1);
		//DMA_ClearITPendingBit(DMA1_Channel1_IRQn);
		//ADC_DMA_Restart();
		Msg_ADC_DMA++;//发送一条“DMA传输完成消息”
	}
}

void OscInit(u32 Rate,u32 Lenth)
{
	Timer4_Init(Rate);//初始化触发定时器Tim4的频率（ADC由TIM4.CCR4触发）
	ADC1_Init(Rate);//按采样率初始化ADC（主要影响采样时间）
	ADC_DMA_Init(Lenth);//DMA初始化（按实际采样点数）
}
