
#pragma once

#include"def.h"
#include"BFC/ctc.h"

_CVX_BEG

using ff::FlagType;
using ff::SelType;
using ff::AccumType;
using ff::IsFloat;
using ff::NLimits;

template<int _cn, bool _vec=false>
struct ccn
{
	enum{ cn_id = _cn, channels = _cn };

	// whether to use Vec pixel when _cn==1 in the operator of for_each_....
	static const bool vec_pixel = _vec; 
public:
	ccn(const int = 0)
	{}
	operator int() const
	{
		return _cn;
	}
};

typedef ccn<1,false> ccn1;
typedef ccn<2,false> ccn2;
typedef ccn<3,false> ccn3;
typedef ccn<4,false> ccn4;

struct dcn
{
	enum{ cn_id = 0 };

	int m_cn;
public:
	dcn(const int cn = 0)
		:m_cn(cn)
	{
	}
	operator int() const
	{
		return m_cn;
	}
};

template<typename _cnT>
inline _cnT default_cn()
{
	static_assert(_cnT::cn_id>0, "default value is not available for dynamic channels");
	return _cnT();
}

//get max channels that can be used for define array as temporary buffer
template<typename _Ty>
struct cn_max
{
	enum{ value = 8 };
};

template<int cn>
struct cn_max<struct ccn<cn> >
{
	enum{ value = cn };
};

template<typename _ValT>
struct pixel_traits_base
{
protected:
	template<int cn, typename _DiffT>
	static _DiffT _diff2(const _ValT *px, const _ValT *py)
	{
		static_assert(cn>0, "invalid channels");
		_DiffT ds = 0;
		for (int i = 0; i<cn; ++i)
		{
			_DiffT d = px[i] - py[i];
			ds += d*d;
		}
		return ds;
	}
	template<typename _DestT, typename _SrcT>
	static _DestT _num_cast(_SrcT val, FlagType<true>)
	{//do round
		return static_cast<_DestT>(val + 0.5f);
	}
	template<typename _DestT, typename _SrcT>
	static _DestT _num_cast(_SrcT val, FlagType<false>)
	{//do cast
		return static_cast<_DestT>(val);
	}
	template<typename _ResultT, typename _NumT, typename _DenormT>
	static _ResultT _num_div(const _NumT n, const _DenormT d, FlagType<0>)
	{//float div
		return num_cast<_ResultT>(n / d);
	}

	template<typename _ResultT, typename _NumT, typename _DenormT>
	static _ResultT _num_div(const _NumT n, const _DenormT d, FlagType<1>)
	{//int = int/int
		return num_cast<_ResultT>((n + (d >> 1)) / d);
	}

	template<typename _ResultT, typename _NumT, typename _DenormT>
	static _ResultT _num_div(const _NumT n, const _DenormT d, FlagType<2>)
	{// float = int/int
		return num_cast<_ResultT>(_ResultT(n) / _ResultT(d));
	}

public:
	template<typename _DestT, typename _SrcT>
	static _DestT num_cast(_SrcT val)
	{
		return _num_cast<_DestT>(val, FlagType< IsFloat<_SrcT>::Yes && !IsFloat<_DestT>::Yes >());
	}

	template<typename _ResultT, typename _NumT, typename _DenormT>
	static _ResultT num_div(const _NumT n, const _DenormT d)
	{
		return _num_div<_ResultT>(n, d, FlagType< IsFloat<_NumT>::Yes || IsFloat<_DenormT>::Yes ? 0 : IsFloat<_ResultT>::Yes ? 2 : 1>());
	}
};

template<typename _ValT, int _NBITS = 8>
struct integer_pixel_traits
	:public pixel_traits_base<_ValT>
{
	typedef int diff_type;
	typedef int  accum_type;
	enum{ levels = (1 << _NBITS) };
    
    typedef pixel_traits_base<_ValT> _BaseT;

	template<int cn>
	static diff_type diff2(const _ValT *px, const _ValT *py)
	{
        return _BaseT::template _diff2<cn, diff_type>(px, py);
	}
	static _ValT max_value()
	{
		return levels - 1;
	}
	template<typename _IValT>
	static _IValT alpha_blend(_IValT f, _ValT a, _IValT b)
	{
		return static_cast<_IValT>(((b << _NBITS) + (f - b)*a) >> _NBITS);
	}
};

template<typename _ValT>
struct float_pixel_traits
	:public pixel_traits_base<_ValT>
{
	typedef _ValT diff_type;
	typedef _ValT accum_type;
    
    typedef pixel_traits_base<_ValT> _BaseT;

	template<int cn>
	static diff_type diff2(const _ValT *px, const _ValT *py)
	{
        return _BaseT::template _diff2<cn, diff_type>(px, py);
	}
	static _ValT max_value()
	{
		return 1.0f;
	}
	template<typename _IValT>
	static _IValT alpha_blend(_IValT f, _ValT a, _IValT b)
	{
		return static_cast<_IValT>(b + (f - b)*a);
	}
};

template<typename _ValT>
struct _channel_traits
	:public SelType< integer_pixel_traits<_ValT>, float_pixel_traits<_ValT>, std::numeric_limits<_ValT>::is_integer>::RType
{
};

template<typename _ValT>
struct channel_traits
	:public _channel_traits < typename ff::Deconst< typename ff::Deref<_ValT>::RType >::RType >
{
};

template<typename _CTp, int cn>
struct _pixel_traits_base
{
	static_assert(!ff::IsClass<_CTp>::Yes, "_CTp should be user defined types");

	enum{ channels = cn, depth = cv::DataType<_CTp>::depth };
	enum{type = CV_MAKETYPE(depth, cn)};

	typedef _CTp channel_type;
	typedef typename channel_traits<channel_type>::diff_type channel_diff_type;
	typedef typename channel_traits<channel_type>::accum_type channel_accum_type;

	typedef Vec<_CTp, cn> vec_value_type;
	typedef Vec<channel_diff_type, cn> vec_diff_type;
	typedef Vec<channel_accum_type, cn> vec_accum_type;

public:
	static ccn<cn> ccn()
	{
		return cv::ccn<cn>();
	}
};

template<typename _ValT>
struct _pixel_traits
	:public _pixel_traits_base < _ValT, 1 >
{
	typedef _pixel_traits_base < _ValT, 1 > _BaseT;

	typedef typename _BaseT::channel_diff_type diff_type;
	typedef typename _BaseT::channel_accum_type accum_type;

	typedef _ValT value_type;
};

template<typename _CTp, int cn>
struct _pixel_traits < Vec<_CTp, cn> >
	:public _pixel_traits_base<_CTp,cn>
{
	typedef _pixel_traits_base<_CTp, cn> _BaseT;

	typedef Vec<typename _BaseT::channel_diff_type, cn> diff_type;
	typedef Vec<typename _BaseT::channel_accum_type, cn> accum_type;

	typedef Vec<_CTp, cn> value_type;
};

template<typename _ValT>
struct pixel_traits
	:public _pixel_traits < typename ff::Deconst< typename ff::Deref<_ValT>::RType >::RType >
{};


//convert pixels to corresponding channels
template<typename _PixT>
inline const typename pixel_traits<_PixT>::channel_type* px2c(const _PixT *p)
{
	return reinterpret_cast<const typename pixel_traits<_PixT>::channel_type*>(p);
}
template<typename _PixT>
inline typename pixel_traits<_PixT>::channel_type* px2c(_PixT *p)
{
	return reinterpret_cast<typename pixel_traits<_PixT>::channel_type*>(p);
}

//copy pixel channels
template<typename _PValT, typename _CNT>
inline void pxcpy(const _PValT *src, _PValT *dest, _CNT cn)
{
	for (int i = 0; i<cn; ++i)
		dest[i] = src[i];
}

template<typename _PValT, int CN>
inline void pxcpy(const _PValT *src, _PValT *dest, ccn<CN> cn)
{
	memcpy(dest, src, sizeof(_PValT)*CN);
}

//compute L2 distance
template<int cn, typename _PValT>
inline typename channel_traits<_PValT>::diff_type pxdiff(const _PValT *a, const _PValT *b)
{
	return channel_traits<_PValT>::template diff2<cn>(a, b);
}
template<typename _PixT>
inline typename pixel_traits<_PixT>::channel_diff_type pxdiff(const _PixT &a, const _PixT &b)
{
	return pxdiff<pixel_traits<_PixT>::channels>(px2c(&a), px2c(&b));
}


_CVX_END


