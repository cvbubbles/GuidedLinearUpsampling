
#pragma once

#include"iop.h"

_CVX_BEG

//===========================================================

template<typename _IntValT>
class integral_image
{
	_IntValT *m_inti_buf, *m_inti;
public:
	int       m_width, m_height;
	int		  m_cn;
	int       m_max_kszx, m_max_kszy;
	int       m_borderLTRB[4];
public:
	integral_image()
		:m_inti_buf(NULL), m_inti(NULL)
	{
	}
	~integral_image()
	{
		this->clear();
	}
	void clear()
	{
		delete[]m_inti_buf;
		m_inti_buf=NULL;
		m_width=m_height=m_cn=m_max_kszx=m_max_kszy=0;
	}

private:
	template<typename _ValT>
	void _asn_val(_ValT *ptr, int count, int cn, int ps, _ValT val)
	{
		for(int i=0; i<count; ++i, ptr+=ps)
		{
			for(int j=0; j<cn; ++j)
				*ptr=val;
		}
	}
	template<typename _ValT>
	void _integral_image(const _ValT *src, int width, int height, int sstride, _IntValT *pint, int istride, int icn)
	{
		_IntValT *prow=new _IntValT[width];

		memset(prow,0,sizeof(_IntValT)*width);

		for(int yi=0; yi<height; ++yi, src+=sstride, pint+=istride)
		{
			_IntValT *pix=pint;
			for(int xi=0; xi<width; ++xi, pix+=icn)
			{
				prow[xi]+=src[xi];
				pix[0]=pix[-icn]+prow[xi];
			}
		}

		delete[]prow;
	}

	template<typename _OValT>
	void _get_sum(const _IntValT *pint, int width, int height, int istride, _OValT *dest, int dstride, const int hkszx, const int hkszy, const int cn)
	{
		// LT, RB, RT, LB
		const int di[]={-( (hkszy+1)*istride+(hkszx+1)*cn ), hkszy*istride+hkszx*cn, -(hkszy+1)*istride+hkszx*cn, hkszy*istride-(hkszx+1)*cn};
		const int W=width*cn;

		for(int yi=0; yi<height; ++yi, pint+=istride, dest+=dstride)
		{
			const _IntValT *pix=pint;

			for(int xi=0; xi<W; ++xi, ++pix)
			{
				dest[xi]=_OValT( pix[di[0]]+pix[di[1]]-pix[di[2]]-pix[di[3]] );
			}
		}
	}
public:
	template<typename _ValT>
	void do_integral(const _ValT *src, int width, int height, int istride, int cn, int max_kszx, int max_kszy, int borderLTRB[])
	{
		int bdr[]={0,0,0,0};

		if(borderLTRB)
			memcpy(bdr,borderLTRB,sizeof(int)*4);

		const int kszx=max_kszx, kszy=max_kszy;
		const int hkszx=kszx/2, hkszy=kszy/2, xwidth=width+hkszx*2, xheight=height+hkszy*2;

		{
			bdr[0]=__min(hkszx,bdr[0]);
			bdr[1]=__min(hkszy,bdr[1]);
			bdr[2]=__min(hkszx,bdr[2]);
			bdr[3]=__min(hkszy,bdr[3]);
		}

		_IntValT *pint_m=new _IntValT[(xwidth+1)*cn*(xheight+1)], *pint=pint_m+(xwidth+2)*cn;

		_asn_val(pint_m,(xwidth+1)*cn,1,1,0);
		_asn_val(pint_m,xheight+1,cn,(xwidth+1)*cn,0);

		_ValT *pin=new _ValT[xwidth*xheight];
		memset(pin,0,sizeof(_ValT)*xwidth*xheight);

		for(int i=0; i<cn; ++i)
		{
			for_each_2(src-bdr[1]*istride-bdr[0]*cn+i,width+bdr[0]+bdr[2],height+bdr[1]+bdr[3],istride,cn,
				pin+(hkszy-bdr[1])*xwidth+(hkszx-bdr[0]),xwidth,1, [](const _ValT *s, _ValT *d){
					*d=*s;
			} );

			_integral_image(pin,xwidth,xheight,xwidth,pint+i,(xwidth+1)*cn,cn);

		}

		this->clear();
		m_inti=pint;
		m_inti_buf=pint_m;
		m_width=width;
		m_height=height;
		m_cn=cn;
		m_max_kszx=kszx;
		m_max_kszy=kszy;
		memcpy(this->m_borderLTRB,bdr,sizeof(bdr));

	//	delete[]pint_m; 
		delete[]pin;
	}

	template<typename _OValT>
	void get_sum(_OValT *dest, int dstride, const int kszx, const int kszy, int x=0, int y=0, int width=-1, int height=-1)
	{
		assert(kszx<=m_max_kszx && kszy<=m_max_kszy);

		if(width<0) 
			width=m_width;

		if(height<0)
			height=m_height;

		int xwidth=width+(m_max_kszx&~1);

		this->_get_sum(m_inti+((m_max_kszy/2+y)*(xwidth+1)+(m_max_kszx/2+x))*m_cn, width, height, (xwidth+1)*m_cn,
			dest, dstride, kszx/2, kszy/2, m_cn
			);
	}
};


//fast sum filter implemented with integral image
template<typename _ValT, typename _SumT>
inline void sum_filter(const _ValT *src, int width, int height, int istep, int cn, _SumT *dest, int dstride, int kszx, int kszy, int borderLTRB[]=NULL)
{
	if(src && dest)
	{
		integral_image<typename channel_traits<_ValT>::accum_type > ii;
		ii.do_integral(src,width,height,istep,cn,kszx,kszy,borderLTRB);
		ii.get_sum(dest,dstride,kszx,kszy);
	}
}

 void mean_filter(const uchar *src, int width, int height, int istep, int cn, uchar *dest, int dstride, int *np, int nstride, int kszx, int kszy, int borderLTRB[]=NULL);

 void mean_filter(const uchar *src, int width, int height, int istep, int cn, int *dest, int dstride, int *np, int nstride, int kszx, int kszy, int borderLTRB[]=NULL);

 void mean_filter(const int *src, int width, int height, int istep, int cn, int *dest, int dstride, int *np, int nstride, int kszx, int kszy, int borderLTRB[]=NULL);


template<typename _SrcValT, typename _DestValT>
inline void mean_filter_1d(const _SrcValT *src, int count, _DestValT *dest, int ksz)
{
	typedef typename AccumType<_SrcValT>::RType _SumT;

	const int hksz=ksz>>1;
	_SumT sum=hksz*src[0];

	for(int i=0; i<hksz; ++i)
		sum+=src[i];

	int k=0;
	for(; k<hksz; ++k)
	{
		sum+=src[k+hksz]; dest[k]=sum/ksz; sum-=src[0];
	}
	const int rb=count-hksz;
	for(; k<rb; ++k)
	{
		sum+=src[k+hksz]; dest[k]=sum/ksz; sum-=src[k-hksz];
	}
	for(; k<count; ++k)
	{
		sum+=src[count-1]; dest[k]=sum/ksz; sum-=src[k-hksz];
	}
}

template<typename _SrcValT, typename _DestValT, typename _CNT>
inline void mean_filter_1d(const _SrcValT *src, int count, _DestValT *dest, int ksz,  _CNT ccn)
{
	typedef typename channel_traits<_SrcValT>::accum_type _SumT;

	const int hksz=ksz>>1;
	_SumT sum[_CNT::channels]={0};
	
	for(int i=0; i<ccn; ++i)
		sum[i]=hksz*src[i];

	for(int i=0; i<hksz; ++i)
	{
		for(int j=0; j<ccn; ++j)
			sum[j]+=src[i*ccn+j];
	}

	int k=0;
	for(; k<hksz; ++k)
	{
		for(int j=0; j<ccn; ++j)
		{
			sum[j]+=src[ (k+hksz)*ccn+j]; dest[k*ccn+j]=sum[j]/ksz; sum[j]-=src[j];
		}
	}
	const int rb=count-hksz;
	for(; k<rb; ++k)
	{
		for(int j=0; j<ccn; ++j)
		{
			sum[j]+=src[(k+hksz)*ccn+j]; dest[k*ccn+j]=sum[j]/ksz; sum[j]-=src[(k-hksz)*ccn+j];
		}
	}
	for(; k<count; ++k)
	{
		for(int j=0; j<ccn; ++j)
		{
			sum[j]+=src[(count-1)*ccn+j]; dest[k*ccn+j]=sum[j]/ksz; sum[j]-=src[(k-hksz)*ccn+j];
		}
	}
}

template<typename _SrcValT, typename _DestValT, typename _CNT>
inline void ipfi_mean_filter_row(const _SrcValT *src, int count, _DestValT *dest, int ksz,  _CNT ccn)
{
	const int hksz=ksz>>1;

	_DestValT sum[cn_max<_CNT>::value];
	assert(ccn<=sizeof(sum)/sizeof(sum[0]));
	
	for(int i=0; i<ccn; ++i)
		sum[i]=hksz*src[i];

	for(int i=0; i<hksz; ++i)
	{
		for(int j=0; j<ccn; ++j)
			sum[j]+=src[i*ccn+j];
	}

	const int hksz_ccn=hksz*ccn;
	int b=0;
	for(; b<hksz_ccn; b+=ccn)
	{
		for(int j=0; j<ccn; ++j)
		{
			//sum[j]+=src[ (k+hksz)*ccn+j]; dest[k*ccn+j]=sum[j]; sum[j]-=src[j];
			sum[j]+=src[ b+hksz_ccn+j]; dest[b+j]=sum[j]; sum[j]-=src[j];
		}
	}
	int rb=(count-hksz)*ccn;
	for(; b<rb;  b+=ccn)
	{
		for(int j=0; j<ccn; ++j)
		{
			//sum[j]+=src[(k+hksz)*ccn+j]; dest[k*ccn+j]=sum[j]; sum[j]-=src[(k-hksz)*ccn+j];
			_DestValT t=sum[j]+src[b+j+hksz_ccn]; dest[b+j]=t; sum[j]=t-src[b+j-hksz_ccn];
		}
	}
	rb=count*ccn;
	for(; b<count; b+=ccn)
	{
		for(int j=0; j<ccn; ++j)
		{
			//sum[j]+=src[(count-1)*ccn+j]; dest[k*ccn+j]=sum[j]; sum[j]-=src[(k-hksz)*ccn+j];
			sum[j]+=src[(count-1)*ccn+j]; dest[b+j]=sum[j]; sum[j]-=src[b-hksz_ccn+j];
		}
	}
}

template<int RSHIFT, typename _SrcValT, typename _DestValT>
inline void ipfi_mean_filter_col(_SrcValT vsum[], int width, const _SrcValT vadd[], const _SrcValT vdel[], const _SrcValT iscale, _DestValT *dest)
{
#define _DSCALE(x)  _DestValT((x)*iscale>>RSHIFT)

	const int cwidth=width&~1;
	int i=0;
	for(; i<cwidth; i+=2)
	{
		_SrcValT t0=vsum[i]+vadd[i], t1= vsum[i+1]+vadd[i+1];
		dest[i]=_DSCALE( t0); dest[i+1]=_DSCALE( t1); 
		vsum[i]=t0-vdel[i];    vsum[i+1]=t1-vdel[i+1];
	}
	if(width&1)
	{
		vsum[i]+=vadd[i]; 
		dest[i]=_DSCALE( vsum[i]); 
		vsum[i]-=vdel[i];
	}
#undef _DSCALE
}

template<typename _SrcValT, typename _DestValT, typename _CNT>
inline void mean_filter_x(const _SrcValT *img, int width, int height, int istride, _CNT ccn, _DestValT *dest, int dstride, const int kszx, const int kszy)
{
	typedef typename channel_traits<_SrcValT>::accum_type _SumT;

	enum{RSHIFT=24};
	const int iscale=int ( double(1<<RSHIFT)/(kszx*kszy) + 0.5);
	_SumT *buf=(_SumT*)new char[sizeof(_SumT)*width*ccn*(kszy+1) + sizeof(_SumT*)* kszy /*+ sizeof(_SumT)*ccn*/];

	const int hkszy=kszy/2;
	_SumT *sum=buf+width*ccn*kszy;
	_SumT **vrows=(_SumT**)(sum+width*ccn);
	//_SumT *row_sum=(_SumT*)(vrows+kszy);
	//_SumT row_sum[8];

	for(int i=0; i<kszy; ++i)
	{
		vrows[i]=buf+width*ccn*i;
	}

	memset(sum,0,sizeof(_SumT)*width*ccn);
	for(int i=0; i<hkszy; ++i)
	{
		ipfi_mean_filter_row(img+i*istride, width, vrows[i], kszx, ccn);

		if(i==0)
		{
			for(int j=0; j<width*ccn; ++j)
				sum[j]=vrows[0][j]*(hkszy+1);
		}
		else
		{
			for(int j=0; j<width*ccn; ++j)
				sum[j]+=vrows[i][j];
		}
	}

	for(int yi=0; yi<height; ++yi)
	{
		if(yi<height-hkszy)
			ipfi_mean_filter_row(img+(yi+hkszy)*istride, width, vrows[(yi+hkszy)%kszy], kszx, ccn);
		
		ipfi_mean_filter_col<RSHIFT>(sum,width*ccn,vrows[__min(yi+hkszy,height-1)%kszy], vrows[__max(yi-hkszy,0)%kszy], (_SumT)iscale, dest+yi*dstride);
	}

	delete[](char*)buf;
}

 void mean_filter(const uchar *src, int width, int height, int istep, int cn, uchar *dest, int dstride, int kszx, int kszy);

 void mean_filter(const int *src, int width, int height, int istep, int cn, int *dest, int dstride, int kszx, int kszy);

//==============================================================================




_CVX_END


