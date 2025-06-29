#include "Clock.h"
#include "oled.h"
#include "usart.h"
#include "Shell2.h"
#include "LED.h"
#include "Key.h"
#include "ds18b20.h"

void Clock_Init(void)
{
	OLED_Clear();
	OLED_Refresh_Gram();
}

void ClockSetTime(void)
{
	s8 DateVal[6],Max[6]={99,12,31,23,59,59},Min[6]={0,1,1,0,0,0};
	u8 i=0,j,Msg_Refresh=1;
	DateVal[0]=Timer.Year;
	DateVal[1]=Timer.Month;
	DateVal[2]=Timer.Day;
	DateVal[3]=Timer.Hour;
	DateVal[4]=Timer.Min;
	DateVal[5]=Timer.Sec;
	OLED_Display_On();
	OLED_Clear();
	OLED_Refresh_Gram();
	while(i<6)
	{
		if(Msg_Refresh)
		{
			Msg_Refresh=0;
			OLED_ShowString(0,0,"Set Time...",0);
			OLED_ShowNum32Rev(0,2,DateVal[3]/10,0xFF*(i==3));
			OLED_ShowNum32Rev(16,2,DateVal[3]%10,0xFF*(i==3));
			OLED_ShowNum32(32,2,10);
			OLED_ShowNum32Rev(48,2,DateVal[4]/10,0xFF*(i==4));
			OLED_ShowNum32Rev(64,2,DateVal[4]%10,0xFF*(i==4));
			OLED_ShowChar(80,4,':',0);
			OLED_ShowChar(88,4,'0'+DateVal[5]/10,0xFF*(i==5));
			OLED_ShowChar(96,4,'0'+DateVal[5]%10,0xFF*(i==5));
			OLED_ShowString(0,6,(u8*)"20",0xFF*(i==0));
			OLED_ShowChar(16,6,'0'+DateVal[0]/10,0xFF*(i==0));
			OLED_ShowChar(24,6,'0'+DateVal[0]%10,0xFF*(i==0));
			OLED_ShowChar(32,6,'.',0);
			OLED_ShowChar(40,6,'0'+DateVal[1]/10,0xFF*(i==1));
			OLED_ShowChar(48,6,'0'+DateVal[1]%10,0xFF*(i==1));
			OLED_ShowChar(56,6,'.',0);
			OLED_ShowChar(64,6,'0'+DateVal[2]/10,0xFF*(i==2));
			OLED_ShowChar(72,6,'0'+DateVal[2]%10,0xFF*(i==2));
		}
		if(getKey(Key0,500)){i++;Msg_Refresh++;}
		if(getKey(Key1,300)){DateVal[i]--;Msg_Refresh++;}
		if(getKey(Key2,300)){DateVal[i]++;Msg_Refresh++;}
		if(getKey(Key3,500))
		{
			OLED_Clear();
			OLED_Refresh_Gram();
			return;
		}
		for(j=0;j<6;j++)
		{
			if(DateVal[j]>Max[j])DateVal[j]=Min[j];
			if(DateVal[j]<Min[j])DateVal[j]=Max[j];
		}
	}
	RTC_Set(2000+DateVal[0],DateVal[1],DateVal[2],DateVal[3],DateVal[4],DateVal[5]);
	RTC_Get();
	Timer.Sec=calendar.sec;
	Timer.Min=calendar.min;
	Timer.Hour=calendar.hour;
	Timer.Day=calendar.w_date;
	Timer.Month=calendar.w_month;
	Timer.Year=calendar.w_year-2000;
	Timer.Week=calendar.week;
	OLED_Clear();
	OLED_Refresh_Gram();
}

void Clock(void)
{
	const char WeekMap[8][4]={"Sun","Mon","Tue","Wed","Thu","Fri","Sat","Sun"};
	u8 Disp[32];
	u8 sec=60,i,DispOn=1,LEDon=0;
	s32 Temp=0;
	LED0=1;LED1=1;
	Clock_Init();
	DS18B20_ON();
	while(1)
	{
		if(USART_RX_STA&0x8000)Shell2(USART_RX_BUF,USART_RX_STA&0x3fff,1);//解析上位机串口发送的命令
		//RTC_Get();
		if(sec!=Timer.Sec)
		{
			sec=Timer.Sec;
			OLED_ShowNum32(0,2,Timer.Hour/10);
			OLED_ShowNum32(16,2,Timer.Hour%10);
			OLED_ShowNum32(32,2,10);
			OLED_ShowNum32(48,2,Timer.Min/10);
			OLED_ShowNum32(64,2,Timer.Min%10);
			OLED_ShowChar(80,4,':',0);
			OLED_ShowChar(88,4,'0'+Timer.Sec/10,0);
			OLED_ShowChar(96,4,'0'+Timer.Sec%10,0);
			OLED_ShowString(0,6,(u8*)"20",0);
			OLED_ShowChar(16,6,'0'+Timer.Year/10,0);
			OLED_ShowChar(24,6,'0'+Timer.Year%10,0);
			OLED_ShowChar(32,6,'.',0);
			OLED_ShowChar(40,6,'0'+Timer.Month/10,0);
			OLED_ShowChar(48,6,'0'+Timer.Month%10,0);
			OLED_ShowChar(56,6,'.',0);
			OLED_ShowChar(64,6,'0'+Timer.Day/10,0);
			OLED_ShowChar(72,6,'0'+Timer.Day%10,0);
			OLED_ShowString(88,6,(u8*)WeekMap[Timer.Week],0);
			//考研时计
//			i=25-Timer.Day+(Timer.Month==11)*30;
//			OLED_ShowChar(112,0,'0'+i/10,0);
//			OLED_ShowChar(120,0,'0'+i%10,0);
//			sprintf(Disp,"%d Days",(int)i);
//			OLED_ShowString(0,0,Disp,0);
			
			Temp=DS18B20_Get_Temp();
			sprintf(Disp,"%2d.%1dC",Temp/10,Temp%10);
			OLED_ShowString(88,0,Disp,0);
		}
		if(getKey(Key0,500)){ClockSetTime();sec=60;}
		if(getKey(Key1,500))
		{
			LEDon++;LEDon%=4;
			LED0=(!(LEDon&1));
			LED1=(!(LEDon&2));
		}
		if(getKey(Key2,500))DispOn=!DispOn;
		if(DispOn)OLED_Display_On();
		else OLED_Display_Off();
		if(getKey(Key3,500))break;
	}
	DS18B20_OFF();
	OLED_Display_On();
}
