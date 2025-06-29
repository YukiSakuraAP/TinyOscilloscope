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
本示波器由YukiSakura_AP @ Bilibili 编写，如需转载，请注明出处
示波器模块设计：
内核：
	示波器采样内核(接口@Osc.h)
	触发与捕获模块(main.c L187-L288)
界面：
	OLED与按键界面(main.c L100-L186)
	上位机命令解析(Shell.c L26-L73)

示波器名词解释
	同步模式与异步模式 Osc_Sync=1/0：
		异步模式：单片机通过串口实时将采样数据上传给上位机，采样率受到串口的限制最大10kHz，不带有触发模式
		同步模式：单片机先采样，保存采样数据，然后将采样数据处理后返回，带有用于将数据与界面同步的包尾

	实际采样点数(buffLenth)与采样窗长（有效采样点数）(Osc_WindowLenth)：
		为实现触发时触发点前后均有完整的数据（触发点恰好在屏幕中央），在采样时并非只采样1个屏幕(窗口)那么长
		|-----------------实际采样点数-----------------|
		|-半窗长-|--------触发检测区域--------|-半窗长-|
		本程序中，实际采样点数=3*窗长
*/

u8 Simulating=0;//没有用

void delay(long long i)
{
	while(i--);
}

#define ClockTimeOut 300*1000
//关于变量的自加：Timebase.c
u32 Msg_500msFlash=0;//500ms的信号量
u32 CNT_ShowInfo=0,CNT_Clock=ClockTimeOut;//ms计数值 0为停止  >=1时每隔1ms会自加

u32 FFT_in[256]={0};
u32 FFT_out[256]={0};
s16 FFT_mag[256]={0};//FFT输入、FFT输出，FFT模长（幅度）
s16 FFT_Re,FFT_Im;//FFT复数输出的实部与虚部

//关于触发模块的一些设置
int TrigLevel=2048,TrigUp=2048+200,TrigDown=2048-200;//触发电压（ADC值），触发上限、下限（为实现精准触发，采用软件施密特触发）
int Window=1000,HalfWindow=500,buffLenth=4000,CurousEnd=3500;//窗口点数（有效数据数），半窗长度（为了简化计算），实际采样点数（全部数据数），窗口终点（简化计算）
int TrigAddr=0,Triged=0;//触发地址（数组索引），施密特触发的输出（触发情况）
u8 SyncSequence[8] = { 'S','Y','N','C',0,255,0,255 };//同步模式下，与上位机通信时，为保证数据同步必须发送的包尾
#define TrigLen 2
int TrigCnt=0,TrigAddrs[TrigLen]={0};


//关于OLED界面的一些设置
u8 DispBuff[32];//显示内容用的缓存
const int  SamplingRateMap[] = { 1000000,500000,250000,100000,50000,25000,10000,5000,2500,1000,500,100,10 };//按键对应的采样率
const int        WindowMap[] = { 1000,   1000,  1000,  1000,  1000, 1000, 1000, 500, 500, 200, 150,100,10 };//采样率与窗长(有效数据数)对应
//const u8 SamplingDispMap[][8]= { "500K","250K","100K"," 50K"," 10K","  5K","  1K"," 500"," 100","  10" };
u8 SamplingRateSetting=6;//与上面两个数组对应，按下Key2后自增，达到12后归零（数组长度）
int ZeroLine=2048;//Y轴缩放时，中心对应的ADC电压值
u8 Zoom=0;//缩放倍数=2^Zoom，按下Key1后自增，最大为4
float Freq=0;
u8 Disp_Osc_FFT=0;//示波器模式与FFT模式的转换
u8 WaveGen=0;//波形输出标志变量，0-关闭，1-输出，2-配置，3-下载

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
u8 ScanRate=2,ScanZoom=5;//最大放大32倍（2^SZ）

extern int Msg_PathFin;
#define DPMax2 (MaxDots*2)
extern u8 DotPath[DPMax2];

u8 Msg_Refresh=0;

int main(void)
{
	s32 i,j,OLED_Flow=0,OLED_Dir=1;
	
	
	//一些初始化，如名称所示
	Stm32_Clock_Init(7);//此处使用56M模式，主要为了兼容ADC的1M模式
	delay_init(56);//适配56MHz模式
	OLED_Init();
	OLED_Clear();
	OLED_Display_On();
	OLED_ShowString(0,0,(u8*)"System Initing..",0xFF);
	OLED_ShowString(0,2,(u8*)" Yuki_Sakura_AP ",0xFF);
	OLED_ShowString(0,4,(u8*)"Bilibili Cheers!",0xFF);
	OLED_ShowString(0,6,(u8*)"uid 9063747 v3.0",0xFF);
	delay_ms(100);
//	OLED_Refresh_Gram();
	uart_init(56,115200);//适配56MHz模式
	printf("This is A test!!!\r\n");
	LED_Init();
	KEY_Init();
	Timer2_Init();//时基初始化（Timebase.c）
	if(RTC_Init()){OLED_ShowString(0,2,(u8*)"RTC Init Failure",0);delay_ms(5000);}
	else OLED_ShowString(0,2,(u8*)"RTC Clock Inited",0);
	RCC->APB1ENR|=1<<28;     //使能电源时钟	    
	RCC->APB1ENR|=1<<27;
	RCC->APB1RSTR|=1<<27;  
	RCC->APB1RSTR&=0xF7FFFFFF;  
	PWR->CR|=1<<8;           //取消备份区写保护
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
		case 0:break;//示波器模式
		case 1:LissajousPlot();break;//
		case 2:BadApplePlayer();break;//
		case 3:Clock();CNT_Clock=ClockTimeOut;break;//开机界面按下Key4进入时钟，示波器界面开启时钟
	}
	OLED_Clear();
	OLED_ShowString(0,0,(u8*)"Osc Initing...   ",0);
	OscInit(Osc_SamplingRate,Osc_WindowLenth);//示波器初始化（采样率，实际采样点数）
	Timer3_Init();//200Hz PWM输出，输出为占空比按Sin函数变化（周期3s）
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
		if(USART_RX_STA&0x8000)Shell2(USART_RX_BUF,USART_RX_STA&0x3fff,1);//解析上位机串口发送的命令
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
		if(getKey(Key0,500))//按键Key0按下时（一直按着会每隔500ms触发一次）
		{
			LED0=!LED0;
			if(CNT_ShowInfo)
			{
				WaveGen=(WaveGen+1)%(WavRecvLen?6:5);//WaveGen模式选择
				if(!WaveGen)OLED_ShowStringG(80,0,(u8*)"      ",0);//清空显示的“WavGen”
				CNT_ShowInfo=1;//开始菜单显示计时
				GPIOA->CRL&=0x0FFFFFFF;//关闭PWM输出（寄存器编程）
				if(WaveGen)
				{
					GPIOA->CRL|=0xB0000000;//按照(PWMout)决定是否重新开启PWM输出
					DAC_Dual_Init();
				}
				else DAC_Dual_DeInit();
				Msg_OscSetting++;
			}
			else
			{
				Disp_Osc_FFT=!Disp_Osc_FFT;//非菜单显示模式下，Key1切换FFT
			}
			printf("Key0 Pressed!!!\r\n");
			CNT_Clock=ClockTimeOut;
		}
		if(getKey(Key1,500))//按键Key1按下时（一直按着会每隔500ms触发一次）
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
				if(CNT_ShowInfo)Zoom++;//菜单显示状态才Zoom++
				Zoom%=5;//限制Zoom在0-4变化
			}
			CNT_ShowInfo=1;//开始菜单显示计时
			Msg_OscSetting++;//发送一条“更新示波器设置消息”
			printf("Key1 Pressed!!!\r\n");
			CNT_Clock=ClockTimeOut;
		}
		if(getKey(Key2,500))//按键Key2按下时（一直按着会每隔500ms触发一次）
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
				if(CNT_ShowInfo)SamplingRateSetting+=11;//菜单显示状态才SamplingRateSetting--
				SamplingRateSetting%=12;//限制SamplingRateSetting在数组有效长短间变化(0-11)
				Osc_SamplingRate=SamplingRateMap[SamplingRateSetting];//更新示波器内核中的采样率记录(Osc_SamplingRate)
				Osc_WindowLenth=WindowMap[SamplingRateSetting];//更新示波器内核中的窗长度记录(Osc_WindowLenth)
				Osc_TrigLevel=2048;//触发电平设置
				Osc_SmitLevel=200;//施密特触发电压设置（TrigLevel ± SimtLevel）
				Osc_Sync=1;//同步模式
			}
			CNT_ShowInfo=1;//开始菜单显示计时
			Msg_OscSetting++;//发送一条“更新示波器设置消息”
			printf("Key2 Pressed!!!\r\n");
			CNT_Clock=ClockTimeOut;
		}
		if(getKey(Key3,500))//按键Key3按下时（一直按着会每隔500ms触发一次）
		{
			CNT_ShowInfo=!CNT_ShowInfo;//打开或关闭菜单显示
			printf("Key3 Pressed!!!\r\n");
			CNT_Clock=ClockTimeOut;
		}
		if(Msg_ADC_DMA&&Osc_Sync)//当存在“DMA传输完成消息”（采样完成），且在同步模式下：
		{//进行触发判决，找到符合触发条件的采样点和区域
			Msg_ADC_DMA=0;//清空“DMA传输完成消息”标志
			Triged=0;//清空施密特触发标志
			TrigAddr=j=HalfWindow;//默认触发点为半窗长（如果找不到触发点，就显示采样数据最前面的内容）
			for(TrigCnt=0;TrigCnt<TrigLen;TrigCnt++)TrigAddrs[TrigCnt]=0;
			TrigCnt=0;
			LED1=1;//灭掉触发标志灯
			for(i=HalfWindow;i<CurousEnd;i++)//遍历全部的采样数据
			{
				if(Triged)//已经检测到足够低的电压了（施密特下限）
				{
					if((ADCbuff[i-1]<TrigLevel)&&(ADCbuff[i]>=TrigLevel))//判断发生触发电平附近的跳变
						{j=i;}//记录位置
					if(ADCbuff[i]>(TrigUp))//又检测到了一次足够高的电压（施密特上限）
					{
						Triged=0;//清空施密特触发标志
						if(TrigAddr!=j)
						{
							TrigAddr=j;//将触发位置记录下来备用
							TrigAddrs[TrigCnt++]=TrigAddr;
							LED1=0;//点亮触发标志灯
						}
						if(TrigCnt>=TrigLen)break;
					}
				}
				else//还没有检测到足够低的电压（施密特下限）
				{
					if(ADCbuff[i]<(TrigDown))
					{
						Triged=1;//检测到了就转变施密特触发状态
					}
				}
			}
			if(TrigAddrs[1]*TrigAddrs[0])//利用触发时间差计算频率
			{
				Freq=TrigAddrs[1]-TrigAddrs[0];//两次触发之间就是一个周期
				Freq=Osc_SamplingRate/Freq;
			}
			else Freq=-1;
			
			//上位机的数据返回
			for(j=0;j<Window;j++)//数据内容
				fputc(ADCbuff[TrigAddr+j-HalfWindow]>>4,NULL);
			for(j=0;j<8;j++)//同步包尾
				fputc(SyncSequence[j],(FILE *)0);
			
			if(TrigAddrs[0])TrigAddr=TrigAddrs[0];//如果TrigAddrs记录了至少1个数据（确实触发了），取首次触发地址作为显示中心
			//OLED显示
			OLED_ClearG();
			if(Disp_Osc_FFT==0)//示波器波形模式
			{
				for(i=0;i<64;i+=10)//绘制X坐标轴刻度
				{
					OLED_DrawLine(64+i,31,64+i,33);
					OLED_DrawLine(64-i,31,64-i,33);
				}
				if(HalfWindow>64)//当采样窗长大于OLED横向点数(半窗>半个OLED显示点数)
					for(i=0;i<127;i++)
						OLED_DrawLineRev(i,32+(ADCbuff[TrigAddr-64+i]-ZeroLine)/(1<<(6-Zoom)),i+1,32+(ADCbuff[TrigAddr-64+i+1]-ZeroLine)/(1<<(6-Zoom)));
				else //如果数据不足以站占满整个屏幕
				{
					for(i=0;((i+TrigAddr+1)<buffLenth)&&(i<63);i++)//绘制正半轴
						OLED_DrawLineRev(64+i,32+(ADCbuff[TrigAddr+i]-ZeroLine)/(1<<(6-Zoom)),64+i+1,32+(ADCbuff[TrigAddr+i+1]-ZeroLine)/(1<<(6-Zoom)));
					for(i=0;((i+TrigAddr-1)>0)&&(i>(-63));i--)//绘制负半轴
						OLED_DrawLineRev(64+i,32+(ADCbuff[TrigAddr+i]-ZeroLine)/(1<<(6-Zoom)),64+i-1,32+(ADCbuff[TrigAddr+i-1]-ZeroLine)/(1<<(6-Zoom)));
				}
				if(TrigAddr!=HalfWindow)//确实触发了，绘制Y轴坐标轴
				{
					OLED_DrawLine(64,0,64,64);
					for(i=0;i<32;i+=10)
					{
						OLED_DrawLine(63,32+i,65,32+i);
						OLED_DrawLine(63,32-i,65,32-i);
					}
				}
				OLED_DrawLine(0,32,128,32);//绘制X坐标轴
			}
			else//FFT显示模式
			{
				if(buffLenth>=256)//保证采样点数比FFT点数多（理论上，可以通过末尾补零来实现FFT）
				{
					//先取到合适的样
					if(TrigAddr>=128)//一般来说，从出触发点附近取值
						for(i=0;i<256;i++)
							FFT_in[i]=ADCbuff[TrigAddr-128+i];
					else //如果从触发点向两侧取值不够128，就从0开始取值
						for(i=0;i<256;i++)
							FFT_in[i]=ADCbuff[i];
cr4_fft_256_stm32(FFT_out,FFT_in,256);//FFT函数（调用库函数）
					for(i=0;i<256;i++)
					{//计算模长，获得FFT的
						FFT_Re=FFT_out[i]&0xFFFF;//实部
						FFT_Im=FFT_out[i]>>16;//虚部
						FFT_mag[i]=sqrt(FFT_Re*FFT_Re+FFT_Im*FFT_Im)*2;//模长，考虑对称性，乘以2得到实际强度
					}
					FFT_mag[0]=FFT_mag[0]/2;//直流分量不需要*2，所以在这里除回来
					for(i=0;i<128;i++)
						OLED_DrawLine(i,0,i,(FFT_mag[i]>>(6-Zoom)));//绘制频谱（stem形式）
					
					for(i=0;i<128;i+=16)//绘制X坐标轴刻度
						OLED_DrawLine(i,0,i,2);
					for(i=0;i<64;i+=16)//绘制Y坐标轴刻度
						OLED_DrawLine(0,i,2,i);
					OLED_ShowStringG(104,0,(u8*)"FFT",0);
				}
				
				else
				{
					OLED_ShowStringG(0,2,(u8*)"FFT Mode OFF",0);
					OLED_ShowStringG(0,4,(u8*)"Dot*3 < 256",0);
				}
			}
			if((WaveGen>=3)&&(WaveGen<=4))//绘图模式
			{
				if(1 || Disp_Osc_FFT==0)//示波器波形模式
				{
					//Wavlen=3;
				if(HalfWindow>64)//当采样窗长大于OLED横向点数(半窗>半个OLED显示点数)
						for(i=0;i<127;i++)
							WavData[i]=XYc_OutputInv(((i-64)<<ScanZoom)+2048,2048-(ADCbuff[TrigAddr-64+i]-ZeroLine)/(1<<(6-ScanZoom-Zoom)));
					else //如果数据不足以站占满整个屏幕
					{
						i=0;
						for(j=-TrigAddr;j<64;j++)
						{
							if(j<-63)continue;
							if(j>buffLenth-TrigAddr)break;
							WavData[i++]=XYc_OutputInv(((j)<<ScanZoom)+2048,2048+(ADCbuff[TrigAddr+j]-ZeroLine)/(1<<(6+ScanZoom-Zoom)));
						}
					}
					for(j=0;j<8;j++)WavData[i++]=XYc_OutputInv(2047+((64-8*j)<<ScanZoom),2048);//右边界
					WavData[i++]=XYc_OutputInv(2048,2048);//中点
					for(j=0;j<4;j++)WavData[i++]=XYc_OutputInv(2048,2048-((8*j)<<ScanZoom));//上边界
					for(j=0;j<=8;j++)WavData[i++]=XYc_OutputInv(2048,2048-((32-8*j)<<ScanZoom));//下边界
					for(j=0;j<4;j++)WavData[i++]=XYc_OutputInv(2048,2048-((8*j-32)<<ScanZoom));//归中
					WavData[i++]=XYc_OutputInv(2048,2048);//中点
					for(j=0;j<=8;j++)WavData[i++]=XYc_OutputInv(2049-((8*j)<<ScanZoom),2048);//左边界
					Wavlen=i;
				}
				else//FFT显示模式
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
		{//同步模式，一直开启菜单显示（显示信息和动画）
			CNT_ShowInfo=1;
		}
		
		OLED_Refresh://菜单显示控制
		if((CNT_ShowInfo<5000)&&(CNT_ShowInfo>0))//当(CNT_ShowInfo)在1-4999之间时，显示信息（5s自动关闭菜单）
		{
			if(Osc_Sync)//同步模式
			{
				i=Osc_SamplingRate;//关于采样率的显示，显示得好看点
				if(i>=1000000)sprintf((char*)DispBuff,"%3dM",(int)i/1000000);
				else if(i>=10000)sprintf((char*)DispBuff,"%3dK",(int)i/1000);
				else if(i>=1000)sprintf((char*)DispBuff,"%.1fK",i/1000.0);
				else sprintf((char*)DispBuff,"%4d",(int)i);
				OLED_ShowStringG(0,0,DispBuff,(WaveGen%2!=1?0xFF:0)*Osc_Sync);//显示到OLED上（左上角）
				if(Disp_Osc_FFT)OLED_ShowStringG(32,0,(u8*)"/2=Fmax",0);//显示到OLED上（左上角）
				i=Freq;//关于频率的显示，显示得好看点
				if(i>=10000)sprintf((char*)DispBuff,"F=%4dK",(int)i/1000);
				else if(i>=1000)sprintf((char*)DispBuff,"F=%.2fK",i/1000.0);
				else sprintf((char*)DispBuff,"F=%5d",(int)i);
				if(Freq>0)OLED_ShowStringG(72,6,DispBuff,0xFF);//在右下角显示频率
				else OLED_ShowStringG(72,6,(u8*)"F= *** ",0xFF);//在右下角显示频率测量失败
				i=1<<Zoom;//计算放大倍数=2^Zoom
				sprintf((char*)DispBuff,"Y:%2dx",(int)i);
				OLED_ShowStringG(0,6,DispBuff,(WaveGen%2!=1?0xFF:0));//显示到OLED左下角
			}
			else//异步模式
			{
				i=Osc_SamplingRate;//关于采样率的显示，显示得好看点
				if(i>=1000000)sprintf((char*)DispBuff,"Rate:%3dM",(int)i/1000000);
				else if(i>=10000)sprintf((char*)DispBuff,"Rate:%3dK",(int)i/1000);
				else if(i>=1000)sprintf((char*)DispBuff,"Rate:%.1fK",i/1000.0);
				else sprintf((char*)DispBuff,"Rate:%4d",(int)i);
				OLED_ShowStringG(0,0,DispBuff,(WaveGen%2!=1?0xFF:0)*Osc_Sync);//显示到OLED上第一行
				i=Osc_WindowLenth;//关于窗长的显示，显示得好看点
				if(i>=10000)sprintf((char*)DispBuff,"Window:%3dK",(int)i/1000);
				else if(i>=1000)sprintf((char*)DispBuff,"Window:%.1fK",i/1000.0);
				else sprintf((char*)DispBuff,"Window:%4d",(int)i);
				OLED_ShowStringG(0,2,DispBuff,0);//在第二行
				sprintf((char*)DispBuff,"Trig:%4d#%d",(int)Osc_TrigLevel,(int)Osc_SmitLevel);
				OLED_ShowStringG(0,4,DispBuff,0x02);//把同步模式的触发信息显示在屏幕第三行，但是划掉了
				OLED_ShowStringG(OLED_Flow-8,6,(u8*)" uploading",0);//显示一个动态的"uploading"
				OLED_Flow+=OLED_Dir;
				if(OLED_Flow==0)OLED_Dir=1;
				if(OLED_Flow>=56)OLED_Dir=-1;
			}
			if(WaveGen)
			{
				OLED_ShowStringG(80,0,(u8*)WaveGenName[WaveGen],WaveGen%2?0xFF:0);//在右上角显示功能模式
				if(WaveGen<=2)
				{
					if(WaveFreqX[WaveSel]<1000)sprintf((char*)DispBuff,"%s%4d",WaveTypeName[WaveType],(int)WaveFreqX[WaveSel]);
					else sprintf((char*)DispBuff,"%s%3dK",WaveTypeName[WaveType],(int)WaveFreqX[WaveSel]/1000);
					OLED_ShowStringG(88,2,(u8*)DispBuff,WaveGen==1?0xFF:0);//显示X频率
					if(WaveFreqY[WaveSel]<1000)sprintf((char*)DispBuff,"%s%4d",WaveTypeName[WaveType],(int)WaveFreqY[WaveSel]);
					else sprintf((char*)DispBuff,"%s%3dK",WaveTypeName[WaveType],(int)WaveFreqY[WaveSel]/1000);
					OLED_ShowStringG(88,4,(u8*)DispBuff,WaveGen==1?0xFF:0);//显示Y频率
				}
				else if(WaveGen<=4)
				{
					if(ScanRateMap[ScanRate]<1000)sprintf((char*)DispBuff,"R:%4d",(int)ScanRateMap[ScanRate]);
					else sprintf((char*)DispBuff,"R:%3dK",(int)ScanRateMap[ScanRate]/1000);
					OLED_ShowStringG(80,2,(u8*)DispBuff,WaveGen==3?0xFF:0);//显示扫描频率
					sprintf((char*)DispBuff,"Z:%3dx",(int)(1<<ScanZoom));
					OLED_ShowStringG(80,4,(u8*)DispBuff,WaveGen==3?0xFF:0);//显示缩放比例
				}
			}
			//else OLED_ShowString(80,0,"      ",0);
			//OLED_Refresh_Gram();
			Msg_Refresh++;
		}
		else CNT_ShowInfo=0;//不满足条件不显示（菜单关闭）
		if((Msg_500msFlash==(Osc_Sync+1)))//在同步模式下1s慢闪（还会与采样完成相关）非同步模式下0.5s快闪（稳定快闪）
		{
			Msg_500msFlash=0;
			LED0=!LED0;
		}
		if(Msg_OscSetting)//当存在“更新示波器设置消息”时触发下列事件：
		{
			Msg_OscSetting=0;//清空“更新示波器设置消息”标志
			TrigUp=Osc_TrigLevel+Osc_SmitLevel;//设置施密特触发上限
			if(TrigUp>=4096)TrigUp=4095;//防止数值超过范围
			TrigDown=Osc_TrigLevel-Osc_SmitLevel;//设置施密特触发下限
			if(TrigDown<0)TrigDown=0;//防止数值超过范围
			TrigLevel=Osc_TrigLevel;//设置触发电平
			Window=Osc_WindowLenth;//设置窗长
			HalfWindow=Window/2+Window%2;//计算半窗长（触发检测区域起点）
			//根据窗长、采样率还有内存大小共同决定实际采样数
			if(Osc_SamplingRate<=30000)buffLenth=(Window>(ADCbuffLenth/3))?ADCbuffLenth:3*Window;
			else buffLenth=ADCbuffLenth;
			CurousEnd=buffLenth-HalfWindow;//计算触发检测区域的终点
			OscInit(Osc_SamplingRate,buffLenth);//重新初始化示波器参数
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
			//OLED_Clear();//清屏重绘
			if(Osc_Sync)
			{
				if(Disp_Osc_FFT==0)
				{
					for(i=0;i<64;i+=10)//绘制X坐标轴刻度
						{OLED_DrawLine(64+i,31,64+i,33);OLED_DrawLine(64-i,31,64-i,33);}
					OLED_DrawLine(0,32,128,32);//绘制X坐标轴
				}
				else
				{
					for(i=0;i<128;i+=16)//绘制X坐标轴刻度
						OLED_DrawLine(i,0,i,2);
					for(i=0;i<64;i+=16)//绘制Y坐标轴刻度
						OLED_DrawLine(0,i,2,i);
				}
			}
			//OLED_Refresh_Gram();
			Msg_Refresh++;
			CNT_Clock=ClockTimeOut;
		}
		if((!CNT_Clock)&&(BKP->DR3==3))//开启时钟模式才进入时钟
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
