#pragma once

#include"def.h"
#include"CVX/cc.h"

_CVX_BEG

struct dpatch_base
{
public:
	template<typename _ValT>
	static _ValT norm_l2(const _ValT *vec, int size)
	{
		_ValT v = _ValT(0);
		for (int i = 0; i < size; ++i)
			v += vec[i] * vec[i];
		return v;
	}

	template<typename _ValT>
	static _ValT dot(const _ValT *u, const _ValT *v, int size)
	{
		_ValT r = _ValT(0);
		for (int i = 0; i < size; ++i)
			r += u[i] * v[i];
		return r;
	}
	template<typename _ValT>
	static void normalize(_ValT *v, int size)
	{
		_ValT len=norm_l2(v, size);
		if (len != _ValT(0))
		{
			_ValT s = _ValT(1)/sqrt(len);
			for (int i = 0; i < size; ++i)
				v[i] *= s;
		}
	}
	template<typename _ValT, typename _DistT, typename _DiffOpT>
	static void knnSearch(const _ValT* query, const _ValT* train, int trainSize, int dim, int vknn[], _DistT vdist[], int k, _DiffOpT calcDist)
	{
		for (int i = 0; i < k; ++i)
			vdist[i] = std::numeric_limits<_DistT>::max();

		for (int i = 0; i < trainSize; ++i, train += dim)
		{
			_DistT dx = (_DistT)calcDist(query, train, dim);
			if (vdist[k - 1]>dx)
			{
				int j = k - 2;
				for (; j>=0 && vdist[j] > dx; --j)
				{
					vdist[j + 1] = vdist[j];
					vknn[j + 1] = vknn[j];
				}
				vdist[j + 1] = dx;
				vknn[j + 1] = i;
			}
		}
	}

	template<typename _ValT, typename _DistT, typename _MaskT, typename _DiffOpT>
	static void knnSearch(const _ValT* query, const _ValT* train, const _MaskT *trainMask, int trainSize, int dim, int vknn[], _DistT vdist[], int k, _DiffOpT calcDist)
	{
		for (int i = 0; i < k; ++i)
			vdist[i] = std::numeric_limits<_DistT>::max();

		for (int i = 0; i < trainSize; ++i, train += dim)
		{
			if (!trainMask[i])
				continue;

			_DistT dx = (_DistT)calcDist(query, train, dim);
			if (vdist[k - 1] > dx)
			{
				int j = k - 2;
				for (; j >= 0 && vdist[j] > dx; --j)
				{
					vdist[j + 1] = vdist[j];
					vknn[j + 1] = vknn[j];
				}
				vdist[j + 1] = dx;
				vknn[j + 1] = i;
			}
		}
	}
};

template<typename _DiffT>
struct dpatch
	:public dpatch_base
{
	template<typename _ValT>
	static _DiffT ssd(const _ValT *u, const _ValT *v, int size)
	{
		_DiffT diff = _DiffT(0);
		for (int i = 0; i < size; ++i)
		{
			_DiffT d = _DiffT(u[i]) - _DiffT(v[i]);
			diff+=d*d;
		}
		return diff;
	}
};

typedef dpatch<float>   dpf;
typedef dpatch<int>     dpi;
typedef dpatch<uchar>   dpb;

_CVX_END

