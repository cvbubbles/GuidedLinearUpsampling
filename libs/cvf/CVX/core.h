
#pragma once

#pragma warning(push)
#pragma warning(disable: 4819)

#include"def.h"
#include"opencv2/core/core.hpp"
#include"opencv2/imgproc/imgproc_c.h"
#include"opencv2/imgproc/imgproc.hpp"

#include"ipf.h"
#include"cc.h"

#include"BFC/ctc.h"

#pragma warning(pop)

_CVX_BEG

//require Mat or Mat_
#define DWHS(img) (img).data,(img).cols,(img).rows,(img).step
#define DWHSC(img) DWHS(img),(img).channels()

#define DS(img) (img).data, (img).step
#define DSC(img) DS(img),(img).channels()

template<typename _Tp>
inline _Tp* dataC(const Mat_<_Tp> &img)
{
	return (_Tp*)img.data;
}
template<typename _Tp, int cn>
inline _Tp* dataC(const Mat_<Vec<_Tp,cn> > &img)
{
	return (_Tp*)img.data;
}
inline uchar* dataC(const Mat &img)
{
	return (uchar*)img.data;
}


template<typename _Tp>
inline int stepC(const Mat_<_Tp> &img)
{
	CV_DbgAssert(img.step%sizeof(_Tp)==0);
	return img.step/sizeof(_Tp);
}
template<typename _Tp, int cn>
inline int stepC(const Mat_<Vec<_Tp,cn> > &img)
{
	CV_DbgAssert(img.step%sizeof(_Tp) == 0);
	return img.step / sizeof(_Tp);
}
inline int stepC(const Mat &img)
{
	return (int)img.step1();
}

template<typename _ImgT>
inline auto dataC(_ImgT &img, int x, int y) ->decltype(dataC(img))
{
	return dataC(img) + y*stepC(img) + x*img.channels();
}

template<typename _Tp>
inline ccn<1,true> gccn(const Mat_<_Tp> &img)
{
	return ccn<1,true>();
}
template<typename _Tp, int cn>
inline ccn<cn, true> gccn(const Mat_<Vec<_Tp, cn> > &img)
{
	return ccn<cn,true>();
}

#define DWHN(img) cv::dataC(img),(img).cols,(img).rows,cv::stepC(img)
#define DWHNC(img) DWHN(img),(img).channels()
#define DWHN0(img) DWHN(img),cv::gccn(img)
#define DWHN1(img) DWHN(img),cv::ccn<1,false>()
#define DWHN2(img) DWHN(img),cv::ccn2()
#define DWHN3(img) DWHN(img),cv::ccn3()
#define DWHN4(img) DWHN(img),cv::ccn4()

#define DN(img) cv::dataC(img),cv::stepC(img)
#define DNC(img) DN(img),(img).channels()
#define DN0(img) DN(img),cv::gccn(img)
#define DN1(img) DN(img),cv::ccn<1,false>()
#define DN2(img) DN(img),cv::ccn2()
#define DN3(img) DN(img),cv::ccn3()
#define DN4(img) DN(img),cv::ccn4()

#define DWHNr(img, roi) cv::dataC(img,(roi).x,(roi).y),(roi).width,(roi).height,cv::stepC(img)
#define DWHNCr(img,roi) DWHNr(img,roi),(img).channels()
#define DWHN0r(img,roi) DWHNr(img,roi),cv::gccn(img)
#define DWHN1r(img,roi) DWHNr(img,roi),cv::ccn<1,false>()
#define DWHN2r(img,roi) DWHNr(img,roi),cv::ccn2()
#define DWHN3r(img,roi) DWHNr(img,roi),cv::ccn3()
#define DWHN4r(img,roi) DWHNr(img,roi),cv::ccn4()

#define DNr(img,roi) cv::dataC(img,(roi).x,(roi).y),cv::stepC(img)
#define DNCr(img,roi) DNr(img,roi),(img).channels()
#define DN0r(img,roi) DNr(img,roi),cv::gccn(img)
#define DN1r(img,roi) DNr(img,roi),cv::ccn<1,false>()
#define DN2r(img,roi) DNr(img,roi),cv::ccn2()
#define DN3r(img,roi) DNr(img,roi),cv::ccn3()
#define DN4r(img,roi) DNr(img,roi),cv::ccn4()

template<typename _MatT>
typename _MatT::value_type _get_Mat_value_type(const _MatT &obj);
template<typename _MatT>
typename pixel_traits<typename _MatT::value_type>::vec_value_type _get_Mat_value_type_v(const _MatT &obj);

template<typename _MatT>
typename _MatT::channel_type _get_Mat_channel_type(const _MatT &obj);

#define pixel_t(Mat_obj) decltype(cv::_get_Mat_value_type(Mat_obj))
#define pixel_tv(Mat_obj) decltype(cv::_get_Mat_value_type_v(Mat_obj))
#define channel_t(Mat_obj) decltype(cv::_get_Mat_channel_type(Mat_obj))


typedef uchar		   P8UC1;
typedef Vec<uchar, 2>  P8UC2;
typedef Vec<uchar, 3>  P8UC3;
typedef Vec<uchar, 4>  P8UC4;
typedef short       	P16SC1;
typedef Vec<short, 2>	P16SC2;
typedef Vec<short, 3>	P16SC3;
typedef Vec<short, 4>	P16SC4;
typedef ushort			 P16UC1;
typedef Vec<ushort, 2> P16UC2;
typedef Vec<ushort, 3> P16UC3;
typedef Vec<ushort, 4> P16UC4;
typedef int		       P32SC1;
typedef Vec<int, 2>       P32SC2;
typedef Vec<int, 3>       P32SC3;
typedef Vec<int, 4>       P32SC4;
typedef float		     P32FC1;
typedef Vec<float,2>     P32FC2;
typedef Vec<float, 3>    P32FC3;
typedef Vec<float, 4>    P32FC4;
typedef Vec<float, 6>    P32FC6;
typedef double		     P64FC1;
typedef Vec<double, 2>     P64FC2;
typedef Vec<double, 3>    P64FC3;
typedef Vec<double, 4>    P64FC4;
typedef Vec<double, 6>    P64FC6;


typedef Mat_<P8UC1>  Im8UC1;
typedef Mat_<P8UC2>  Im8UC2;
typedef Mat_<P8UC3>  Im8UC3;
typedef Mat_<P8UC4>  Im8UC4;
typedef Mat_<P16SC1>  Im16SC1;
typedef Mat_<P16SC2>  Im16SC2;
typedef Mat_<P16SC3>  Im16SC3;
typedef Mat_<P16SC4>  Im16SC4;
typedef Mat_<P16UC1>  Im16UC1;
typedef Mat_<P16UC2>  Im16UC2;
typedef Mat_<P16UC3>  Im16UC3;
typedef Mat_<P16UC4>  Im16UC4;
typedef Mat_<P32SC1>  Im32SC1;
typedef Mat_<P32SC2>  Im32SC2;
typedef Mat_<P32SC3>  Im32SC3;
typedef Mat_<P32SC4>  Im32SC4;
typedef Mat_<P32FC1>  Im32FC1;
typedef Mat_<P32FC2>  Im32FC2;
typedef Mat_<P32FC3>  Im32FC3;
typedef Mat_<P32FC4>  Im32FC4;
typedef Mat_<P32FC6>  Im32FC6;
typedef Mat_<P64FC1>  Im64FC1;
typedef Mat_<P64FC2>  Im64FC2;
typedef Mat_<P64FC3>  Im64FC3;
typedef Mat_<P64FC4>  Im64FC4;
typedef Mat_<P64FC6>  Im64FC6;

template<typename _PixT>
inline Mat_<_PixT>& image_cast(const Mat &src, const Mat_<_PixT> *)
{
	return *(Mat_<_PixT>*)(&src);
}

template<typename _DestT, typename _SrcT>
inline _DestT& imcast(_SrcT &src)
{
	return image_cast(src, (_DestT*)NULL);
}


template<typename _ValT>
inline bool rectEmpty(const Rect_<_ValT> &rect)
{
	return rect.width == 0 || rect.height == 0;
}

template<typename _ValT>
inline Rect_<_ValT> rectMerge(const Rect_<_ValT> &rect0, const Rect_<_ValT> &rect1)
{
	if (rectEmpty(rect0))
		return rect1;
	else
		if (rectEmpty(rect1))
			return rect0;

	_ValT l(__min(rect0.x, rect1.x)), t(__min(rect0.y, rect1.y)), r(__max(rect0.x+rect0.width, rect1.x+rect1.width)), b(__max(rect0.y+rect0.height, rect1.y+rect1.height));

	return Rect_<_ValT>(l, t, r - l, b - t);
}
template<typename _ValT>
inline void rectAppend(Rect_<_ValT> &r, _ValT dLeft, _ValT dTop, _ValT dRight, _ValT dBottom)
{
	_ValT right = r.x + r.width + dRight, bottom = r.y + r.height + dBottom;

	r.x -= dLeft; r.y -= dTop;

	r.width = right - r.x;
	if (r.width<0)
		r.width = 0;

	r.height = bottom - r.y;
	if (r.height<0)
		r.height = 0;
}
//Get the overlapped part of two rectangles @rect0 and @rect1.
template<typename _ValT>
inline Rect_<_ValT> rectOverlapped(const Rect_<_ValT> &rect0, const Rect_<_ValT> &rect1)
{
	Rect_<_ValT> _empty(0, 0, 0, 0);
	_ValT left = rect1.x, top = rect1.y, right = rect1.x+rect1.width, bottom = rect1.y+rect1.height;

	_ValT y = rect0.y;
	if (y>top&&y<bottom)
		top = y;
	else
		if (y >= bottom)
			return _empty;

	y = rect0.y+rect0.height;
	if (y>top&&y<bottom)
		bottom = y;
	else
		if (y <= top)
			return _empty;

	int x = rect0.x;
	if (x>left&&x<right)
		left = x;
	else
		if (x >= right)
			return _empty;
	x = rect0.x+rect0.width;
	if (x>left&&x<right)
		right = x;
	else
		if (x <= left)
			return _empty;

	return Rect_<_ValT>(left, top, right - left, bottom - top);
}


inline Rect rectDilate(Rect r, int D, const Size& imgSize)
{
	rectAppend(r, D, D, D, D);
	return rectOverlapped(r, Rect(0, 0, imgSize.width, imgSize.height));
}

//==================================================

_CVX_API void   setPixels(void* pOut, const int width, const int height, const int step, const int pw, const void* pval, const int cps);

inline void setPixels(Mat m, const void *pval, const int psize=0)
{
	setPixels(DWHS(m), (int)m.elemSize(), pval, psize<=0? (int)m.elemSize():psize);
}

inline void setMem(Mat m, char val)
{
	memset_2d(m.data, (int)m.elemSize()*m.cols, m.rows, (int)m.step, val);
}
template<typename _ValT>
inline void setMem(_ValT *data, size_t count, char val)
{
	memset(data, val, sizeof(_ValT)*count);
}

inline void copyMem(const Mat &src, Mat dest)
{
	assert(src.elemSize()*src.cols == dest.elemSize()*dest.cols && src.rows == dest.rows);

	memcpy_2d(src.data, src.cols*src.elemSize(), src.rows, src.step, dest.data, dest.step);
}

template<typename _PixT>
inline _PixT maxElem(const Mat_<_PixT> &img)
{
	_PixT vmax;
	max_elem(DWHN0(img), px2c(&vmax), pixel_traits<_PixT>::ccn());
	return vmax;
}

template<typename _PixT>
inline void maxFilter(const Mat_<_PixT> &src, Mat_<_PixT> &dest, int ksz)
{
	dest.create(src.size());
	max_filter(DWHN(src), pixel_traits<_PixT>::ccn(), DN(dest), ksz, 1, NULL);
}

template<typename _PixT>
inline _PixT minElem(const Mat_<_PixT> &img)
{
	_PixT vmin;
	min_elem(DWHN0(img), px2c(&vmin), pixel_traits<_PixT>::ccn());
	return vmin;
}

template<typename _PixT>
inline void minFilter(const Mat_<_PixT> &src, Mat_<_PixT> &dest, int ksz)
{
	dest.create(src.size());
	min_filter(DWHN(src), pixel_traits<_PixT>::ccn(), DN(dest), ksz, 1, NULL);
}

template<typename _PixT>
inline void meanFilter(const Mat_<_PixT> &src, Mat_<_PixT> &dest, int kszx, int kszy=-1)
{
	dest.create(src.size());
	mean_filter_x(DWHN(src), gccn(src), DN(dest), kszx, kszy > 0 ? kszy : kszx);
}

template<typename _PixT>
inline Mat_<_PixT> threshold(const Mat_<_PixT> &img, double T, double v0, double v1)
{
	Mat_<_PixT> dst(img.size());
	threshold(DWHN(img), DN(dst), (_PixT)T, (_PixT)v0, (_PixT)v1);
	return dst;
}

_CVX_API void  convertBGRChannels(const Mat &src, Mat &dest, int dcn);

inline Mat convertBGRChannels(const Mat &src, int dcn)
{
	Mat dest;
	convertBGRChannels(src, dest, dcn);
	return dest;
}

_CVX_API Mat  convertType(const Mat &src, int dtype, double alpha = 1.0, double beta = 0);

_CVX_API Mat getChannel(const Mat &src, int ic);

//for compatibility only
inline void  getChannel(const Mat &src, Mat &dest, int ic)
{
	dest = getChannel(src, ic);
}

_CVX_API Mat  mergeChannels(const Mat vsrc[], int nsrc, int dtype=0);

inline Mat mergeChannels(const Mat &im1, const Mat &im2, int dtype = 0)
{
	Mat vsrc[] = { im1,im2 };
	return mergeChannels(vsrc, 2, dtype);
}
inline Mat mergeChannels(const Mat &im1, const Mat &im2, const Mat &im3, int dtype = 0)
{
	Mat vsrc[] = { im1,im2 , im3};
	return mergeChannels(vsrc, 3, dtype);
}

_CVX_API void drawCross(Mat &img, const Point &pt, int radius, const Scalar &clrRGB, int thick=1);

_CVX_API void writeAsPng(const std::string &file, const Mat &img);

_CVX_API Mat readFromPng(const std::string &file, int type);

_CVX_API Mat  imscale(const Mat &img, double scale, int interp = cv::INTER_NEAREST);

_CVX_API Mat  imscale(const Mat &img, Size dsize, int interp = cv::INTER_NEAREST);

//aid functions of cv::distanceTransform
//get a map from pixel label to image coordinates. mask[x]==0 for seed pixel x.
//@mapSize : size of the returned map (==number of seed pixels plus one)
_CVX_API std::shared_ptr<Point> dtGetLabelMap(const Mat1b &mask, int *mapSize=nullptr);

//get seed mask by render contours (mask[x]==0 for seeds), which are usually obtained from cv::findContours
_CVX_API Mat1b  dtGetSeedMask(Size dsize, const std::vector<std::vector<cv::Point>> &poly);

//connected component for 4-n and 8-n
template<typename _PixT>
inline Mat1i ccSegN4(const Mat_<_PixT> &img)
{
	Mat1i cc(img.size());
	cc_seg::n4(&img(0, 0), img.cols, img.rows, img.stepT(), 1, DN(cc), [](const _PixT *a, const _PixT *b){
		return *a == *b;
	});
	return cc;
}

template<typename _PixT>
inline Mat1i ccSegN4(const Mat_<_PixT> &img, int dT)
{
	Mat1i cc(img.size());
	cc_seg::n4(&img(0, 0), img.cols, img.rows, img.stepT(), 1, DN(cc), [dT](const _PixT *a, const _PixT *b){
		return pxdiff(*a,*b)<dT;
	});
	return cc;
}

template<typename _PixT>
inline Mat1i ccSegN8(const Mat_<_PixT> &img)
{
	Mat1i cc(img.size());
	cc_seg::n8(&img(0, 0), img.cols, img.rows, img.stepT(), 1, DN(cc), [](const _PixT *a, const _PixT *b){
		return *a == *b;
	});
	return cc;
}

template<typename _PixT>
inline Mat1i ccSegN8(const Mat_<_PixT> &img, int dT)
{
	Mat1i cc(img.size());
	cc_seg::n8(&img(0, 0), img.cols, img.rows, img.stepT(), 1, DN(cc), [dT](const _PixT *a, const _PixT *b){
		return pxdiff(*a, *b)<dT;
	});
	return cc;
}

//=======================================
template<typename _ResultValT, typename _ValT, int cn>
inline bool  resampleNN(const Mat_<Vec<_ValT, cn> > &img, Vec<_ResultValT, cn> &val, double x, double y)
{
	bool ok = false;
	int	ix = int(x + 0.5), iy = int(y + 0.5);
	if (unsigned(ix) < unsigned(img.cols) && unsigned(iy) < unsigned(img.rows))
	{
		const Vec<_ValT, cn> &p = *img.template ptr<Vec<_ValT, cn>>(iy, ix);
		for (int i = 0; i < cn; ++i)
			val[i] = ff::NumCast<_ResultValT>(p[i]);
		ok = true;
	}
	
	return ok;
}
template<typename _ResultValT, typename _ValT>
inline bool  resampleNN(const Mat_<_ValT> &img, _ResultValT &val, double x, double y)
{
	return resampleNN<_ResultValT>(reinterpret_cast<const Mat_<Vec<_ValT, 1> >&>(img), reinterpret_cast<Vec<_ResultValT, 1>&>(val), x, y);
}

template<typename _ResultValT, typename _ValT, int cn>
inline bool  resampleBL(const Mat_<Vec<_ValT, cn> > &img, Vec<_ResultValT,cn> &val, double x, double y)
{
	bool ok = true;
	int ix = (int)x, iy = (int)y;
	if (unsigned(ix)<unsigned(img.cols - 1) && unsigned(iy)<unsigned(img.rows - 1))
	{
		double rx = x - ix, ry = y - iy;
		const Vec<_ValT, cn> *p = img.template ptr<Vec<_ValT, cn> >(iy, ix);
		const Vec<_ValT, cn> *q = (Vec<_ValT, cn>*)(reinterpret_cast<const char*>(p) + img.step);

		
		for (int i = 0; i < cn; ++i)
		{
			double h0 = p[0][i] + (p[1][i] - p[0][i])*rx;
			double h1 = q[0][i] + (q[1][i] - q[0][i])*ry;
			val[i] = ff::NumCast<_ResultValT>( h0 + (h1 - h0)*ry );
		}
	}
	else
	{
		ix = int(x + 0.5), iy = int(y + 0.5);
		if (unsigned(ix) < unsigned(img.cols) && unsigned(iy) < unsigned(img.rows))
		{
			const Vec<_ValT, cn> &p = *img.template ptr<Vec<_ValT, cn> >(iy, ix);
			for (int i = 0; i < cn; ++i)
				val[i] = ff::NumCast<_ResultValT>(p[i]);
		}
		else
			ok = false;
	}
	return ok;
}
template<typename _ResultValT, typename _ValT>
inline bool  resampleBL(const Mat_<_ValT> &img, _ResultValT &val, double x, double y)
{
	return resampleBL<_ResultValT>(reinterpret_cast<const Mat_<Vec<_ValT, 1> >&>(img), reinterpret_cast<Vec<_ResultValT,1>&>(val), x, y);
}

//=======================================
//alpha blend with constant background color (B)
template<typename _ValT, int cn, typename _AlphaValT>
inline Mat_<Vec<_ValT, cn> > alphaBlend(const Mat_<Vec<_ValT, cn> > &F, const Mat_<_AlphaValT> &alpha, double alphaScale, const _ValT B[])
{
	Mat_<Vec<_ValT, cn> > C(F.size());
	for_each_3(DWHNC(F), DN1(alpha), DNC(C), [B,alphaScale](const _ValT *f, _AlphaValT a, _ValT *c){
		double w = a * alphaScale;
		for (int i = 0; i < cn; ++i)
			c[i] = _ValT(B[i] + w*(f[i] - B[i]));

	});
	return C;
}
//alpha blend with background image (B)
template<typename _ValT, int cn, typename _AlphaValT>
inline Mat_<Vec<_ValT, cn> > alphaBlend(const Mat_<Vec<_ValT, cn> > &F, const Mat_<_AlphaValT> &alpha, double alphaScale, const Mat_<Vec<_ValT, cn> > &B)
{
	Mat_<Vec<_ValT, cn> > C(F.size());
	for_each_4(DWHNC(F), DN1(alpha), DNC(B), DNC(C),  [alphaScale](const _ValT *f, _AlphaValT a, const _ValT *b, _ValT *c){
		double w = a * alphaScale;
		for (int i = 0; i < cn; ++i)
			c[i] = _ValT(b[i] + w*(f[i] - b[i]));

	});
	return C;
}

//============================================

template<typename _T1, typename _T2, typename _HValT>
inline void transH(_T1 x, _T1 y, _T2 &tx, _T2 &ty, const _HValT *H)
{
	double s = 1.0 /(H[6] * double(x) + H[7] * double(y) + H[8]);
	tx = _T2((H[0] * x + H[1] * y + H[2]) *s);
	ty = _T2((H[3] * x + H[4] * y + H[5]) *s);
};
template<typename _T1, typename _HValT>
inline Point2f transH(_T1 x, _T1 y, const _HValT *H)
{
	double s = 1.0 / (H[6] * double(x) + H[7] * double(y) + H[8]);
	return Point2f(float((H[0] * x + H[1] * y + H[2]) *s), float((H[3] * x + H[4] * y + H[5]) *s));
};
template<typename _T1, typename _HValT>
inline Point2f transH(const Point_<_T1> &pt, const _HValT *H)
{
	return transH(pt.x, pt.y, H);
};
template<typename _DPointT, typename _T1, typename _HValT>
inline std::vector<_DPointT> transH(const std::vector<Point_<_T1>> &pt, const _HValT *H)
{
	std::vector<_DPointT> dp(pt.size());
	for (size_t i = 0; i < pt.size(); ++i)
		dp[i] = _DPointT(transH(pt[i], H));
	return dp;
};


template<typename _T1, typename _T2, typename _AValT>
inline void transA(_T1 x, _T1 y, _T2 &tx, _T2 &ty, const _AValT *A)
{
	tx = _T2((A[0] * x + A[1] * y + A[2]));
	ty = _T2((A[3] * x + A[4] * y + A[5]));
};
template<typename _T1, typename _AValT>
inline Point2f transA(_T1 x, _T1 y, const _AValT *A)
{
	return Point2f(float(A[0] * x + A[1] * y + A[2]), float(A[3] * x + A[4] * y + A[5]));
};
template<typename _T1, typename _AValT>
inline Point2f transA(const Point_<_T1> &pt, const _AValT *A)
{
	return transA(pt.x, pt.y, A);
};
template<typename _DPointT, typename _T1, typename _HValT>
inline std::vector<_DPointT> transA(const std::vector<Point_<_T1>> &pt, const _HValT *H)
{
	std::vector<_DPointT> dp(pt.size());
	for (size_t i = 0; i < pt.size(); ++i)
		dp[i] = _DPointT(transA(pt[i], H));
	return dp;
};

inline bool _isIn(int x, int y, const Size &sz)
{
	return uint(x) < uint(sz.width) && uint(y) < uint(sz.height);
}
template<typename _ValT>
inline bool isIn(_ValT x, _ValT y, const Size &size)
{
	return _isIn(int(x), int(y), size);
}
template<typename _ValT>
inline bool isIn(const Point_<_ValT> &pt, const Size &size)
{
	return isIn(pt.x, pt.y, size);
}
inline bool _isIn(int x, int y, const Rect &roi)
{
	return uint(x)>=uint(roi.x) && uint(y)>=uint(roi.y) && uint(x) < uint(roi.x+roi.width) && uint(y) < uint(roi.x+roi.height);
}
template<typename _ValT>
inline bool isIn(_ValT x, _ValT y, const Rect &roi)
{
	return _isIn(int(x), int(y), roi);
}
template<typename _ValT>
inline bool isIn(const Point_<_ValT> &pt, const Rect &roi)
{
	return isIn(pt.x, pt.y, roi);
}

//============================================================================
//2D affine utils
template<typename _Tp>
inline Matx<_Tp, 2, 3> operator*(const Matx<_Tp, 2, 3> &am, const Matx<_Tp, 2, 3> &bm)
{
	const _Tp *a = am.val, *b = bm.val;
	return Matx<_Tp, 2, 3>(
		a[0] * b[0] + a[1] * b[3], a[0] * b[1] + a[1] * b[4], a[0] * b[2] + a[1] * b[5] + a[2],
		a[3] * b[0] + a[4] * b[3], a[3] * b[1] + a[4] * b[4], a[3] * b[2] + a[4] * b[5] + a[5]
		);
}
template<typename _Tp>
inline Matx<_Tp, 2, 3> invertAffine(const Matx<_Tp, 2, 3> &am)
{
	const _Tp *a = am.val;
	auto m = Matx<_Tp, 2, 2>(a[0], a[1], a[3], a[4]).inv();
	Vec<_Tp, 2> v = m*Vec<_Tp, 2>(-a[2], -a[5]);
	return Matx<_Tp, 2, 3>(m.val[0], m.val[1], v[0], m.val[2], m.val[3], v[1]);
}


template<typename _Tp>
inline Rect_<_Tp> _getBoundingBox2D(const std::vector<Point_<_Tp>>& pts)
{
	if (pts.empty())
		return Rect_<_Tp>(0, 0, 0, 0);
	_Tp L = pts[0].x, T = pts[0].y, R = L, B = T;
	for (auto& p : pts)
	{
		if (p.x < L)
			L = p.x;
		if (p.x > R)
			R = p.x;
		if (p.y < T)
			T = p.y;
		if (p.y > B)
			B = p.y;
	}
	return Rect_<_Tp>(L, T, R - L, B - T);
}

inline Rect getBoundingBox2D(const std::vector<Point>& pts)
{
	return _getBoundingBox2D(pts);
}
inline Rect_<float> getBoundingBox2D(const std::vector<Point2f>& pts)
{
	return _getBoundingBox2D(pts);
}


_CVX_END

