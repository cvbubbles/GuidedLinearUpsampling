#pragma once

#include"CVX/core.h"

_CVX_BEG

template<typename _OBST>
inline void BSWrite(_OBST &os, const Mat &img)
{
	os << (int)img.rows << (int)img.cols << (int)img.type();
	if (!img.empty())
	{
		size_t rowSize = img.elemSize()*img.cols;
		for (int y = 0; y < img.rows; ++y)
			os.Write(img.ptr(y), rowSize, 1);
	}
}
template<typename _BST>
inline void BSRead(_BST &is, Mat &img)
{
	int rows, cols, type;
	is >> rows >> cols >> type;
	if (rows > 0 && cols > 0)
	{
		img.create(rows, cols, type);
		
		size_t rowSize = img.elemSize()*img.cols;
		for (int y = 0; y < img.rows; ++y)
			is.Read(img.ptr(y), rowSize, 1);
	}
}

template<typename _OBST, typename _Tp>
inline void BSWrite(_OBST &os, const Mat_<_Tp> &img)
{
	BSWrite(os, reinterpret_cast<const Mat&>(img));
}
template<typename _IBST, typename _Tp>
inline void BSRead(_IBST &is, Mat_<_Tp> &img)
{
	BSRead(is, reinterpret_cast<Mat&>(img));
}

template<typename _Tp,int cn>
inline char AllowCopyWithMemory(cv::Vec<_Tp,cn>);

template<typename _Tp>
inline char AllowCopyWithMemory(cv::Point_<_Tp>);

template<typename _Tp>
inline char AllowCopyWithMemory(cv::Point3_<_Tp>);

template<typename _Tp,int m, int n>
inline char AllowCopyWithMemory(cv::Matx<_Tp,m,n>);

template<typename _Tp>
inline char AllowCopyWithMemory(cv::Rect_<_Tp>);

template<typename _Tp>
inline char AllowCopyWithMemory(cv::Size_<_Tp>);

_CVX_END


