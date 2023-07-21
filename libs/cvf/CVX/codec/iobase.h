

#ifndef _GRFMT_BASE_H_
#define _GRFMT_BASE_H_

#include "bitstrm.h"


///////////////////////////////// base class for readers ////////////////////////
class   ImageReader
{
public:
    
    ImageReader( const char* filename );
    virtual ~ImageReader();

    int   GetWidth()  { return m_width; };
    int   GetHeight() { return m_height; };
    bool  IsColor()   { return m_iscolor; };
    
    virtual bool  ReadHeader() = 0;
    virtual bool  ReadData( uchar* data, int step, int color ) = 0;
    virtual void  Close();
	virtual int     Channels() =0;
protected:

    bool    m_iscolor;
    int     m_width;    // width  of the image ( filled by ReadHeader )
    int     m_height;   // height of the image ( filled by ReadHeader )
    char    *m_filename; // filename
};


///////////////////////////// base class for writers ////////////////////////////
class   ImageWriter
{
public:

    ImageWriter( const char* filename );
    virtual ~ImageWriter();

    virtual bool  WriteImage( const uchar* data, int step,
                              int width, int height,
                              int channels ) = 0;
protected:
    char    *m_filename; // filename
};


#endif/*_GRFMT_BASE_H_*/
