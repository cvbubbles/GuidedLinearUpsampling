
#pragma once

#include"def.h"
#include<memory>

_CVX_BEG



//===========================================================

/*合并等价类

@eq, @neq : 等价类关系图，如果(i,j)为@eq中的一个元素，则类i,j为等价类

@ncc : 合并前的类的个数

@remap : 输出合并后的结果，remap[i]为合并前的类i在合并后的ID

@返回值 : 合并后的类的个数

*/
inline int merge_eq_class(const int(*eq)[2], int neq, int ncc, int *remap, bool keepOriginalLabel=false)
{
	struct _eqx_elem
	{
		int *peq; //id of neighbors
		int  n;   //number of neighbors
		int  i;   //index of the last neighbor
	public:
		void _add(int v)
		{
			peq[i] = v;
			++i;
		}
	};

	int *eqx_buf = new int[neq * 2];
	_eqx_elem *eqx = new _eqx_elem[ncc];

	memset(eqx, 0, sizeof(_eqx_elem)*ncc);

	for (int i = 0; i<neq; ++i)
	{//get the number of neighbors
		++eqx[eq[i][0]].n;
		++eqx[eq[i][1]].n;
	}

	int n = 0;
	for (int i = 0; i<ncc; ++i)
	{//init. @peq
		eqx[i].peq = eqx_buf + n;
		n += eqx[i].n;
	}

	for (int i = 0; i<neq; ++i)
	{//record neighbors
		eqx[eq[i][0]]._add(eq[i][1]);

		eqx[eq[i][1]]._add(eq[i][0]);
	}

	int mv = -1; //current id of merged class

	//visit mask
	int tv = 0;
	int *vtag = new int[ncc];
	memset(vtag, 0xFF, sizeof(int)*ncc); //init. as -1

	const int stk_size = neq>ncc ? neq : ncc;
	//stack data
	int *stk = new int[stk_size], istk = -1;

	for (int i = 0; i<ncc; ++i)
	{
		if (vtag[i]<0) //if not visited
		{
			++mv;  //begin new class
			const int cid = keepOriginalLabel ? i : mv;

			if (eqx[i].n != 0) //if there are neighbors
			{
				istk = 0;
				stk[istk] = i;
				++tv;  //new tag

				while (istk >= 0) //while stack is not empty
				{
					int top = stk[istk];
					--istk;

					vtag[top] = tv; //set as visited
					remap[top] = cid; //set as the same class

					const int *pex = eqx[top].peq, nex = eqx[top].n;

					for (int j = 0; j<nex; ++j) //push neighbors of current node
					{
						if (vtag[pex[j]] != tv) //if not visited
						{
							stk[++istk] = pex[j];
						}
					}
				}
			}
			else
			{
				remap[i] = cid;
				vtag[i] = 0;
			}
		}
	}

	delete[]eqx_buf;
	delete[]eqx;
	delete[]vtag;
	delete[]stk;

	return mv + 1;
}


/*快速连通域算法

@mask, @width, @height, @mstep : 连通域掩码，具有相同mask值连通像素属于同一连通域

@cc, @cstride : 与@mask大小相同的图像，用于接收每个像素的连通域ID。同一连通域的像素具有相同ID。

@返回值 : 连通域的个数
*/
inline int connected_component(const uchar *mask, const int width, const int height, const int mstep, int *cc, const int cstride)
{
	if (!mask || !cc)
		return 0;

	const int np = width*height;
	int(*eq0)[2] = new int[__min(np * 2, 1000 * 1000)][2], (*eq)[2] = eq0 + 1, neq = -1;
	int *pcc = cc;

	eq0[0][0] = eq0[0][1] = 0;

	int id = 0;

	for (int yi = 0; yi<height; ++yi, mask += mstep, pcc += cstride)
	{
		int *pcx = pcc;
		const uchar *pmx = mask, *pme = mask + width;

		for (; pmx != pme; ++pmx, ++pcx)
		{
			int id0 = -1;

			if (pmx != mask)  //if xi>0
			{
				if (pmx[-1] == *pmx)
				{
					*pcx = id0 = pcx[-1];
				}
			}

			if (yi>0)
			{
				if (pmx[-mstep] == *pmx)
				{
					int idx = pcx[-cstride];

					if (id0 >= 0)
					{
						if (id0 != idx && //id0 and idx should be eq.
							(eq[neq][0] != id0 || eq[neq][1] != idx) //if the last element is not (id0,idx)
							)
						{
							++neq;
							eq[neq][0] = id0;
							eq[neq][1] = idx;
						}
					}
					else
						*pcx = id0 = idx;
				}
			}

			if (id0<0)
				*pcx = id++;
		}
	}
	
	//merge eq. class
	int *remap = new int[id];
	id = merge_eq_class(eq, neq + 1, id, remap);

	//apply remap
	pcc = cc;
	for (int yi = 0; yi<height; ++yi, pcc += cstride)
	{
		for (int xi = 0; xi<width; ++xi)
			pcc[xi] = remap[pcc[xi]];
	}

	delete[]eq0;
	delete[]remap;

	return id;
}


class cc_seg
{
public:
	struct eq_c1
	{
		template<typename _IPtrT>
		bool operator()(_IPtrT px, _IPtrT py) const
		{
			return *px == *py;
		}
	};

	template<typename _DiffT>
	struct eq_T_c1
	{
		_DiffT mT;

		eq_T_c1(_DiffT T)
			:mT(T)
		{}

		template<typename _IPtrT>
		bool operator()(_IPtrT px, _IPtrT py) const
		{
			_DiffT d = *px - *py;
			return d*d<mT;
		}
	};

	template<typename _DiffT>
	struct eq_T_c3
	{
		_DiffT mT;

		eq_T_c3(_DiffT T)
			:mT(T)
		{}

		template<typename _IPtrT>
		bool operator()(_IPtrT px, _IPtrT py) const
		{
			return pxdiff<3>(px, py)<mT;
		}
	};

public:
	template<typename _IPtrT, typename _EqT>
	static int n4(_IPtrT img, const int width, const int height, const int istep, const int cn, int *cc, const int cstride, _EqT _is_eq)
	{
		if (!img || !cc)
			return 0;

		const int np = width*height;
		int(*eq0)[2] = new int[__min(np * 2, 1000 * 1000)][2], (*eq)[2] = eq0 + 1, neq = -1;
		int *pcc = cc;

		eq0[0][0] = eq0[0][1] = 0;

		int id = 0;

		for (int yi = 0; yi<height; ++yi, img += istep, pcc += cstride)
		{
			int *pcx = pcc;
			_IPtrT pmx = img, pme = img + width*cn;

			for (; pmx != pme; pmx += cn, ++pcx)
			{
				int id0 = -1;

				if (pmx != img)  //if xi>0
				{
					if (_is_eq(pmx - cn, pmx))
					{
						*pcx = id0 = pcx[-1];
					}
				}

				if (yi>0)
				{
					if (_is_eq(pmx - istep, pmx))
					{
						int idx = pcx[-cstride];

						if (id0 >= 0)
						{
							if (id0 != idx && //id0 and idx should be eq.
								(eq[neq][0] != id0 || eq[neq][1] != idx) //if the last element is not (id0,idx)
								)
							{
								++neq;
								eq[neq][0] = id0;
								eq[neq][1] = idx;
							}
						}
						else
							*pcx = id0 = idx;
					}
				}

				if (id0<0)
					*pcx = id++;
			}
		}

		//return id;

		//merge eq. class
		int *remap = new int[id];
		id = merge_eq_class(eq, neq + 1, id, remap);

		//apply remap
		pcc = cc;
		for (int yi = 0; yi<height; ++yi, pcc += cstride)
		{
			for (int xi = 0; xi<width; ++xi)
				pcc[xi] = remap[pcc[xi]];
		}

		delete[]eq0;
		delete[]remap;

		return id;
	}

	static int n4m(const uchar *nbr, const int width, const int height, const int nstep, int *cc, const int cstride)
	{
		if (!nbr || !cc)
			return 0;

		const int np = width*height;
		int(*eq0)[2] = new int[__min(np * 2, 1000 * 1000)][2], (*eq)[2] = eq0 + 1, neq = -1;
		int *pcc = cc;

		eq0[0][0] = eq0[0][1] = 0;

		int id = 0;

		for (int yi = 0; yi<height; ++yi, nbr += nstep, pcc += cstride)
		{
			int *pcx = pcc;
			for (int xi = 0; xi < width; ++xi)
			{
				uchar nm = nbr[xi];
				int id0 = -1;
				if (nm & 1)
					*pcx = id0 = pcx[xi - 1];
				if (nm & 2)
				{
					int idx = pcx[xi - cstride];
					if (id0 >= 0)
					{
						if (id0 != idx && //id0 and idx should be eq.
							(eq[neq][0] != id0 || eq[neq][1] != idx) //if the last element is not (id0,idx)
							)
						{
							++neq;
							eq[neq][0] = id0;
							eq[neq][1] = idx;
						}
					}
					else
						pcx[xi] = id0 = idx;
				}
				if (id0<0)
					*pcx = id++;
			}
		}

		//merge eq. class
		int *remap = new int[id];
		id = merge_eq_class(eq, neq + 1, id, remap);

		//apply remap
		pcc = cc;
		for (int yi = 0; yi<height; ++yi, pcc += cstride)
		{
			for (int xi = 0; xi<width; ++xi)
				pcc[xi] = remap[pcc[xi]];
		}

		delete[]eq0;
		delete[]remap;

		return id;
	}

	template<typename _IPtrT, typename _EqT>
	static int n4_with_ei(_IPtrT img, const int width, const int height, const int istep, const int cn, int *cc, const int cstride, _EqT _is_eq)
	{
		if (!img || !cc)
			return 0;

		const int np = width*height;
		int(*eq0)[2] = new int[__min(np * 2, 1000 * 1000)][2], (*eq)[2] = eq0 + 1, neq = -1;
		int *pcc = cc;

		eq0[0][0] = eq0[0][1] = 0;

		int id = 0;

		for (int yi = 0; yi<height; ++yi, img += istep, pcc += cstride)
		{
			int *pcx = pcc;
			_IPtrT pmx = img, pme = img + width*cn;

			for (; pmx != pme; pmx += cn, ++pcx)
			{
				int id0 = -1;

				if (pmx != img)  //if xi>0
				{
					if (_is_eq(pmx - cn, pmx, 0))
					{
						*pcx = id0 = pcx[-1];
					}
				}

				if (yi>0)
				{
					if (_is_eq(pmx - istep, pmx, 2))
					{
						int idx = pcx[-cstride];

						if (id0 >= 0)
						{
							if (id0 != idx && //id0 and idx should be eq.
								(eq[neq][0] != id0 || eq[neq][1] != idx) //if the last element is not (id0,idx)
								)
							{
								++neq;
								eq[neq][0] = id0;
								eq[neq][1] = idx;
							}
						}
						else
							*pcx = id0 = idx;
					}
				}

				if (id0<0)
					*pcx = id++;
			}
		}

		//merge eq. class
		int *remap = new int[id];
		id = merge_eq_class(eq, neq + 1, id, remap);

		//apply remap
		pcc = cc;
		for (int yi = 0; yi<height; ++yi, pcc += cstride)
		{
			for (int xi = 0; xi<width; ++xi)
				pcc[xi] = remap[pcc[xi]];
		}

		delete[]eq0;
		delete[]remap;

		return id;
	}


	template<typename _IPtrT, typename _EqT>
	static int n8(_IPtrT img, const int width, const int height, const int istep, const int cn, int *cc, const int cstride, _EqT _is_eq)
	{
		if (!img || !cc)
			return 0;

		const int np = width*height;
		int(*eq0)[2] = new int[__min(np * 4,1000*1000)][2], (*eq)[2] = eq0 + 1, neq = -1;
		int *pcc = cc;

		eq0[0][0] = eq0[0][1] = 0;

		int id = 0;

		for (int yi = 0; yi<height; ++yi, img += istep, pcc += cstride)
		{
			int *pcx = pcc;
			_IPtrT pmx = img;

			for (int xi = 0; xi<width; ++xi, pmx += cn, ++pcx)
			{
				int id0 = -1;

#define _add_eq(idx)  if(id0!=idx&& (eq[neq][0]!=id0||eq[neq][1]!=idx) ) \
											{++neq; eq[neq][0]=id0;	eq[neq][1]=idx;	}

				if (pmx != img)  //if xi>0
				{
					if (_is_eq(pmx - cn, pmx))
					{
						*pcx = id0 = pcx[-1];
					}

					if (yi>0 && _is_eq(pmx - istep - cn, pmx))
					{
						int idx = pcx[-cstride - 1];
						if (id0 >= 0)
						{
							_add_eq(idx);
						}
						else
							*pcx = id0 = idx;
					}
				}
				if (yi>0)
				{
					if (_is_eq(pmx - istep, pmx))
					{
						int idx = pcx[-cstride];

						if (id0 >= 0)
						{
							_add_eq(idx);
						}
						else
							*pcx = id0 = idx;
					}

					if (xi != width - 1 && _is_eq(pmx - istep + cn, pmx))
					{
						int idx = pcx[-cstride + 1];
						if (id0 >= 0)
						{
							_add_eq(idx);
						}
						else
							*pcx = id0 = idx;
					}
				}

				if (id0<0)
					*pcx = id++;
			}
		}
//#undef _add_eq

		//merge eq. class
		int *remap = new int[id];
		id = merge_eq_class(eq, neq + 1, id, remap);

		//apply remap
		pcc = cc;
		for (int yi = 0; yi<height; ++yi, pcc += cstride)
		{
			for (int xi = 0; xi<width; ++xi)
				pcc[xi] = remap[pcc[xi]];
		}

		delete[]eq0;
		delete[]remap;

		return id;
	}
	
	static int n8m(const uchar *nbr, const int width, const int height, const int nstep, int *cc, const int cstride)
	{
		if (!nbr || !cc)
			return 0;

		const int np = width*height;
		int(*eq0)[2] = new int[__min(np * 4, 1000 * 1000)][2], (*eq)[2] = eq0 + 1, neq = -1;
		int *pcc = cc;

		eq0[0][0] = eq0[0][1] = 0;

		int id = 0;

		for (int yi = 0; yi<height; ++yi, nbr += nstep, pcc += cstride)
		{
			int *pcx = pcc;

			for (int xi = 0; xi<width; ++xi, ++pcx)
			{
				uchar nm = nbr[xi];
				int id0 = -1;

//#define _add_eq(idx)  if(id0!=idx&& (eq[neq][0]!=id0||eq[neq][1]!=idx) ) \
//											{++neq; eq[neq][0]=id0;	eq[neq][1]=idx;	}

				//if (pmx != img)  //if xi>0
				{
					if (nm&1)
					{
						*pcx = id0 = pcx[-1];
					}

					//if (yi>0 && _is_eq(pmx - istep - cn, pmx))
					if(nm&4)
					{
						int idx = pcx[-cstride - 1];
						if (id0 >= 0)
						{
							_add_eq(idx);
						}
						else
							*pcx = id0 = idx;
					}
				}
				//if (yi>0)
				{
					//if (_is_eq(pmx - istep, pmx))
					if(nm&2)
					{
						int idx = pcx[-cstride];

						if (id0 >= 0)
						{
							_add_eq(idx);
						}
						else
							*pcx = id0 = idx;
					}

					//if (xi != width - 1 && _is_eq(pmx - istep + cn, pmx))
					if(nm&8)
					{
						int idx = pcx[-cstride + 1];
						if (id0 >= 0)
						{
							_add_eq(idx);
						}
						else
							*pcx = id0 = idx;
					}
				}

				if (id0<0)
					*pcx = id++;
			}
		}
#undef _add_eq

		//merge eq. class
		int *remap = new int[id];
		id = merge_eq_class(eq, neq + 1, id, remap);

		//apply remap
		pcc = cc;
		for (int yi = 0; yi<height; ++yi, pcc += cstride)
		{
			for (int xi = 0; xi<width; ++xi)
				pcc[xi] = remap[pcc[xi]];
		}

		delete[]eq0;
		delete[]remap;

		return id;
	}
};

template<typename _IPtrT>
inline int cc_seg_n4c1(_IPtrT img, const int width, const int height, const int istride, int cn, int *cc, const int cstride)
{
	return cc_seg::n4(img, width, height, istride, cn, cc, cstride, cc_seg::eq_c1());
}

template<typename _IPtrT, typename _DiffT>
inline int cc_seg_n4c1(_IPtrT img, const int width, const int height, const int istride, int cn, int *cc, const int cstride, _DiffT T)
{
	return cc_seg::n4(img, width, height, istride, cn, cc, cstride, cc_seg::eq_T_c1<_DiffT>(T));
}

template<typename _IPtrT>
inline int cc_seg_n8c1(_IPtrT img, const int width, const int height, const int istride, int cn, int *cc, const int cstride)
{
	return cc_seg::n8(img, width, height, istride, cn, cc, cstride, cc_seg::eq_c1());
}

template<typename _IPtrT, typename _DiffT>
inline int cc_seg_n8c1(_IPtrT img, const int width, const int height, const int istride, int cn, int *cc, const int cstride, _DiffT T)
{
	return cc_seg::n8(img, width, height, istride, cn, cc, cstride, cc_seg::eq_T_c1<_DiffT>(T));
}

template<typename _IPtrT, typename _DiffT>
inline int cc_seg_n4c3(_IPtrT img, const int width, const int height, const int istride, int cn, int *cc, const int cstride, _DiffT T)
{
	return cc_seg::n4(img, width, height, istride, cn, cc, cstride, cc_seg::eq_T_c3<_DiffT>(T));
}

template<typename _IPtrT, typename _DiffT>
inline int cc_seg_n8c3(_IPtrT img, const int width, const int height, const int istride, int cn, int *cc, const int cstride, _DiffT T)
{
	return cc_seg::n8(img, width, height, istride, cn, cc, cstride, cc_seg::eq_T_c3<_DiffT>(T));
}

_CVX_END

