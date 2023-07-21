
#pragma once

//#include<tchar.h>
#include"CVX/def.h"

using namespace ff;

const int FI_IO_IGNORE_ALPHA=0x01;

const int QIM_SWAP_RB=0x01;
const int QIM_DITHER=0x02;

int  QuantizeImageMedian(const uchar *pData,const int width,const int height,const int step,const int cn,
				   uchar* pIndex,uchar (*pMap)[3],const int mapSize,int mode=QIM_SWAP_RB|QIM_DITHER);


