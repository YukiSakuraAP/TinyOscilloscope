//  GND    ��Դ��
//  VCC  ��5V��3.3v��Դ
//  D0   P1^0��SCL��
//  D1   P1^1��SDA��
//  RES
//  DC
//  CS              
//	Ϊ��͵��ֱ�Ӹ�д

#include "oled.h"
//#include "stdlib.h"
#include "oledfont.h"  	 
//#include "delay.h"
#include "delay.h"
//OLED���Դ�
//��Ÿ�ʽ����.
//[0]0 1 2 3 ... 127	
//[1]0 1 2 3 ... 127	
//[2]0 1 2 3 ... 127	
//[3]0 1 2 3 ... 127	
//[4]0 1 2 3 ... 127	
//[5]0 1 2 3 ... 127	
//[6]0 1 2 3 ... 127	
//[7]0 1 2 3 ... 127
u8 OLED_GRAM[128][8];	
//�����Դ浽LCD		  

#define high 1
#define low 0 

/**********************************************
//IIC Start
**********************************************/
void IIC_Start()  
{
	IIC_Output;
	DelayOLED
	SDA = high;													//Fixed
	DelayOLED
	SCL = high; 
	DelayOLED
	SDA = low;
	DelayOLED
	SCL = low;
	DelayOLED
}

/**********************************************
//IIC Stop
**********************************************/
void IIC_Stop()
{
	IIC_Output;
	DelayOLED
	SCL = low;
	DelayOLED
	SDA = low;
	DelayOLED
	SCL = high;
	DelayOLED
	SDA = high;
	DelayOLED
}
/**********************************************
// IIC Write byte
**********************************************/
u8 Write_IIC_Byte(unsigned char IIC_Byte)
{
	unsigned char i;
	u8 Ack_Bit;                    //Ӧ���ź�
	IIC_Output;
	DelayOLED
	SCL = low;													//Fixed
	for(i=0;i<8;i++)		
	{
		SDA=(IIC_Byte&0x80)?1:0;
	DelayOLED
		SCL=high;
	DelayOLED
		SCL=low;
		IIC_Byte<<=1;			//loop
	DelayOLED
	}
	IIC_Input;
	SDA = high;		                //�ͷ�IIC SDA����Ϊ���������մ���������Ӧ���ź�	
	DelayOLED
	SCL = high;                     //��9��ʱ������
	DelayOLED
	Ack_Bit = SDAin;		            //��ȡӦ���ź�
	DelayOLED
	SCL = low;
	IIC_Output;													//Fixed
	DelayOLED
	return Ack_Bit;	
}  


//unsigned char Read_IIC_Byte(void)
//{
//	unsigned char da;
//	unsigned char i;
//	IIC_Input;
//	for(i=0;i<8;i++)
//	{   
//		SCL = 1;
//		DelayOLED;
//		da <<= 1;
//		if(SDAin) 
//			da |= 0x01;
//		SCL = 0;
//		DelayOLED;
//	}
//	IIC_Output;
//	SDA = 1;
//	DelayOLED;
//	SCL = 1;
//	DelayOLED;
//	SCL = 0;
//	SDA = 1; 
//	DelayOLED;
//	return da;
//}
/**********************************************
// IIC Write Command
**********************************************/
void Write_IIC_Command(unsigned char IIC_Command)
{
	INTX_DISABLE();
	DelayOLED
   IIC_Start();
	DelayOLED
   Write_IIC_Byte(0x78);            //Slave address,SA0=0
	DelayOLED
   Write_IIC_Byte(0x00);			//write command
	DelayOLED
   Write_IIC_Byte(IIC_Command); 
	DelayOLED
   IIC_Stop();
	DelayOLED
	INTX_ENABLE();
}
/**********************************************
// IIC Write Data
**********************************************/
void Write_IIC_Data(unsigned char IIC_Data)
{
	INTX_DISABLE();
	DelayOLED
   IIC_Start();
	DelayOLED
   Write_IIC_Byte(0x78);			//D/C#=0; R/W#=0
	DelayOLED
   Write_IIC_Byte(0x40);			//write data
	DelayOLED
   Write_IIC_Byte(IIC_Data);
	DelayOLED
   IIC_Stop();
	DelayOLED
	INTX_ENABLE();
}

void OLED_Set_Pos(unsigned char x, unsigned char y) 
{ 
	Write_IIC_Command(0xb0+y);
	DelayOLED
	Write_IIC_Command(((x&0xf0)>>4)|0x10);
	DelayOLED
	Write_IIC_Command((x&0x0f)); 
	DelayOLED
}   	  
//����OLED��ʾ    
void OLED_Display_On(void)
{
	Write_IIC_Command(0X8D);  //SET DCDC����
	Write_IIC_Command(0X14);  //DCDC ON
	Write_IIC_Command(0XAF);  //DISPLAY ON
}
//�ر�OLED��ʾ     
void OLED_Display_Off(void)
{
	Write_IIC_Command(0X8D);  //SET DCDC����
	Write_IIC_Command(0X10);  //DCDC OFF
	Write_IIC_Command(0XAE);  //DISPLAY OFF
}		   			 
//��������,������,������Ļ�Ǻ�ɫ��!��û����һ��!!!	  
void OLED_Clear(void)  
{  
	u8 i,n;		    
	for(i=0;i<8;i++)  
	{  
		Write_IIC_Command(0xb0+i);		//page0-page1
		Write_IIC_Command(0x00);		//low column start address
		Write_IIC_Command(0x10);
		for(n=0;n<128;n++)
			OLED_GRAM[n][i]=0;
	} //������ʾ
	//OLED_Refresh_Gram();
}
void OLED_ClearG(void)  
{  
	u8 i,n;		    
	for(i=0;i<8;i++)  
	{
		for(n=0;n<128;n++)
			OLED_GRAM[n][i]=0;
	} //������ʾ
	//OLED_Refresh_Gram();
}


//��ָ��λ����ʾһ���ַ�,���������ַ�
//x:0~127
//y:0~63
//mode:0,������ʾ;1,������ʾ				 
//size:ѡ������ 16/12 
void OLED_ShowChar(u8 x,u8 y,u8 chr,u8 anti)
{      	
	unsigned char c=0,i=0,j=0;	
	c=chr-' ';//�õ�ƫ�ƺ��ֵ		
	if(x>Max_Column-1){x=0;y=y+2;}
//	OLED_Set_Pos(x,y);	
//	for(i=0;i<8;i++)
//		Write_IIC_Data(F8X16[c*16+i]);
//	OLED_Set_Pos(x,y+1);
//	for(i=0;i<8;i++)
//		Write_IIC_Data(F8X16[c*16+i+8]);
//	for(j=0,OLED_Set_Pos(x,y);j<2;j++,OLED_Set_Pos(x,y+1))
//		for(i=0;i<8;i++)
//			Write_IIC_Data(F8X16[c*16+i+8*j]);
	
	for(j=0,OLED_Set_Pos(x,y);j<2;j++,OLED_Set_Pos(x,y+1))
		for(i=0;i<8;i++)
			Write_IIC_Data(F8X16[c*16+i+8*j]^anti);


//	for(j=0;j<2;j++)
//		for(i=0;i<8;i++)
//			OLED_GRAM[x+i][y+j]=(F8X16[c*16+i+8*j]^anti);
}

void OLED_ShowCharG(u8 x,u8 y,u8 chr,u8 anti)//��ʾ��ͼ����
{      	
	unsigned char c=0,i=0,j=0;	
	c=chr-' ';//�õ�ƫ�ƺ��ֵ		
	if(x>Max_Column-1){x=0;y=y+2;}
//	OLED_Set_Pos(x,y);	
//	for(i=0;i<8;i++)
//		Write_IIC_Data(F8X16[c*16+i]);
//	OLED_Set_Pos(x,y+1);
//	for(i=0;i<8;i++)
//		Write_IIC_Data(F8X16[c*16+i+8]);
//	for(j=0,OLED_Set_Pos(x,y);j<2;j++,OLED_Set_Pos(x,y+1))
//		for(i=0;i<8;i++)
//			Write_IIC_Data(F8X16[c*16+i+8*j]);
	
//	for(j=0,OLED_Set_Pos(x,y);j<2;j++,OLED_Set_Pos(x,y+1))
//		for(i=0;i<8;i++)
//			Write_IIC_Data(F8X16[c*16+i+8*j]^anti);


	for(j=0;j<2;j++)
		for(i=0;i<8;i++)
			OLED_GRAM[x+i][y+j]=(F8X16[c*16+i+8*j]^anti);
}


void OLED_ShowNum32(u8 x,u8 y,u8 num)
{      	
	unsigned char i,j;	
	if(x>Max_Column-1){x=0;y=y+4;}
	for(j=0;j<4;j++)
	{
		OLED_Set_Pos(x,y+j);	
		for(i=0;i<16;i++)
			Write_IIC_Data(F16X32[num][i+j*16]);
	}
}

void OLED_ShowNum32Rev(u8 x,u8 y,u8 num, u8 Rev)
{      	
	unsigned char i,j;	
	if(x>Max_Column-1){x=0;y=y+4;}
	for(j=0;j<4;j++)
	{
		OLED_Set_Pos(x,y+j);	
		for(i=0;i<16;i++)
			Write_IIC_Data(F16X32[num][i+j*16]^Rev);
	}
}

//m^n����
//u32 oled_pow(u8 m,u8 n)
//{
//	u32 result=1;	 
//	while(n--)result*=m;    
//	return result;
//}				  
//��ʾ2������
//x,y :�������	 
//len :���ֵ�λ��
//size:�����С
//mode:ģʽ	0,���ģʽ;1,����ģʽ
//num:��ֵ(0~4294967295);	 		  
//void OLED_ShowNum(u8 x,u8 y,u32 num,u8 len,u8 size2)
//{         	
//	u8 t,temp;
//	u8 enshow=0;						   
//	for(t=0;t<len;t++)
//	{
//		temp=(num/oled_pow(10,len-t-1))%10;
//		if(enshow==0&&t<(len-1))
//		{
//			if(temp==0)
//			{
//				OLED_ShowChar(x+(size2/2)*t,y,' ');
//				continue;
//			}else enshow=1; 
//		 	 
//		}
//	 	OLED_ShowChar(x+(size2/2)*t,y,temp+'0'); 
//	}
//} 
//��ʾһ���ַ��Ŵ�
void OLED_ShowString(u8 x,u8 y,u8 *chr,u8 anti)
{
	unsigned char j=0;
	while (chr[j]!='\0')
	{
		OLED_ShowChar(x,y,chr[j],anti);
		x+=8;
		if(x>120){x=0;y+=2;}j++;
	}
}

void OLED_ShowStringG(u8 x,u8 y,u8 *chr,u8 anti)//��ʾ��ͼ����
{
	unsigned char j=0;
	while (chr[j]!='\0')
	{
		OLED_ShowCharG(x,y,chr[j],anti);
		x+=8;
		if(x>120){x=0;y+=2;}j++;
	}
}

//��ʼ��SSD1306					    
void OLED_Init(void)
{
	const u8 Sequence[]={0xAE,0x20,0x10,0xb0,0xc8,0x00,0x10,0x40,0x81,0x7f,0xa1,0xa6,0xa8,0x3F,0xa4,0xd3,
	0x00,0xd5,0xf0,0xd9,0x22,0xda,0x12,0xdb,0x20,0x8d,0x14,0xaf,0xff};//End as 0Xff
	u8 i;
	RCC->APB2ENR|=RCC_APB2Periph_GPIOC;   //ʹ��PORTC��ʱ��  
	GPIOC->CRL&=0xFF00FFFF;
	GPIOC->CRL|=0x00330000;
	IIC_Start();
	delay_ms(10);
	IIC_Stop();
	delay_ms(80);
	for(i=0;Sequence[i]!=0xff;i++)Write_IIC_Command(Sequence[i]);
	
}


void OLED_Refresh_Gram(void)
{
	u8 i,n;		    
	for(i=0;i<8;i++)  
	{  
		Write_IIC_Command (0xb0+i);    //����ҳ��ַ��0~7��
		Write_IIC_Command (0x00);      //������ʾλ�á��е͵�ַ
		Write_IIC_Command (0x10);      //������ʾλ�á��иߵ�ַ   
		for(n=0;n<128;n++)Write_IIC_Data(OLED_GRAM[n][i]); 
	}   
}
//���� 
//x:0~127
//y:0~63
//t:1 ��� 0,���				   
void OLED_DrawPoint(u8 x,u8 y,u8 t)
{
	u8 pos,bx,temp=0;
	if(x>127||y>63)return;//������Χ��.
	pos=7-y/8;
	bx=y%8;
	temp=1<<(7-bx);
	if(t==1)OLED_GRAM[x][pos]|=temp;//������
	else if(t==2)OLED_GRAM[x][pos]^=temp;//��ת��
	else OLED_GRAM[x][pos]&=~temp;//��յ�
}
//����
//x1,y1:�������
//x2,y2:�յ�����  
void OLED_DrawLine(u16 x1, u16 y1, u16 x2, u16 y2)
{
	u16 t; 
	int xerr=0,yerr=0,delta_x,delta_y,distance; 
	int incx,incy,uRow,uCol; 
	delta_x=x2-x1; //������������ 
	delta_y=y2-y1; 
	uRow=x1; 
	uCol=y1; 
	if(delta_x>0)incx=1; //���õ������� 
	else if(delta_x==0)incx=0;//��ֱ�� 
	else {incx=-1;delta_x=-delta_x;} 
	if(delta_y>0)incy=1; 
	else if(delta_y==0)incy=0;//ˮƽ�� 
	else{incy=-1;delta_y=-delta_y;} 
	if( delta_x>delta_y)distance=delta_x; //ѡȡ�������������� 
	else distance=delta_y; 
	for(t=0;t<=distance+1;t++ )//������� 
	{  
		OLED_DrawPoint(uRow,uCol,1);//���� 
		xerr+=delta_x ; 
		yerr+=delta_y ; 
		if(xerr>distance) 
		{ 
			xerr-=distance; 
			uRow+=incx; 
		} 
		if(yerr>distance) 
		{ 
			yerr-=distance; 
			uCol+=incy; 
		} 
	}  
}    
//����
//x1,y1:�������
//x2,y2:�յ�����  
void OLED_DrawLineRev(u16 x1, u16 y1, u16 x2, u16 y2)
{
	u16 t; 
	int xerr=0,yerr=0,delta_x,delta_y,distance; 
	int incx,incy,uRow,uCol; 
	delta_x=x2-x1; //������������ 
	delta_y=y2-y1; 
	uRow=x1; 
	uCol=y1; 
	if(delta_x>0)incx=1; //���õ������� 
	else if(delta_x==0)incx=0;//��ֱ�� 
	else {incx=-1;delta_x=-delta_x;} 
	if(delta_y>0)incy=1; 
	else if(delta_y==0)incy=0;//ˮƽ�� 
	else{incy=-1;delta_y=-delta_y;} 
	if( delta_x>delta_y)distance=delta_x; //ѡȡ�������������� 
	else distance=delta_y; 
	for(t=0;t<=distance+1;t++ )//������� 
	{  
		OLED_DrawPoint(uRow,uCol,2);//���� 
		xerr+=delta_x ; 
		yerr+=delta_y ; 
		if(xerr>distance) 
		{ 
			xerr-=distance; 
			uRow+=incx; 
		} 
		if(yerr>distance) 
		{ 
			yerr-=distance; 
			uCol+=incy; 
		} 
	}  
}    
