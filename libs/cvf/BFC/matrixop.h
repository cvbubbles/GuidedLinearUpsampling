
#ifndef _FVT_INC_MATRIX_OP_H
#define _FVT_INC_MATRIX_OP_H


#include<cassert>
#include<cmath>

#include"ffdef.h"

_FF_BEG

template<typename _ValT>
inline void MatrixTranspose(const _ValT* matrix,_ValT* omatrix,unsigned m,unsigned n)
{
	for(unsigned i=0;i<m;++i)
			for(unsigned j=0;j<n;++j)
				omatrix[j*m+i]=matrix[i*n+j];
}
template<typename _ValT>
inline void MatrixTranspose(const _ValT* matrix,_ValT* omatrix,unsigned m)
{
	MatrixTranspose(matrix,omatrix,m,m);
}
template<typename _ValT>
inline void MatrixTranspose(_ValT* matrix,unsigned m)
{
	for(unsigned i=1;i<m;++i)
		for(unsigned j=0;j<i;++j)
		{
			_ValT &mij=matrix[i*m+j],&mji=matrix[j*m+i];
			_ValT t=mij;
			mij=mji,mji=t;
		}
}
template<typename _ValT>
inline void MatrixTranspose(_ValT* matrix,unsigned m,unsigned n)
{
	if(m==n)
		MatrixTranspose(matrix,m);	
	else
	{
		unsigned sz=m*n;
		_ValT* pbuf=new _ValT[sz];
		memcpy(pbuf,matrix,sizeof(_ValT)*sz);
		MatrixTranspose(pbuf,matrix,m,n);
		delete[]pbuf;
	}
}

template<typename _ValT, int N>
struct _MatrixDetermine
{
	static _ValT Determine(const _ValT* data)
	{
		//not implement for high dimensions
		CTCAssert(0);
	}
};

//Dimension 1
template <typename _ValT>
struct _MatrixDetermine<_ValT, 1>
{
	static _ValT Determine(const _ValT* data)
	{
		return data[0];
	}
};

//Dimension 2
template <typename _ValT>
struct _MatrixDetermine<_ValT, 2>
{
	static _ValT Determine(const _ValT* data)
	{
		return data[0]*data[3]-data[1]*data[2];
	}	
};

//Dimension 3
template <typename _ValT>
struct _MatrixDetermine<_ValT, 3>
{
	static _ValT Determine(const _ValT* data)
	{
		return data[0]*data[4]*data[8]
			+ data[2]*data[3]*data[7]
			+ data[1]*data[5]*data[6]
			- data[2]*data[4]*data[6]
			- data[0]*data[5]*data[7]
			- data[1]*data[3]*data[8];		
	}	
};


template<int N, typename _ValT>
inline _ValT MatrixDetermine(const _ValT* matrix)
{
	return _MatrixDetermine<_ValT, N>::Determine(matrix);
}

template<int N, typename _ValT, typename _ResultT>
struct _MatrixAccompany
{
	static bool Accompany(const _ValT* src, _ResultT* dst);
};

//dimension 1
template<typename _ValT, typename _ResultT>
struct _MatrixAccompany<1, _ValT, _ResultT>
{
	static void Accompany(const _ValT* src, _ResultT* dst)
	{
		dst[0] = _Result(src[0]);
	}
};

//dimension 2
template<typename _ValT, typename _ResultT>
struct _MatrixAccompany<2, _ValT, _ResultT>
{
	static void Accompany(const _ValT* src, _ResultT* dst)
	{
		dst[0] = _ResultT(src[3]);
		dst[1] = _ResultT(-src[1]);
		dst[2] = _ResultT(-src[2]);
		dst[3] = _ResultT(src[0]);
	}
};

//dimension 3
template<typename _ValT, typename _ResultT>
struct _MatrixAccompany<3, _ValT, _ResultT>
{
	static void Accompany(const _ValT* src, _ResultT* dst)
	{
		dst[0] = _ResultT(src[4]*src[8]-src[5]*src[7]);
		dst[1] = -_ResultT(src[1]*src[8]-src[2]*src[7]);
		dst[2] = _ResultT(src[1]*src[5]-src[2]*src[4]);
		dst[3] = -_ResultT(src[3]*src[8]-src[5]*src[6]);
		dst[4] = _ResultT(src[0]*src[8]-src[2]*src[6]);
		dst[5] = -_ResultT(src[0]*src[5]-src[2]*src[3]);
		dst[6] = _ResultT(src[3]*src[7]-src[4]*src[6]);
		dst[7] = -_ResultT(src[0]*src[7]-src[1]*src[6]);
		dst[8] = _ResultT(src[0]*src[4]-src[1]*src[3]);
	}
};

template<int N, typename _ValT, typename _ResultT>
inline void MatrixAccompany(const _ValT* src, _ResultT* dst)
{
	_MatrixAccompany<N, _ValT, _ResultT>::Accompany(src, dst);
}

template <typename _ValT, typename _ResultT, int N>
struct _MatrixReverse
{
	static bool Reverse(const _ValT* src, _ResultT* dst)
	{
		assert((void const*)src != (void*)dst);
		_ValT det = MatrixDetermine<N>(src);

		if (IsZero(det,1e-20))
		{
			return false;
		}
		else
		{
			MatrixAccompany<N>(src, dst);

			_ResultT inv = _ResultT(1)/_ResultT(det);			
			for (int i = 0; i < N*N; ++i)
			{
				dst[i] *= inv;
			}
			return true;
		}
	}
};

template<int N, typename _ValT, typename _ResultT>
inline bool MatrixReverse(const _ValT* src, _ResultT* dst)
{	
	return _MatrixReverse<_ValT, _ResultT, N>::Reverse(src, dst);
}

template<typename _ValT>
inline void MatrixMultiply(const _ValT* matrix0,const _ValT* matrix1,_ValT* omatrix,unsigned m,unsigned k,unsigned n)
{
	for(uint ri=0;ri<m;++ri)
		for(uint ci=0;ci<n;++ci)
		{
			const _ValT* ptr0=matrix0+ri*k;
			const _ValT* ptr1=matrix1+ci;
			_ValT uv=0;
			for(uint i=0;i<k;++i,++ptr0,ptr1+=n)
				uv+=*ptr0*(*ptr1);
			omatrix[ri*n+ci]=uv;
		}
}
template<typename _ValT>
inline void MatrixAdd(const _ValT* matrix0,const _ValT* matrix1,_ValT* omatrix,unsigned m,unsigned n)
{
	uint sz=m*n;
	for(uint i=0;i<sz;++i)
		omatrix[i]=matrix0[i]+matrix1[i];
}
template<typename _ValT>
inline void MatrixAdd(_ValT* matrix,const _ValT* matrix1,unsigned m,unsigned n)
{
	uint sz=m*n;
	for(uint i=0;i<sz;++i)
		matrix[i]+=matrix1[i];
}
template<typename _ValT>
inline void MatrixSub(const _ValT* matrix0,const _ValT* matrix1,_ValT* omatrix,unsigned m,unsigned n)
{
	uint sz=m*n;
	for(uint i=0;i<sz;++i)
		omatrix[i]=matrix0[i]-matrix1[i];
}
template<typename _ValT>
inline void MatrixSub(_ValT* matrix,const _ValT* matrix1,unsigned m,unsigned n)
{
	uint sz=m*n;
	for(uint i=0;i<sz;++i)
		matrix[i]-=matrix1[i];
}

template<typename _ValT>
inline void MatrixScale(_ValT* matrix,_ValT val,unsigned m,unsigned n)
{
	uint sz=m*n;
	for(uint i=0;i<sz;++i,++matrix)
		(*matrix)*=val;
}
template<typename _ValT>
inline void MatrixScale(const _ValT* matrix,_ValT* omatrix,_ValT val,unsigned m,unsigned n)
{
	uint sz=m*n;
	for(uint i=0;i<sz;++i,++matrix,++omatrix)
		(*omatrix)=(*matrix)*val;
}

//set matrix as diagonal matrix with each element on the diagonal be @dig.
//that is M=s*I.
template<typename _ValT>
inline void MatrixAssign(_ValT* matrix,_ValT dig,unsigned m)
{
	memset(matrix,0,sizeof(_ValT)*m*m);
	for(uint i=0;i<m;++i,matrix+=m)
		matrix[i]=dig;
}
//M==s(where M is a matrix and @s is a number) if M==s*I.
template<typename _ValT>
inline int MatrixCompare(const _ValT* matrix,_ValT dig,unsigned m,double max_error=1e-6)
{
	for(uint i=0;i<m;++i)
		for(uint j=0;j<m;++j)
			if(is_zero(matrix[i*m+j]-(i==j? dig:0),max_error))
				return 1;
	return 0;
}

_FF_END

#endif
