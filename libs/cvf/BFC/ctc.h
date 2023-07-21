
#pragma once

#include"BFC/fwd.h"

#include<limits>
#include<math.h>

#ifndef CTCAssert

template<bool>
struct compile_time_assertion_failed
{
private:
	typedef int type;
};
template<>
struct compile_time_assertion_failed<true>
{
	typedef int type;
};

//To assert the truth of a compile-time constant, a compiler error will present if the expresion @exp is evaluated to false.
#define CTCAssert(exp) \
	typedef typename compile_time_assertion_failed<(exp)!=0>::type _COMPILE_TIME_ASSERTION_TYPEDEF

#endif

_FF_BEG
	
//To determine whether a type is class or struct, if it is not, it must be
//compiler inner type such as int,float, enum, pointer or function.
template<typename T>
class IsClass
{
	template<typename _Tx>
	static char _imp(...);

	template<typename _Tx>
	static int _imp(int _Tx::*);
public:
	static const bool Yes=(sizeof(_imp<T>(0))==sizeof(int));
};

//create type from integer.
template<int flag>
class FlagType
{
	enum{FLAG=flag};
};

//to test whether two types are equal, for example, to ensure two types T1 and T2 are the same 
//in your program, you may write as follows:
//		CTCAssert(TypeEqual<T1,T2>::Yes);

template<typename T1,typename T2>
class TypeEqual
{
public:
	static const bool Yes=false;
};
template<typename T>
class TypeEqual<T,T>
{
public:
	static const bool Yes=true;
};

//select between two types according to a boolean value.
//the @RType will be @_TrueT if @_Flag is true, else it will be @_FalseT;
template<typename _TrueT,typename _FalseT,bool _Flag>
class SelType
{
public:
	typedef _TrueT RType;
};
template<typename _TrueT,typename _FalseT>
class SelType<_TrueT,_FalseT,false>
{
public:
	typedef _FalseT RType;
};

// retrive type from its corresponding reference type, that is, int from int&, const float from const float&,etc.
template<typename T>
class Deref
{
	template<typename _Tx>
	struct _imp
		{typedef _Tx type;};
	template<typename _Tx>
	struct _imp<_Tx&>
		{typedef _Tx type;};
public:
	typedef typename _imp<T>::type RType;
};

//retrive type from its const- decorated counterpart, that is, int from const int, int& from const int&, etc.
template<typename T>
class Deconst
{
	template<typename _Tx>
	struct _imp
	{typedef _Tx type;};
	template<typename _Tx>
	struct _imp<const _Tx>
	{typedef _Tx type;};
public:
	typedef typename _imp<T>::type RType;
};

//to make a "virtual" object that represent this type.
template<typename _T>
struct VMake //virtual-make
{
	static _T Value; //undefined!!!
};

//whether a type is floating number, which can be used to distinguish floating number from integer.

template<typename _T>
class IsFloat
{
	template<typename _Ty>
	static char _type_id(_Ty);

	static int  _type_id(float);

	static int  _type_id(double);

	static int  _type_id(long double);
public:
	static const bool Yes=sizeof(_type_id(VMake<_T>::Value))==sizeof(int);
};

template<typename _T>
class IsSigned
{
	template<typename _Ty>
	static char _type_id(_Ty);

	static int  _type_id(bool);

	static int  _type_id(char);

	static int  _type_id(short);

	static int  _type_id(int);

	static int  _type_id(long);

	static int  _type_id(long long);
public:
	static const bool Yes=(sizeof(_type_id(VMake<_T>::Value))==sizeof(int));
};

template<typename _T>
class IsUnsigned
{
	template<typename _Ty>
	static char _type_id(_Ty);

	static int  _type_id(unsigned char);

	static int  _type_id(unsigned short);

	static int  _type_id(unsigned int);

	static int  _type_id(unsigned long);

	static int  _type_id(unsigned long long);
public:
	static const bool Yes=(sizeof(_type_id(VMake<_T>::Value))==sizeof(int));
};

template<typename _T>
class IsInteger
{
public:
	static const bool Yes=IsSigned<_T>::Yes||IsUnsigned<_T>::Yes;
};



#define CTC_IMP_DEFINED_TEST(name) \
template<typename T>\
struct defined_const_##name\
{\
	template<typename Tx>\
	static char _imp(...);\
	template<typename Tx>\
	static int _imp(FlagType<Tx::name>*);\
public:\
	static const bool yes=(sizeof(_imp<T>(0))==sizeof(int));\
};

#define CTC_DEFINED(class_name,const_name) (defined_const_##const_name<class_name>::yes)

CTC_IMP_DEFINED_TEST(IS_MEMCPY);

template<typename _T>
struct IsVoid
{
	static const bool Yes = false;
};
template<>
struct IsVoid<void>
{
	static const bool Yes = true;
};

template<typename _T>
inline void AllowCopyWithMemory(_T) {}

//whether the type can be assigned by memory copy.
template<typename _T>
struct IsMemcpy
{
	template<typename _Tx,typename _ValT>
	struct _is_mp
	{
		static const bool Yes=false;
	};
	template<typename _Tx,typename _ArrValT,int _N>
	struct _is_mp<_Tx,Vector<_ArrValT,_N> >
	{
		static const bool Yes=IsMemcpy<_ArrValT>::Yes;
	};
	template<typename _Tx,typename _ArrValT,int _N>
	struct _is_mp<_Tx,Array<_ArrValT,_N> >
	{
		static const bool Yes=IsMemcpy<_ArrValT>::Yes;
	};
	template<typename _Tx,typename _ArrValT,int _N>
	struct _is_mp<_Tx,_ArrValT[_N]>
	{
		static const bool Yes=IsMemcpy<_ArrValT>::Yes;
	};
	static _T& makeT();
public:
	static const bool Yes=!IsClass<_T>::Yes||_is_mp<void,_T>::Yes||CTC_DEFINED(_T,IS_MEMCPY)||!IsVoid<decltype(AllowCopyWithMemory(makeT()))>::Yes;
};


#define CTC_TYPE_MAP(src,dest)  \
	template<typename _TMapTx>  \
	struct _type_map<_TMapTx,src> \
	{typedef dest Type;}

//get a type that can be used to store the sum of the specified type.

template<typename _T>
class AccumType
{
	template<typename _MapTx,typename _SrcT>
	struct _type_map
	{
		typedef _SrcT Type;
	};
	
	CTC_TYPE_MAP(bool,int);  CTC_TYPE_MAP(char,int);  CTC_TYPE_MAP(short,int);  

	CTC_TYPE_MAP(unsigned char,unsigned int);  CTC_TYPE_MAP(unsigned short,unsigned int); 
public:
	typedef typename _type_map<void,_T>::Type RType;
};

//get a type that can be used to store the difference of the specified type.

template<typename _T>
class DiffType
{
	template<typename _MapTx,typename _SrcT>
	struct _type_map
	{
		typedef _SrcT Type;
	};
	
	CTC_TYPE_MAP(bool,char);  CTC_TYPE_MAP(char,short);  CTC_TYPE_MAP(short,int);  

	CTC_TYPE_MAP(unsigned char,short);  CTC_TYPE_MAP(unsigned short,int); 

	CTC_TYPE_MAP(unsigned int,int);  CTC_TYPE_MAP(unsigned long,long);  CTC_TYPE_MAP(unsigned long long,long long);  
public:
	typedef typename _type_map<void,_T>::Type RType;
};

//get the "larger" type between two types.

template<typename _T0,typename _T1>
class LargerType
{
	template<typename _Ty,typename _Tx0,typename _Tx1>
	struct _larger
	{
		typedef typename SelType<_Tx0,_Tx1,(sizeof(_Tx0)>sizeof(_Tx1))>::RType Type;
	};

#define _LARGER_TX(_SmallerT,_LargerT) template<typename _Ty> \
	struct _larger<_Ty,_SmallerT,_LargerT> \
	{ \
		typedef _LargerT Type;  \
	}; \
	template<typename _Ty> \
	struct _larger<_Ty,_LargerT,_SmallerT> \
	{ \
		typedef _LargerT Type;  \
	}

	_LARGER_TX(bool, char) ;   _LARGER_TX(bool, unsigned char); _LARGER_TX(char,unsigned char) ;

	_LARGER_TX(short,unsigned short);  _LARGER_TX(int,unsigned int); 
	
	_LARGER_TX(int,float);		_LARGER_TX(unsigned int,float);

	_LARGER_TX(long long,unsigned long long);  _LARGER_TX(long long,double);  

	_LARGER_TX(unsigned long long,double);

#undef  _LARGER_TX
	
public:
	typedef typename _larger<void,_T0,_T1>::Type RType;
};

//get arithmetic result type which compatible with C/C++ language.

template<typename _T0,typename _T1>
class ResultType
{
	typedef typename LargerType<_T0,_T1>::RType _LargerT;
public:
	typedef typename SelType<int,_LargerT,sizeof(_LargerT)<sizeof(int)&&IsInteger<_LargerT>::Yes>::RType RType;
};

//////////////////////////////////////////////////////////////////////////

//whether a numberic value is zero.
//@torr : the tolerance to distinguish @val from zero, used for only floating number.
template<typename T>
inline bool IsZero(T val,double torr=1e-6)
{
	return IsFloat<T>::Yes? fabs(double(val))<torr:val==0;
}

template<typename T>
inline bool _CTCEqualTest(const T v0,const T v1,FlagType<true>,double torr)
{
	return fabs(v0-v1)<torr;
}
template<typename T>
inline bool _CTCEqualTest(const T &v0,const T &v1,FlagType<false>,double)
{
	return v0==v1;
}
template<typename T>
inline bool IsEqual(const T& v0,const T& v1,double torr=1e-6)
{
	return _CTCEqualTest(v0,v0,FlagType<IsFloat<T>::Yes>(),torr);
}

template<typename _DestT,typename _SrcT>
inline _DestT NumCast(_SrcT val)
{
	return !IsFloat<_DestT>::Yes&&IsFloat<_SrcT>::Yes? _DestT(val+0.5):_DestT(val);
}



#ifdef min

#define _FF_DEF_MIN

#undef min
#undef max

#endif

template<typename _Ty>
class NLimits
	:public std::numeric_limits<_Ty>
{
	typedef std::numeric_limits<_Ty> _BaseT;
public:
	static _Ty  Max() 
	{
		return  _BaseT::max();
	}

	static _Ty Min()
	{
		return _BaseT::min();
	}
};


#ifdef _FF_DEF_MIN


#ifndef max
#define max(a,b)            (((a) > (b)) ? (a) : (b))
#endif

#ifndef min
#define min(a,b)            (((a) < (b)) ? (a) : (b))
#endif

#undef _FF_DEF_MIN

#endif



_FF_END




