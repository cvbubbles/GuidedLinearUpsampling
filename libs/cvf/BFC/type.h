
#ifndef _FF_BFC_TYPE_H
#define _FF_BFC_TYPE_H


#include"BFC/fwd.h"
#include"BFC/vector.h"
#include"BFC/matrix.h"
#include<stdlib.h>

_FF_BEG


template<typename _ValT>
class RectT
{
public:
	enum{IS_MEMCPY};
public:
	typedef _ValT ValueType;
public:
	_ValT x,y,width,height;
public:
	RectT()
		:x(0),y(0),width(0),height(0)
	{
	}
	RectT(_ValT _x,_ValT _y,_ValT _width,_ValT _height)
		:x(_x),y(_y),width(_width),height(_height)
	{
	}
	RectT(const _ValT LTRB[4])
		:x(LTRB[0]), y(LTRB[1]), width(LTRB[2]-LTRB[0]), height(LTRB[3]-LTRB[1])
	{
	}
	_ValT X() const
	{
		return x;
	}
	_ValT Y() const
	{
		return y;
	}
	_ValT Width() const
	{
		return width;
	}
	_ValT Height() const
	{
		return height;
	}
	_ValT Left() const
	{
		return x;
	}
	_ValT Right() const
	{
		return x+width;
	}
	_ValT Top() const
	{
		return y;
	}
	_ValT Bottom() const
	{
		return y+height;
	}
	_ValT Area() const
	{
		return width*height;
	}
	Vector<_ValT,2> CornerLT() const
	{
		return Vector<_ValT,2>(x,y);
	}
	Vector<_ValT,2> CornerRT() const
	{
		return Vector<_ValT,2>(x+width,y);
	}
	Vector<_ValT,2> CornerRB() const
	{
		return Vector<_ValT,2>(x+width,y+height);
	}
	Vector<_ValT,2> CornerLB() const
	{
		return Vector<_ValT,2>(x,y+height);
	}

	//whether the rectangle is empty.
	bool IsEmpty() const
	{
		return width<=0||height<=0;
	}
	//whether the rectangle contain a 2D point.
	template<typename _CorValT>
	bool ContainPoint(_CorValT x,_CorValT y) const
	{
		return x>=this->Left()&&x<this->Right()&&y>=Top()&&y<=Bottom();
	}
	template<typename _PointT>
	bool ContainPoint(const _PointT& point) const
	{
		return this->ContainPoint(point[0],point[1]);
	}
	//translate without changing rect size.
	template<typename _VecValT>
	RectT<_ValT>& operator+=(const Vector<_VecValT,2> &dv)
	{
		x=NumCast<_ValT>(x+dv[0]);
		y=NumCast<_ValT>(y+dv[1]);
		return *this;
	}
	template<typename _VecValT>
	RectT<_ValT>& operator-=(const Vector<_VecValT,2> &dv)
	{
		x=NumCast<_ValT>(x-dv[0]);
		y=NumCast<_ValT>(y-dv[1]);
		return *this;
	}
	//scale with respect to center.
	void Scale(double sx,double sy)
	{
		double c=x+width/2,s=width*sx;
		width=NumCast<_ValT>(s);
		x=NumCast<_ValT>(c-s/2);

		c=y+height/2,s=height*sy;
		height=NumCast<_ValT>(s);
		y=NumCast<_ValT>(c-s/2);
	}

	void Append(_ValT dLeft, _ValT dTop, _ValT dRight, _ValT dBottom)
	{
		_ValT right=x+width+dRight, bottom=y+height+dBottom;

		x-=dLeft; y-=dTop;

		width=right-x;
		if(width<0)
			width=0;

		height=bottom-y;
		if(height<0)
			height=0;
	}
};

template<typename _ValT>
inline bool
operator==(const RectT<_ValT> & left,const RectT<_ValT> &right)
{
	return left.X()==right.X()&&left.Y()==right.Y()&&left.Width()==right.Width()&&left.Height()==right.Height();
}
template<typename _ValT>
inline bool
operator!=(const RectT<_ValT> & left,const RectT<_ValT> &right)
{
	return !(left==right);
}
template<typename _RectValT,typename _VecValT>
inline RectT<_RectValT>
operator+(const RectT<_RectValT> &rect,const Vector<_VecValT,2> &dv)
{
	return RectT<_RectValT>(
		NumCast<_RectValT>(rect.x+dv[0]),NumCast<_RectValT>(rect.y+dv[1]),
		rect.width,rect.height
		);
}
template<typename _RectValT,typename _VecValT>
inline RectT<_RectValT>
operator-(const RectT<_RectValT> &rect,const Vector<_VecValT,2> &dv)
{
	return RectT<_RectValT>(
		NumCast<_RectValT>(rect.x-dv[0]),NumCast<_RectValT>(rect.y-dv[1]),
		rect.width,rect.height
		);
}

template<typename _ValT>
inline RectT<_ValT> MergeRect(const RectT<_ValT> &rect0, const RectT<_ValT> &rect1)
{
	if(rect0.IsEmpty())
		return rect1;
	else 
		if(rect1.IsEmpty())
			return rect0;

	_ValT l(__min(rect0.x,rect1.x)), t(__min(rect0.y,rect1.y)), r(__max(rect0.Right(),rect1.Right())),b(__max(rect0.Bottom(),rect1.Bottom()));

	return RectT<_ValT>(l,t,r-l,b-t);
}

//Get the overlapped part of two rectangles @rect0 and @rect1.
template<typename _ValT>
inline RectT<_ValT> OverlappedRect(const RectT<_ValT> &rect0,const RectT<_ValT> &rect1)
{
	RectT<_ValT> _empty(0,0,0,0);
	_ValT left=rect1.Left(),top=rect1.Top(),right=rect1.Right(),bottom=rect1.Bottom();
	
	_ValT y=rect0.Y();
	if(y>top&&y<bottom)
		top=y;
	else
		if(y>=bottom)
			return _empty;
	
	y=rect0.Bottom();
	if(y>top&&y<bottom)
		bottom=y;
	else
		if(y<=top)
			return _empty;

	int x=rect0.X();
	if(x>left&&x<right)
		left=x;
	else 
		if(x>=right)
			return _empty;
	x=rect0.Right();
	if(x>left&&x<right)
		right=x;
	else
		if(x<=left)
			return _empty;

	return RectT<_ValT>(left,top,right-left,bottom-top);
}



template<typename _ValT>
class SizeT
{
public:
	enum{IS_MEMCPY};
public:
	typedef _ValT ValueType;
public:
	_ValT width,height;
public:
	SizeT()
		:width(0),height(0)
	{
	}
	SizeT(_ValT _width,_ValT _height)
		:width(_width),height(_height)
	{
	}
	_ValT Width() const
	{
		return width;
	}
	_ValT Height() const
	{
		return height;
	}
};

template<typename _ValT>
inline bool
operator==(const SizeT<_ValT> & left,const SizeT<_ValT> &right)
{
	return left.Width()==right.Width()&&left.Height()==right.Height();
}
template<typename _ValT>
inline bool
operator!=(const SizeT<_ValT> & left,const SizeT<_ValT> &right)
{
	return !(left==right);
}

#ifdef USE_BFC_TYPES

typedef RectT<int>	Rect;
typedef SizeT<int>  Size;

typedef Vector<int, 2>		Point2i;
typedef Vector<float, 2>		Point2f;
typedef Vector<double, 2>	Point2d;

typedef Vector<int, 3>		Point3i;
typedef Vector<float, 3>		Point3f;
typedef Vector<double, 3>	Point3d;



typedef Array<Point2i, 2>	Line2i;
typedef Array<Point2f, 2>    Line2f;
typedef Array<Point2d, 2>    Line2d;

typedef Array<Point3i, 2>	Line3i;
typedef Array<Point3f, 2>    Line3f;
typedef Array<Point3d, 2>    Line3d;

#endif

typedef Vector<uchar, 3>		Color3ub;
typedef Vector<float, 3>		Color3f;


typedef SmallMatrix<float, 2, 2>	Matrix22f;
typedef SmallMatrix<float, 2, 3>	Matrix23f;
typedef SmallMatrix<float, 3, 3>	Matrix33f;
typedef SmallMatrix<float, 3, 4>	Matrix34f;
typedef SmallMatrix<float, 4, 4>	Matrix44f;


typedef Vector<uchar, 4>		Color4ub;
typedef Vector<float, 4>		Color4f;



_FF_END










#endif

