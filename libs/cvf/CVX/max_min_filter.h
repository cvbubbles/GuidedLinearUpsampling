
#pragma once

#include"def.h"

_CVX_BEG


//===========================================================
//Van Herk fast max/min filter

namespace i_max_min_filter
{
	template<typename _ValT>
	struct _GR
	{
		_ValT operator()(_ValT a, _ValT b)
		{
			return a>b ? a : b;
		}
	};

	template<typename _ValT>
	struct _LE
	{
		_ValT operator()(_ValT a, _ValT b)
		{
			return a<b ? a : b;
		}
	};

	//Van Herk fast max/min filter

	template<typename _ValT, typename _OpT>
	inline void _mm_filter_1D(const _ValT *pix, const int width, _ValT *pox, const int ops, const int ksz, _ValT *T, _OpT op)
	{
		assert(width%ksz == 0 && (ksz & 1));

		const int L = 2 * ksz - 1, M = L / 2, hksz = ksz / 2, hkszx = ksz + hksz;

		pix -= (ksz - 1) / 2;

		for (int i = 0; i<width; i += ksz, pix += ksz)
		{
			T[M] = pix[M];

			for (int j = M - 1; j >= 0; --j)
			{
				T[j] = op(pix[j], T[j + 1]);
			}

			for (int j = M + 1; j<L; ++j)
			{
				T[j] = op(pix[j], T[j - 1]);
			}

			for (int j = hksz; j<hkszx; ++j, pox += ops)
			{
				*pox = op(T[j - hksz], T[j + hksz]);
			}
		}
	}

	template<typename _ValT, typename _OpT>
	inline void _mm_filter_2D_x(const _ValT *pix, const int width, const int height, const int irs, const int ips, _ValT *pox, const int ors, const int ops, const int ksz, _ValT *T, _ValT fill, _OpT op)
	{
		const int xwidth = width%ksz == 0 ? width : (width / ksz + 1)*ksz;

		_ValT *row = new _ValT[xwidth + ksz];

		for (int i = 0; i<ksz / 2; ++i)
			row[i] = fill;

		for (int i = ksz / 2 + width; i<xwidth + ksz; ++i)
			row[i] = fill;

		row += ksz / 2;

		for (int yi = 0; yi<height; ++yi, pix += irs, pox += ors)
		{
			const _ValT *px = pix;

			for (int xi = 0; xi<width; ++xi, px += ips)
				row[xi] = *px;

			_mm_filter_1D(row, xwidth, pox, ops, ksz, T, op);
		}

		delete[](row - ksz / 2);
	}


	template<typename _ValT, typename _OpT>
	inline void _mm_filter_2D_y(const _ValT *pix, const int width, const int height, int irs, const int ips, _ValT *pox, const int ors, const int ops, const int ksz, _ValT *T, _ValT fill, _OpT op)
	{
		_ValT *obuf = new _ValT[(width + 2)*(height + 2 * ksz)];

		//scan rows
		_mm_filter_2D_x(pix, width, height, irs, ips, obuf + 2, width + 2, 1, ksz, T, fill, op);

		//scan columns
		_mm_filter_2D_x(obuf + 2, height, width, 1, width + 2, obuf, 1, width + 2, ksz, T, fill, op);

		pix = obuf;
		irs = width + 2;

		//copy to output buffer
		for (int yi = 0; yi<height; ++yi, pox += ors, pix += irs)
		{
			_ValT *px = pox;

			for (int xi = 0; xi<width; ++xi, px += ops)
			{
				*px = pix[xi];
			}
		}

		delete[]obuf;
	}

	template<typename _ValT, typename _OpT>
	inline void _mm_filter_2D(const _ValT *src, int width, int height, int istride, int cn, _ValT *dest, int dstride, int ksz, int nitr, _ValT fill, _OpT op)
	{
		_ValT *T = new _ValT[2 * ksz + 1];

		for (int i = 0; i<cn; ++i)
		{
			_mm_filter_2D_y(src + i, width, height, istride, cn, dest + i, dstride, cn, ksz, T, fill, op);
		}

		for (int k = 1; k<nitr; ++k)
		{
			for (int i = 0; i<cn; ++i)
			{
				_mm_filter_2D_y(dest + i, width, height, dstride, cn, dest + i, dstride, cn, ksz, T, fill, op);
			}
		}

		delete[]T;
	}

	template<typename _ValT, typename _OpT>
	inline void _mm_filter_2D_wb(const _ValT *src, int width, int height, int istride, int cn, _ValT *dest, int dstride, int ksz, int nitr, _ValT fill, _OpT op, int borderLTRB[])
	{
		if (!borderLTRB)
			_mm_filter_2D(src, width, height, istride, cn, dest, dstride, ksz, nitr, fill, op);
		else
		{
			int vbw[] = { ksz, ksz, ksz, ksz };

			for (int i = 0; i<4; ++i)
			{
				vbw[i] = __min(vbw[i], borderLTRB[i]);
				vbw[i] = __max(vbw[i], 0);
			}

			const int W = width + vbw[0] + vbw[2], H = height + vbw[1] + vbw[3];
			_ValT *buf = new _ValT[W*cn*H];

			_mm_filter_2D(src - vbw[1] * istride - vbw[0] * cn, W, H, istride, cn, buf, W*cn, ksz, nitr, fill, op);

			memcpy_2d(buf + W*vbw[1] + cn*vbw[0], width*cn*sizeof(_ValT), height, W*cn*sizeof(_ValT), dest, dstride*sizeof(_ValT));

			delete[]buf;
		}
	}
};

//!@ksz muast be odd
//!@dest can share the same memory with @src
template<typename _ValT>
inline void min_filter(const _ValT *src, int width, int height, int istride, int cn, _ValT *dest, int dstride, int ksz, int nitr = 1, int borderLTRB[] = NULL, _ValT maxVal= channel_traits<_ValT>::max_value())
{
	using namespace i_max_min_filter;
	if (src && dest)
	{
		if (ksz <= 1)
			memcpy_2d(src, width*sizeof(_ValT), height, istride*sizeof(_ValT), dest, dstride*sizeof(_ValT));
		else
			_mm_filter_2D_wb(src, width, height, istride, cn, dest, dstride, ksz, nitr, maxVal, _LE<_ValT>(), borderLTRB);
	}
}

template<typename _ValT>
inline void max_filter(const _ValT *src, int width, int height, int istride, int cn, _ValT *dest, int dstride, int ksz, int nitr = 1, int borderLTRB[] = NULL, _ValT minVal = (_ValT)0)
{
	using namespace i_max_min_filter;
	if (src && dest)
	{
		if (ksz <= 1)
			memcpy_2d(src, width*sizeof(_ValT), height, istride*sizeof(_ValT), dest, dstride*sizeof(_ValT));
		else
			_mm_filter_2D_wb(src, width, height, istride, cn, dest, dstride, ksz, nitr, minVal, _GR<_ValT>(), borderLTRB);
	}
}



_CVX_END

