
#include<assert.h>
#include<stdlib.h>
#include<memory.h>
#include<list>

#include"mean_filter.h"
#include"BFC/autores.h"

_CVX_BEG


template<typename _ValT>
inline void count_filter_pixels(_ValT *np, int width, int height, int nstride, int kszx, int kszy, int borderLTRB[] = NULL)
{
	const int hkszx = kszx / 2, hkszy = kszy / 2;

	int def_bdr[] = { 0, 0, 0, 0 };

	if (!borderLTRB)
		borderLTRB = def_bdr;

	if (borderLTRB[0] >= hkszx && borderLTRB[1] >= hkszy && borderLTRB[2] >= hkszx && borderLTRB[3] >= hkszy)
	{
		const int n = kszx*kszy;

		for (int yi = 0; yi<height; ++yi, np += nstride)
		{
			for (int xi = 0; xi<width; ++xi)
				np[xi] = n;
		}
	}
	else
	{
		const int W = width + borderLTRB[0] + borderLTRB[2], H = height + borderLTRB[1] + borderLTRB[3];

		uchar *mask = new uchar[W*H];
		memset(mask, 1, W*H);

		sum_filter(mask + borderLTRB[1] * W + borderLTRB[0], width, height, W, 1, np, nstride, kszx, kszy, borderLTRB);

		delete[]mask;
	}
}

template<typename _NValT>
class filter_np_buffer
{
	struct config
	{
		int  m_width, m_height;
		int  m_kszx, m_kszy;
		int  m_borderLTRB[4];
	public:
		config()
		{}
		config(int width, int height, int kszx, int kszy, int borderLTRB[])
			:m_width(width), m_height(height), m_kszx(kszx), m_kszy(kszy)
		{
			if (!borderLTRB)
				memset(m_borderLTRB, 0, sizeof(m_borderLTRB));
			else
				memcpy(m_borderLTRB, borderLTRB, sizeof(m_borderLTRB));
		}

		friend bool operator==(const config &left, const config &right)
		{
			return memcmp(&left, &right, sizeof(config)) == 0;
		}
	};

	struct np_item
	{
		config m_cfg;
		ff::AutoArrayPtr<_NValT> m_np;
	public:
		np_item()
		{}

		np_item(const config &cfg, ff::AutoArrayPtr<_NValT> np)
			:m_cfg(cfg), m_np(np)
		{
		}
	};

	std::list<np_item> m_list;
public:
	typedef _NValT value_type;
public:
	const _NValT* get(int width, int height, int kszx, int kszy, int borderLTRB[])
	{
		config cfg(width, height, kszx, kszy, borderLTRB);

		typename std::list<np_item>::iterator itr(m_list.begin());
		for (; itr != m_list.end(); ++itr)
		{
			if (itr->m_cfg == cfg)
				return itr->m_np;
		}

		_NValT* np = new _NValT[width*height];
		count_filter_pixels(np, width, height, width, kszx, kszy, borderLTRB);

		m_list.push_back(np_item(cfg, ff::AutoArrayPtr<_NValT>(np)));

		return np;
	}
};

template<typename _IntegralImageT, typename _NPBufferT, typename _DValT, typename _CNT>
inline void mean_filter(_IntegralImageT &sum_filter, _NPBufferT &np_buffer, _DValT *dest, int dstride, _CNT cn, int kszx, int kszy, int x = 0, int y = 0, int width = -1, int height = -1)
{
	if (width<0)
		width = sum_filter.m_width - x;

	if (height<0)
		height = sum_filter.m_height - y;

    typedef typename channel_traits<_DValT>::accum_type _SumValT;
	_SumValT  *sum_buf = new _SumValT[width*cn*height];
	sum_filter.get_sum(sum_buf, width*cn, kszx, kszy, x, y, width, height);

	const int *np = np_buffer.get(sum_filter.m_width, sum_filter.m_height, kszx, kszy, sum_filter.m_borderLTRB);

	for_each_3(sum_buf, width, height, width*cn, cn, np + y*sum_filter.m_width + x, sum_filter.m_width, ccn1(), dest, dstride, cn, [cn](_SumValT *s, int np, _DValT *d){
		for (int i = 0; i < cn; ++i)
			d[i] = s[i] / np;
	});
    
	delete[]sum_buf;
}

template<typename _DValT, typename _SValT, typename _CNT, typename _NPBufferT>
inline void mean_filter(const _SValT *src, int width, int height, int istep, _CNT cn, _DValT *dest, int dstride, _NPBufferT &np_buffer, int kszx, int kszy, int borderLTRB[] = NULL)
{
	typedef typename channel_traits<_DValT>::accum_type _SumValT;
	_SumValT  *sum_buf = new _SumValT[width*cn*height];

	sum_filter(src, width, height, istep, cn, sum_buf, width*cn, kszx, kszy, borderLTRB);

	const typename _NPBufferT::value_type *np = np_buffer.get(width, height, kszx, kszy, borderLTRB);

	for_each_3(sum_buf, width, height, width*cn, cn, np, width, ccn1(), dest, dstride, cn, [cn](_SumValT *s, int np, _DValT *d){
		for (int i = 0; i < cn; ++i)
			d[i] = s[i] / np;
	});
    
	delete[]sum_buf;
}

template<typename _DValT, typename _SValT, typename _CNT>
inline void ipfi_mean_filter(const _SValT *src, int width, int height, int istep, _CNT cn, _DValT *dest, int dstride, int *np, int nstride, int kszx, int kszy, int borderLTRB[] = NULL)
{
	typedef typename channel_traits<_SValT>::accum_type _SumValT;
	_SumValT  *sum_buf = new _SumValT[width*cn*height], *psy = sum_buf;
	int *np_buf = NULL;

	if (!np)
	{
		np_buf = np = new int[width*height];
		nstride = width;
	}

	sum_filter(src, width, height, istep, cn, sum_buf, width*cn, kszx, kszy, borderLTRB);
	count_filter_pixels(np, width, height, nstride, kszx, kszy, borderLTRB);

	//	for_each_pixel_3(psy,width,height,width*cn,cn,np,nstride,1,dest,dstride,cn,iop_div_p<_CNT>(cn));

	delete[]np_buf;
	delete[]sum_buf;
}

void mean_filter(const uchar *src, int width, int height, int istep, int cn, uchar *dest, int dstride, int *np, int nstride, int kszx, int kszy, int borderLTRB[])
{
	if (!np && !borderLTRB)
		mean_filter(src, width, height, istep, cn, dest, dstride, kszx, kszy);
	else
		ipfi_mean_filter(src, width, height, istep, cn, dest, dstride, np, nstride, kszx, kszy, borderLTRB);
}

void mean_filter(const uchar *src, int width, int height, int istep, int cn, int *dest, int dstride, int *np, int nstride, int kszx, int kszy, int borderLTRB[])
{
	/*if(!np && !borderLTRB)
	mean_filter(src,width,height,istep,cn,dest,dstride,kszx,kszy);
	else*/
	ipfi_mean_filter(src, width, height, istep, cn, dest, dstride, np, nstride, kszx, kszy, borderLTRB);
}

void mean_filter(const int *src, int width, int height, int istep, int cn, int *dest, int dstride, int *np, int nstride, int kszx, int kszy, int borderLTRB[])
{
	if (!np && !borderLTRB)
		mean_filter(src, width, height, istep, cn, dest, dstride, kszx, kszy);
	else
		ipfi_mean_filter(src, width, height, istep, cn, dest, dstride, np, nstride, kszx, kszy, borderLTRB);
}

template<typename _SrcValT, typename _DestValT>
inline void ipfi_mean_filter_cx(const _SrcValT *img, int width, int height, int istride, int ccn, _DestValT *dest, int dstride, const int kszx, const int kszy)
{
	if (ccn == 1)
		mean_filter_x(img, width, height, istride, ccn1(), dest, dstride, kszx, kszy);
	else if (ccn == 2)
		mean_filter_x(img, width, height, istride, ccn2(), dest, dstride, kszx, kszy);
	else if (ccn == 3)
		mean_filter_x(img, width, height, istride, ccn3(), dest, dstride, kszx, kszy);
	else
		mean_filter_x(img, width, height, istride, ccn, dest, dstride, kszx, kszy);
}

void mean_filter(const uchar *src, int width, int height, int istep, int cn, uchar *dest, int dstride, int kszx, int kszy)
{
	ipfi_mean_filter_cx(src, width, height, istep, cn, dest, dstride, kszx, kszy);
}

void mean_filter(const int *src, int width, int height, int istep, int cn, int *dest, int dstride, int kszx, int kszy)
{
	ipfi_mean_filter_cx(src, width, height, istep, cn, dest, dstride, kszx, kszy);
}



#if 0
struct fgt_3x3
{
	struct item
	{
		int m_w0, m_w1;
		int *m_tab;
	public:
		item(int w0 = 0, int w1 = 0, int *tab = NULL)
			:m_w0(w0), m_w1(w1), m_tab(tab){}
		~item()
		{
			delete[]m_tab;
		}
	};

	std::list<item> m_list;

public:
	const int *get(int w0, int w1)
	{
		std::list<item>::iterator itr(m_list.begin());
		for (; itr != m_list.end(); ++itr)
		{
			if (itr->m_w0 == w0 && itr->m_w1 == w1)
			{
				return itr->m_tab;
			}
		}

		{
			int *tab = new int[256 + 512];
			for (int i = 0; i<256; ++i)
			{
				tab[i] = i*w0;
			}
			for (int i = 0; i<512; ++i)
			{
				tab[256 + i] = i*w1;
			}
			m_list.push_back(item(w0, w1, NULL));
			m_list.back().m_tab = tab;
		}
		return m_list.back().m_tab;
	}

};

const int *get_3x3_fixed_gaussian_table(int w0, int w1)
{
	static fgt_3x3 tab;

	return tab.get(w0, w1);
}
#endif

_CVX_END

