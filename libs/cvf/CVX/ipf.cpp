
#include"ipf.h"

_CVX_BEG

void  memcpy_2d(const void* src, int row_size, int height, int istep, void* dest, int ostep)
{
	if (src&&dest)
	{
		//if both source and dest is continuous.
		if (istep == ostep&&istep == row_size)
			memcpy(dest, src, height*istep);
		else
		{ //else copy line by line.
			const uchar* pi = (uchar*)src;
			uchar* po = (uchar*)dest;

			for (int yi = 0; yi<height; ++yi, pi += istep, po += ostep)
				memcpy(po, pi, row_size);
		}
	}
}

template<int cps>
static void _fiuiCopyPixel(const void *pIn, const int width, const int height, const int istep, const int ips,
	void *pOut, const int ostep, const int ops)
{
	if (pIn&&pOut)
	{
		uchar *pi = (uchar*)pIn, *po = (uchar*)pOut;
		for (int yi = 0; yi<height; ++yi, pi += istep, po += ostep)
		{
			uchar *pix = pi, *pox = po;
			for (int xi = 0; xi<width; ++xi, pix += ips, pox += ops)
				memcpy(pox, pix, cps);
		}
	}
}
static void _fiuiCopyPixel(const void *pIn, const int width, const int height, const int istep, const int ips,
	void *pOut, const int ostep, const int ops, const int cps)
{
	if (pIn&&pOut)
	{
		uchar *pi = (uchar*)pIn, *po = (uchar*)pOut;
		for (int yi = 0; yi<height; ++yi, pi += istep, po += ostep)
		{
			uchar *pix = pi, *pox = po;
			for (int xi = 0; xi<width; ++xi, pix += ips, pox += ops)
				memcpy(pox, pix, cps);
		}
	}
}

void  memcpy_c(const void *pIn, int width, int height, int istep, int ips,	void *pOut, int ostep, int ops, int cps)
{
	typedef void(*_FuncT)(const void *pIn, const int width, const int height, const int istep, const int ips,
		void *pOut, const int ostep, const int ops);

	static const _FuncT _pFuncTab[] =
	{
		_fiuiCopyPixel<1>, _fiuiCopyPixel<2>, _fiuiCopyPixel<3>, _fiuiCopyPixel<4>
	};

	if (uint(cps) <= 4)
		(_pFuncTab[cps - 1])(pIn, width, height, istep, ips, pOut, ostep, ops);
	else if (cps == 8)
		_fiuiCopyPixel<8>(pIn, width, height, istep, ips, pOut, ostep, ops);
	else
		_fiuiCopyPixel(pIn, width, height, istep, ips, pOut, ostep, ops, cps);
}

void   memset_2d(void* buf, const int row_size, const int height, const int step, char val)
{
	if (buf)
	{
		if (row_size == step)
			memset(buf, val, row_size*height);
		else
		{
			uchar* pox = (uchar*)buf;

			for (int i = 0; i<height; ++i, pox += step)
				memset(pox, val, row_size);
		}
	}
}

_CVX_END
