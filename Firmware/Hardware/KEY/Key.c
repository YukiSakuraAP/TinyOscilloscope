#include "Key.h"
#include "sys.h"

void KEY_Init(void)
{
	GPIO_InitTypeDef Init;
	
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB,ENABLE);
	
	Init.GPIO_Mode=GPIO_Mode_IPU;
	Init.GPIO_Speed=GPIO_Speed_50MHz;
	Init.GPIO_Pin=GPIO_Pin_4|GPIO_Pin_5|GPIO_Pin_6|GPIO_Pin_7;
	GPIO_Init(GPIOB,&Init);
	
	JTAG_Set(SWD_ENABLE);
	
}

struct key Keys;

u32 KeyMap(void)
{
	u32 map=0;
	GPIOB->BSRR|=0x000000F0;
	if(GPIO_ReadInputDataBit(GPIOB,GPIO_Pin_4)==0)map|=Key0;
	if(GPIO_ReadInputDataBit(GPIOB,GPIO_Pin_5)==0)map|=Key1;
	if(GPIO_ReadInputDataBit(GPIOB,GPIO_Pin_6)==0)map|=Key2;
	if(GPIO_ReadInputDataBit(GPIOB,GPIO_Pin_7)==0)map|=Key3;
	return map;
}

void KeyMagic(void)
{
	u8 i=0;
	i=KeyMap();
	if(Keys.Key==i)Keys.Count++;
	else {Keys.Key=i;Keys.Count=0;}
	if(Keys.Count>=50000)Keys.Count=50000;
	if(i==0)Keys.lastKey=0;
}

void resetKey(void)
{
	Keys.Count=0;
	Keys.Key=0;
	Keys.lastKey=0;
}

u8 getKey(u8 key,u16 delay)
{
	static u32 i;
	if(Keys.Key==key)
	{
		if(Keys.lastKey!=key){i=KeyDelay;Keys.lastKey=key;}
		if(Keys.Count>=i){i+=delay;return 1;}
	}
	return 0;
}
