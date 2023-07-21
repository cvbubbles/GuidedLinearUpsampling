#pragma once

#include"ffdef.h"

/*
namespace std
{
	template<typename _VT>
	class allocator;

	template<typename _VT>
	struct char_traits;

	template<typename _ElemT,typename _TraitsT,typename _AT>
	class basic_string;

	template<typename _VT,typename _AT>
	class vector;
	template<typename _VT,typename _AT>
	class deque;

	template<typename _VT,typename _CT>
	class stack;
	template<typename _VT,typename _AT>
	class list;
	template<typename _VT,typename _CmT,typename _AT>
	class set;
	template<typename _VT,typename _CmT,typename _AT>
	class multiset;

	template<typename _KeyT,typename _ValT,typename _PrT,typename _AllocT>
	class map;

	template<typename _KeyT,typename _ValT,typename _PrT,typename _AllocT>
	class multimap;


	template<typename _T1,typename T2>
	struct pair;

    //typedef basic_string<char,char_traits<char>,allocator<char> > string;
//	typedef basic_string<wchar_t,char_traits<wchar_t>,allocator<wchar_t> > wstring;
};*/



_FF_BEG

template<typename _ValT,int size>
class Array;

template<typename _ValT,int _NDim>
class Vector;

template<typename _ValT>
class RectT;

template<typename _ValT>
class SizeT;

class IBFStream;

class IBMStream;

class OBFStream;

class OBMStream;

class BFStream;

class BMStream;

_FF_END








