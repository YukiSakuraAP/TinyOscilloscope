#include "stm32f10x.h"
#include "sys.h"
#include "delay.h"
#include "usart.h"
#include "LED.h"
#include "Key.h"
#include "TimeBase.h"
#include "stm32_dsp.h"
#include "table_fft.h"
#include "math.h"
#include "Osc.h"
#include "PWM.h"
#include "Shell2.h"
#include "oled.h"
#include "DAC.h"
#include "WaveGen.h"
#include "Clock.h"
/*
��ʾ������YukiSakura_AP @ Bilibili ��д������ת�أ���ע������
ʾ����ģ����ƣ�
�ںˣ�
	ʾ���������ں�(�ӿ�@Osc.h)
	�����벶��ģ��(main.c L187-L288)
���棺
	OLED�밴������(main.c L100-L186)
	��λ���������(Shell.c L26-L73)

ʾ�������ʽ���
	ͬ��ģʽ���첽ģʽ Osc_Sync=1/0��
		�첽ģʽ����Ƭ��ͨ������ʵʱ�����������ϴ�����λ�����������ܵ����ڵ��������10kHz�������д���ģʽ
		ͬ��ģʽ����Ƭ���Ȳ���������������ݣ�Ȼ�󽫲������ݴ���󷵻أ��������ڽ����������ͬ���İ�β

	ʵ�ʲ�������(buffLenth)�������������Ч����������(Osc_WindowLenth)��
		Ϊʵ�ִ���ʱ������ǰ��������������ݣ�������ǡ������Ļ���룩���ڲ���ʱ����ֻ����1����Ļ(����)��ô��
		|-----------------ʵ�ʲ�������-----------------|
		|-�봰��-|--------�����������--------|-�봰��-|
		�������У�ʵ�ʲ�������=3*����
*/

u8 Simulating=0;//û����

void delay(long long i)
{
	while(i--);
}

#define ClockTimeOut 300*1000
//���ڱ������Լӣ�Timebase.c
u32 Msg_500msFlash=0;//500ms���ź���
u32 CNT_ShowInfo=0,CNT_Clock=ClockTimeOut;//ms����ֵ 0Ϊֹͣ  >=1ʱÿ��1ms���Լ�

u32 FFT_in[256]={0};
u32 FFT_out[256]={0};
s16 FFT_mag[256]={0};//FFT���롢FFT�����FFTģ�������ȣ�
s16 FFT_Re,FFT_Im;//FFT���������ʵ�����鲿

//���ڴ���ģ���һЩ����
int TrigLevel=2048,TrigUp=2048+200,TrigDown=2048-200;//������ѹ��ADCֵ�����������ޡ����ޣ�Ϊʵ�־�׼�������������ʩ���ش�����
int Window=1000,HalfWindow=500,buffLenth=4000,CurousEnd=3500;//���ڵ�������Ч�����������봰���ȣ�Ϊ�˼򻯼��㣩��ʵ�ʲ���������ȫ�����������������յ㣨�򻯼��㣩
int TrigAddr=0,Triged=0;//������ַ��������������ʩ���ش�������������������
u8 SyncSequence[8] = { 'S','Y','N','C',0,255,0,255 };//ͬ��ģʽ�£�����λ��ͨ��ʱ��Ϊ��֤����ͬ�����뷢�͵İ�β
#define TrigLen 2
int TrigCnt=0,TrigAddrs[TrigLen]={0};


//����OLED�����һЩ����
u8 DispBuff[32];//��ʾ�����õĻ���
const int  SamplingRateMap[] = { 1000000,500000,250000,100000,50000,25000,10000,5000,2500,1000,500,100,10 };//������Ӧ�Ĳ�����
const int        WindowMap[] = { 1000,   1000,  1000,  1000,  1000, 1000, 1000, 500, 500, 200, 150,100,10 };//�������봰��(��Ч������)��Ӧ
//const u8 SamplingDispMap[][8]= { "500K","250K","100K"," 50K"," 10K","  5K","  1K"," 500"," 100","  10" };
u8 SamplingRateSetting=6;//���������������Ӧ������Key2���������ﵽ12����㣨���鳤�ȣ�
int ZeroLine=2048;//Y������ʱ�����Ķ�Ӧ��ADC��ѹֵ
u8 Zoom=0;//���ű���=2^Zoom������Key1�����������Ϊ4
float Freq=0;
u8 Disp_Osc_FFT=0;//ʾ����ģʽ��FFTģʽ��ת��
u8 WaveGen=0;//���������־������0-�رգ�1-�����2-���ã�3-����

#define WaveSelMax 6
#define WaveTypeMax 3
const u8 WaveGenName[WaveSelMax][10]={"      ","WavSet","WavGen","AlgSet","AlgOsc","PlyWav"};
const char WaveTypeName[WaveTypeMax][3]={"S","T","O"};
const int WaveFreqX[WaveSelMax]={20,  50,   100,  300,  500,};
const int WaveFreqY[WaveSelMax]={80,  100,  200,  600,  1000,};
const int WGTbaseF[WaveSelMax]= {4000,10000,20000,18000,20000};
u8 WaveSel=0,WaveType=0;
u32 WavRecvLen=0;
const int ScanRateMap[]={1000,2000,4000,6000,10000,12500};
u8 ScanRate=2,ScanZoom=5;//���Ŵ�32����2^SZ��

extern int Msg_PathFin;
#define DPMax2 (MaxDots*2)
extern u8 DotPath[DPMax2];

u8 Msg_Refresh=0;

int main(void)
{
	s32 i,j,OLED_Flow=0,OLED_Dir=1;
	
	
	//һЩ��ʼ������������ʾ
	Stm32_Clock_Init(7);//�˴�ʹ��56Mģʽ����ҪΪ�˼���ADC��1Mģʽ
	delay_init(56);//����56MHzģʽ
	OLED_Init();
	OLED_Clear();
	OLED_Display_On();
	OLED_ShowString(0,0,(u8*)"System Initing..",0xFF);
	OLED_ShowString(0,2,(u8*)" Yuki_Sakura_AP ",0xFF);
	OLED_ShowString(0,4,(u8*)"Bilibili Cheers!",0xFF);
	OLED_ShowString(0,6,(u8*)"uid 9063747 v3.0",0xFF);
	delay_ms(100);
//	OLED_Refresh_Gram();
	uart_init(56,115200);//����56MHzģʽ
	printf("This is A test!!!\r\n");
	LED_Init();
	KEY_Init();
	Timer2_Init();//ʱ����ʼ����Timebase.c��
	if(RTC_Init()){OLED_ShowString(0,2,(u8*)"RTC Init Failure",0);delay_ms(5000);}
	else OLED_ShowString(0,2,(u8*)"RTC Clock Inited",0);
	RCC->APB1ENR|=1<<28;     //ʹ�ܵ�Դʱ��	    
	RCC->APB1ENR|=1<<27;
	RCC->APB1RSTR|=1<<27;  
	RCC->APB1RSTR&=0xF7FFFFFF;  
	PWR->CR|=1<<8;           //ȡ��������д����
	DAC_Dual_DeInit();
	WaveGen_Init();
//	for(i=0;i<50;i++)
//	{
//		DACbuff[0][i]=2048+2000*sin(3.1415926*2*i/10);//50Hz
//		DACbuff[1][i]=2048+2000*sin(3.1415926*2*i/50);//100Hz
//	}
	WaveGenSine(WaveFreqX[0],WaveFreqY[0],WGTbaseF[0]);
	
	OLED_ShowString(0,0,(u8*)"System Inited...",0xFF);
	OLED_ShowString(0,4,(u8*)"K0:OSC K1:XY Osc",0);
	OLED_ShowString(0,6,(u8*)"K2:Wav K4:Clock ",0);
	delay_ms(700);
	if(getKey(Key0,10000))BKP->DR3=0;
	if(getKey(Key1,10000))BKP->DR3=1;
	if(getKey(Key2,10000))BKP->DR3=2;
	if(getKey(Key3,10000))BKP->DR3=3;
	switch(BKP->DR3)
	{
		case 0:break;//ʾ����ģʽ
		case 1:LissajousPlot();break;//
		case 2:BadApplePlayer();break;//
		case 3:Clock();CNT_Clock=ClockTimeOut;break;//�������水��Key4����ʱ�ӣ�ʾ�������濪��ʱ��
	}
	OLED_Clear();
	OLED_ShowString(0,0,(u8*)"Osc Initing...   ",0);
	OscInit(Osc_SamplingRate,Osc_WindowLenth);//ʾ������ʼ���������ʣ�ʵ�ʲ���������
	Timer3_Init();//200Hz PWM��������Ϊռ�ձȰ�Sin�����仯������3s��
	OLED_ShowString(0,6,(u8*)"Tim Inited...   ",0);
//	OLED_Refresh_Gram();
	delay_ms(300);
	OLED_Clear();
	OLED_DrawLine(0,0,128,64);
	OLED_DrawLine(0,64,128,0);
	OLED_DrawLine(0,32,128,32);
	OLED_DrawLine(64,0,64,64);
	OLED_Refresh_Gram();
	Mod_Recv=Recv_Cmd|Recv_Pack;
	while(1)
	{
		if(USART_RX_STA&0x8000)Shell2(USART_RX_BUF,USART_RX_STA&0x3fff,1);//������λ�����ڷ��͵�����
		if(Msg_PathFin)
		{
			Wavlen=5;
			for(i=0;i<Msg_PathFin;i++)
			{
				WavData[i]=XYc_OutputInv(DotPath[2*i+1]*24,DotPath[2*i]*24);
			}
			if(Msg_PathFin>=MaxDots)Wavlen=MaxDots;
			else if(Msg_PathFin>=3)Wavlen=Msg_PathFin;
			else Wavlen=Msg_PathFin+2;
			WaveGen=3;
			WavRecvLen=Msg_PathFin;
			SetTimer5BaseFreq(12500);
			Msg_PathFin=0;
			Msg_OscSetting++;
//			USART1->DR='A';
		}
		if(getKey(Key0,500))//����Key0����ʱ��һֱ���Ż�ÿ��500ms����һ�Σ�
		{
			LED0=!LED0;
			if(CNT_ShowInfo)
			{
				WaveGen=(WaveGen+1)%(WavRecvLen?6:5);//WaveGenģʽѡ��
				if(!WaveGen)OLED_ShowStringG(80,0,(u8*)"      ",0);//�����ʾ�ġ�WavGen��
				CNT_ShowInfo=1;//��ʼ�˵���ʾ��ʱ
				GPIOA->CRL&=0x0FFFFFFF;//�ر�PWM������Ĵ�����̣�
				if(WaveGen)
				{
					GPIOA->CRL|=0xB0000000;//����(PWMout)�����Ƿ����¿���PWM���
					DAC_Dual_Init();
				}
				else DAC_Dual_DeInit();
				Msg_OscSetting++;
			}
			else
			{
				Disp_Osc_FFT=!Disp_Osc_FFT;//�ǲ˵���ʾģʽ�£�Key1�л�FFT
			}
			printf("Key0 Pressed!!!\r\n");
			CNT_Clock=ClockTimeOut;
		}
		if(getKey(Key1,500))//����Key1����ʱ��һֱ���Ż�ÿ��500ms����һ�Σ�
		{
			if(WaveGen==1)
			{
				if(CNT_ShowInfo)WaveType++;
				WaveType%=3;
			}
			else if(WaveGen==3)
			{
				if(CNT_ShowInfo)ScanZoom++;
				ScanZoom%=6;
			}
			else
			{
				//LED1=!LED1;
				if(CNT_ShowInfo)Zoom++;//�˵���ʾ״̬��Zoom++
				Zoom%=5;//����Zoom��0-4�仯
			}
			CNT_ShowInfo=1;//��ʼ�˵���ʾ��ʱ
			Msg_OscSetting++;//����һ��������ʾ����������Ϣ��
			printf("Key1 Pressed!!!\r\n");
			CNT_Clock=ClockTimeOut;
		}
		if(getKey(Key2,500))//����Key2����ʱ��һֱ���Ż�ÿ��500ms����һ�Σ�
		{
			if(WaveGen==1)
			{
				if(CNT_ShowInfo)WaveSel++;
				WaveSel%=WaveSelMax;
			}
			else if(WaveGen==3)
			{
				if(CNT_ShowInfo)ScanRate++;
				ScanRate%=5;
			}
			else
			{
				if(CNT_ShowInfo)SamplingRateSetting+=11;//�˵���ʾ״̬��SamplingRateSetting--
				SamplingRateSetting%=12;//����SamplingRateSetting��������Ч���̼�仯(0-11)
				Osc_SamplingRate=SamplingRateMap[SamplingRateSetting];//����ʾ�����ں��еĲ����ʼ�¼(Osc_SamplingRate)
				Osc_WindowLenth=WindowMap[SamplingRateSetting];//����ʾ�����ں��еĴ����ȼ�¼(Osc_WindowLenth)
				Osc_TrigLevel=2048;//������ƽ����
				Osc_SmitLevel=200;//ʩ���ش�����ѹ���ã�TrigLevel �� SimtLevel��
				Osc_Sync=1;//ͬ��ģʽ
			}
			CNT_ShowInfo=1;//��ʼ�˵���ʾ��ʱ
			Msg_OscSetting++;//����һ��������ʾ����������Ϣ��
			printf("Key2 Pressed!!!\r\n");
			CNT_Clock=ClockTimeOut;
		}
		if(getKey(Key3,500))//����Key3����ʱ��һֱ���Ż�ÿ��500ms����һ�Σ�
		{
			CNT_ShowInfo=!CNT_ShowInfo;//�򿪻�رղ˵���ʾ
			printf("Key3 Pressed!!!\r\n");
			CNT_Clock=ClockTimeOut;
		}
		if(Msg_ADC_DMA&&Osc_Sync)//�����ڡ�DMA���������Ϣ����������ɣ�������ͬ��ģʽ�£�
		{//���д����о����ҵ����ϴ��������Ĳ����������
			Msg_ADC_DMA=0;//��ա�DMA���������Ϣ����־
			Triged=0;//���ʩ���ش�����־
			TrigAddr=j=HalfWindow;//Ĭ�ϴ�����Ϊ�봰��������Ҳ��������㣬����ʾ����������ǰ������ݣ�
			for(TrigCnt=0;TrigCnt<TrigLen;TrigCnt++)TrigAddrs[TrigCnt]=0;
			TrigCnt=0;
			LED1=1;//���������־��
			for(i=HalfWindow;i<CurousEnd;i++)//����ȫ���Ĳ�������
			{
				if(Triged)//�Ѿ���⵽�㹻�͵ĵ�ѹ�ˣ�ʩ�������ޣ�
				{
					if((ADCbuff[i-1]<TrigLevel)&&(ADCbuff[i]>=TrigLevel))//�жϷ���������ƽ����������
						{j=i;}//��¼λ��
					if(ADCbuff[i]>(TrigUp))//�ּ�⵽��һ���㹻�ߵĵ�ѹ��ʩ�������ޣ�
					{
						Triged=0;//���ʩ���ش�����־
						if(TrigAddr!=j)
						{
							TrigAddr=j;//������λ�ü�¼��������
							TrigAddrs[TrigCnt++]=TrigAddr;
							LED1=0;//����������־��
						}
						if(TrigCnt>=TrigLen)break;
					}
				}
				else//��û�м�⵽�㹻�͵ĵ�ѹ��ʩ�������ޣ�
				{
					if(ADCbuff[i]<(TrigDown))
					{
						Triged=1;//��⵽�˾�ת��ʩ���ش���״̬
					}
				}
			}
			if(TrigAddrs[1]*TrigAddrs[0])//���ô���ʱ������Ƶ��
			{
				Freq=TrigAddrs[1]-TrigAddrs[0];//���δ���֮�����һ������
				Freq=Osc_SamplingRate/Freq;
			}
			else Freq=-1;
			
			//��λ�������ݷ���
			for(j=0;j<Window;j++)//��������
				fputc(ADCbuff[TrigAddr+j-HalfWindow]>>4,NULL);
			for(j=0;j<8;j++)//ͬ����β
				fputc(SyncSequence[j],(FILE *)0);
			
			if(TrigAddrs[0])TrigAddr=TrigAddrs[0];//���TrigAddrs��¼������1�����ݣ�ȷʵ�����ˣ���ȡ�״δ�����ַ��Ϊ��ʾ����
			//OLED��ʾ
			OLED_ClearG();
			if(Disp_Osc_FFT==0)//ʾ��������ģʽ
			{
				for(i=0;i<64;i+=10)//����X������̶�
				{
					OLED_DrawLine(64+i,31,64+i,33);
					OLED_DrawLine(64-i,31,64-i,33);
				}
				if(HalfWindow>64)//��������������OLED�������(�봰>���OLED��ʾ����)
					for(i=0;i<127;i++)
						OLED_DrawLineRev(i,32+(ADCbuff[TrigAddr-64+i]-ZeroLine)/(1<<(6-Zoom)),i+1,32+(ADCbuff[TrigAddr-64+i+1]-ZeroLine)/(1<<(6-Zoom)));
				else //������ݲ�����վռ��������Ļ
				{
					for(i=0;((i+TrigAddr+1)<buffLenth)&&(i<63);i++)//����������
						OLED_DrawLineRev(64+i,32+(ADCbuff[TrigAddr+i]-ZeroLine)/(1<<(6-Zoom)),64+i+1,32+(ADCbuff[TrigAddr+i+1]-ZeroLine)/(1<<(6-Zoom)));
					for(i=0;((i+TrigAddr-1)>0)&&(i>(-63));i--)//���Ƹ�����
						OLED_DrawLineRev(64+i,32+(ADCbuff[TrigAddr+i]-ZeroLine)/(1<<(6-Zoom)),64+i-1,32+(ADCbuff[TrigAddr+i-1]-ZeroLine)/(1<<(6-Zoom)));
				}
				if(TrigAddr!=HalfWindow)//ȷʵ�����ˣ�����Y��������
				{
					OLED_DrawLine(64,0,64,64);
					for(i=0;i<32;i+=10)
					{
						OLED_DrawLine(63,32+i,65,32+i);
						OLED_DrawLine(63,32-i,65,32-i);
					}
				}
				OLED_DrawLine(0,32,128,32);//����X������
			}
			else//FFT��ʾģʽ
			{
				if(buffLenth>=256)//��֤����������FFT�����ࣨ�����ϣ�����ͨ��ĩβ������ʵ��FFT��
				{
					//��ȡ�����ʵ���
					if(TrigAddr>=128)//һ����˵���ӳ������㸽��ȡֵ
						for(i=0;i<256;i++)
							FFT_in[i]=ADCbuff[TrigAddr-128+i];
					else //����Ӵ�����������ȡֵ����128���ʹ�0��ʼȡֵ
						for(i=0;i<256;i++)
							FFT_in[i]=ADCbuff[i];
cr4_fft_256_stm32(FFT_out,FFT_in,256);//FFT���������ÿ⺯����
					for(i=0;i<256;i++)
					{//����ģ�������FFT��
						FFT_Re=FFT_out[i]&0xFFFF;//ʵ��
						FFT_Im=FFT_out[i]>>16;//�鲿
						FFT_mag[i]=sqrt(FFT_Re*FFT_Re+FFT_Im*FFT_Im)*2;//ģ�������ǶԳ��ԣ�����2�õ�ʵ��ǿ��
					}
					FFT_mag[0]=FFT_mag[0]/2;//ֱ����������Ҫ*2�����������������
					for(i=0;i<128;i++)
						OLED_DrawLine(i,0,i,(FFT_mag[i]>>(6-Zoom)));//����Ƶ�ף�stem��ʽ��
					
					for(i=0;i<128;i+=16)//����X������̶�
						OLED_DrawLine(i,0,i,2);
					for(i=0;i<64;i+=16)//����Y������̶�
						OLED_DrawLine(0,i,2,i);
					OLED_ShowStringG(104,0,(u8*)"FFT",0);
				}
				
				else
				{
					OLED_ShowStringG(0,2,(u8*)"FFT Mode OFF",0);
					OLED_ShowStringG(0,4,(u8*)"Dot*3 < 256",0);
				}
			}
			if((WaveGen>=3)&&(WaveGen<=4))//��ͼģʽ
			{
				if(1 || Disp_Osc_FFT==0)//ʾ��������ģʽ
				{
					//Wavlen=3;
				if(HalfWindow>64)//��������������OLED�������(�봰>���OLED��ʾ����)
						for(i=0;i<127;i++)
							WavData[i]=XYc_OutputInv(((i-64)<<ScanZoom)+2048,2048-(ADCbuff[TrigAddr-64+i]-ZeroLine)/(1<<(6-ScanZoom-Zoom)));
					else //������ݲ�����վռ��������Ļ
					{
						i=0;
						for(j=-TrigAddr;j<64;j++)
						{
							if(j<-63)continue;
							if(j>buffLenth-TrigAddr)break;
							WavData[i++]=XYc_OutputInv(((j)<<ScanZoom)+2048,2048+(ADCbuff[TrigAddr+j]-ZeroLine)/(1<<(6+ScanZoom-Zoom)));
						}
					}
					for(j=0;j<8;j++)WavData[i++]=XYc_OutputInv(2047+((64-8*j)<<ScanZoom),2048);//�ұ߽�
					WavData[i++]=XYc_OutputInv(2048,2048);//�е�
					for(j=0;j<4;j++)WavData[i++]=XYc_OutputInv(2048,2048-((8*j)<<ScanZoom));//�ϱ߽�
					for(j=0;j<=8;j++)WavData[i++]=XYc_OutputInv(2048,2048-((32-8*j)<<ScanZoom));//�±߽�
					for(j=0;j<4;j++)WavData[i++]=XYc_OutputInv(2048,2048-((8*j-32)<<ScanZoom));//����
					WavData[i++]=XYc_OutputInv(2048,2048);//�е�
					for(j=0;j<=8;j++)WavData[i++]=XYc_OutputInv(2049-((8*j)<<ScanZoom),2048);//��߽�
					Wavlen=i;
				}
				else//FFT��ʾģʽ
				{
				}
			}
			ADC_DMA_Restart();
			//OLED_Refresh_Gram();
			Msg_Refresh++;
			LED0=!LED0;
//			if(!CNT_ShowInfo)OLED_Refresh_Gram();
//			else goto OLED_Refresh;
		}
		else if(!Osc_Sync)
		{//ͬ��ģʽ��һֱ�����˵���ʾ����ʾ��Ϣ�Ͷ�����
			CNT_ShowInfo=1;
		}
		
		OLED_Refresh://�˵���ʾ����
		if((CNT_ShowInfo<5000)&&(CNT_ShowInfo>0))//��(CNT_ShowInfo)��1-4999֮��ʱ����ʾ��Ϣ��5s�Զ��رղ˵���
		{
			if(Osc_Sync)//ͬ��ģʽ
			{
				i=Osc_SamplingRate;//���ڲ����ʵ���ʾ����ʾ�úÿ���
				if(i>=1000000)sprintf((char*)DispBuff,"%3dM",(int)i/1000000);
				else if(i>=10000)sprintf((char*)DispBuff,"%3dK",(int)i/1000);
				else if(i>=1000)sprintf((char*)DispBuff,"%.1fK",i/1000.0);
				else sprintf((char*)DispBuff,"%4d",(int)i);
				OLED_ShowStringG(0,0,DispBuff,(WaveGen%2!=1?0xFF:0)*Osc_Sync);//��ʾ��OLED�ϣ����Ͻǣ�
				if(Disp_Osc_FFT)OLED_ShowStringG(32,0,(u8*)"/2=Fmax",0);//��ʾ��OLED�ϣ����Ͻǣ�
				i=Freq;//����Ƶ�ʵ���ʾ����ʾ�úÿ���
				if(i>=10000)sprintf((char*)DispBuff,"F=%4dK",(int)i/1000);
				else if(i>=1000)sprintf((char*)DispBuff,"F=%.2fK",i/1000.0);
				else sprintf((char*)DispBuff,"F=%5d",(int)i);
				if(Freq>0)OLED_ShowStringG(72,6,DispBuff,0xFF);//�����½���ʾƵ��
				else OLED_ShowStringG(72,6,(u8*)"F= *** ",0xFF);//�����½���ʾƵ�ʲ���ʧ��
				i=1<<Zoom;//����Ŵ���=2^Zoom
				sprintf((char*)DispBuff,"Y:%2dx",(int)i);
				OLED_ShowStringG(0,6,DispBuff,(WaveGen%2!=1?0xFF:0));//��ʾ��OLED���½�
			}
			else//�첽ģʽ
			{
				i=Osc_SamplingRate;//���ڲ����ʵ���ʾ����ʾ�úÿ���
				if(i>=1000000)sprintf((char*)DispBuff,"Rate:%3dM",(int)i/1000000);
				else if(i>=10000)sprintf((char*)DispBuff,"Rate:%3dK",(int)i/1000);
				else if(i>=1000)sprintf((char*)DispBuff,"Rate:%.1fK",i/1000.0);
				else sprintf((char*)DispBuff,"Rate:%4d",(int)i);
				OLED_ShowStringG(0,0,DispBuff,(WaveGen%2!=1?0xFF:0)*Osc_Sync);//��ʾ��OLED�ϵ�һ��
				i=Osc_WindowLenth;//���ڴ�������ʾ����ʾ�úÿ���
				if(i>=10000)sprintf((char*)DispBuff,"Window:%3dK",(int)i/1000);
				else if(i>=1000)sprintf((char*)DispBuff,"Window:%.1fK",i/1000.0);
				else sprintf((char*)DispBuff,"Window:%4d",(int)i);
				OLED_ShowStringG(0,2,DispBuff,0);//�ڵڶ���
				sprintf((char*)DispBuff,"Trig:%4d#%d",(int)Osc_TrigLevel,(int)Osc_SmitLevel);
				OLED_ShowStringG(0,4,DispBuff,0x02);//��ͬ��ģʽ�Ĵ�����Ϣ��ʾ����Ļ�����У����ǻ�����
				OLED_ShowStringG(OLED_Flow-8,6,(u8*)" uploading",0);//��ʾһ����̬��"uploading"
				OLED_Flow+=OLED_Dir;
				if(OLED_Flow==0)OLED_Dir=1;
				if(OLED_Flow>=56)OLED_Dir=-1;
			}
			if(WaveGen)
			{
				OLED_ShowStringG(80,0,(u8*)WaveGenName[WaveGen],WaveGen%2?0xFF:0);//�����Ͻ���ʾ����ģʽ
				if(WaveGen<=2)
				{
					if(WaveFreqX[WaveSel]<1000)sprintf((char*)DispBuff,"%s%4d",WaveTypeName[WaveType],(int)WaveFreqX[WaveSel]);
					else sprintf((char*)DispBuff,"%s%3dK",WaveTypeName[WaveType],(int)WaveFreqX[WaveSel]/1000);
					OLED_ShowStringG(88,2,(u8*)DispBuff,WaveGen==1?0xFF:0);//��ʾXƵ��
					if(WaveFreqY[WaveSel]<1000)sprintf((char*)DispBuff,"%s%4d",WaveTypeName[WaveType],(int)WaveFreqY[WaveSel]);
					else sprintf((char*)DispBuff,"%s%3dK",WaveTypeName[WaveType],(int)WaveFreqY[WaveSel]/1000);
					OLED_ShowStringG(88,4,(u8*)DispBuff,WaveGen==1?0xFF:0);//��ʾYƵ��
				}
				else if(WaveGen<=4)
				{
					if(ScanRateMap[ScanRate]<1000)sprintf((char*)DispBuff,"R:%4d",(int)ScanRateMap[ScanRate]);
					else sprintf((char*)DispBuff,"R:%3dK",(int)ScanRateMap[ScanRate]/1000);
					OLED_ShowStringG(80,2,(u8*)DispBuff,WaveGen==3?0xFF:0);//��ʾɨ��Ƶ��
					sprintf((char*)DispBuff,"Z:%3dx",(int)(1<<ScanZoom));
					OLED_ShowStringG(80,4,(u8*)DispBuff,WaveGen==3?0xFF:0);//��ʾ���ű���
				}
			}
			//else OLED_ShowString(80,0,"      ",0);
			//OLED_Refresh_Gram();
			Msg_Refresh++;
		}
		else CNT_ShowInfo=0;//��������������ʾ���˵��رգ�
		if((Msg_500msFlash==(Osc_Sync+1)))//��ͬ��ģʽ��1s��������������������أ���ͬ��ģʽ��0.5s�������ȶ�������
		{
			Msg_500msFlash=0;
			LED0=!LED0;
		}
		if(Msg_OscSetting)//�����ڡ�����ʾ����������Ϣ��ʱ���������¼���
		{
			Msg_OscSetting=0;//��ա�����ʾ����������Ϣ����־
			TrigUp=Osc_TrigLevel+Osc_SmitLevel;//����ʩ���ش�������
			if(TrigUp>=4096)TrigUp=4095;//��ֹ��ֵ������Χ
			TrigDown=Osc_TrigLevel-Osc_SmitLevel;//����ʩ���ش�������
			if(TrigDown<0)TrigDown=0;//��ֹ��ֵ������Χ
			TrigLevel=Osc_TrigLevel;//���ô�����ƽ
			Window=Osc_WindowLenth;//���ô���
			HalfWindow=Window/2+Window%2;//����봰�����������������㣩
			//���ݴ����������ʻ����ڴ��С��ͬ����ʵ�ʲ�����
			if(Osc_SamplingRate<=30000)buffLenth=(Window>(ADCbuffLenth/3))?ADCbuffLenth:3*Window;
			else buffLenth=ADCbuffLenth;
			CurousEnd=buffLenth-HalfWindow;//���㴥�����������յ�
			OscInit(Osc_SamplingRate,buffLenth);//���³�ʼ��ʾ��������
			switch(WaveGen)
			{
				case 0:break;
				case 1:
				case 2:
					if(WaveType==0)WaveGenSine(WaveFreqX[WaveSel],WaveFreqY[WaveSel],WGTbaseF[WaveSel]);
					if(WaveType==1)WaveGenTriA(WaveFreqX[WaveSel],WaveFreqY[WaveSel],WGTbaseF[WaveSel]);
					if(WaveType==2)WaveGenTASW(WaveFreqX[WaveSel],WaveFreqY[WaveSel],WGTbaseF[WaveSel]);
						break;
				case 3:SetTimer5BaseFreq(ScanRateMap[ScanRate]);break;
				case 4:SetTimer5BaseFreq(ScanRateMap[ScanRate]);break;
				case 5:Msg_PathFin=WavRecvLen;break;
				default:break;
			}
			//OLED_Clear();//�����ػ�
			if(Osc_Sync)
			{
				if(Disp_Osc_FFT==0)
				{
					for(i=0;i<64;i+=10)//����X������̶�
						{OLED_DrawLine(64+i,31,64+i,33);OLED_DrawLine(64-i,31,64-i,33);}
					OLED_DrawLine(0,32,128,32);//����X������
				}
				else
				{
					for(i=0;i<128;i+=16)//����X������̶�
						OLED_DrawLine(i,0,i,2);
					for(i=0;i<64;i+=16)//����Y������̶�
						OLED_DrawLine(0,i,2,i);
				}
			}
			//OLED_Refresh_Gram();
			Msg_Refresh++;
			CNT_Clock=ClockTimeOut;
		}
		if((!CNT_Clock)&&(BKP->DR3==3))//����ʱ��ģʽ�Ž���ʱ��
		{
			Clock();
			CNT_Clock=ClockTimeOut;
		}
		if(Msg_Refresh)
		{
			OLED_Refresh_Gram();
		}
	}
}
