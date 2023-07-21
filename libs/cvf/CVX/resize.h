#pragma once

#include"iop.h"

_CVX_BEG


_CVX_API void calc_resize_tab_nn(int ssize, int dsize, int tab[], int cn);


template<typename _PValT, typename _ICNT, typename _OCNT, typename _CNT>
inline void resize_nn(const _PValT *src, int width, int height, int istride, const _ICNT icn, _PValT *dest, int dwidth, int dheight, int ostride, const _OCNT  ocn, _CNT cn)
{
	if (width == dwidth && height == dheight && icn == cn && ocn == cn)
		memcpy_2d(src, width*sizeof(_PValT)*cn, height, sizeof(_PValT)*istride, dest, sizeof(_PValT)*ostride);
	else
	{
		int *xt = new int[dwidth + dheight], *yt = xt + dwidth;

		calc_resize_tab_nn(width, dwidth, xt, icn);
		calc_resize_tab_nn(height, dheight, yt, 1);

		const int cwidth = dwidth&~1;
		for (int yi = 0; yi<dheight; ++yi, dest += ostride)
		{
			const _PValT *psy = src + istride*yt[yi];
			_PValT *pdx = dest;

			int xi = 0;
			for (; xi<cwidth; xi += 2)
			{
				pxcpy(psy + xt[xi], pdx + xi*ocn, cn);
				pxcpy(psy + xt[xi + 1], pdx + (xi + 1)*ocn, cn);
			}
			if (cwidth<dwidth)
				pxcpy(psy + xt[xi], pdx + xi*ocn, cn);
		}

		delete[]xt;
	}
}

_CVX_API void calc_resize_tab_bl(int ssize, int dsize, int itab[], float ftab[], int cn);

_CVX_API void calc_resize_tab_bl(int ssize, int dsize, int itab[], int ftab[], int cn, int iscale);


template<typename _IPValT, typename _OPValT, typename _CNT>
inline void _resize_bl_t(const _IPValT *src, int width, int height, int istride, const int icn, _OPValT *dest, int dwidth, int dheight, int ostride, const int ocn, const _CNT cn, FlagType<false>)
{
	float *xt = new float[dwidth + dheight], *yt = xt + dwidth;
	int  *ixt = new int[dwidth + dheight], *iyt = ixt + dwidth;

	calc_resize_tab_bl(width, dwidth, ixt, xt, icn);
	calc_resize_tab_bl(height, dheight, iyt, yt, 1);

	for (int yi = 0; yi<dheight; ++yi, dest += ostride)
	{
		const _IPValT *psy = src + istride*iyt[yi];
		const float ry = yt[yi];

		_OPValT *pdx = dest;

		for (int xi = 0; xi<dwidth; ++xi, pdx += ocn)
		{
			const _IPValT *pi = psy + ixt[xi];
			const float rx = xt[xi];

			for (int i = 0; i<cn; ++i, ++pi)
			{
				const _IPValT *pix = pi;

				float tx0 = float(*pix + (*(pix + icn) - *pix)*rx);

				pix += istride;
				float tx1 = float(*pix + (*(pix + icn) - *pix)*rx);

				pdx[i] = static_cast<_OPValT>(tx0 + (tx1 - tx0)*ry);
			}
		}
	}

	delete[]xt;
	delete[]yt;
	delete[]ixt;
	delete[]iyt;
}

template<int SHIFT, typename _IPValT, typename _DValT, typename _ICNT, typename _CNT>
inline void _resize_bl_i_row(const _IPValT *src, const int width, const _ICNT icn, _DValT *dest, const int dwidth, const _CNT cn, const int it[], const int rt[])
{
	for (int i = 0; i<dwidth; ++i, dest += cn)
	{
		const _IPValT *psx = src + it[i];
		const int r = rt[i];
		if (r == 0)
		{
			for (int j = 0; j<cn; ++j)
				dest[j] = psx[j];
		}
		else
		{
			for (int j = 0; j<cn; ++j)
			{
				const _IPValT vj = psx[j];
				dest[j] = _DValT(((vj << SHIFT) + (psx[j + icn] - vj)*r) >> SHIFT);
			}
		}
	}
}

template<typename _IPValT, typename _OPValT, typename _CNT>
inline void _resize_bl_t(const _IPValT *src, int width, int height, int istride, const int icn, _OPValT *dest, int dwidth, int dheight, int ostride, const int ocn, const _CNT cn, FlagType<true>)
{
	int *xt = new int[(dwidth + dheight) * 2 + (sizeof(_OPValT)*dwidth * 2 * cn + sizeof(int)) / sizeof(int)], *yt = xt + dwidth;
	int  *ixt = yt + dheight, *iyt = ixt + dwidth;

	enum{ SHIFT = 16 };

	calc_resize_tab_bl(width, dwidth, ixt, xt, icn, 1 << SHIFT);
	calc_resize_tab_bl(height, dheight, iyt, yt, 1, 1 << SHIFT);

	//	int *drow0=new int[dwidth*2*cn], *drow1=drow0+dwidth*cn;
	_OPValT *drow0 = (_OPValT*)(iyt + dheight), *drow1 = drow0 + dwidth*cn;
	int pre_sy0 = -1, pre_sy1 = -1;

	for (int yi = 0; yi<dheight; ++yi, dest += ostride)
	{
		int sy0 = iyt[yi], sy1 = sy0 + 1;
		if (sy0 != pre_sy0)
		{
			if (sy0 == pre_sy1)
				std::swap(drow0, drow1);
			else
				_resize_bl_i_row<SHIFT>(src + sy0*istride, width, icn, drow0, dwidth, cn, ixt, xt);

			_resize_bl_i_row<SHIFT>(src + sy1*istride, width, icn, drow1, dwidth, cn, ixt, xt);

			pre_sy0 = sy0; pre_sy1 = sy1;
		}
		const int ry = yt[yi];
		_OPValT *pdx = dest;

		if (ry == 0)
		{
			if (ocn == cn)
				memcpy(dest, drow0, sizeof(_OPValT)*ocn*dwidth);
			else
			{
				for (int xi = 0; xi<dwidth; ++xi, pdx += ocn)
				{
					for (int i = 0; i<cn; ++i)
						pdx[i] = (_OPValT)drow0[xi*cn + i];
				}
			}
		}
		else
		{
			for (int xi = 0; xi<dwidth; ++xi, pdx += ocn)
			{
				for (int i = 0; i<cn; ++i)
				{
					const int tx0 = static_cast<int>(drow0[xi*cn + i]), tx1 = static_cast<int>(drow1[xi*cn + i]);

					pdx[i] = static_cast<_OPValT>(((tx0 << SHIFT) + (tx1 - tx0)*ry) >> SHIFT);
				}
			}
		}
	}

	delete[]xt;
}


template<typename _IPValT, typename _OPValT, typename _CNT>
inline void resize_bl(const _IPValT *src, int width, int height, int istride, const int icn, _OPValT *dest, int dwidth, int dheight, int ostride, const int ocn, const _CNT cn)
{
	_resize_bl_t(src, width, height, istride, icn, dest, dwidth, dheight, ostride, ocn, cn, FlagType<std::numeric_limits<_IPValT>::is_integer && std::numeric_limits<_OPValT>::is_integer>());
}

//==============================================================================================

template<typename _IPValT, typename _OPValT, typename _CNT>
inline void dsam_x2_mean(const _IPValT *img, int width, int height, int istride, const _CNT cn, _OPValT *dimg, int dstride)
{
	assert(width % 2 == 0 && height % 2 == 0);

	const int dwidth = width / 2, dheight = height / 2;
	const int cn2 = 2 * cn;

	for (int yi = 0; yi<dheight; ++yi, dimg += dstride, img += istride * 2)
	{
		_OPValT *pdx = dimg;
		const _IPValT *pix = img;

		for (int xi = 0; xi<dwidth; ++xi, pdx += cn, pix += cn2)
		{
			for (int i = 0; i<cn; ++i)
				pdx[i] = (int(pix[i]) + pix[i + cn] + pix[i + istride] + pix[i + istride + cn]) >> 2;
		}
	}
}

template<typename _IPValT, typename _OPValT, typename _CNT>
inline void dsam_x4_mean(const _IPValT *img, int width, int height, int istride, const _CNT cn, _OPValT *dimg, int dstride)
{
	assert(width % 4 == 0 && height % 4 == 0);

	const int dwidth = width / 4, dheight = height / 4;
	const int cn4 = 4 * cn, cn2 = 2 * cn, istride2 = istride * 2;

	for (int yi = 0; yi<dheight; ++yi, dimg += dstride, img += istride * 4)
	{
		_OPValT *pdx = dimg;
		const _IPValT *pix = img;

		for (int xi = 0; xi<dwidth; ++xi, pdx += cn, pix += cn4)
		{
			for (int i = 0; i<cn; ++i)
				pdx[i] = (int(pix[i]) + pix[i + cn2] + pix[i + istride2] + pix[i + istride2 + cn2]) >> 2;
		}
	}
}

template<typename _IPValT, typename _OPValT>
inline void _usam_x2c1_bl_row(const _IPValT *a, _OPValT *A, const int w)
{
	for (int j = 0; j<w - 1; ++j, a++, A += 2)
	{
		A[0] = _OPValT(a[0]);
		A[1] = _OPValT(((a[0]) + a[1]) >> 1);
	}
	A[0] = A[1] = _OPValT(a[0]);
}

template<typename _IPValT, typename _OPValT>
inline void _usam_x4c1_bl_row(const _IPValT *a, _OPValT *A, const int w)
{
	for (int j = 0; j<w - 1; ++j, a++, A += 4)
	{
		A[0] = _OPValT(a[0]);

		A[2] = _OPValT(((a[0]) + a[1]) >> 1);
		A[1] = _OPValT(((A[0]) + A[2]) >> 1);
		A[3] = _OPValT(((A[2]) + a[1]) >> 1);
	}
	A[0] = A[1] = A[2] = A[3] = _OPValT(a[0]);
}

template<typename _PValT>
inline void _usam_x4c1_bl_col(int W, _PValT *A0, int Astep)
{
	uchar *A1 = A0 + Astep * 2, *A2 = A0 + Astep * 4;
	for (int i = 0; i<W; ++i)
	{
		A1[i] = _PValT(((A0[i]) + A2[i]) >> 1);
		A1[i - Astep] = _PValT(((A0[i]) + A1[i]) >> 1);
		A1[i + Astep] = _PValT(((A2[i]) + A1[i]) >> 1);
	}
}

template<typename _PValT>
inline void _usam_x2c1_bl_col(int W, _PValT *A0, int Astep)
{
	uchar *A1 = A0 + Astep, *A2 = A0 + Astep * 2;
	for (int i = 0; i<W; ++i)
	{
		A1[i] = _PValT(((A0[i]) + A2[i]) >> 1);
	}
}

template<typename _IPValT, typename _OPValT>
inline void usam_x2c1_bl(const _IPValT *a, int width, int height, int astride, _OPValT *A, int Astride)
{
 const int WIDTH = width * 2, HEIGHT = height * 2;

	_usam_x2c1_bl_row(a, A, width);

	for (int yi = 1; yi<height; ++yi, A += Astride * 2, a += astride)
	{
		_usam_x2c1_bl_row(a + astride, A + Astride * 2, width);

		_usam_x2c1_bl_col(WIDTH, A, Astride);
	}

	const _OPValT *A0 = A;
	A += Astride;
	for (int j = 0; j<WIDTH; ++j)
		A[j] = (_OPValT)A0[j];
}

template<typename _IPValT, typename _OPValT>
inline void usam_x4c1_bl(const _IPValT *a, int width, int height, int astride, _OPValT *A, int Astride)
{
	const int WIDTH = width * 4, HEIGHT = height * 4;

	_usam_x4c1_bl_row(a, A, width);

	for (int yi = 1; yi<height; ++yi, A += Astride * 4, a += astride)
	{
		_usam_x4c1_bl_row(a + astride, A + Astride * 4, width);

		_usam_x4c1_bl_col(WIDTH, A, Astride);
	}

	const _OPValT *A0 = A;
	for (int i = 0; i<3; ++i)
	{
		A += Astride;
		for (int j = 0; j<WIDTH; ++j)
			A[j] = (_OPValT)A0[j];
	}
}


_CVX_END

