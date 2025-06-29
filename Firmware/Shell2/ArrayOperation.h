#ifndef _ArrayOperation_
#define _ArrayOperation_

#include "sys.h"
#include "delay.h"

void ArrayCopy(u8* Target,u8* Source,u32 lenth);
u32 ArrayMatch(u8* Target, u8* Source, u32 lenth);
s32 ArrayFind(u8* Source,u8 *Core,u32 lenthS,u32 lenthC);
void StringCopy(u8* Target,u8* Source);
u32 StringMatch(u8* Target,u8* Source);
u32 StrLenMatch(u8* Target, u8* Source, u32 Len);
u32 StringFind(u8* Source,u8 *Core);
float AF2Float(u8 *ascii);

#endif
