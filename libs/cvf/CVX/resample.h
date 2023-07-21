

#pragma once


#include"def.h"
#include"BFC/ctc.h"
#include<cassert>

_CVX_BEG

class _CVX_API Resampler
{
protected:
	const char* _pImg;
	int _width,_height,_step;
public:
	void Init(const void* pData,int width,int height,int step, int =0);
	
};

struct  _CVX_API NNResampler
	:public Resampler
{
public:
	template<int,int _NC,typename _SrcT,typename _DestT>
	bool resample(double x,double y,_DestT* pDest)
	{
		enum{_PW=sizeof(_SrcT)*_NC};
		int ix=int(x+0.5),iy=int(y+0.5);
		if(unsigned(ix)<unsigned(_width)&&unsigned(iy)<unsigned(_height))
		{
		//	memcpy(pDest,_pImg+iy*_step+ix*_PW,_PW);
			
			const _SrcT *pi=(_SrcT*)(_pImg+iy*_step+ix*_PW);
			for(int i=0;i<_NC;++i)
				pDest[i]=static_cast<_DestT>(pi[i]);

			return true;
		}
		return false;
	}
};
struct _CVX_API  FixNNResampler
	:public Resampler
{
public:
	template<int _Shift,int _NC,typename _SrcT,typename _DestT>
	bool resample(int x,int y,_DestT* pDest)
	{
		enum{_PW=_NC*sizeof(_SrcT),_PAD=1<<(_Shift-1)};
		x=(x+_PAD)>>_Shift;
		y=(y+_PAD)>>_Shift;
		if(unsigned(x)<unsigned(_width)&&unsigned(y)<unsigned(_height))
		{
		//	memcpy(pDest,_pImg+y*_step+x*_PW,_PW);
			const _SrcT *pi=(_SrcT*)(_pImg+y*_step+x*_PW);
			for(int i=0;i<_NC;++i)
				pDest[i]=static_cast<_DestT>(pi[i]);

			return true;
		}
		return false;
	}
};
struct  _CVX_API BL4Resampler
	:public Resampler
{
public:
	template<int,int _NC,typename _SrcT,typename _DestT>
	bool resample(double x,double y,_DestT* pDest)
	{
		enum{_PW=_NC*sizeof(_SrcT)};
		int ix=(int)x,iy=(int)y;

		if(unsigned(ix)<unsigned(_width-1)&&unsigned(iy)<unsigned(_height-1))
		{
			double rx=x-ix,ry=y-iy;
			const _SrcT* pi=(_SrcT*)(_pImg+iy*_step+ix*_PW);
			for(int i=0;i<_NC;++i,++pi,++pDest)
			{
				const _SrcT* pix=pi;
				double tx0=*pix+(*(pix+_NC)-*pix)*rx;
				pix=(_SrcT*)((char*)pix+_step);
				double tx1=*pix+(*(pix+_NC)-*pix)*rx;
				*pDest=static_cast<_DestT>(tx0+(tx1-tx0)*ry);
			}
			return true;
		}
		else
			if(unsigned(ix)<unsigned(_width)&&unsigned(iy)<unsigned(_height))
			{
				const _SrcT *pi=(_SrcT*)(_pImg+iy*_step+ix*_PW);
				for(int i=0;i<_NC;++i)
					pDest[i]=static_cast<_DestT>(pi[i]);
				return true;
			}
		return false;
	}
};

struct  _CVX_API FixBL4Resampler
	:public Resampler
{
public:
	template<int _Shift,int _NC,typename _SrcT,typename _DestT>
	bool resample(int x,int y,_DestT* pDest)
	{ 
		enum{_PW=_NC*sizeof(_SrcT)};
		enum{_MASK=(1<<_Shift)-1,_SCALE=1<<_Shift};
		int rx=x&_MASK,ry=y&_MASK;
		x>>=_Shift,y>>=_Shift;
		if(unsigned(x)<unsigned(_width-1)&&unsigned(y)<unsigned(_height-1))
		{
			const _SrcT* pi=(_SrcT*)(_pImg+y*_step+x*_PW);
			for(int i=0;i<_NC;++i,++pi,++pDest)
			{
				const _SrcT* pix=pi;
				int tx0=int(*pix*_SCALE+(*(pix+_NC)-*pix)*rx);
				pix=(_SrcT*)((char*)pix+_step);
				int tx1=*pix*_SCALE+(*(pix+_NC)-*pix)*rx;
				*pDest=_DestT((
					tx0+(tx1-tx0)*ry/_SCALE
					)/_SCALE);
			}
			return true;
		}
		else
			if(unsigned(x)<unsigned(_width)&&unsigned(y)<unsigned(_height))
			{
				const _SrcT *pi=(_SrcT*)(_pImg+y*_step+x*_PW);
				for(int i=0;i<_NC;++i)
					pDest[i]=static_cast<_DestT>(pi[i]);
				return true;
			}
		return false;
	}
};


template<typename _DestT,typename _SrcT>
inline _DestT _MMTrunc(_SrcT val)
{
	const _DestT _MIN=NLimits<_DestT>::Min(),
		_MAX=NLimits<_DestT>::Max();

	return val<_MIN? _MIN:val>_MAX? _MAX:_DestT(val);
}

struct _CVX_API  CubicResampler
	:public Resampler
{
	static void cubicWeight(double* pw,double t)
	{
		assert(t>=1&&t<=2);
		pw[0]=((-t+6)*t-11)*t/6+1;
		pw[1]=((3*t-15)*t+18)*t/6;
		pw[2]=((-3*t+12)*t-9)*t/6;
		pw[3]=((t-3)*t+2)*t/6;
	}
public:
	template<int,int _NC,typename _SrcT,typename _DestT>
	bool resample(double x,double y,_DestT* pDest)
	{
		enum{_PW=_NC*sizeof(_SrcT)};
		int ix=int(x)-1,iy=int(y)-1;
		if(unsigned(ix)<unsigned(_width-3)&&unsigned(iy)<unsigned(_height-3))
		{
			double rx=x-ix,ry=y-iy;
			double wx[4],wy[4],yv[4];
			cubicWeight(wx,rx);
			cubicWeight(wy,ry);
		
			const _SrcT* pi=(_SrcT*)(_pImg+iy*_step+ix*_PW);
			for(int i=0;i<_NC;++i,++pi,++pDest)
			{
				const _SrcT* pix=pi;
				for(int k=0;k<4;++k,pix=(_SrcT*)((char*)pix+_step))
					yv[k]=*pix*wx[0]+*(pix+_NC)*wx[1]+*(pix+_NC+_NC)*wx[2]+*(pix+_NC+_NC+_NC)*wx[3];
				double fv=wy[0]*yv[0]+wy[1]*yv[1]+wy[2]*yv[2]+wy[3]*yv[3];
				*pDest=_MMTrunc<_DestT>(fv);				
			//	*pDest=static_cast<_DestT>(fv);
			}
			return true;
		}
		else
			if(unsigned(ix)<unsigned(_width)&&unsigned(iy)<unsigned(_height))
			{
				const _SrcT *pi=(_SrcT*)(_pImg+iy*_step+ix*_PW);
				for(int i=0;i<_NC;++i)
					pDest[i]=static_cast<_DestT>(pi[i]);
				return true;
			}
		return false;
	}
};

struct  _CVX_API FixCubicResampler
	:public Resampler
{
	
protected:
	const int* _pTab;
	int		   _nTabSize;

	static int* CreateLookUpTable(int shift);

	static int* _pLookUpTab[32];

public:
	static int*	GetLookUpTable(int shift);

public:
	FixCubicResampler();
		
	void set_tab(const int* pTab,int size);

	void Init(const void* pData,int width,int height,int step, int shift);
	
	template<int _Shift,int _NC,typename _SrcT,typename _DestT>
	bool resample(int x,int y,_DestT* pDest)
	{
		assert(_nTabSize==1<<_Shift);
		enum{_PW=_NC*sizeof(_SrcT)};
		enum{_MASK=(1<<_Shift)-1,_SCALE=1<<_Shift};
		int rx=x&_MASK,ry=y&_MASK;
		x=(x>>_Shift)-1,y=(y>>_Shift)-1;
		if(unsigned(x)<unsigned(_width-3)&&unsigned(y)<unsigned(_height-3))
		{
			const int* wx=&_pTab[4*rx],*wy=&_pTab[4*ry];
			const _SrcT* pi=(_SrcT*)(_pImg+y*_step+x*_PW);
			int yv[4]; 
			for(int i=0;i<_NC;++i,++pi,++pDest)
			{
				const _SrcT* pix=pi;
				for(int k=0;k<4;++k,pix=(_SrcT*)((char*)pix+_step))
					yv[k]=(*pix*wx[0]+*(pix+_NC)*wx[1]+*(pix+_NC+_NC)*wx[2]+*(pix+_NC+_NC+_NC)*wx[3])/_SCALE;
			//	*pDest=_MMTrunc<_SrcT>((wy[0]*yv[0]+wy[1]*yv[1]+wy[2]*yv[2]+wy[3]*yv[3])/_SCALE);
				int vx=(wy[0]*yv[0]+wy[1]*yv[1]+wy[2]*yv[2]+wy[3]*yv[3])/_SCALE;
				*pDest=_MMTrunc<_DestT>(vx);
			}
			return true;
		}
		else
			if(unsigned(x)<unsigned(_width)&&unsigned(y)<unsigned(_height))
			{
				const _SrcT *pi=(_SrcT*)(_pImg+y*_step+x*_PW);
				for(int i=0;i<_NC;++i)
					pDest[i]=static_cast<_DestT>(pi[i]);
				return true;
			}
		return false;
	}
};


_CVX_END

