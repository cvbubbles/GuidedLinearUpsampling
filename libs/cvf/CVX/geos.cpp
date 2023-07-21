
#include"geos.h"
#include<stdio.h>

_CVX_BEG


class GeoSeg::WorkPixel
{
public:
	uint   nbrw[4];
};

GeoSeg::GeoSeg()
:m_wpx(NULL), m_dist(NULL)
{
	m_width=m_height=m_stride=0;
}

GeoSeg::~GeoSeg()
{
	delete[]m_wpx;
	delete[]m_dist;
}

namespace i_geos{

inline float _px_diff(const uchar *px, const uchar *py)
{
	int d[]={int(px[0])-py[0], int(px[1])-py[1], int(px[2])-py[2]};

	return (float)(d[0]*d[0]+d[1]*d[1]+d[2]*d[2]);
}

static void _calc_gs_dist_c4(const uchar *img, int width, int height, int istep, int icn, GeoSeg::WorkPixel *wpx, int stride, const int _RMEAN, const float RSCALE, const int SPATIAL_DIST)
{
	float *diff=new float[width*height*4], *pdx=diff;
	int  dstride=4*width;
	double mean=0;

	for(int yi=0; yi<height; ++yi, img+=istep, pdx+=dstride)
	{
		float *ppx=pdx;
		const uchar *pix=img;

		for(int xi=0; xi<width; ++xi, pix+=icn, ppx+=4)
		{
			ppx[0]= xi==0? 0:ppx[-2];
			
			ppx[1]= yi==0? 0:ppx[-dstride+3];

			mean+=ppx[2]= xi==width-1? 0: _px_diff(pix, pix+icn);

			mean+=ppx[3]= yi==height-1? 0: _px_diff(pix, pix+istep);
		}
	}

	mean/=width*height*2;

	float scale=1.0f;
	if(_RMEAN>=0)
		scale=float(_RMEAN/mean);

	scale=scale*RSCALE;

	pdx=diff;
	for(int yi=0; yi<height; ++yi, wpx+=stride, pdx+=dstride)
	{
		GeoSeg::WorkPixel *ppx=wpx;
		float *pdxx=pdx;

		for(int xi=0; xi<width; ++xi, ++ppx, pdxx+=4)
		{
			for(int i=0; i<4; ++i)
			{
				ppx->nbrw[i]=(int)(scale*pdxx[i])+SPATIAL_DIST;
			}
		}
	}

	delete[]diff;
}

//
//static void _calc_gs_dist_c8(const uchar *img, int width, int height, int istep, int icn, GeoSeg::WorkPixel *wp, int stride, float beta_scale)
//{
//	GeoSeg::WorkPixel *wpx=wp;
//	double mean=0;
//
//	for(int yi=0; yi<height; ++yi, img+=istep, wpx+=stride)
//	{
//		GeoSeg::WorkPixel *ppx=wpx;
//		const uchar *pix=img;
//
//		for(int xi=0; xi<width; ++xi, pix+=icn, ++ppx)
//		{
//			if(xi!=0)
//			{
//				ppx->nbrw[0]= ppx[-1].nbrw[2];
//
//				if(yi!=0)
//					ppx->nbrw[4]= ppx[-stride-1].nbrw[6];
//			}
//			else
//			{
//				ppx->nbrw[0]=ppx->nbrw[4]=1;
//			}
//			
//			if(yi!=0)
//			{
//				ppx->nbrw[1]= ppx[-stride].nbrw[3];
//
//				if(xi!=width-1)
//					ppx->nbrw[5]=ppx[-stride+1].nbrw[7];
//			}
//			else
//			{
//				ppx->nbrw[1]=ppx->nbrw[5]=1;
//			}
//			
//
//			if(xi!=width-1)
//			{
//				mean+=ppx->nbrw[2]=_px_diff(pix, pix+icn)+1;
//
//				if(yi!=height-1)
//					mean+=ppx->nbrw[6]=_px_diff(pix,pix+istep+icn)+1;
//			}
//			else
//			{
//				ppx->nbrw[2]=ppx->nbrw[6]=1;
//				mean+=2;
//			}
//
//			if(yi!=height-1)
//			{
//				mean+=ppx->nbrw[3]=_px_diff(pix,pix+istep)+1;
//
//				if(xi!=0)
//					mean+=ppx->nbrw[7]=_px_diff(pix,pix+istep-icn)+1;
//			}
//			else
//			{
//				ppx->nbrw[2]=ppx->nbrw[7]=1;
//				mean+=2;
//			}
//		}
//	}
//
//	mean/=width*height*4;
//	double scale=beta_scale/(mean*2);
//
//	wpx=wp;
//	for(int yi=0; yi<height; ++yi, wpx+=stride)
//	{
//		GeoSeg::WorkPixel *ppx=wpx;
//
//		for(int xi=0; xi<width; ++xi, ++ppx)
//		{
//			for(int i=0; i<8; ++i)
//			{
//				ppx->nbrw[i]=(float)log(scale*ppx->nbrw[i]);
//			}
//		}
//	}
//}

}//end of i_geos

int GeoSeg::SetImage(const uchar *img, int width, int height, int istep, int icn, const int _RMEAN, const float RSCALE, const int SPATIAL_DIST)
{
	using namespace i_geos;
	int ec=-1;

	if(m_wpx)
	{
		delete[]m_wpx;
		m_wpx=NULL;
	}

	if(m_dist)
	{
		delete[]m_dist;
		m_dist=NULL;
	}

	if(img)
	{
		m_wpx=new WorkPixel[width*height];
		m_dist=new uint[width*height];

		m_width=width;
		m_height=height;
		m_stride=width;

		_calc_gs_dist_c4(img,width,height,istep,icn,m_wpx,m_stride, _RMEAN, RSCALE, SPATIAL_DIST);

		ec=0;
	}	

	return ec;
}

int GeoSeg::SetL1(int width, int height)
{
	using namespace i_geos;
	int ec=-1;

	if(m_wpx)
	{
		delete[]m_wpx;
		m_wpx=NULL;
	}

	if(m_dist)
	{
		delete[]m_dist;
		m_dist=NULL;
	}

//	if(img)
	{
		m_wpx=new WorkPixel[width*height];
		m_dist=new uint[width*height];

		m_width=width;
		m_height=height;
		m_stride=width;

	//	_calc_gs_dist_c4(img,width,height,istep,icn,m_wpx,m_stride, _RMEAN, RSCALE, SPATIAL_DIST);
		for(int i=0; i<width*height; ++i)
		{
			for(int j=0; j<4; ++j)
				m_wpx[i].nbrw[j]=1;
		}

		ec=0;
	}	

	return ec;
}

namespace i_geos{

const int _MAX_DIST=100000000;

static void _init_dist(uint *dist, int width, int height, int dstride)
{
	for(int yi=0; yi<height; ++yi, dist+=dstride)
	{
		for(int xi=0; xi<width; ++xi)
		{
			dist[xi]=_MAX_DIST;
		}
	}
}

inline uint _min_3(uint a,uint b, uint c)
{
	a=a>b? b:a;
	return a>c? c:a;
}


//#define _DTXX(d0, d1, mvx) (mvx==mv? 0: mvx!=0? _MAX_DIST: _min_3(d0,d1,dist[xi]))
#define _DTXX(d0, d1, mvx) (mvx==mv? 0: _min_3(d0,d1,dist[xi]))

static void _raster_LT_c4(const GeoSeg::WorkPixel *wpx, int width, int height, int stride, uint *dist, int dstride, const uchar *mask,int mstep, uchar mv)
{
	for(int yi=0; yi<height; ++yi, wpx+=stride, dist+=dstride, mask+=mstep)
	{
		for(int xi=0; xi<width; ++xi)
		{
			dist[xi]=_DTXX(wpx[xi].nbrw[0]+dist[xi-1], wpx[xi].nbrw[1]+dist[xi-dstride], mask[xi]);
		}
	}
}

static void _raster_RB_c4(const GeoSeg::WorkPixel *wpx, int width, int height, int stride,uint *dist, int dstride, const uchar *mask,int mstep, uchar mv)
{
	width=-width;

	for(int yi=0; yi<height; ++yi, wpx-=stride, dist-=dstride, mask-=mstep)
	{
		for(int xi=0; xi>width; --xi)
		{
			dist[xi]=_DTXX(wpx[xi].nbrw[2]+dist[xi+1], wpx[xi].nbrw[3]+dist[xi+dstride], mask[xi]); 
		}
	}
}

static void _raster_RT_c4(const GeoSeg::WorkPixel *wpx, int width, int height, int stride, uint *dist, int dstride,const uchar *mask,int mstep, uchar mv)
{
	width=-width;

	for(int yi=0; yi<height; ++yi, wpx+=stride, mask+=mstep, dist+=dstride)
	{
		for(int xi=0; xi>width; --xi)
		{
			dist[xi]=_DTXX(wpx[xi].nbrw[2]+dist[xi+1], wpx[xi].nbrw[1]+dist[xi-dstride], mask[xi]);
		}
	}
}

static void _raster_LB_c4(const GeoSeg::WorkPixel *wpx, int width, int height, int stride, uint *dist, int dstride,const uchar *mask,int mstep, uchar mv)
{
	for(int yi=0; yi<height; ++yi, wpx-=stride, mask-=mstep, dist-=dstride)
	{
		for(int xi=0; xi<width; ++xi)
		{
			dist[xi]=_DTXX(wpx[xi].nbrw[0]+dist[xi-1], wpx[xi].nbrw[3]+dist[xi+dstride], mask[xi]);
		}
	}
}

#undef _DTXX

static void _calc_geo_dist_c4(const GeoSeg::WorkPixel *wpx, int width, int height, int stride, uint *dist, int dstride, const uchar *mask, int mstep, uchar mv)
{
#define _WPP(x,y) (wpx+(y)*stride+(x))
#define _WMP(x,y) (mask+(y)*mstep+(x))
#define _WDP(x,y) (dist+(y)*dstride+(x))
	
	for(int i=0; i<5; ++i)
	{
		_raster_LT_c4(_WPP(1,1), width-1, height-1, stride, _WDP(1,1),dstride, _WMP(1,1), mstep,mv);

		_raster_RB_c4(_WPP(width-2,height-2), width-1, height-1, stride, _WDP(width-2,height-2), dstride,_WMP(width-2,height-2),mstep,mv);

		_raster_RT_c4(_WPP(width-2,1), width-1, height-1, stride, _WDP(width-2,1),dstride, _WMP(width-2,1),mstep,mv);

		_raster_LB_c4(_WPP(1,height-2), width-1, height-1, stride, _WDP(1,height-2), dstride,_WMP(1,height-2),mstep,mv);
	}
#undef _WPP
#undef _WMP
#undef _WDP
}

static void _get_dist(uint *dist, int width, int height, int dstride, float *prob, int pstride)
{
	for(int yi=0; yi<height; ++yi, dist+=dstride, prob+=pstride)
	{
		for(int xi=0; xi<width; ++xi)
		{
			prob[xi]=(float)dist[xi];
		}
	}
}

static void _get_prob(uint *dist, int width, int height, int dstride, float *prob, int pstride)
{
	for(int yi=0; yi<height; ++yi, dist+=dstride, prob+=pstride)
	{
		for(int xi=0; xi<width; ++xi)
		{
			float db=(float)dist[xi];

			prob[xi]=db/(prob[xi]+db);
		}
	}
}

} //....

int GeoSeg::GetProb(const uchar *mask, int mstep, float *prob, int stride)
{
	using namespace i_geos;

	_init_dist(m_dist, m_width, m_height, m_width);
	_calc_geo_dist_c4(m_wpx,m_width,m_height,m_stride, m_dist,m_width, mask,mstep,2);

	_get_dist(m_dist,m_width,m_height,m_width, prob, stride);

	_init_dist(m_dist, m_width, m_height, m_width);
	_calc_geo_dist_c4(m_wpx,m_width,m_height,m_stride,m_dist,m_width, mask,mstep,1);

	_get_prob(m_dist,m_width,m_height,m_width,prob,stride);

	return 0;
}

int GeoSeg::GetDist(const uchar *mask, int mstep, uint *dist, int dstride, uchar mv)
{
	using namespace i_geos;

	_init_dist(dist,m_width,m_height,dstride);
	_calc_geo_dist_c4(m_wpx,m_width,m_height,m_stride,dist,dstride,mask,mstep,mv);

	return 0;
}

namespace i_geos{

static void _init_dist(GSPoint *dist, int width, int height, int dstride)
{
	for(int yi=0; yi<height; ++yi, dist+=dstride)
	{
		for(int xi=0; xi<width; ++xi)
		{
			dist[xi].m_dist=_MAX_DIST;
			dist[xi].m_x=(ushort)xi;
			dist[xi].m_y=(ushort)yi;
		}
	}
}


//#define _DTXX(d0, d1, mvx) (mvx==mv? 0: mvx!=0? _MAX_DIST: _min_3(d0,d1,dist[xi]))
//#define _DTXX(d0, d1, mvx) (mvx==mv? 0: _min_3(d0,d1,dist[xi]))

//inline void _update_dist(GSPoint &cp, uint d0, GSPoint &pt0, uint d1, GSPoint &pt1, uchar mv)
#define _update_dist(cp, d0, pt0, d1, pt1, mvx) if(mvx==mv) cp.m_dist=0; else { uint md=d0; GSPoint *mp=&pt0; if(d1<d0) {md=d1; mp=&pt1;} if(md<cp.m_dist) {cp.m_dist=md; cp.m_x=mp->m_x; cp.m_y=mp->m_y; } }

static void _raster_LT_c4(const GeoSeg::WorkPixel *wpx, int width, int height, int stride, GSPoint *dist, int dstride, const uchar *mask,int mstep, uchar mv)
{
	for(int yi=0; yi<height; ++yi, wpx+=stride, dist+=dstride, mask+=mstep)
	{
		for(int xi=0; xi<width; ++xi)
		{
			_update_dist(dist[xi],wpx[xi].nbrw[0]+dist[xi-1].m_dist, dist[xi-1], wpx[xi].nbrw[1]+dist[xi-dstride].m_dist, dist[xi-dstride], mask[xi]);
		}
	}
}

static void _raster_RB_c4(const GeoSeg::WorkPixel *wpx, int width, int height, int stride,GSPoint *dist, int dstride, const uchar *mask,int mstep, uchar mv)
{
	width=-width;

	for(int yi=0; yi<height; ++yi, wpx-=stride, dist-=dstride, mask-=mstep)
	{
		for(int xi=0; xi>width; --xi)
		{
			_update_dist(dist[xi],wpx[xi].nbrw[2]+dist[xi+1].m_dist, dist[xi+1], wpx[xi].nbrw[3]+dist[xi+dstride].m_dist, dist[xi+dstride], mask[xi]); 
		}
	}
}

static void _raster_RT_c4(const GeoSeg::WorkPixel *wpx, int width, int height, int stride, GSPoint *dist, int dstride,const uchar *mask,int mstep, uchar mv)
{
	width=-width;

	for(int yi=0; yi<height; ++yi, wpx+=stride, mask+=mstep, dist+=dstride)
	{
		for(int xi=0; xi>width; --xi)
		{
			_update_dist(dist[xi],wpx[xi].nbrw[2]+dist[xi+1].m_dist, dist[xi+1], wpx[xi].nbrw[1]+dist[xi-dstride].m_dist, dist[xi-dstride], mask[xi]);
		}
	}
}

static void _raster_LB_c4(const GeoSeg::WorkPixel *wpx, int width, int height, int stride, GSPoint *dist, int dstride,const uchar *mask,int mstep, uchar mv)
{
	for(int yi=0; yi<height; ++yi, wpx-=stride, mask-=mstep, dist-=dstride)
	{
		for(int xi=0; xi<width; ++xi)
		{
			_update_dist(dist[xi],wpx[xi].nbrw[0]+dist[xi-1].m_dist, dist[xi-1], wpx[xi].nbrw[3]+dist[xi+dstride].m_dist, dist[xi+dstride], mask[xi]);
		}
	}
}



static void _calc_geo_dist_c4(const GeoSeg::WorkPixel *wpx, int width, int height, int stride, GSPoint *dist, int dstride, const uchar *mask, int mstep, uchar mv)
{
#define _WPP(x,y) (wpx+(y)*stride+(x))
#define _WMP(x,y) (mask+(y)*mstep+(x))
#define _WDP(x,y) (dist+(y)*dstride+(x))
	
	for(int i=0; i<1; ++i)
	{
		_raster_LT_c4(_WPP(1,1), width-1, height-1, stride, _WDP(1,1),dstride, _WMP(1,1), mstep,mv);

		_raster_RB_c4(_WPP(width-2,height-2), width-1, height-1, stride, _WDP(width-2,height-2), dstride,_WMP(width-2,height-2),mstep,mv);

		_raster_RT_c4(_WPP(width-2,1), width-1, height-1, stride, _WDP(width-2,1),dstride, _WMP(width-2,1),mstep,mv);

		_raster_LB_c4(_WPP(1,height-2), width-1, height-1, stride, _WDP(1,height-2), dstride,_WMP(1,height-2),mstep,mv);
	}
#undef _WPP
#undef _WMP
#undef _WDP
}

}

int GeoSeg::GetDistWithSource(const uchar *mask, int mstep, GSPoint *dist, int dstride, uchar mv)
{
	using namespace i_geos;

	_init_dist(dist,m_width,m_height,dstride);
	_calc_geo_dist_c4(m_wpx,m_width,m_height,m_stride,dist,dstride,mask,mstep,mv);

	return 0;
}


_CVX_END

