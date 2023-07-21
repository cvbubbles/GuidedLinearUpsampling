
#ifndef _UTILS_H_
#define _UTILS_H_

typedef unsigned char uchar;

struct PaletteEntry 
{
    unsigned char b, g, r, a;
};

struct Size2i
{
	int width,height;

	Size2i(int _width=0,int _height=0)
		:width(_width),height(_height)
	{
	}
};


#define WRITE_PIX( ptr, clr )       \
    (((uchar*)(ptr))[0] = (clr).b,  \
     ((uchar*)(ptr))[1] = (clr).g,  \
     ((uchar*)(ptr))[2] = (clr).r)

#define  descale(x,n)  (((x) + (1 << ((n)-1))) >> (n))
#define  saturate(x)   (uchar)(((x) & ~255) == 0 ? (x) : ~((x)>>31))

void _fi_icvCvt_BGR2Gray_8u_C3C1R( const uchar* bgr, int bgr_step,
                               uchar* gray, int gray_step,
                               Size2i size, int swap_rb=0 );
void _fi_icvCvt_BGRA2Gray_8u_C4C1R( const uchar* bgra, int bgra_step,
                                uchar* gray, int gray_step,
                                Size2i size, int swap_rb=0 );
void _fi_icvCvt_Gray2BGR_8u_C1C3R( const uchar* gray, int gray_step,
                               uchar* bgr, int bgr_step, Size2i size );
void _fi_icvCvt_BGRA2BGR_8u_C4C3R( const uchar* bgra, int bgra_step,
                               uchar* bgr, int bgr_step,
                               Size2i size, int swap_rb=0 );
void _fi_icvCvt_BGR2RGB_8u_C3R( const uchar* bgr, int bgr_step,
                            uchar* rgb, int rgb_step, Size2i size );
#define _fi_icvCvt_RGB2BGR_8u_C3R _fi_icvCvt_BGR2RGB_8u_C3R

void _fi_icvCvt_BGRA2RGBA_8u_C4R( const uchar* bgra, int bgra_step,
                              uchar* rgba, int rgba_step, Size2i size );
#define _fi_icvCvt_RGBA2BGRA_8u_C4R _fi_icvCvt_BGRA2RGBA_8u_C4R

void _fi_icvCvt_BGR5552Gray_8u_C2C1R( const uchar* bgr555, int bgr555_step,
                                  uchar* gray, int gray_step, Size2i size );
void _fi_icvCvt_BGR5652Gray_8u_C2C1R( const uchar* bgr565, int bgr565_step,
                                  uchar* gray, int gray_step, Size2i size );
void _fi_icvCvt_BGR5552BGR_8u_C2C3R( const uchar* bgr555, int bgr555_step,
                                 uchar* bgr, int bgr_step, Size2i size );
void _fi_icvCvt_BGR5652BGR_8u_C2C3R( const uchar* bgr565, int bgr565_step,
                                 uchar* bgr, int bgr_step, Size2i size );

void  _fi_FillGrayPalette( PaletteEntry* palette, int bpp, bool negative = false );
bool  _fi_IsColorPalette( PaletteEntry* palette, int bpp );
void  _fi_CvtPaletteToGray( const PaletteEntry* palette, uchar* grayPalette, int entries );
uchar* _fi_FillUniColor( uchar* data, uchar*& line_end, int step, int width3,
                     int& y, int height, int count3, PaletteEntry clr );
uchar* _fi_FillUniGray( uchar* data, uchar*& line_end, int step, int width3,
                     int& y, int height, int count3, uchar clr );

uchar* _fi_FillColorRow8( uchar* data, uchar* indices, int len, PaletteEntry* palette );
uchar* _fi_FillGrayRow8( uchar* data, uchar* indices, int len, uchar* palette );
uchar* _fi_FillColorRow4( uchar* data, uchar* indices, int len, PaletteEntry* palette );
uchar* _fi_FillGrayRow4( uchar* data, uchar* indices, int len, uchar* palette );
uchar* _fi_FillColorRow1( uchar* data, uchar* indices, int len, PaletteEntry* palette );
uchar* _fi_FillGrayRow1( uchar* data, uchar* indices, int len, uchar* palette );


#endif/*_UTILS_H_*/
