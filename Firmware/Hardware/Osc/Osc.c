#include "Osc.h"

/*
����ʾ������λ�� for SSv4.2E @ Yuki Sakura

ʹ��ʱ�뽫���ļ������������ļ����У����� OscInit() ��������ʼ������
��Դռ�ã�
72MHz��Ƶ
GPIOB.9 PWM�������ADC����ͬ��
GPIOA.6 ADC���룬ʾ������������
��ʱ��4 100us���ɹ���
ADC1 CH6 ����ģʽ 240����ģʽ����Դ��ռ
���� >=115200bps����Դ��ռ
*/

u32 Osc_TrigLevel=2048,Osc_SmitLevel=200,Osc_SamplingRate=10000,Osc_WindowLenth=1000,Osc_Sync=1;

//�˴�ֻ��ʼ��������Timer4
void Timer4_Init(u32 Rate)
{
	TIM_TimeBaseInitTypeDef timeInit;
	TIM_OCInitTypeDef timeOCinit;
	NVIC_InitTypeDef nvicInit;
	
	GPIO_InitTypeDef dat;//��ʼ��GPIOB.9(PWM����������ͬ��)
	dat.GPIO_Mode=GPIO_Mode_AF_PP;
	dat.GPIO_Pin=GPIO_Pin_9;
	dat.GPIO_Speed=GPIO_Speed_50MHz;
	
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB,ENABLE);
	GPIO_Init(GPIOB,&dat);
	
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
	
	//��ʱ���жϣ�û����
	//TIM_ITConfig(TIM4,TIM_IT_Update|TIM_IT_CC4,ENABLE);
	
	//MY_NVIC_Init(1,3,TIM2_IRQn,2);
//	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
//	nvicInit.NVIC_IRQChannel=TIM4_IRQn;
//	nvicInit.NVIC_IRQChannelCmd=ENABLE;
//	nvicInit.NVIC_IRQChannelPreemptionPriority=2;
//	nvicInit.NVIC_IRQChannelSubPriority=2;
//	NVIC_Init(&nvicInit);
	
	TIM_Cmd(TIM4,ENABLE);//��ʱ����������
}

//��ʱ��4�жϣ��Զ�����ADC1��6ͨ��������û�����жϣ����������
void TIM4_IRQHandler(void)
{
	if(TIM_GetITStatus(TIM4,TIM_IT_Update))
	{
		TIM_ClearITPendingBit(TIM4,TIM_IT_Update);
	}
	if(TIM_GetITStatus(TIM4,TIM_IT_CC4))
	{
		TIM_ClearITPendingBit(TIM4,TIM_IT_CC4);
		//ADC_SoftwareStartConvCmd(ADC1, ENABLE);		//ʹ��ָ����ADC1�����ת����������	
	}
}

//��ʼ��ADC1ΪTimer4-CH4����ģʽ
void ADC1_Init(u32 Rate)
{
	ADC_InitTypeDef ADCinit;
	NVIC_InitTypeDef nvicInit;
	GPIO_InitTypeDef GPIO_InitStructure;
	
	RCC_APB1PeriphClockCmd(RCC_APB2Periph_GPIOA|RCC_APB2ENR_ADC1EN,ENABLE);

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1	, ENABLE );	  //ʹ��ADC1ͨ��ʱ��
	RCC_ADCCLKConfig(RCC_PCLK2_Div4);   //����ADC��Ƶ����4 56M/4=14,ADC���ʱ�䲻�ܳ���14M

	//PA6 ��Ϊģ��ͨ����������                         
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;
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
	ADCinit.ADC_Mode=ADC_Mode_Independent;
	ADCinit.ADC_NbrOfChannel=1;
	ADCinit.ADC_ScanConvMode=DISABLE;
	ADC_Init(ADC1 ,&ADCinit);
	ADC_ExternalTrigConvCmd(ADC1,ENABLE);
  	//���ղ���Ƶ������ָ��ADC�Ĺ�����ͨ����һ�����У�����ʱ��
	if(Rate>=500000)ADC_RegularChannelConfig(ADC1, ADC_Channel_6, 1, ADC_SampleTime_1Cycles5 );	//ADC1,ADCͨ��,����ʱ��Ϊ1.5+12.5=14����	  			    
	else if(Rate>=200000)ADC_RegularChannelConfig(ADC1, ADC_Channel_6, 1, ADC_SampleTime_7Cycles5 );	//ADC1,ADCͨ��,����ʱ��Ϊ7.5+12.5=20����	  			    
	else if(Rate>=100000)ADC_RegularChannelConfig(ADC1, ADC_Channel_6, 1, ADC_SampleTime_13Cycles5 );	//ADC1,ADCͨ��,����ʱ��Ϊ13.5+12.5=26����	  			    
	else if(Rate>=40000)ADC_RegularChannelConfig(ADC1, ADC_Channel_6, 1, ADC_SampleTime_55Cycles5 );	//ADC1,ADCͨ��,����ʱ��Ϊ55.5+12.5=68����	  			    
	else ADC_RegularChannelConfig(ADC1, ADC_Channel_6, 1, ADC_SampleTime_239Cycles5 );	//ADC1,ADCͨ��,����ʱ��Ϊ239.5+12.5=252����	  			    
	ADC_ITConfig(ADC1,ADC_IT_EOC,ENABLE);

    ADC_DMACmd(ADC1, ENABLE);//ADC��DMA����ʹ��
	ADC_Cmd(ADC1, ENABLE);	//ʹ��ָ����ADC1
	
	ADC_ResetCalibration(ADC1);	//ʹ�ܸ�λУ׼  
	while(ADC_GetResetCalibrationStatus(ADC1));	//�ȴ���λУ׼����
	ADC_StartCalibration(ADC1);	 //����ADУ׼
	while(ADC_GetCalibrationStatus(ADC1));	 //�ȴ�У׼����
 
if(Rate>=MaxRate)ADC_SoftwareStartConvCmd(ADC1,ENABLE);//���ڳ���1Mʱ��������ת�������ڱ߽���������
	
	//MY_NVIC_Init(1,3,TIM2_IRQn,2);
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
	nvicInit.NVIC_IRQChannel=ADC1_2_IRQn;
	nvicInit.NVIC_IRQChannelCmd=(Rate<=20000)?ENABLE:DISABLE;//20K���²ſ����ж�
	nvicInit.NVIC_IRQChannelPreemptionPriority=1;
	nvicInit.NVIC_IRQChannelSubPriority=1;
	NVIC_Init(&nvicInit);
}
//�յ����ݺ�����ͨ�������ϴ���Ҫ������>=115200�������ʸ���20K���Զ��ض�
void ADC1_2_IRQHandler(void)
{
	ADC_ClearITPendingBit(ADC1,ADC_IT_EOC);
	if(Osc_Sync==0)USART1->DR=ADC1->DR>>4;
}

volatile u16 ADCbuff[ADCbuffLenth];//ADC�洢��

DMA_InitTypeDef ADC_DMA_InitStructure;//���̼��٣�ע�ⲻ������

void ADC_DMA_Init(u32 buffLenth)
{
	NVIC_InitTypeDef nvicInit;

	if(buffLenth>ADCbuffLenth)buffLenth=ADCbuffLenth;

	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);//ʹ��ʱ��

	DMA_DeInit(DMA1_Channel1);    //��ͨ��һ�Ĵ�����ΪĬ��ֵ
	ADC_DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)&(ADC1->DR);//�ò������Զ���DMA�������ַ
	ADC_DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t)&ADCbuff;//�ò������Զ���DMA�ڴ����ַ(ת���������ĵ�ַ)
	ADC_DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralSRC;//�ò����涨����������Ϊ���ݴ����Ŀ�ĵػ�����Դ���˴�����Ϊ��Դ
	ADC_DMA_InitStructure.DMA_BufferSize = buffLenth;//����ָ��DMAͨ����DMA����Ĵ�С,��λΪ���ݵ�λ������Ҳ����ADCConvertedValue�Ĵ�С
	ADC_DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;//�趨�����ַ�Ĵ����������,�˴���Ϊ���� Disable
	ADC_DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;//�����趨�ڴ��ַ�Ĵ����������,�˴���Ϊ������Enable
	ADC_DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;//���ݿ��Ϊ16λ
	ADC_DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;//���ݿ��Ϊ16λ
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

void ADC_DMA_Restart(void)//����DMA���䣨����������
{
	DMA_DeInit(DMA1_Channel1);
	DMA_Init(DMA1_Channel1, &ADC_DMA_InitStructure);
	DMA_ITConfig(DMA1_Channel1, DMA_IT_TC, ENABLE);
	DMA_Cmd(DMA1_Channel1, ENABLE);
}

u8 Msg_ADC_DMA=0,Msg_OscSetting=0;

void DMA1_Channel1_IRQHandler(void)//DMA�ж�
{
	if(DMA_GetITStatus(DMA1_IT_TC1))//�������
	{
		DMA_ClearITPendingBit(DMA1_IT_GL1);
		//DMA_ClearITPendingBit(DMA1_Channel1_IRQn);
		//ADC_DMA_Restart();
		Msg_ADC_DMA++;//����һ����DMA���������Ϣ��
	}
}

void OscInit(u32 Rate,u32 Lenth)
{
	Timer4_Init(Rate);//��ʼ��������ʱ��Tim4��Ƶ�ʣ�ADC��TIM4.CCR4������
	ADC1_Init(Rate);//�������ʳ�ʼ��ADC����ҪӰ�����ʱ�䣩
	ADC_DMA_Init(Lenth);//DMA��ʼ������ʵ�ʲ���������
}
