
#ifndef _FF_BFC_VECTOR_H
#define _FF_BFC_VECTOR_H

#include<math.h>
#include"BFC/array.h"

_FF_BEG

template<typename _ValT,int _Size>
class Vector
	:public Array<_ValT,_Size>
{
	typedef Array<_ValT,_Size>  _MyBaseT;
	typedef Vector<_ValT,_Size> _MyT;
protected:
	typedef typename _MyBaseT::_CParamT _CParamT;
public:
	Vector()
	{
	}
	explicit Vector(_CParamT vx)
		:_MyBaseT(vx)
	{
	}
	Vector(_CParamT v0,_CParamT v1)
		:_MyBaseT(v0,v1)
	{
	}
	Vector(_CParamT v0,_CParamT v1,_CParamT v2)
		:_MyBaseT(v0,v1,v2)
	{
	}
	Vector(_CParamT v0,_CParamT v1,_CParamT v2,_CParamT v3)
		:_MyBaseT(v0,v1,v2,v3)
	{
	}
	Vector(_CParamT v0,_CParamT v1,_CParamT v2,_CParamT v3,_CParamT v4)
		:_MyBaseT(v0,v1,v2,v3,v4)
	{
	}
	Vector(_CParamT v0,_CParamT v1,_CParamT v2,_CParamT v3,_CParamT v4,_CParamT v5)
		:_MyBaseT(v0,v1,v2,v3,v4,v5)
	{
	}
	template<typename _Tx>
	explicit Vector(const Vector<_Tx,_Size>& right)
		:_MyBaseT(right)
	{
	}
private:
	_CParamT _ElemX(int idx,FlagType<true>) const
	{
		return (*this)[idx];
	}
	_CParamT _ElemX(int idx,FlagType<false>) const
	{
		return _ValT();
	}
public:
	template<int idx>
	_CParamT Elem() const
	{
		return this->_ElemX(idx,FlagType<(idx<_Size)>());
	}
	_CParamT X() const
	{
		return this->Elem<0>();
	}
	//set x.
	void X(_CParamT x)
	{
		(*this)[0]=x;
	}
	_CParamT Y() const
	{
		return this->Elem<1>();
	}
	//set y if the vector is 2 or higher dimension.
	void Y(_CParamT y)
	{
		CTCAssert(_Size>=2);
		(*this)[1]=y;
	}
	_CParamT Z() const
	{
		return this->Elem<2>();
	}
	//set z if the vector is 3 or higher dimension.
	void Z(_CParamT z)
	{
		CTCAssert(_Size>=3);
		(*this)[2]=z;
	}

	//v1+=v2;
	_MyT& operator+=(const _MyT& right)
	{
		for(int i=0;i<_Size;++i)
			(*this)[i]+=right[i];
		return *this;
	}
	//v1-=v2;
	_MyT& operator-=(const _MyT& right)
	{
		for(int i=0;i<_Size;++i)
			(*this)[i]-=right[i];
		return *this;
	}
	//v*=s. 
	template<typename _FactT>
	_MyT& operator*=(_FactT fact)
	{
		for(int i=0;i<_Size;++i)
			(*this)[i]*=fact;
		return *this;
	}
	//v/=s.
	template<typename _FactT>
	_MyT& operator/=(_FactT fact)
	{
		for(int i=0;i<_Size;++i)
			(*this)[i]/=fact;
		return *this;
	}
	//-v.
	_MyT operator-() const
	{
		return _ValT(-1)*(*this);
	}
	//get the length.
	double Length() const
	{
		return sqrt(double((*this)*(*this)));
	}
	//normalize the vector.
	void Normalize()
	{
		double len=this->Length();
		if(len!=0)
		{
			len=1.0/len;
			for(int i=0;i<_Size;++i)
				(*this)[i]=_ValT((*this)[i]*len);
		}
	}
	//whether the vector is unit.
	bool  IsUnit(double tor=1e-6) const
	{
		return ff::IsZero((*this)*(*this)-1,tor);
	}
	//whether the vector is zero.
	bool IsZero(double tor=1e-6) const
	{
		for(int i=0;i<_Size;++i)
			if(!ff::IsZero((*this)[i],tor))
				return false;
		return true;
	}
};

//v=v1+v2.
template<typename _ValT,int sz>
inline Vector<_ValT,sz> 
operator+(Vector<_ValT,sz> left,const Vector<_ValT,sz>& right)
{
	return left+=right;
}
//v=v1-v2
template<typename _ValT,int sz>
inline Vector<_ValT,sz> 
operator-(Vector<_ValT,sz> left,const Vector<_ValT,sz>& right)
{
	return left-=right;
}
//v=v*s
template<typename _ValT,int sz,typename _FactT>
inline Vector<_ValT,sz> 
operator*(Vector<_ValT,sz> left,_FactT fact)
{
	return left*=fact;
}
//v=s*v;
template<typename _ValT,int sz,typename _FactT>
inline Vector<_ValT,sz> 
operator*(_FactT fact,Vector<_ValT,sz> right)
{
	return right*=fact;
}
//v=v/s;
template<typename _ValT,int sz,typename _FactT>
inline Vector<_ValT,sz> 
operator/(Vector<_ValT,sz> left,_FactT fact)
{
	return left/=fact;
}

//inner product: s=v1*v2
template<typename _ValT,int sz>
inline typename AccumType<_ValT>::RType
operator*(const Vector<_ValT,sz>& left,const Vector<_ValT,sz>& right)
{
	typename AccumType<_ValT>::RType rv=0;
	for(int i=0;i<sz;++i)
		rv+=left[i]*right[i];
	return rv;
}

//vector product for 3-vector only.
template<typename _ValT>
inline Vector<_ValT,3>
Product(const Vector<_ValT,3> &left,const Vector<_ValT,3> &right)
{
	return Vector<_ValT,3>(
		left.Y()*right.Z()-left.Z()*right.Y(),
		left.Z()*right.X()-left.X()*right.Z(),
		left.X()*right.Y()-left.Y()*right.X()
		);
}

template<typename _ValT0,typename _ValT1,int _NDim>
inline void
ConvertVector(const Vector<_ValT0,_NDim> *pSrc,Vector<_ValT1,_NDim> *pDest,int nCount)
{
	for(int i=0;i<nCount;++i)
	{
		for(int j=0;j<_NDim;++j)
			pDest[i][j]=NumCast<_ValT1>(pSrc[i][j]);
	}
}


/////////////////////////////////////////////////////////////////////////////////

template<typename _IT0,typename _IT1,typename _OT>
inline void ArrAdd(const _IT0 *pIn0,const _IT1 *pIn1,_OT *pOut,int dim)
{
	for(int i=0;i<dim;++i)
		pOut[i]=_OT(pIn0[i]+pIn1[i]);
}
template<typename _IT0,typename _IT1,typename _OT>
inline void ArrAdd(const _IT0 *pIn0,const _IT1 &val,_OT *pOut,int dim)
{
	for(int i=0;i<dim;++i)
		pOut[i]=_OT(pIn0[i]+val);
}
template<typename _IT0,typename _IT1,typename _OT>
inline void ArrSub(const _IT0 *pIn0,const _IT1 *pIn1,_OT *pOut,int dim)
{
	for(int i=0;i<dim;++i)
		pOut[i]=_OT(pIn0[i]-pIn1[i]);
}
template<typename _IT0,typename _IT1,typename _OT>
inline void ArrSub(const _IT0 *pIn0,const _IT1 &val,_OT *pOut,int dim)
{
	for(int i=0;i<dim;++i)
		pOut[i]=_OT(pIn0[i]-val);
}
template<typename _IT0,typename _IT1,typename _OT>
inline void ArrMul(const _IT0 *pIn0,const _IT1 *pIn1,_OT *pOut,int dim)
{
	for(int i=0;i<dim;++i)
		pOut[i]=_OT(pIn0[i]*pIn1[i]);
}
template<typename _IT0,typename _IT1,typename _OT>
inline void ArrMul(const _IT0 *pIn0,const _IT1 &val,_OT *pOut,int dim)
{
	for(int i=0;i<dim;++i)
		pOut[i]=_OT(pIn0[i]*val);
}
template<typename _IT0,typename _IT1,typename _OT>
inline void ArrDiv(const _IT0 *pIn0,const _IT1 *pIn1,_OT *pOut,int dim)
{
	for(int i=0;i<dim;++i)
		pOut[i]=_OT(pIn0[i]/pIn1[i]);
}
template<typename _IT0,typename _IT1,typename _OT>
inline void ArrDiv(const _IT0 *pIn0,const _IT1 &val,_OT *pOut,int dim)
{
	for(int i=0;i<dim;++i)
		pOut[i]=_OT(pIn0[i]/val);
}

template<typename _IT0,typename _IT1>
inline typename AccumType<typename ResultType<_IT0,_IT1>::RType>::RType
ArrIProduct(const _IT0 *pIn0,const _IT1 *pIn1,int dim)
{
	typename AccumType<typename ResultType<_IT0,_IT1>::RType>::RType sum=0;

	for(int i=0;i<dim;++i)
		sum+=pIn0[i]*pIn1[i];

	return sum;
}

//for dim==3 only
template<typename _IT0,typename _IT1,typename _OT>
inline void
ArrVProduct(const _IT0 *pLeft,const _IT1 *pRight,_OT *pOut)
{
	pOut[0]=pLeft[1]*pRight[2]-pLeft[2]*pRight[1];
	pOut[1]=pLeft[2]*pRight[0]-pLeft[0]*pRight[2];
	pOut[2]=pLeft[0]*pRight[1]-pLeft[1]*pRight[0];
}

template<typename _AT>
inline void ArrNegate(_AT *pArr,int dim)
{
	for(int i=0;i<dim;++i)
		pArr[i]=-pArr[i];
}
template<typename _AT>
inline typename AccumType<_AT>::RType ArrNorm2(const _AT *pArr,int dim)
{
	return ArrIProduct(pArr,pArr,dim);
}
template<typename _AT>
inline double ArrLength(const _AT *pArr,int dim)
{
	return sqrt(double(ArrNorm2(pArr,dim)));
}

template<typename _AT>
inline void ArrNormalize(_AT *pArr,int dim)
{
	double len=ArrLength(pArr,dim);

	if(len!=0)
	{
		len=1.0/len;
		for(int i=0;i<dim;++i)
			pArr[i]*=len;
	}
}

template<typename _AT>
inline void ArrZero(_AT *pArr,int dim)
{
	memset(pArr,sizeof(_AT)*dim);
}
template<typename _AT>
inline bool ArrIsZero(const _AT *pArr,int dim,const double torr=1e-6)
{
	int i=0;
	for(;i<dim;++i)
	{
		if(!IsZero(pArr[i],torr))
			break;
	}
	return i==dim;
}
template<typename _AT>
inline bool ArrIsUnit(const _AT *pArr,int dim,const double torr=1e-6)
{
	return IsZero(ArrNorm2(pArr,dim)-1,torr);
}


_FF_END


#endif

