
#pragma once

#include"opencv2/core/matx.hpp"
#include"pixel.h"
#include<memory>

_CVX_BEG

template<typename _DPtrTraitsT>
class diff_type
{
	typedef diff_type<_DPtrTraitsT> _MyT;
	int  m_diff;
public:
	explicit diff_type(int diff = 0)
		:m_diff(diff)
	{}

	int get() const
	{
		return m_diff;
	}

	template<typename _PtrT>
	friend _PtrT operator+=(_PtrT &ptr, const _MyT &diff)
	{
        typedef typename _DPtrTraitsT::template get_t<_PtrT>::diff_ptr_type _DPtrT;
		return ptr = reinterpret_cast<_PtrT>((_DPtrT)(ptr)+diff.m_diff);
	}
	template<typename _PtrT>
	friend _PtrT operator+(_PtrT ptr, const _MyT &diff)
	{
		typedef typename _DPtrTraitsT::template get_t<_PtrT>::diff_ptr_type _DPtrT;
		//	return reinterpret_cast<_PtrT>( reinterpret_cast<_DPtrT>(ptr)+diff.m_diff);
		return reinterpret_cast<_PtrT>((_DPtrT)(ptr)+diff.m_diff);
	}
	friend _MyT operator+(const _MyT &left, const _MyT &right)
	{
		return _MyT(left.get() + right.get());
	}

	friend _MyT operator-(const _MyT &left, const _MyT &right)
	{
		return _MyT(left.get() - right.get());
	}

	friend _MyT operator*(const _MyT &left, const int right)
	{
		return _MyT(left.get()*right);
	}
	friend _MyT operator*(const int left, const _MyT &right)
	{
		return _MyT(left*right.get());
	}
	friend _MyT operator/(const _MyT &left, const int right)
	{
		return _MyT(left.get() / right);
	}
};

struct _channel_diff_traits
{
	template<typename _ValT>
	struct _channel_value_type
	{
		typedef _ValT value_type;
	};
	template<typename _ValT, int _cn>
	struct _channel_value_type< Vec<_ValT, _cn> >
	{
		typedef _ValT value_type;
	};

	template<typename _PtrT>
	struct get_t
	{
		typedef typename _channel_value_type< typename std::iterator_traits<_PtrT>::value_type>::value_type value_type;
		typedef value_type* diff_ptr_type;
	};
};

typedef diff_type<_channel_diff_traits> channel_diff;

struct _byte_diff_traits
{
	template<typename _PtrT>
	struct get_t
	{
		typedef char value_type;
		typedef value_type* diff_ptr_type;
	};
};

typedef diff_type<_byte_diff_traits> byte_diff;

class diffx
{
public:
	template<typename _DiffT>
	static int get(const _DiffT &d)
	{
		return d;
	}
	template<typename _Tx>
	static int get(const diff_type<_Tx> &d)
	{
		return d.get();
	}
	template<typename _PtrT, typename _DiffT>
	static int bytes(const _DiffT &d)
	{
		return sizeof(typename std::iterator_traits<_PtrT>::value_type) * d;
	}
	template<typename _PtrT, typename _Tx>
	static int bytes(const diff_type<_Tx> &d)
	{
		return sizeof(typename _Tx::template get_t<_PtrT>::value_type) * get(d);
	}
	template<typename _PtrT, typename _DiffT>
	static int channels(const _DiffT &d)
	{
		enum{ csize = sizeof(typename _channel_diff_traits::get_t<_PtrT>::value_type) };
		const int bsize = bytes<_PtrT>(d);
		if (bsize%csize != 0)
			//throw std::bad_cast("failed to convert ptr difference to channels");
            throw std::bad_cast();
            
		return bsize / csize;
	}
};

//=================================================

template<typename _IPtrT, int CN, bool _vec>
inline Vec<typename std::iterator_traits<_IPtrT>::value_type, CN>& _PARSE_PIX_VAL(_IPtrT ptr, ccn<CN, _vec>)
{
	return *(Vec<typename std::iterator_traits<_IPtrT>::value_type, CN>*)ptr;
}

template<typename _IPtrT>
inline typename std::iterator_traits<_IPtrT>::value_type& _PARSE_PIX_VAL(_IPtrT ptr, ccn<1, false>)
{
	return *(typename std::iterator_traits<_IPtrT>::value_type*)ptr;
}

template<typename _IPtrT, typename _CNT>
inline _IPtrT _PARSE_PIX_VAL(_IPtrT ptr, _CNT)
{
	return ptr;
}

#define PIX_VAL(i) _PARSE_PIX_VAL(ptr##i+xi*cn##i, cn##i)

template<typename _IPtrT1, typename _StrideT1, typename _CNT1, typename _OpT>
inline void for_each_1(_IPtrT1 ptr1, const int width, const int height, const _StrideT1 istride1, const _CNT1 cn1, _OpT op)
{
	for (int yi = 0; yi<height; ++yi, ptr1 += istride1)
	{
		for (int xi = 0; xi<width; ++xi)
			op(PIX_VAL(1));
	}
}

template<typename _IPtrT1, typename _StrideT1, typename _CNT1, typename _OpT>
inline void for_each_1c(_IPtrT1 ptr1, const int width, const int height, const _StrideT1 istride1, const _CNT1 cn1, _OpT op)
{
	for (int yi = 0; yi<height; ++yi, ptr1 += istride1)
	{
		for (int xi = 0; xi<width; ++xi)
			op(PIX_VAL(1), xi, yi);
	}
}

template<typename _IPtrT1, typename _StrideT1, typename _CNT1, typename _OpT>
inline void for_each_1b(_IPtrT1 ptr1, const int width, const int height, const _StrideT1 istride1, const _CNT1 cn1, int L, int T, int R, int B, _OpT op)
{
	for_each_1(ptr1, width, T, istride1, cn1, op);
	for_each_1(ptr1 + (height - B)*istride1, width, B, istride1, cn1, op);
	for_each_1(ptr1 + T*istride1, L, height - T - B, istride1, cn1, op);
	for_each_1(ptr1 + T*istride1 + (width - R)*cn1, R, height - T - B, istride1, cn1, op);
}

template<typename _IPtrT1, typename _StrideT1, typename _CNT1, typename _OpT>
inline void _for_each_1cx(int x0, int y0,_IPtrT1 ptr1, const int width, const int height, const _StrideT1 istride1, const _CNT1 cn1, _OpT op)
{
	for (int yi = 0; yi<height; ++yi, ptr1 += istride1)
	{
		for (int xi = 0; xi<width; ++xi)
			op(PIX_VAL(1),xi+x0, yi+y0);
	}
}

template<typename _IPtrT1, typename _StrideT1, typename _CNT1, typename _OpT>
inline void for_each_1bc(_IPtrT1 ptr1, const int width, const int height, const _StrideT1 istride1, const _CNT1 cn1, int L, int T, int R, int B, _OpT op)
{
	_for_each_1cx(0,0,ptr1, width, T, istride1, cn1, op);
	_for_each_1cx(0,height-B,ptr1 + (height - B)*istride1, width, B, istride1, cn1, op);
	_for_each_1cx(0,T,ptr1 + T*istride1, L, height - T - B, istride1, cn1, op);
	_for_each_1cx(width-R,T,ptr1 + T*istride1 + (width - R)*cn1, R, height - T - B, istride1, cn1, op);
}


template<typename _IPtrT1, typename _StrideT1, typename _CNT1, typename _IPtrT2, typename _StrideT2, typename _CNT2, typename _OpT>
inline void for_each_2(_IPtrT1 ptr1, const int width, const int height, const _StrideT1 istride1, const _CNT1 cn1, 
					   _IPtrT2 ptr2, const _StrideT2 istride2, const _CNT2 cn2, _OpT op)
{
	for (int yi = 0; yi<height; ++yi, ptr1 += istride1, ptr2+=istride2)
	{
		for (int xi = 0; xi<width; ++xi)
			op(PIX_VAL(1), PIX_VAL(2));
	}
}

template<typename _IPtrT1, typename _StrideT1, typename _CNT1, typename _IPtrT2, typename _StrideT2, typename _CNT2, typename _OpT>
inline void for_each_2c(_IPtrT1 ptr1, const int width, const int height, const _StrideT1 istride1, const _CNT1 cn1,
	                    _IPtrT2 ptr2, const _StrideT2 istride2, const _CNT2 cn2, _OpT op)
{
	for (int yi = 0; yi<height; ++yi, ptr1 += istride1, ptr2 += istride2)
	{
		for (int xi = 0; xi<width; ++xi)
			op(PIX_VAL(1), PIX_VAL(2), xi, yi);
	}
}

template<typename _IPtrT1, typename _StrideT1, typename _CNT1, typename _IPtrT2, typename _StrideT2, typename _CNT2, typename _IPtrT3, typename _StrideT3, typename _CNT3, typename _OpT>
inline void for_each_3(_IPtrT1 ptr1, const int width, const int height, const _StrideT1 istride1, const _CNT1 cn1,
	_IPtrT2 ptr2, const _StrideT2 istride2, const _CNT2 cn2, _IPtrT3 ptr3, const _StrideT3 istride3, const _CNT3 cn3, _OpT op)
{
	for (int yi = 0; yi<height; ++yi, ptr1 += istride1, ptr2 += istride2, ptr3+=istride3)
	{
		for (int xi = 0; xi<width; ++xi)
			op(PIX_VAL(1), PIX_VAL(2), PIX_VAL(3) );
	}
}
template<typename _IPtrT1, typename _StrideT1, typename _CNT1, typename _IPtrT2, typename _StrideT2, typename _CNT2, typename _IPtrT3, typename _StrideT3, typename _CNT3, typename _OpT>
inline void for_each_3c(_IPtrT1 ptr1, const int width, const int height, const _StrideT1 istride1, const _CNT1 cn1,
	_IPtrT2 ptr2, const _StrideT2 istride2, const _CNT2 cn2, _IPtrT3 ptr3, const _StrideT3 istride3, const _CNT3 cn3, _OpT op)
{
	for (int yi = 0; yi<height; ++yi, ptr1 += istride1, ptr2 += istride2, ptr3 += istride3)
	{
		for (int xi = 0; xi<width; ++xi)
			op(PIX_VAL(1), PIX_VAL(2), PIX_VAL(3), xi, yi);
	}
}

template<typename _IPtrT1, typename _StrideT1, typename _CNT1, typename _IPtrT2, typename _StrideT2, typename _CNT2, typename _IPtrT3, typename _StrideT3, typename _CNT3, 
	typename _IPtrT4, typename _StrideT4, typename _CNT4, typename _OpT>
inline void for_each_4(_IPtrT1 ptr1, const int width, const int height, const _StrideT1 istride1, const _CNT1 cn1,
	_IPtrT2 ptr2, const _StrideT2 istride2, const _CNT2 cn2, _IPtrT3 ptr3, const _StrideT3 istride3, const _CNT3 cn3, 
	_IPtrT4 ptr4, const _StrideT4 istride4, const _CNT4 cn4, _OpT op)
{
	for (int yi = 0; yi<height; ++yi, ptr1 += istride1, ptr2 += istride2, ptr3 += istride3, ptr4+=istride4)
	{
		for (int xi = 0; xi<width; ++xi)
			op(PIX_VAL(1), PIX_VAL(2), PIX_VAL(3), PIX_VAL(4) );
	}
}

template<typename _IPtrT1, typename _StrideT1, typename _CNT1, typename _IPtrT2, typename _StrideT2, typename _CNT2, typename _IPtrT3, typename _StrideT3, typename _CNT3,
	typename _IPtrT4, typename _StrideT4, typename _CNT4, typename _OpT>
inline void for_each_4c(_IPtrT1 ptr1, const int width, const int height, const _StrideT1 istride1, const _CNT1 cn1,
	_IPtrT2 ptr2, const _StrideT2 istride2, const _CNT2 cn2, _IPtrT3 ptr3, const _StrideT3 istride3, const _CNT3 cn3,
	_IPtrT4 ptr4, const _StrideT4 istride4, const _CNT4 cn4, _OpT op)
{
	for (int yi = 0; yi<height; ++yi, ptr1 += istride1, ptr2 += istride2, ptr3 += istride3, ptr4 += istride4)
	{
		for (int xi = 0; xi<width; ++xi)
			op(PIX_VAL(1), PIX_VAL(2), PIX_VAL(3), PIX_VAL(4),xi,yi);
	}
}

template<typename _IPtrT1, typename _StrideT1, typename _CNT1, typename _IPtrT2, typename _StrideT2, typename _CNT2, typename _IPtrT3, typename _StrideT3, typename _CNT3,
	typename _IPtrT4, typename _StrideT4, typename _CNT4, typename _IPtrT5, typename _StrideT5, typename _CNT5, typename _OpT>
	inline void for_each_5(_IPtrT1 ptr1, const int width, const int height, const _StrideT1 istride1, const _CNT1 cn1,
	_IPtrT2 ptr2, const _StrideT2 istride2, const _CNT2 cn2, _IPtrT3 ptr3, const _StrideT3 istride3, const _CNT3 cn3,
	_IPtrT4 ptr4, const _StrideT4 istride4, const _CNT4 cn4, _IPtrT5 ptr5, const _StrideT5 istride5, const _CNT5 cn5, _OpT op)
{
	for (int yi = 0; yi<height; ++yi, ptr1 += istride1, ptr2 += istride2, ptr3 += istride3, ptr4 += istride4, ptr5+=istride5)
	{
		for (int xi = 0; xi<width; ++xi)
			op(PIX_VAL(1), PIX_VAL(2), PIX_VAL(3), PIX_VAL(4), PIX_VAL(5));
	}
}

template<typename _IPtrT1, typename _StrideT1, typename _CNT1, typename _IPtrT2, typename _StrideT2, typename _CNT2, typename _IPtrT3, typename _StrideT3, typename _CNT3,
	typename _IPtrT4, typename _StrideT4, typename _CNT4, typename _IPtrT5, typename _StrideT5, typename _CNT5, typename _OpT>
	inline void for_each_5c(_IPtrT1 ptr1, const int width, const int height, const _StrideT1 istride1, const _CNT1 cn1,
	_IPtrT2 ptr2, const _StrideT2 istride2, const _CNT2 cn2, _IPtrT3 ptr3, const _StrideT3 istride3, const _CNT3 cn3,
	_IPtrT4 ptr4, const _StrideT4 istride4, const _CNT4 cn4, _IPtrT5 ptr5, const _StrideT5 istride5, const _CNT5 cn5, _OpT op)
{
	for (int yi = 0; yi<height; ++yi, ptr1 += istride1, ptr2 += istride2, ptr3 += istride3, ptr4 += istride4, ptr5 += istride5)
	{
		for (int xi = 0; xi<width; ++xi)
			op(PIX_VAL(1), PIX_VAL(2), PIX_VAL(3), PIX_VAL(4), PIX_VAL(5), xi, yi);
	}
}


#undef PIX_VAL


//===============================================================

template<typename _IPixT, typename _APixT>
inline void iop_alpha_blend(const _IPixT &f, const _APixT &a, const _IPixT &b, _IPixT &c)
{
	for (int i = 0; i<_IPixT::channels; ++i)
		c[i] = a.traits().alpha_blend(f[i], a[0], b[i]);
}

template<typename _IPixT, typename _DPixT, typename _MapValT>
inline void iop_map(const _IPixT &p, _DPixT &d, _MapValT vmap[])
{
	for (int i = 0; i<_IPixT::channels; ++i)
		d[i] = static_cast<typename _DPixT::value_type>(vmap[p[i]]);
}

template<typename _IPixT, typename _DPixT, typename _TValT, typename _PValT>
inline void iop_threshold(const _IPixT &p, _DPixT &d, _TValT T, _PValT v0, _PValT v1)
{
	for (int i = 0; i<_IPixT::channels; ++i)
		d[i] = static_cast<typename _DPixT::value_type>(p[i]< static_cast<typename _IPixT::value_type>(T) ? v0 : v1);
}

_CVX_END

