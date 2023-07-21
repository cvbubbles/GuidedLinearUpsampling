
#include<assert.h>

#include"resample.h"
#include"BFC/cfg.h"

_CVX_BEG

void Resampler::Init(const void* pData,int width,int height,int step, int )
{
	_pImg=(char*)pData;
	_width=width,_height=height,_step=step;
}

int *FixCubicResampler::_pLookUpTab[32]={0};

int* FixCubicResampler::CreateLookUpTable(int shift)
{
	const int SZ=1<<shift;
	//int* pTab=(int*)ff_galloc(sizeof(int)*4*SZ),*px=pTab;
	int* pTab=new int[4*SZ],*px=pTab;
	double wx[4];
	double d=1.0/SZ,v=1.0;
	for(int i=0;i<SZ;++i,v+=d,px+=4)
	{
		CubicResampler::cubicWeight(wx,v);
		for(int k=0;k<4;++k)
			px[k]=int((SZ-1)*wx[k]+0.5);
	}
	return pTab;
}
int* FixCubicResampler::GetLookUpTable(int shift)
{
	assert(shift>=0&&shift<32);

	if(_pLookUpTab[shift]==0)
		_pLookUpTab[shift]=FixCubicResampler::CreateLookUpTable(shift);
	return _pLookUpTab[shift];
}

FixCubicResampler::FixCubicResampler()
:_pTab(0),_nTabSize(0)
{
}

void FixCubicResampler::set_tab(const int *pTab, int size)
{
	_pTab=pTab;
	_nTabSize=size;
}

void FixCubicResampler::Init(const void *pData, int width, int height, int step, int shift)
{
	Resampler::Init(pData,width,height,step,shift);

	if(!_pTab)
	{
		this->set_tab(GetLookUpTable(shift),1<<shift);
	}
}

_CVX_END
