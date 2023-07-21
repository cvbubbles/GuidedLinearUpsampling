
#include"core.h"
#include"ipf.h"
#include<assert.h>
#include"opencv2/highgui.hpp"

_CVX_BEG


template<int cps>
static void _fiuiSetPixel(uchar* po, const int width, const int height, const int step,
	const int pw, const void* pval)
{
	for (int yi = 0; yi<height; ++yi, po += step)
	{
		uchar *pox = po;
		for (int xi = 0; xi<width; ++xi, pox += pw)
			memcpy(pox, pval, cps);
	}
}
static void _fiuiSetPixel(uchar* po, const int width, const int height, const int step,
	const int pw, const void* pval, int cps)
{
	for (int yi = 0; yi<height; ++yi, po += step)
	{
		uchar *pox = po;
		for (int xi = 0; xi<width; ++xi, pox += pw)
			memcpy(pox, pval, cps);
	}
}
static void _fiuiSetContPixel(uchar* po, const int width, const int height, const int step,
	const int pw, const void* pval)
{
	uchar *pox = po;
	//fill the first line pixel by pixel.
	for (int i = 0; i<width; ++i, pox += pw)
		memcpy(pox, pval, pw);
	//copy first line to fill other lines.
	const int rowSize = width*pw;
	pox = (uchar*)po + step;
	for (int i = 1; i<height; ++i, pox += step)
		memcpy(pox, po, rowSize);
}

void   setPixels(void* pOut, const int width, const int height, const int step, const int pw, const void* pval, const int cps)
{
	typedef void(*_FuncT)(uchar* po, const int width, const int height, const int step,
		const int pw, const void* pval);

	static const _FuncT _pFuncTab[] =
	{
		_fiuiSetPixel<1>, _fiuiSetPixel<2>, _fiuiSetPixel<3>, _fiuiSetPixel<4>
	};

	if (pOut&&pval)
	{
		assert(width*pw <= step);

		if (pw == cps)
			_fiuiSetContPixel((uchar*)pOut, width, height, step, pw, pval);
		else
		{
			if (uint(cps) <= 4)
				(_pFuncTab[cps - 1])((uchar*)pOut, width, height, step, pw, pval);
			else
				_fiuiSetPixel((uchar*)pOut, width, height, step, pw, pval, cps);
		}
	}
}


void  convertBGRChannels(const Mat &src, Mat &dest, int dcn)
{
	bool bFail = false;
	const int scn = src.channels();
	if (scn == dcn)
		src.copyTo(dest);
	else
	{
		int code = 0;
		if (scn > dcn)
			code = dcn == 1 ? (scn == 3 ? COLOR_BGR2GRAY : scn == 4 ? COLOR_BGRA2GRAY : -1) : dcn == 3 && scn == 4 ? COLOR_BGRA2BGR : -1;
		else
			code = scn == 1 ? (dcn == 3 ? COLOR_GRAY2BGR : dcn == 4 ? COLOR_GRAY2BGRA : -1) : scn == 3 && dcn == 4 ? COLOR_BGR2BGRA : -1;

		if (code != -1)
			cvtColor(src, dest, code);
		else
			CV_Error(0, "invalid conversion");
	}
}

_CVX_API Mat  convertType(const Mat &src, int dtype, double alpha, double beta)
{
	Mat r = src;
	if (src.type() != dtype)
	{
		int dcn = CV_MAT_CN(dtype), ddepth = CV_MAT_DEPTH(dtype);

		if (src.channels() != dcn)
		{
			Mat t; convertBGRChannels(src, t, dcn); r = t;
		}
		if (src.depth() != ddepth)
		{
			Mat t; r.convertTo(t, dtype, alpha, beta); r = t;
		}
	}
	return r;
}

_CVX_API Mat  getChannel(const Mat &src, int ic)
{
	Mat dest(src.size(), CV_MAKETYPE(src.depth(), 1));

	memcpy_c(src.data + ic*src.elemSize1(), src.cols, src.rows, src.step, src.elemSize(), DS(dest), dest.elemSize(), src.elemSize1());

	return dest;
}

_CVX_API Mat  mergeChannels(const Mat vsrc[], int nsrc, int dtype)
{
	Mat dest;
	if (nsrc > 0)
	{
		if (dtype <= 0)
		{
			int ddepth = vsrc[0].depth();
			int dcn = 0;
			for (int i = 0; i < nsrc; ++i)
			{
				CV_Assert(vsrc[i].depth() == ddepth);
				dcn += vsrc[i].channels();
			}
			dtype = CV_MAKETYPE(ddepth, dcn);
		}

		dest.create(vsrc[0].size(), dtype);

		int offset = 0;
		for (int i = 0; i < nsrc; ++i)
		{
			memcpy_c(vsrc[i].data, dest.cols, dest.rows, vsrc[i].step, vsrc[i].elemSize(), dest.data + offset, dest.step, dest.elemSize(), vsrc[i].elemSize());
			offset += vsrc[i].elemSize();
		}

		CV_DbgAssert(dest.elemSize() >= (size_t)offset);
	}
	return dest;
}

#ifndef CVX_NO_GUI

void drawCross(Mat &img, const Point &pt, int radius, const Scalar &clrRGB, int thick)
{
	line(img, pt - Point(radius, 0), pt + Point(radius, 0), clrRGB, thick, 4);
	line(img, pt - Point(0, radius), pt + Point(0, radius), clrRGB, thick, 4);
}

#endif

#ifndef CVX_NO_CODEC

void writeAsPng(const std::string &file, const Mat &img)
{
	CV_Assert(img.elemSize() == 4);

	Mat pngImg(img.size(), CV_8UC4);
	memcpy_2d(img.data, 4 * img.cols, img.rows, img.step, pngImg.data, pngImg.step);
	if (!imwrite(file, pngImg))
		CV_Error(0, file.c_str());
}

Mat readFromPng(const std::string &file, int type)
{
	Mat pngImg = imread(file, IMREAD_UNCHANGED);

	CV_Assert(CV_ELEM_SIZE(type) == 4);
	CV_Assert(pngImg.elemSize() == 4);

	Mat dimg(pngImg.size(), type);
	memcpy_2d(pngImg.data, 4 * pngImg.cols, pngImg.rows, pngImg.step, dimg.data, dimg.step);

	return dimg;
}
#endif

_CVX_API Mat  imscale(const Mat &img, double scale, int interp)
{
	Size dsize(int(img.cols*scale + 0.5), int(img.rows*scale + 0.5));
	return imscale(img,dsize,interp);
}

_CVX_API Mat  imscale(const Mat &img, Size dsize, int interp)
{
	if (dsize == img.size())
		return img.clone();

	Mat dimg;
	resize(img, dimg, dsize, 0, 0, interp);
	return dimg;
}

_CVX_API std::shared_ptr<Point> dtGetLabelMap(const Mat1b &mask, int *mapSize)
{
	std::shared_ptr<Point> _map(new Point[mask.rows*mask.cols]);
	Point *pmap = _map.get();
	pmap[0] = Point(0, 0);

	int nz = 1; ////label start with 1
	for_each_1c(DWHN1(mask), [&nz, pmap](uchar m, int x, int y) {
		if (m == 0)
		{
			pmap[nz] = Point(x, y);
			++nz;
		}
	});

	Point *dst = new Point[nz];
	memcpy(dst, pmap, sizeof(Point)*nz);

	if (mapSize)
		*mapSize = nz;

	return std::shared_ptr<Point>(dst);
}

Mat1b  dtGetSeedMask(Size dsize, const std::vector<std::vector<cv::Point>> &poly)
{
	Mat1b dtMask(dsize);
	setMem(dtMask, 0xFF);

	//std::vector<std::vector<Point>> poly;
	//cvtVector(data.segSilhouette, poly);
	cv::polylines(dtMask, poly, true, Scalar(0), 1, 0);
	return dtMask;
}



_CVX_END



