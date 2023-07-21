
#pragma once

#include"def.h"
#include"iop.h"

#include"BFC/ctc.h"
#include"opencv2/core/types.hpp"

_CVX_BEG

void  memcpy_2d(const void* src, int row_size, int height, int istep, void* dest, int ostep);

void  memcpy_c(const void *src, int width, int height, int istep, int ips, void *dest, int ostep, int ops, int cps);

void  memset_2d(void* buf, const int row_size, const int height, const int step, char val);

template<typename _PValT, typename _ICN, typename _CNT>
inline void max_elem(const _PValT *img, int width, int height, int stride, const _ICN cn, _PValT _max[], _CNT acn)
{
	pxcpy(img, _max, acn);

	for_each_1(img, width, height, stride, int(cn), [acn, _max](const _PValT *v){
		for (int i = 0; i<acn; ++i)
		{
			if (v[i]>_max[i])
				_max[i] = v[i];
		}
	});
}

template<typename _PValT, typename _ICN, typename _CNT>
inline void min_elem(const _PValT *img, int width, int height, int stride, const _ICN cn, _PValT _min[], _CNT acn)
{
	pxcpy(img, _min, acn);

	for_each_1(img, width, height, stride, int(cn), [acn, _min](const _PValT *v){
		for (int i = 0; i<acn; ++i)
		{
			if (v[i]<_min[i])
				_min[i] = v[i];
		}
	});
}

template<typename _IValT>
inline void max_c1(const _IValT *imx, int width, int height, int xstep, const _IValT *imy, int ystep, _IValT *dest, int dstep)
{
	const int cwidth = width&~1;
	for (int yi = 0; yi<height; ++yi, imx += xstep, imy += ystep, dest += dstep)
	{
		int xi = 0;
		for (; xi<cwidth; xi += 2)
		{
			dest[xi] = __max(imx[xi], imy[xi]);
			dest[xi + 1] = __max(imx[xi + 1], imy[xi + 1]);
		}
		if (cwidth<width)
			dest[xi] = __max(imx[xi], imy[xi]);
	}
}

template<typename _IValT>
inline void min_c1(const _IValT *imx, int width, int height, int xstep, const _IValT *imy, int ystep, _IValT *dest, int dstep)
{
	const int cwidth = width&~1;
	for (int yi = 0; yi<height; ++yi, imx += xstep, imy += ystep, dest += dstep)
	{
		int xi = 0;
		for (; xi<cwidth; xi += 2)
		{
			dest[xi] = __min(imx[xi], imy[xi]);
			dest[xi + 1] = __min(imx[xi + 1], imy[xi + 1]);
		}
		if (cwidth<width)
			dest[xi] = __min(imx[xi], imy[xi]);
	}
}

template<typename _IPtrT, typename _SumT>
inline void accumulate(_IPtrT src, int width, int height, int sstride, _SumT &sum, _SumT init_val = _SumT(0))
{
	sum = init_val;

	for_each_1(src, width, height, sstride, ccn1(), [&sum](const typename std::iterator_traits<_IPtrT>::value_type & s){
		sum += s;
	});
}

// <T0: B, >=T1: F, otherwise: U
template<typename _DValT, typename _PValT>
inline void threshold(const uchar *src, int width, int height, int istep, _DValT *dest, int dstep, uchar T0, uchar T1, _PValT v0, _PValT vx, _PValT v1)
{
	if (src && dest)
	{
		_DValT mapx[256];

		int i = 0;
		for (; i<T0; ++i)
			mapx[i] = (_DValT)v0;
		for (; i<T1; ++i)
			mapx[i] = (_DValT)vx;
		for (; i<256; ++i)
			mapx[i] = (_DValT)v1;

		for_each_2(src, width, height, istep, ccn1(), dest, dstep, ccn1(), [&mapx](uchar v, _DValT &d){
			d = mapx[v];
		});
	}
}

template<typename _IValT, typename _DValT, typename _TValT, typename _PValT>
inline void threshold(const _IValT *src, int width, int height, int sstride, _DValT *dest, int dstride, _TValT _T, _PValT v0, _PValT v1)
{
	const _IValT T = (_IValT)_T;
	for_each_2(src, width, height, sstride, ccn1(), dest, dstride, ccn1(), [T, v0,v1](_IValT v, _DValT &d){
		d = v>T ? v1 : v0;
	});
}
template<typename _DValT, typename _TValT, typename _PValT>
inline void threshold(const uchar *src, int width, int height, int istep, _DValT *dest, int dstep, _TValT T, _PValT v0, _PValT v1)
{
	threshold(src, width, height, istep, dest, dstep, T, T, v0, v0, v1);
}





template<typename _ValT>
inline void clip_window_1d(_ValT &window_begin, _ValT &window_end, _ValT image_begin, _ValT image_end)
{
	if (window_begin<image_begin)
		window_begin = image_begin;

	if (window_end>image_end)
		window_end = image_end;
}

template<typename _OffsetT>
inline void calc_nbr_offset(int hw, int hh, int step, int cn, _OffsetT *offset, bool exclude_me = true)
{
	int n = 0;

	if (!offset)
		return;

	for (int i = -hh; i <= hh; ++i)
	{
		if (!exclude_me)
		{
			for (int j = -hw; j <= hw; ++j, ++n)
			{
				offset[n] = static_cast<_OffsetT>(i*step + j*cn);
			}
		}
		else
		{
			for (int j = -hw; j <= hw; ++j)
			{
				if (i != 0 || j != 0)
				{
					offset[n] = static_cast<_OffsetT>(i*step + j*cn);
					++n;
				}
			}
		}
	}
}

template<typename _MaskValT>
inline int get_mask_range(const _MaskValT *mask, int count, int &imin, int &imax)
{
	imin = imax = -1;

	if (mask)
	{
		for (int i = 0; i<count; ++i)
		{
			if (mask[i] != 0)
			{
				imin = i;
				break;
			}
		}

		if (imin >= 0)
		{
			for (int i = count; --i >= 0;)
			{
				if (mask[i] != 0)
				{
					imax = i + 1; //right open
					break;
				}
			}

			return 1;
		}
	}

	return 0;
}

template<typename _MaskValT>
inline int get_mask_roi(const _MaskValT *mask, int width, int height, int mstep, int LTRB[], uchar T = 0)
{
	if (!mask || !LTRB)
		return 0; //0==empty

	int ec = 1;

	uchar *mrow = new uchar[width], *mcol = new uchar[height];

	memset(mrow, 0, width);
	memset(mcol, 0, height);

	for (int yi = 0; yi<height; ++yi, mask += mstep)
	{
		for (int xi = 0; xi<width; ++xi)
		{
			if (mask[xi]>T)
			{
				mrow[xi] = 1;
				mcol[yi] = 1;
			}
		}
	}

	get_mask_range(mrow, width, LTRB[0], LTRB[2]);
	get_mask_range(mcol, height, LTRB[1], LTRB[3]);

	if (LTRB[0] >= LTRB[2] || LTRB[1] >= LTRB[3])
	{ //set empty
		memset(LTRB, 0, sizeof(int) * 4);
		ec = 0;
	}

	delete[]mrow;
	delete[]mcol;

	return ec;
}

template<typename _MaskValT>
inline Rect get_mask_roi(const _MaskValT *mask, int width, int height, int mstep, uchar T = 0)
{
	int LTRB[4];
	get_mask_roi(mask, width, height, mstep, LTRB, T);
	return Rect(LTRB[0], LTRB[1], LTRB[2] - LTRB[0], LTRB[3] - LTRB[1]);
}

template<typename _MaskValT>
inline int get_mask_diff_roi(const _MaskValT *maskx, int width, int height, int xstep, const _MaskValT *masky, int ystep, int LTRB[])
{
	if (!maskx || !masky || !LTRB)
		return 0; //0==empty;

	int ec = 1;

	uchar *mrow = new uchar[width], *mcol = new uchar[height];

	memset(mrow, 0, width);
	memset(mcol, 0, height);

	for (int yi = 0; yi<height; ++yi, maskx += xstep, masky += ystep)
	{
		for (int xi = 0; xi<width; ++xi)
		{
			if (maskx[xi] != masky[xi])
			{
				mrow[xi] = 1;
				mcol[yi] = 1;
			}
		}
	}

	get_mask_range(mrow, width, LTRB[0], LTRB[2]);
	get_mask_range(mcol, height, LTRB[1], LTRB[3]);

	if (LTRB[0] >= LTRB[2] || LTRB[1] >= LTRB[3])
	{ //set empty
		memset(LTRB, 0, sizeof(int) * 4);
		ec = 0;
	}

	delete[]mrow;
	delete[]mcol;

	return ec;
}

template<typename _PValT, typename _FValT, typename _CNT>
inline void warp_by_flow_nn(const _PValT *src, int width, int height, int sstride, _CNT scn, _PValT *dest, int dstride, const _FValT *reverse_flow_xy, int fstride)
{
	for (int yi = 0; yi<height; ++yi, dest += dstride, reverse_flow_xy += fstride)
	{
		_PValT *pdx = dest;
		for (int xi = 0; xi<width; ++xi, pdx += scn)
		{
			int x = int(xi + reverse_flow_xy[xi * 2] + 0.5), y = int(yi + reverse_flow_xy[xi * 2 + 1] + 0.5);
			if (uint(x) >= uint(width) || uint(y) >= uint(height))
			{
				x = xi; y = yi;
			}

			pxcpy(src + y*sstride + x*scn, pdx, scn);
		}
	}
}


_CVX_END

#include"max_min_filter.h"
#include"cc.h"
#include"mean_filter.h"
#include"resize.h"
#include"resample.h"
#include"gaussian_filter.h"
