
#pragma once

#include"core.h"
#include<random>

_CVX_BEG

template<typename _ValT>
inline void _normMinMax(const _ValT *img, int width, int height, int stride, int cn, uchar *dimg, int dstep)
{
	_ValT vmin = *img, vmax = *img;
	for_each_1(img, width*cn, height, stride, ccn1(), [&vmin, &vmax](_ValT v) {
		if (v < vmin)
			vmin = v;
		else if (v>vmax)
			vmax = v;
	});
	for_each_2(img, width*cn, height, stride, ccn1(), dimg, dstep, [vmin, vmax](_ValT v, uchar &m) {
		m = uchar(255 * (v - vmin) / (vmax - vmin));
	});
}

inline Mat vis(const Mat &img)
{
	Mat dimg;
	if (img.depth() != CV_8U)
	{
		//cv::normalize(img, dimg, 255.0, 0, NORM_MINMAX, CV_8U);
		img.convertTo(dimg, CV_8U, 255);
	}
	else
		dimg = img;

	return dimg;
}
inline Mat visf(const Mat &img, double maxVal = -1.0)
{
	Mat dimg;
	if (img.depth() != CV_8U)
	{
		if (maxVal <= 0)
		{
			Mat imgf = img;
			if (img.depth() != CV_32F)
				img.convertTo(imgf, CV_32F);
			for_each_1(imgf.ptr<float>(), imgf.cols*imgf.channels(), imgf.rows, stepC(imgf), ccn1(), [&maxVal](float v) {
				if (v > maxVal)
					maxVal = v;
			});
		}

		img.convertTo(dimg, CV_8U, 255/maxVal);
	}
	else
		dimg = img;

	return dimg;
}

inline void genColorTable(Vec3b *vcolor, int size, bool shuffle=true)
{
	double d = 255.0 / pow(double(size), 1.0 / 3);

	int i = 0;
	for(double b=255.0; b>=0.0; b-=d)
		for(double g=255.0; g>=0.0; g-=d)
			for (double r = 255.0; r >= 0.0; r -= d)
			{
				vcolor[i++] = Vec3b(uchar(b), uchar(g), uchar(r));
				if (i >= size)
					goto _BREAK;
			}
_BREAK:
	if (shuffle)
		//std::random_shuffle(vcolor, vcolor + size);
		std::shuffle(vcolor, vcolor + size, std::default_random_engine(0));
}

template<typename _IndexPtrT>
inline void _vis_index(cv::Mat &cimg, _IndexPtrT cc, int width, int height, int cstride, int color_table_size, const uchar *vcolor = NULL)
{
	uchar *_vcolor = NULL;

	if (!vcolor)
	{
		static_assert(sizeof(Vec3b) == sizeof(uchar) * 3, "");
		_vcolor = new uchar[color_table_size * 3];
		genColorTable((Vec3b*)_vcolor, color_table_size);
		vcolor = _vcolor;
	}

	cimg.create(height, width, CV_8UC3);
	for (int yi = 0; yi < height; ++yi, cc += cstride)
	{
		for (int xi = 0; xi < width; ++xi)
		{
			const int idx = cc[xi];
			if (idx < 0)
				memset(cimg.ptr(yi, xi), 0, 3);
			else
				memcpy(cimg.ptr(yi, xi), vcolor + cc[xi] * 3, 3);
		}
	}

	delete[]_vcolor;
}

template<typename _IndexT>
inline Mat3b visIndex(const Mat_<_IndexT> &cc, const uchar *vcolor = NULL)
{
	Mat3b img;
	int colorTableSize = maxElem(cc) + 1;
	_vis_index(img, DWHN(cc), colorTableSize, vcolor);
	return img;
}


template<typename _IndexPtrT>
inline void _mask_region_boundary(_IndexPtrT cc, int width, int height, int cstride, uchar *img, int istep, const uchar bcolor[])
{
	for (int yi = 0; yi<height - 1; ++yi, cc += cstride, img += istep)
	{
		uchar *px = img;
		for (int xi = 0; xi<width - 1; ++xi, px += 3)
		{
#if 1
			bool b = false;
			if (cc[xi] != cc[xi + 1])
				memcpy(px + 3, bcolor, 3), b = true;
			if (cc[xi] != cc[xi + cstride])
				memcpy(px + istep, bcolor, 3), b = true;
			if (cc[xi] != cc[xi + cstride + 1])
				memcpy(px + istep + 3, bcolor, 3), b = true;
			if (cc[xi] != cc[xi + cstride - 1])
				memcpy(px + istep - 3, bcolor, 3), b = true;
			if (b)
				memcpy(px, bcolor, 3);
#else
			if (cc[xi] != cc[xi + 1] || cc[xi] != cc[xi + cstride])
				memcpy(px, bcolor, 3);
#endif
		}
	}
}

template<typename _IndexT>
inline Mat3b visRegionBoundary(const Mat_<_IndexT> &cc, const Mat3b &img, Scalar mcolor = Scalar(0, 255, 255) )
{
	uchar bcolor[] = { (uchar)mcolor[0], (uchar)mcolor[1], (uchar)mcolor[2] };

	Mat3b dimg(img.clone());
	_mask_region_boundary(DWHN(cc), DN(dimg), bcolor);
	return dimg;
}

template<typename _IndexPtrT>
inline Mat3b _vis_region_mean(_IndexPtrT cc, int width, int height, int cstride, int ncc, const uchar *img, int istep)
{
	typedef cv::Vec4i Point4i;
	std::vector<Point4i>  vmean(ncc, Point4i(0, 0, 0, 0));
	for (int yi = 0; yi<height; ++yi, cc += cstride, img += istep)
	{
		const uchar *px = img;
		for (int xi = 0; xi<width; ++xi, px += 3)
		{
			Point4i &pt(vmean[cc[xi]]);
			for (int j = 0; j<3; ++j)
				pt[j] += px[j];
			pt[3]++;
		}
	}
	for (int i = 0; i<ncc; ++i)
	{
		if (vmean[i][3]>0)
		{
			for (int j = 0; j<3; ++j)
				vmean[i][j] /= vmean[i][3];
		}
	}
	cc -= cstride*height;
	Mat dimg(height, width, CV_8UC3);
	for (int yi = 0; yi<height; ++yi, cc += cstride)
	{
		for (int xi = 0; xi<width; ++xi)
		{
			const Point4i &pt(vmean[cc[xi]]);
			uchar *px = dimg.ptr(yi, xi);
			for (int j = 0; j<3; ++j)
				px[j] = (uchar)pt[j];
		}
	}
//	imshow(isw, dimg);
	return dimg;
}


template<typename _IndexT>
inline Mat3b visRegionMean(const Mat_<_IndexT> &cc, const Mat3b &img)
{
	int ncc = maxElem(cc) + 1;
	return _vis_region_mean(DWHN(cc), ncc, DN(img));
}


inline Mat3b visMatch(const std::vector<Point> &L, const std::vector<Point> &R, const std::vector<int> &match, const Mat &imgL, const Mat &imgR, int step = 1)
{
	const int width = imgL.cols, height = imgL.rows;
	Mat3b dest(height,width*2);

	Mat roi = dest(cv::Rect(0, 0, width, height));
	convertBGRChannels(imgL, roi, 3);

	roi = dest(cv::Rect(width, 0, width, height));
	convertBGRChannels(imgR, roi, 3);

	for (size_t i = 0; i<L.size(); i += step)
	{
		if (match[i] >= 0)
		{
			Point dp(R[match[i]]);
			dp.x += width;
			Scalar clr((uchar)rand(), (uchar)rand(), (uchar)rand());
			line(dest, L[i], dp, clr, 1);
			circle(dest, L[i], 4, clr, -1);
			circle(dest, dp, 4, clr, -1);
		}
	}
	return dest;
}
inline Mat3b visMatch(const std::vector<Point> &L, const std::vector<Point> &R, const Mat &imgL, const Mat &imgR, int step = 1, double scale=1.0)
{
	std::vector<int> match(__min(L.size(), R.size()));
	for (size_t i = 0; i<match.size(); ++i)
		match[i] = i;

	Mat3b dimg;
	if(scale==1.0)
		dimg= visMatch(L, R, match, imgL, imgR, step);
	else
	{
		std::vector<Point> Lx(L.size()), Rx(R.size());
		for (size_t i = 0; i < L.size(); ++i)
		{
			Lx[i] = L[i] * scale;
			Rx[i] = R[i] * scale;
		}
		dimg= visMatch(Lx, Rx, imscale(imgL, scale, cv::INTER_LINEAR), imscale(imgR, scale, cv::INTER_LINEAR), step);
	}

	return dimg;
}

inline Mat visFeatures(const Mat &img, const std::vector<Point> &fea, const Scalar &color = Scalar(255, 255, 0))
{
	Mat dimg(img.clone());
	for (size_t i = 0; i < fea.size(); ++i)
	{
		drawCross(dimg, fea[i], 2, color, 1);
	}
	return dimg;
}

//vis specified contour (or contourIDX=-1 for all) in a contour list
inline Mat visContours(const Mat &img, const std::vector<std::vector<Point> > &contours, int contourIDX, const Scalar& color = Scalar(0, 255, 255), int thickness = 2, int lineType = 8)
{
	Mat dimg(img.clone());
	drawContours(dimg, contours, contourIDX, color, thickness, lineType);
	return dimg;
}
//vis one contour
inline Mat visContours(const Mat &img, const std::vector<Point> &contours, const Scalar& color = Scalar(0, 255, 255), int thickness = 2, int lineType = 8)
{
	Mat dimg(img.clone());
	std::vector<std::vector<Point> > vcont(1);
	std::vector<Point> &c(const_cast< std::vector<Point>&>(contours));
	vcont[0].swap(c);
	drawContours(dimg, vcont, 0, color, thickness, lineType);
	vcont[0].swap(c);

	return dimg;
}
//vis contours of masked regions
inline Mat drawContours(const Mat &img, const Mat &mask, const Scalar& color = Scalar(0, 255, 255), int thickness = 2, int lineType = 8)
{
	Mat _mask(mask.size(), CV_8UC1);
	cv::threshold(DWHN(mask), DN(_mask), 127, 0, 255);
	std::vector<std::vector<Point> > cont;
	cv::findContours(_mask, cont, CV_RETR_LIST, CV_CHAIN_APPROX_NONE);

	Mat dimg(img.clone());
	drawContours(dimg, cont, -1, color, thickness, lineType);
	return dimg;
}

template<typename _PtListT>
inline std::vector<Point> cvtPoint(const _PtListT &pts)
{
	std::vector<Point> vpt;
	vpt.reserve(pts.size());
	for (typename _PtListT::const_iterator itr(pts.begin()); itr != pts.end(); ++itr)
	{
		vpt.push_back(Point(int(itr->x), int(itr->y)));
	}
	return vpt;
}


_CVX_END
