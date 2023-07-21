
#pragma once

#include"def.h"

_CVX_BEG


namespace i_smooth {

#if 1
	template<typename _IValT, typename _DValT, typename _KValT, int cn>
	inline void convolve(const _IValT *data, ccn<cn> _cn, int px_stride, const _KValT kw[], int size, _DValT dest[], int shift)
	{
		_KValT dv[cn] = { 0 };

		for (int i = 0; i<size; ++i, data += px_stride)
		{
			for (int j = 0; j<cn; ++j)
				dv[j] += data[j] * kw[i];
		}
		for (int i = 0; i<cn; ++i)
			dest[i] = _DValT(dv[i] >> shift);
	}
#else
	template<typename _IValT, typename _DValT, typename _KValT, int cn>
	inline void convolve(const _IValT *data, ccn<cn> _cn, int px_stride, const _KValT kw[], int size, _DValT dest[], int shift)
	{
		const int hsize = size / 2;
		_KValT dv[cn];

		for (int i = 0; i<cn; ++i)
			dv[j] = data[hsize] * kw[hsize];

		for (int i = 0; i<hsize; ++i, data += px_stride)
		{
			for (int j = 0; j<cn; ++j)
				dv[j] += (data[j] * kw[i];
		}
		for (int i = 0; i<cn; ++i)
			//dest[i]=_DValT(dv[i]>>shift);
			dest[i] = _DValT(dv[i] >> 12);
	}
#endif

	template<typename _IValT, typename _DValT, typename _KValT, typename _CNT>
	inline void smooth_1d(const _IValT *data, int count, _CNT cn, int px_stride, _DValT *dest, int dpx_stride, const _KValT kw[], int ksz, int shift)
	{
		const int hksz = ksz / 2;

		_DValT *dx = dest;

		for (int i = 1; i <= hksz; ++i, dx += dpx_stride)
		{
			convolve(data, cn, px_stride, kw + hksz - i, hksz + i + 1, dx, shift);
		}

		const _IValT *px = data;
		for (int i = hksz * 2; i<count; ++i, px += px_stride, dx += dpx_stride)
		{
			convolve(px, cn, px_stride, kw, ksz, dx, shift);
		}

		for (int i = 1; i <= hksz; ++i, px += px_stride, dx += dpx_stride)
		{
			convolve(px, cn, px_stride, kw, ksz - i, dx, shift);
		}
	}

	template<typename _IValT, typename _DValT, typename _KValT, typename _CNT>
	inline void smooth(const _IValT *img, int width, int height, int istride, _CNT cn, _DValT *dimg, int dstride, const _KValT kw[], int ksz, int shift)
	{
		_KValT *buf = new _KValT[width*cn*height];

		for (int xi = 0; xi<width; ++xi)
		{
			smooth_1d(img + xi*cn, height, cn, istride, buf + xi*cn, width*cn, kw, ksz, shift);
		}
		for (int yi = 0; yi<height; ++yi)
		{
			smooth_1d(buf + yi*width*cn, width, cn, cn, dimg + dstride*yi, cn, kw, ksz, shift);
		}

		delete[]buf;
	}

} //end of i_smooth

template<typename _KValT>
inline void make_gaussian_kernel(double sigma, _KValT kw[], int ksz)
{
	const int hksz = ksz / 2;
	sigma = 2 * sigma*sigma;
	_KValT sum = 0;
	for (int i = 0; i<ksz; ++i)
	{
		const int d = i - hksz;
		kw[i] = exp(-d*d / sigma);
		sum += kw[i];
	}
	for (int i = 0; i<ksz; ++i)
		kw[i] /= sum;
}

template<typename _KValT>
inline void make_gaussian_kernel_fp(double sigma, _KValT kw[], int ksz, int shift)
{
	double *dkw = new double[ksz];
	make_gaussian_kernel(sigma, dkw, ksz);

	for (int i = 0; i<ksz; ++i)
		kw[i] = _KValT(dkw[i] * (1 << shift) + 0.5);

	delete[]dkw;
}

template<typename _IValT, typename _DValT, typename _CNT>
inline void gaussian_filter(const _IValT *img, int width, int height, int istride, _CNT cn, _DValT *dimg, int dstride, int ksz, double sigma)
{
	using namespace i_smooth;

	const int shift = 12;
	int *kw = new int[ksz];

	make_gaussian_kernel_fp(sigma, kw, ksz, shift);
	smooth(img, width, height, istride, cn, dimg, dstride, kw, ksz, shift);
	delete[]kw;
}


template<typename _IValT, typename _ICNT>
inline void ipfi_gaussian_filter_3x3_c1_row(const _IValT *img, int width, _ICNT icn, int *dest, const int w0, const int w1, const int rshift)
{
	dest[0] = (w1*(img[0] + img[icn]) + w0*img[0]) >> rshift;
	const _IValT *px = img + icn;
	const int icn2 = icn + icn, cwidth = (width - 2)&~1 + 1;
	for (int i = 1; i<cwidth; i += 2, px += icn2)
	{
		dest[i] = (w1* (px[-icn] + px[icn]) + w0*  (*px)) >> rshift;
		dest[i + 1] = (w1* (px[0] + px[icn2]) + w0*  px[icn]) >> rshift;
	}
	if (width & 1)
	{
		dest[width - 2] = (w1*(px[-icn] + px[icn]) + w0*  (*px)) >> rshift;
		px += icn;
	}
	dest[width - 1] = (w1*(px[-icn] + *px) + w0* (*px)) >> rshift;
}

template<typename _IValT, typename _DCNT>
inline void ipfi_gaussian_filter_3x3_c1_col(const int *r0, const int *r1, const int *r2, int width, _IValT *dimg, _DCNT dcn, const int w0, const int w1, const int rshift)
{
	const int cwidth = width&~1;
	int i = 0;
	for (; i<cwidth; i += 2, dimg += dcn * 2)
	{
		*dimg = _IValT((w1* (r0[i] + r2[i]) + w0*r1[i]) >> rshift);
		dimg[dcn] = _IValT((w1* (r0[i + 1] + r2[i + 1]) + w0*r1[i + 1]) >> rshift);
	}
	if (width & 1)
		*dimg = _IValT((w1* (r0[i] + r2[i]) + w0*r1[i]) >> rshift);
}

template<typename _IValT, typename _DValT, typename _ICNT, typename _DCNT>
inline void gaussian_filter_3x3_c1(const _IValT *img, int width, int height, int istep, _ICNT icn, _DValT *dimg, int dstep, _DCNT dcn, const int w0 = 10, const int w1 = 4, const int rshift = 4)
{
	assert((w0 + w1 * 2) == (1 << rshift));
	int *buf = new int[width * 3];
	int *_vrows[] = { buf + width * 2, buf, buf + width, buf + width * 2, buf };
	int **vrows = _vrows + 1;

	for (int i = 0; i<2; ++i)
	{
		ipfi_gaussian_filter_3x3_c1_row(img + istep*i, width, icn, vrows[i], w0, w1, rshift);
	}
	ipfi_gaussian_filter_3x3_c1_col(vrows[0], vrows[0], vrows[1], width, dimg, dcn, w0, w1, rshift);

	const _IValT *piy = img + istep * 2;
	_DValT *pdy = dimg + dstep;
	for (int yi = 2; yi<height; ++yi, piy += istep, pdy += dstep)
	{
		const int ri = (yi - 1) % 3;
		ipfi_gaussian_filter_3x3_c1_row(piy, width, icn, vrows[ri + 1], w0, w1, rshift);

		ipfi_gaussian_filter_3x3_c1_col(vrows[ri - 1], vrows[ri], vrows[ri + 1], width, pdy, dcn, w0, w1, rshift);
	}

	const int ri = (height - 1) % 3;
	ipfi_gaussian_filter_3x3_c1_col(vrows[ri - 1], vrows[ri], vrows[ri], width, pdy, dcn, w0, w1, rshift);

	delete[]buf;
}


template<typename _IValT, typename _ICNT, typename _CNT>
inline void ipfi_gaussian_filter_3x3_row(const _IValT *img, int width, _ICNT icn, _CNT cn, int *dest, const int w0, const int w1, const int rshift)
{
	for (int k = 0; k<cn; ++k)
		dest[k] = (w1*(img[k] + img[icn + k]) + w0*img[k]) >> rshift;

	const _IValT *px = img + icn;
	int *pdx = dest + cn;
	width -= 1;
	for (int i = 1; i<width; ++i, px += icn, pdx += cn)
	{
		for (int k = 0; k<cn; ++k)
			pdx[k] = (w1* (px[-icn + k] + px[icn + k]) + w0*  px[k]) >> rshift;
	}
	for (int k = 0; k<cn; ++k)
		pdx[k] = (w1*(px[-icn + k] + px[k]) + w0* px[k]) >> rshift;
}

template<typename _IValT, typename _DCNT, typename _CNT>
inline void ipfi_gaussian_filter_3x3_col(const int *r0, const int *r1, const int *r2, int width, _IValT *dimg, _DCNT dcn, _CNT cn, const int w0, const int w1, const int rshift)
{
	int j = 0;
	for (int i = 0; i<width; ++i, dimg += dcn)
	{
		for (int k = 0; k<cn; ++k, ++j)
			dimg[k] = _IValT((w1* (r0[j] + r2[j]) + w0*r1[j]) >> rshift);
	}
}

template<typename _IValT, typename _DValT, typename _ICNT, typename _DCNT, typename _CNT>
inline void gaussian_filter_3x3_c(const _IValT *img, int width, int height, int istep, _ICNT icn, _DValT *dimg, int dstep, _DCNT dcn, _CNT cn, const int w0 = 2, const int w1 = 1, const int rshift = 2)
{
	assert((w0 + w1 * 2) == (1 << rshift));
	int *buf = new int[width * 3 * cn];
	int *_vrows[] = { buf + width * 2 * cn, buf, buf + width*cn, buf + width * 2 * cn, buf };
	int **vrows = _vrows + 1;

	for (int i = 0; i<2; ++i)
	{
		ipfi_gaussian_filter_3x3_row(img + istep*i, width, icn, cn, vrows[i], w0, w1, rshift);
	}
	ipfi_gaussian_filter_3x3_col(vrows[0], vrows[0], vrows[1], width, dimg, dcn, cn, w0, w1, rshift);

	const _IValT *piy = img + istep * 2;
	_DValT *pdy = dimg + dstep;
	for (int yi = 2; yi<height; ++yi, piy += istep, pdy += dstep)
	{
		const int ri = (yi - 1) % 3;
		ipfi_gaussian_filter_3x3_row(piy, width, icn, cn, vrows[ri + 1], w0, w1, rshift);

		ipfi_gaussian_filter_3x3_col(vrows[ri - 1], vrows[ri], vrows[ri + 1], width, pdy, dcn, cn, w0, w1, rshift);
	}

	const int ri = (height - 1) % 3;
	ipfi_gaussian_filter_3x3_col(vrows[ri - 1], vrows[ri], vrows[ri], width, pdy, dcn, cn, w0, w1, rshift);

	delete[]buf;
}

template<typename _IImgT, typename _OImgT>
inline void gaussian_filter_3x3(const _IImgT &src, _OImgT &dest, const int w0 = 2, const int w1 = 1, const int rshift = 2)
{
	dest.reset_if(src.width(), src.height());
	gaussian_filter_3x3_c(_dwhsc(src), _dsc(dest), src.cn(), w0, w1, rshift);
}
template<typename _ImgT>
inline void gaussian_filter_3x3(_ImgT &img, const int w0 = 2, const int w1 = 1, const int rshift = 2)
{
	_ImgT dest;
	gaussian_filter_3x3(img, dest, w0, w1, rshift);
	img.swap(dest);
}

_CVX_END

