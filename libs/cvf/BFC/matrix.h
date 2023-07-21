
#ifndef _FVT_INC_SMALLMAT_H
#define _FVT_INC_SMALLMAT_H

#include <string.h>//memcpy

#include"BFC/vector.h"
#include"BFC/matrixop.h"

_FF_BEG


template<typename _ValT, int _NRows, int _NCols>
class SmallMatrix
	:public Array<Vector<_ValT,_NCols>,_NRows>
{
	typedef Array<Vector<_ValT,_NCols>,_NRows>  _MyBaseT;
	typedef SmallMatrix<_ValT, _NRows, _NCols>	 _MyT;


protected:
	typedef typename _MyBaseT::_CParamT _CParamT;
	typedef _ValT* _PtrT;
	typedef const _ValT* _CPtrT;
public:
	SmallMatrix(){}

	explicit SmallMatrix(_ValT vx);

	explicit SmallMatrix(const _ValT* pData);

	SmallMatrix(const _MyT& right)
		:_MyBaseT(right){}

	template<typename _Tx>
	explicit SmallMatrix(SmallMatrix<_Tx, _NRows, _NCols> const& right)
		:_MyBaseT(right){}

	static int Rows(){return _NRows;}
	static int Cols(){return _NCols;}
	static int Size(){return Rows()*Cols();}

	_ValT& operator()(int row, int col);
	const _ValT& operator()(int row, int col)const;

	//m+=am
	_MyT& operator +=(_MyT const& am);

	//m-=am
	_MyT& operator -=(_MyT const& am);

	//m*=s
	template< typename _FacT >
	_MyT& operator *=(_FacT s);

	//m/=s
	template< typename _FacT >
	_MyT& operator /=(_FacT s);

};


//specialization of square matrix
template < typename _ValT, int _Nx >
class SmallMatrix<_ValT, _Nx, _Nx>
	:public Array<Vector<_ValT,_Nx>,_Nx>
{
	typedef Array<Vector<_ValT,_Nx>,_Nx>  _MyBaseT;
	typedef SmallMatrix<_ValT, _Nx, _Nx>	 _MyT;

protected:
	typedef typename _MyBaseT::_CParamT _CParamT;
	typedef _ValT* _PtrT;
	typedef const _ValT* _CPtrT;

public:
	SmallMatrix(){}

	explicit SmallMatrix(_ValT vx);

	explicit SmallMatrix(const _ValT* pData);

	SmallMatrix(const _MyT& right)
		:_MyBaseT(right){}

	template<typename _Tx>
	explicit SmallMatrix(SmallMatrix<_Tx, _Nx, _Nx> const& right)
		:_MyBaseT(right){}

	static int Rows(){return _Nx;}
	static int Cols(){return _Nx;}
	static int Size(){return Rows()*Cols();}

	_ValT& operator()(int row, int col);
	const _ValT& operator()(int row, int col)const;

	bool IsIdentity()const;

	_ValT Trace()const;

	_ValT Determine()const;

	//reverse and then return the matrix itself.
	//@pOK : if is not null, *pOK indicates whether the matrix is reversed successfully. The
	//	reverse operation would be fail if the matrix is singular.
	_MyT& Reverse(bool *pOK=NULL);

	_MyT& Transpose();

	_MyT& SetIdentity();

	//m+=am
	_MyT& operator +=(_MyT const& am);

	//m-=am
	_MyT& operator -=(_MyT const& am);

	//m*=am
	_MyT& operator *=(_MyT const& am);

	//m*=s
	template< typename _FacT >
	_MyT& operator *=(_FacT s);

	//m/=s
	template< typename _FacT >
	_MyT& operator /=(_FacT s);

};


//M = M1*M2
template <typename _ValT, int _NRows, int _NCols, int _Nx>
SmallMatrix<_ValT, _NRows, _Nx>
inline operator*(const SmallMatrix<_ValT, _NRows, _NCols>& left,
		  const SmallMatrix<_ValT, _NCols, _Nx>& right)
{
	SmallMatrix<_ValT, _NRows, _Nx> rm;
	MatrixMultiply(&left(0,0), &right(0,0),	&rm(0,0),_NRows,_NCols,_Nx);
	return rm;
}

//V = M1*V1
template<typename _ValT,int _NRows,int _NCols> 
inline  Vector<_ValT,_NRows> 
operator*(const SmallMatrix<_ValT,_NRows,_NCols>& m,
		  const Vector<_ValT,_NCols>& av)
{
	Vector<_ValT,_NRows> rv;
	for(int i=0;i<_NRows;++i)
		rv[i]=m[i]*av;
	return rv;
}



//V = V1*M1
template<typename _ValT,int _NRows,int _NCols> 
inline  Vector<_ValT,_NCols> 
operator*(const Vector<_ValT,_NRows>& av,
		  const SmallMatrix<_ValT,_NRows,_NCols>& m)
{
	Vector<_ValT,_NCols> rv;
	for(int i=0;i<_NCols;++i)
	{
		_ValT rvi=0;
		for(int j=0;j<_NRows;++j)
			rvi+=av[j]*m[j][i];
		rv[i]=rvi;
	}
	return rv;
}

////disable dismatching multiplies
//template <typename _ValT, int _NRowsL, int _NColsL, int _NRowsR, int _NColsR>
//void
//operator*(SmallMatrix<_ValT, _NRowsL, _NColsL> const& left,
//		  SmallMatrix<_ValT, _NRowsR, _NColsR> const& right)
//{
//	//dismatching of matrix dimensions
//	CTCAssert(0);
//}

template<typename _ValT,int _NRows,int _NCols> 
inline void 
Transpose(SmallMatrix<_ValT,_NRows,_NCols> const& src, 
		  SmallMatrix<_ValT,_NCols,_NRows>& dst)
{	
	MatrixTranspose(&src(0,0), &dst(0,0), _NRows,_NCols);
}

template<typename _ValT,int _NRows,int _NCols> 
inline SmallMatrix<_ValT,_NCols,_NRows> 
Transpose(SmallMatrix<_ValT,_NRows,_NCols> const& m)
{
	SmallMatrix<_ValT,_NCols,_NRows> rm;
	Transpose(m, rm);
	return rm;
}

//M=M1+M2
template <typename _ValT, int _NRows, int _NCols>
inline SmallMatrix<_ValT, _NRows, _NCols>
operator+(SmallMatrix<_ValT, _NRows, _NCols> m1,
		  SmallMatrix<_ValT, _NRows, _NCols> const& m2)
{
	return m1+=m2;
}

//M=M1-M2
template <typename _ValT, int _NRows, int _NCols>
inline SmallMatrix<_ValT, _NRows, _NCols>
operator-(SmallMatrix<_ValT, _NRows, _NCols> m1,
		  SmallMatrix<_ValT, _NRows, _NCols> const& m2)
{
	return m1-=m2;
}

//M=s*M1
template <typename _ValT, int _NRows, int _NCols, typename _FacT>
inline SmallMatrix<_ValT, _NRows, _NCols>
operator*(_FacT s,  SmallMatrix<_ValT, _NRows, _NCols> m1)
{
	return m1*=s;
}

//M=M1*s
template <typename _ValT, int _NRows, int _NCols, typename _FacT>
inline SmallMatrix<_ValT, _NRows, _NCols>
operator*(SmallMatrix<_ValT, _NRows, _NCols> m1, _FacT s)
{
	return m1*=s;
}


//M=M1/s
template <typename _ValT, int _NRows, int _NCols, typename _FacT>
inline SmallMatrix<_ValT, _NRows, _NCols>
operator/(SmallMatrix<_ValT, _NRows, _NCols> m1, _FacT s)
{
	return m1/=s;
}



//////////////////////////////////////////////////////////////////////////
////Implement of SmallMatrix(general)
//////////////////////////////////////////////////////////////////////////
template<typename _ValT, int _NRows, int _NCols>
inline SmallMatrix<_ValT, _NRows, _NCols>::SmallMatrix(_ValT val)
{
	_PtrT pd = &(*this)(0,0);
	for ( int i = 0; i < Size(); ++i)
	{
		pd[i] = val;
	}
}

template<typename _ValT, int _NRows, int _NCols>
inline SmallMatrix<_ValT, _NRows, _NCols>::SmallMatrix(const _ValT* pData)
{
	//CTCAssert(!IsClass<_ValT>::Yes);
	memcpy(&(*this)(0,0), pData, Size()*sizeof(_ValT));
}


template<typename _ValT, int _NRows, int _NCols>
inline _ValT&
SmallMatrix<_ValT, _NRows, _NCols>::operator()(int row, int col)
{
	return (*this)[row][col];
}

template<typename _ValT, int _NRows, int _NCols>
inline _ValT const&
SmallMatrix<_ValT, _NRows, _NCols>::operator()(int row, int col)const
{
	return (*this)[row][col];
}

//M+=M1
template<typename _ValT, int _NRows, int _NCols>
inline SmallMatrix<_ValT, _NRows, _NCols>&
SmallMatrix<_ValT, _NRows, _NCols>::operator+=(
	SmallMatrix<_ValT, _NRows, _NCols> const& am)
{
	_PtrT pthis = &(*this)(0,0);
	_CPtrT pam = &am(0,0);
	for (int i = 0; i < Size(); ++i)
	{
		pthis[i] += pam[i];
	}
	return *this;
}

//M-=M1
template<typename _ValT, int _NRows, int _NCols>
inline SmallMatrix<_ValT, _NRows, _NCols>&
SmallMatrix<_ValT, _NRows, _NCols>::operator-=(
	SmallMatrix<_ValT, _NRows, _NCols> const& am)
{
	_PtrT pthis = &(*this)(0,0);
	_CPtrT pam = &am(0,0);
	for (int i = 0; i < Size(); ++i)
	{
		pthis[i] -= pam[i];
	}
	return *this;
}

//M*=s
template<typename _ValT, int _NRows, int _NCols>
template<typename _FacT>
inline SmallMatrix<_ValT, _NRows, _NCols>&
SmallMatrix<_ValT, _NRows, _NCols>::operator*=(_FacT f)
{
	_PtrT pthis = &(*this)(0,0);
	for (int i = 0; i < Size(); ++i)
	{
		pthis[i] *= f;
	}
	return *this;
}

//M/=s
template<typename _ValT, int _NRows, int _NCols>
template<typename _FacT>
inline SmallMatrix<_ValT, _NRows, _NCols>&
SmallMatrix<_ValT, _NRows, _NCols>::operator/=(_FacT f)
{
	_PtrT pthis = &(*this)(0,0);
	for (int i = 0; i < Size(); ++i)
	{
		pthis[i] /= f;
	}
	return *this;
}


//////////////////////////////////////////////////////////////////////////
/////Implement of SmallMatrix(square specialization)
//////////////////////////////////////////////////////////////////////////

template<typename _ValT, int _Nx>
inline SmallMatrix<_ValT, _Nx, _Nx>::SmallMatrix(_ValT val)
{
	_PtrT pd = &(*this)(0,0);
	for ( int i = 0; i < Size(); ++i)
	{
		pd[i] = val;
	}
}

template<typename _ValT, int _Nx>
inline SmallMatrix<_ValT, _Nx, _Nx>::SmallMatrix(const _ValT* pData)
{
	//CTCAssert(!IsClass<_ValT>::Yes);
	memcpy(&(*this)(0,0), pData, Size()*sizeof(_ValT));
}


template<typename _ValT, int _Nx>
inline _ValT&
SmallMatrix<_ValT, _Nx, _Nx>::operator()(int row, int col)
{
	return (*this)[row][col];
}

template<typename _ValT, int _Nx>
inline _ValT const&
SmallMatrix<_ValT, _Nx, _Nx>::operator()(int row, int col)const
{
	return (*this)[row][col];
}

//M+=M1
template<typename _ValT, int _Nx>
inline SmallMatrix<_ValT, _Nx, _Nx>&
SmallMatrix<_ValT, _Nx, _Nx>::operator+=(
	SmallMatrix<_ValT, _Nx, _Nx> const& am)
{
	_PtrT pthis = &(*this)(0,0);
	_CPtrT pam = &am(0,0);
	for (int i = 0; i < Size(); ++i)
	{
		pthis[i] += pam[i];
	}
	return *this;
}

//M-=M1
template<typename _ValT, int _Nx>
inline SmallMatrix<_ValT, _Nx, _Nx>&
SmallMatrix<_ValT, _Nx, _Nx>::operator-=(
	SmallMatrix<_ValT, _Nx, _Nx> const& am)
{
	_PtrT pthis = &(*this)(0,0);
	_CPtrT pam = &am(0,0);
	for (int i = 0; i < Size(); ++i)
	{
		pthis[i] -= pam[i];
	}
	return *this;
}

//M*=s
template<typename _ValT, int _Nx>
template<typename _FacT>
inline SmallMatrix<_ValT, _Nx, _Nx>&
SmallMatrix<_ValT, _Nx, _Nx>::operator*=(_FacT f)
{
	_PtrT pthis = &(*this)(0,0);
	for (int i = 0; i < Size(); ++i)
	{
		pthis[i] *= f;
	}
	return *this;
}

//M/=s
template<typename _ValT, int _Nx>
template<typename _FacT>
inline SmallMatrix<_ValT, _Nx, _Nx>&
SmallMatrix<_ValT, _Nx, _Nx>::operator/=(_FacT f)
{
	_PtrT pthis = &(*this)(0,0);
	for (int i = 0; i < Size(); ++i)
	{
		pthis[i] /= f;
	}
	return *this;
}

template<typename _ValT, int _Nx>
inline SmallMatrix<_ValT, _Nx, _Nx>&
SmallMatrix<_ValT, _Nx, _Nx>::operator*=(
	SmallMatrix<_ValT, _Nx, _Nx> const& am)
{
	*this = *this*am;
	return *this;
}

template<typename _ValT, int _Nx>
inline _ValT SmallMatrix<_ValT, _Nx, _Nx>::Trace()const
{
	_ValT rt = _ValT();
	for (int i = 0; i < _Nx; ++i)
		rt += (*this)(i, i);
	return rt;
}

template<typename _ValT, int _Nx>
inline _ValT SmallMatrix<_ValT, _Nx, _Nx>::Determine()const
{
	return MatrixDetermine<_Nx>(&(*this)(0,0));
}

template<typename _ValT, int _Nx>
inline SmallMatrix<_ValT, _Nx, _Nx>& 
SmallMatrix<_ValT, _Nx, _Nx>::Transpose()
{
	for (int i = 0; i < _Nx; ++i)
		for (int j = i+1; j < _Nx; ++j)
		{
			_ValT tmp = (*this)(i,j);
			(*this)(i,j) = (*this)(j,i);
			(*this)(j,i) = tmp;
		}
	return *this;
}

template<typename _ValT,int _Nx>
inline SmallMatrix<_ValT,_Nx,_Nx>
Reverse(SmallMatrix<_ValT,_Nx,_Nx> const &right, bool *pOK=NULL)
{
	SmallMatrix<_ValT,_Nx,_Nx> result;
	bool rv=MatrixReverse<_Nx>(&right(0,0),&result(0,0));
	if(pOK)
		*pOK=rv;
	return result;
}

template<typename _ValT, int _Nx>
//template<typename _ResultT>
inline SmallMatrix<_ValT,_Nx,_Nx>&
SmallMatrix<_ValT, _Nx, _Nx>::Reverse(bool *pOK)
{
	SmallMatrix<_ValT,_Nx,_Nx> result;
	bool rv=MatrixReverse<_Nx>(&(*this)(0,0),&result(0,0));
	if(rv)
		*this=result;
	if(pOK)
		*pOK=rv;
	return *this;
}

template<typename _ValT, int _Nx>
inline SmallMatrix<_ValT, _Nx, _Nx>& 
SmallMatrix<_ValT, _Nx, _Nx>::SetIdentity()
{		
	memset(this, 0, Size()*sizeof(_ValT));

	for (int i = 0; i < _Nx; ++i)
	{
		(*this)(i,i) = _ValT(1);
	}
	return *this;
}

template<typename _ValT, int _Nx>
inline bool SmallMatrix<_ValT, _Nx, _Nx>::IsIdentity()const
{
	for (int i = 0; i < _Nx; ++i)
	{
		for (int j = 0; j < _Nx; ++j)
		{
			if (i==j)
			{
				if (!IsZero((*this)(i,j)-_ValT(1)))
				{
					return false;
				}
			}
			else
			{
				if (!IsZero((*this)(i,j)))
				{
					return false;
				}
			}
		}
	}
	return true;
}

//=====================================================================================

//axis must be normalized.
template<typename _MatValT,typename _VecValT>
inline void MakeRotateMatrix(SmallMatrix<_MatValT,3,3> &R,const Vector<_VecValT,3> &axis,double angle)
{
	assert(axis.IsUnit(0.1));

	_MatValT cosa=_MatValT(cos(angle));
	memset(&R,0,sizeof(R));
	R[0][0]=R[1][1]=R[2][2]=cosa;

	_MatValT sina=_MatValT(sin(angle));
	_VecValT x=axis[0],y=axis[1],z=axis[2];
	
	_MatValT av[]={0,-z*sina,y*sina,z*sina,0,-x*sina,-y*sina,x*sina,0};
	_MatValT rrt[]={x*x,x*y,x*z,y*x,y*y,y*z,z*x,z*y,z*z};

	_MatValT *pr=&R[0][0];
	cosa=1-cosa;
	for(int i=0;i<9;++i)
		pr[i]+=av[i]+rrt[i]*cosa;	
}



_FF_END


#endif
