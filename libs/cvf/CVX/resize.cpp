
#include"resize.h"

_CVX_BEG


_CVX_API void calc_resize_tab_nn(int ssize, int dsize, int tab[], int cn)
{
	if (!tab)
		return;

	if (dsize == 1)
	{
		tab[0] = 0;
	}
	else
		if (dsize>1)
		{
			float scale = float(ssize) / (dsize);

			for (int i = 0; i<dsize; ++i)
			{
				tab[i] = int(i*scale + 0.5)*cn;
			}

			//checking boundary
			for (int i = dsize - 1; i >= 0; --i)
			{
				if (tab[i] >= (ssize - 1)*cn)
					tab[i] = (ssize - 1)*cn;
				else
					break;
			}
		}
}

_CVX_API void calc_resize_tab_bl(int ssize, int dsize, int itab[], float ftab[], int cn)
{
	if (!itab || !ftab)
		return;

	if (dsize == 1)
	{
		itab[0] = 0;
		ftab[0] = 0;
	}
	else
		if (dsize>1)
		{
			float scale = float(ssize) / (dsize);

			for (int i = 0; i<dsize; ++i)
			{
				float x = i*scale;
				int ix = int(x);

				itab[i] = ix*cn;
				ftab[i] = x - ix;
			}

			//checking boundary
			for (int i = dsize - 1; i >= 0; --i)
			{
				if (itab[i] >= (ssize - 1)*cn)
				{
					itab[i] = (ssize - 2)*cn;
					ftab[i] = 1.0f;
				}
				else
					break;
			}
		}
}

_CVX_API void calc_resize_tab_bl(int ssize, int dsize, int itab[], int ftab[], int cn, int iscale)
{
	if (!itab || !ftab)
		return;

	if (dsize == 1)
	{
		itab[0] = 0;
		ftab[0] = 0;
	}
	else
		if (dsize>1)
		{
			const float scale = float(ssize) / (dsize), EPS = scale*1e-2f;

			for (int i = 0; i<dsize; ++i)
			{
				float x = i*scale;
				int ix = int(x);
				float rx = x - ix;

				itab[i] = ix*cn;

				if (rx<EPS) //no interpolation, copy left pixel
					ftab[i] = 0;
				else if (rx>1.0f - EPS) //no interpolation, copy right pixel
				{
					ftab[i] = 0;
					itab[i] += cn;
				}
				else
					ftab[i] = static_cast<int>(rx*iscale);
			}

			//checking boundary
			for (int i = dsize - 1; i >= 0; --i)
			{
				if (itab[i] >= (ssize - 1)*cn)
				{
					itab[i] = (ssize - 2)*cn;
					ftab[i] = iscale;
				}
				else
					break;
			}
		}
}



_CVX_END

