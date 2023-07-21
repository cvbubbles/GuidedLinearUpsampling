
#ifndef _GRFMT_BMP_H_
#define _GRFMT_BMP_H_

#include "iobase.h"
#include "utils.h"

enum BmpCompression
{
    BMP_RGB = 0,
    BMP_RLE8 = 1,
    BMP_RLE4 = 2,
    BMP_BITFIELDS = 3
};


// Windows Bitmap reader
class BmpReader : public ImageReader
{
	//struct PaletteEntry 
	//{
	//	unsigned char b, g, r, a;
	//};
public:
    
    BmpReader( const char* filename );
    ~BmpReader();
    
    bool  ReadData( uchar* data, int step, int color );
    bool  ReadHeader();
    void  Close();
	virtual int Channels() ;
protected:
    
    RLByteStream    m_strm;
    int             m_bpp;
    int             m_offset;
    BmpCompression  m_rle_code;
	PaletteEntry    *m_palette;
};


// ... writer
class BmpWriter : public ImageWriter
{
public:
    
    BmpWriter( const char* filename );
    ~BmpWriter();
    
    bool  WriteImage( const uchar* data, int step,
                      int width, int height, int channels );
protected:

    WLByteStream  m_strm;
};

#endif/*_GRFMT_BMP_H_*/
