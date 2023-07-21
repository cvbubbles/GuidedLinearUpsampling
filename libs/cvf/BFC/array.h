
#ifndef _FF_BFC_ARRAY_H
#define _FF_BFC_ARRAY_H

#include"ffdef.h"
#include"BFC/ctc.h"
#include<array>
//#define CTCAssert(...) 

_FF_BEG

template<typename _ValT,int _Size>
class Array
	:public std::array<_ValT, _Size>
{
protected:
	typedef typename SelType<const _ValT&,_ValT,IsClass<_ValT>::Yes>::RType _CParamT; //const parameter type.

public:
	//default constructor do nothing.
	//note if @_ValT is a compiler inner type such as int,float,etc., array is not initialized to zero.
	Array()
	{
	}
	//initialize all elements to @vx.
	explicit Array(_CParamT vx)
	{
		for(int i=0;i<_Size;++i)
			(*this)[i]=vx;
	}
	//the following constructors are used to initialize the array with specific _Size.
	//note if you call the constructor with more or less arguments than the array required, the
	//@CTCAssert assertion in the constructor would failed and a compiler error would present,
	//for example:
	//	Array<int,3> arr(1,2);
	//is not valid, this is useful to prevent you from misusing partial initialized array.

	Array(_CParamT v0,_CParamT v1)
	{
		static_assert(_Size==2, "array _Size should be 2");
		(*this)[0]=v0,(*this)[1]=v1;
	}
	Array(_CParamT v0,_CParamT v1,_CParamT v2)
	{
		static_assert(_Size==3, "array _Size should be 3");
		(*this)[0]=v0;(*this)[1]=v1;(*this)[2]=v2;
	}
	Array(_CParamT v0,_CParamT v1,_CParamT v2,_CParamT v3)
	{
		static_assert(_Size==4, "array _Size should be 4");
		(*this)[0]=v0;(*this)[1]=v1;(*this)[2]=v2;(*this)[3]=v3;
	}
	Array(_CParamT v0,_CParamT v1,_CParamT v2,_CParamT v3,_CParamT v4)
	{
		static_assert(_Size==5, "array _Size should be 5");
		(*this)[0]=v0;(*this)[1]=v1;(*this)[2]=v2;(*this)[3]=v3;(*this)[4]=v4;
	}
	Array(_CParamT v0,_CParamT v1,_CParamT v2,_CParamT v3,_CParamT v4,_CParamT v5)
	{
		static_assert(_Size==6, "array _Size should be 6");
		(*this)[0]=v0;(*this)[1]=v1;(*this)[2]=v2;(*this)[3]=v3;(*this)[4]=v4;(*this)[5]=v5;
	}

	//convert from array of other type with the same _Size.
	template<typename _Tx>
	explicit Array(const Array<_Tx,_Size>& right)
	{
		for(int i=0;i<_Size;++i)
			(*this)[i]=static_cast<_ValT>(right[i]);
	}
};



_FF_END

#endif

