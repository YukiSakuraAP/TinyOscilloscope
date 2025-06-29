#ifndef __OLED_H
#define __OLED_H
#include "sys.h"
//#include "stdlib.h"
#define  u8 unsigned char 
#define  u16 unsigned int
#define  u32 unsigned long 
#define OLED_CMD  0	//写命令
#define OLED_DATA 1	//写数据
#define OLED_MODE 0


//OLED模式设置
//0:4线串行模式
//1:并行8080模式

//#define SIZE 16
//#define XLevelL		0x02
//#define XLevelH		0x10
#define Max_Column	128
#define Max_Row		64
//#define	Brightness	0xFF 
//#define X_WIDTH 	128
//#define Y_WIDTH 	64	    						  
//-----------------OLED端口定义----------------  					   

#define Anti 0xff

//OLED控制用函数
void OLED_Display_On(void);
void OLED_Display_Off(void);	   							   		    
void OLED_Init(void);
void OLED_Clear(void);
void OLED_ClearG(void);
void OLED_ShowChar(u8 x,u8 y,u8 chr,u8 anti);
void OLED_ShowCharG(u8 x,u8 y,u8 chr,u8 anti);
void OLED_ShowNum(u8 x,u8 y,u32 num,u8 len,u8 size2);
void OLED_ShowString(u8 x,u8 y, u8 *p,u8 anti);	 
void OLED_ShowStringG(u8 x,u8 y, u8 *p,u8 anti);	 
void OLED_Set_Pos(unsigned char x, unsigned char y);
void OLED_ShowNum32(u8 x,u8 y,u8 num);
void OLED_ShowNum32Rev(u8 x,u8 y,u8 num, u8 Rev);

void OLED_Refresh_Gram(void);
void OLED_DrawPoint(u8 x,u8 y,u8 t);
void OLED_DrawLine(u16 x1, u16 y1, u16 x2, u16 y2);
void OLED_DrawLineRev(u16 x1, u16 y1, u16 x2, u16 y2);

u8 Write_IIC_Byte(unsigned char IIC_Byte);
unsigned char Read_IIC_Byte(void);
void IIC_Start(void);
void IIC_Stop(void);

/*************Pin Define***************/
#define SCL PCout(5)
#define SDA PCout(4)
#define SDAin PCin(4)
//#define DelayOLED delay_us(100);
#define DelayOLED ;

#define IIC_Output {GPIOC->CRL&=0xFFF0FFFF;GPIOC->CRL|=0x00030000;}
#define IIC_Input {GPIOC->CRL&=0xFFF0FFFF;GPIOC->CRL|=0x00080000;SDA=1;}

#endif  
