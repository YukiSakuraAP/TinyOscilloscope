#include "Shell2.h"
#include "ArrayOperation.h"
#include "stdlib.h"
#include "Osc.h"
#include "Clock.h"

typedef struct
{
	const char *Name;
	void (*CommandHook)(void);
}CMDblock;

u8 CmdParameter[256];

void FeedBack(char *datas);

extern u8 Simulating;
void CMD_Simulate(void)
{
	Simulating=CmdParameter[0]-'0';
	sprintf((char*)CmdParameter,"Simulate=%d;\r\n",Simulating);
	FeedBack((char*)CmdParameter);
}

void CMD_SetTime(void)
{
	u8 i,j[6]={0},k=1;
	for(i=0;i<2;i++)
		if(CmdParameter[3*i+2]!='.')k=0;
	for(i=0;i<2;i++)
		if(CmdParameter[9+3*i+2]!=':')k=0;
	if(CmdParameter[8]!=' ')k=0;
	if(CmdParameter[17]!=';')k=0;
	for(i=0;i<6;i++)
		j[i]=atoi((char*)&CmdParameter[3*i]);
	if(k)RTC_Set(2000+j[0],j[1],j[2],j[3],j[4],j[5]);
	RTC_Get();
	if(!k)FeedBack("Format ERROR !\r\n");
	sprintf((char*)CmdParameter,"Time=%02d.%02d.%02d %02d:%02d:%02d;\r\n",(int)Timer.Year,(int)Timer.Month,(int)Timer.Day,(int)Timer.Hour,(int)Timer.Min,(int)Timer.Sec);
	FeedBack((char*)CmdParameter);
}

char FeedBackBuff[128];

//示波器的串口界面接口，需要在下方注册
void CMD_TrigLevel(void)//更新示波器内核中的触发电平记录(Osc_TrigLevel)
{
	Osc_TrigLevel=AF2Float(CmdParameter)*40.96;
	sprintf((char*)CmdParameter,"Osc_TrigLevel=%d;\r\n",Osc_TrigLevel);
	FeedBack((char*)CmdParameter);
	Msg_OscSetting++;
}
void CMD_SmitLevel(void)//更新示波器内核中的施密特触发记录(Osc_SmitLevel)
{
	Osc_SmitLevel=AF2Float(CmdParameter)*20.48;
	sprintf((char*)CmdParameter,"Osc_SmitLevel=%d;\r\n",Osc_SmitLevel);
	FeedBack((char*)CmdParameter);
	Msg_OscSetting++;
}
void CMD_SimpleRate(void)//更新示波器内核中的采样率记录(Osc_SamplingRate)
{
	Osc_SamplingRate=AF2Float(CmdParameter);
	sprintf((char*)CmdParameter,"Osc_SimpleRate=%d;\r\n",Osc_SamplingRate);
	FeedBack((char*)CmdParameter);
	Msg_OscSetting++;
}
void CMD_WindowLenth(void)//更新示波器内核中的窗长度记录(Osc_WindowLenth)
{
	Osc_WindowLenth=AF2Float(CmdParameter);
	sprintf((char*)CmdParameter,"Osc_WindowLenth=%d;\r\n",Osc_WindowLenth);
	FeedBack((char*)CmdParameter);
	Msg_OscSetting++;
}

void CMD_OscSync(void)//更新示波器内核中的同步模式(Osc_Sync)
{
	Osc_Sync=AF2Float(CmdParameter);
	sprintf((char*)CmdParameter,"Osc_Sync=%d;\r\n",Osc_Sync);
	FeedBack((char*)CmdParameter);
	Msg_OscSetting++;
}

//命令函数注册，注册后可以使用形如"system:CMD_Simulate.*****;\r\n"的形式访问函数
CMDblock systemCMD[]={
	{"CMD_Simulate",CMD_Simulate},
	{"SetTime",CMD_SetTime},
	{"system\0EndOfBlock",(void(*)(void))0xffffffff}
};
CMDblock functionCMD[]={
	{"TrigLevel",CMD_TrigLevel},
	{"SmitLevel",CMD_SmitLevel},
	{"SimpleRate",CMD_SimpleRate},
	{"WindowLenth",CMD_WindowLenth},
	{"OscSync",CMD_OscSync},
	{"function\0EndOfBlock",(void(*)(void))0xffffffff}
};
CMDblock registerCMD[]={
	{"register\0EndOfBlock",(void(*)(void))0xffffffff}
};


CMDblock HeadCommand[]={
	{"system",(void(*)(void))systemCMD},
	{"function",(void(*)(void))functionCMD},
	{"register",(void(*)(void))registerCMD},
	{"EndOfBlock",(void(*)(void))0xffffffff}
};
void FeedBack(char *datas)
{
	printf("%s",datas);
}

extern u16 USART_RX_STA;//接收状态标记
void Clean_Buff(u8* String, u16 len)
{
	u32 i;
	for(i=0;i<len;i++)String[i]=0;
	USART_RX_STA=0;
}

u8 Shell2(u8* String, u16 len, u8 clean)
{
	u32 i = 0, j = 0;
	u32 Poi = 0, Pos = 0, CMD = 0, Error=0, unitCount = 0;
	CMDblock* cmdBlock;
	u32 SymbolPosition[3];
	const u8 SymbolMatch[] = { ":.;" };
	if(len)
	{
		while (1)
		{
			Poi = i = Pos;
			for (j = 0; (i < len) && (j < 3); i++)
				if (String[i] == SymbolMatch[j])
				{
					SymbolPosition[j] = i; j++;
				}
			if (j != 3){Error= unitCount;break;}
			unitCount++; Pos = i;
			for (i = 0; (u32)HeadCommand[i].CommandHook != 0xffffffff; i++)
				if (StrLenMatch(&String[Poi], (u8*)HeadCommand[i].Name,SymbolPosition[0]-Poi))break;
			if ((u32)HeadCommand[i].CommandHook == 0xffffffff)
			{
				FeedBack("HeadCommand is Not_Matched\r\n"); 
				Error=0xf0;break;
			}
			CMD = i * 0x40;
			cmdBlock = (CMDblock*)(HeadCommand[i].CommandHook);
			for (i = 0; (u32)cmdBlock[i].CommandHook != 0xffffffff; i++)
				if (StrLenMatch(&String[SymbolPosition[0] + 1], (u8*)cmdBlock[i].Name,SymbolPosition[1]-SymbolPosition[0]-1))break;
			if ((u32)cmdBlock[i].CommandHook == 0xffffffff)
			{
				FeedBack((char*)cmdBlock[i].Name); FeedBack("_Body is Not_Matched\r\n"); 
				Error=0xf1;break;
			}
			if (cmdBlock[i].CommandHook == 0)
			{
				FeedBack((char*)"There is a definition of this command, but the API is undefined.\r\n"); 
				Error=0xf2;break;
			}
			CMD=i;
			if (i >= 0x40)
			{
				FeedBack("There is TOO MUCH Commands in One Group\r\n");
				Error=0xff;break;
			}
			for(i=0;i<256;i++)CmdParameter[i]=0;
			j=0;
			for(i=SymbolPosition[1]+1;i<=SymbolPosition[2];i++,j++)
				CmdParameter[j]=String[i];
			cmdBlock[CMD].CommandHook();
		}
		
		if(clean)Clean_Buff(String,len);
	}
	return Error;
}
