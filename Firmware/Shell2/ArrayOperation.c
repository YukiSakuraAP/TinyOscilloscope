#include "ArrayOperation.h"

void ArrayCopy(u8* Target, u8* Source, u32 lenth)
{
	u32 i;
	for (i = 0; i < lenth; i++)
		Target[i] = Source[i];
}

u32 ArrayMatch(u8* Target, u8* Source, u32 lenth)
{
	u32 i;
	for (i = 0; i < lenth; i++)
		if (Target[i] != Source[i])break;
	return (i == lenth);
}

s32 ArrayFind(u8* Source, u8* Core, u32 lenthS, u32 lenthC)
{
	u32 i, j;
	for (j = 0; j < lenthS - lenthC; j++)
	{
		for (i = 0; i < lenthC; i++)
			if (Core[i] != Source[i + j])break;
		if (i == lenthC)return j;
	}
	return -1;
}

void StringCopy(u8* Target, u8* Source)
{
	u32 i;
	for (i = 0; Source[i]; i++)
		Target[i] = Source[i];
}

u32 StringMatch(u8* Target, u8* Source)
{
	u32 i;
	for (i = 0; Source[i]; i++)
		if (Target[i] != Source[i])break;
	return !(Source[i]/* + Target[i]*/);
}

u32 StrLenMatch(u8* Target, u8* Source, u32 Len)
{
	u32 i;
	for (i = 0; Source[i]; i++)
		if (Target[i] != Source[i])break;
	return !(Source[i]||(i!=Len)/* + Target[i]*/);
}

u32 StringFind(u8* Source, u8* Core)
{
	u32 i, j;
	for (j = 0; Source[j]; j++)
	{
		for (i = 0; Core[i]; i++)
			if (Core[i] != Source[i + j])break;
		if (!Core[i])return j;
	}
	return 0xffffffff;
}

float AF2Float(u8 *ascii)
{
	double a=0,b=0;
	u8 i,j,k;
	for(i=0;(ascii[i]!=';')&&(ascii[i]!='.')&&(ascii[i]!=',');i++);
	if(ascii[i]=='.')
		for(j=i+1;(ascii[j]!=';')&&(ascii[j]!='.')&&(ascii[j]!=',');j++);
	for(a=k=0;k<i;k++)a*=10,a+=ascii[k]-'0';
	if(ascii[i]=='.')for(b=0,k=1;j-k>i;k++,b/=10.0)b+=ascii[j-k]-'0';
	return a+b;
}
