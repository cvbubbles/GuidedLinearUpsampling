
#include "bmpio.h"
#include <assert.h>
#include <string.h>
#include <memory.h>
//
static const char* fmtSignBmp = "BM";

/************************ BMP reader *****************************/

BmpReader::BmpReader( const char* filename )
: ImageReader( filename )
{
	m_palette=new PaletteEntry[256];
    m_offset = -1;
}


BmpReader::~BmpReader()
{
	delete[]m_palette;
}


void  BmpReader::Close()
{
    m_strm.Close();
    ImageReader::Close();
}

int BmpReader::Channels()
{
	return !m_iscolor? 1:
		m_bpp==32? 4:3;
}

bool  BmpReader::ReadHeader()
{
    bool result = false;
    
    assert( strlen(m_filename) != 0 );
    if( !m_strm.Open( m_filename )) return false;

   // if( setjmp( m_strm.JmpBuf()) == 0 )
    {
        m_strm.Skip( 10 );
        m_offset = m_strm.GetDWord();

        int  size = m_strm.GetDWord();

        if( size >= 36 )
        {
            m_width  = m_strm.GetDWord();
            m_height = m_strm.GetDWord();
            m_bpp    = m_strm.GetDWord() >> 16;
            m_rle_code = (BmpCompression)m_strm.GetDWord();
            m_strm.Skip(12);
            int clrused = m_strm.GetDWord();
            m_strm.Skip( size - 36 );

            if( m_width > 0 && m_height > 0 &&
             (((m_bpp == 1 || m_bpp == 4 || m_bpp == 8 ||
                m_bpp == 24 || m_bpp == 32 ) && m_rle_code == BMP_RGB) ||
               (m_bpp == 16 && (m_rle_code == BMP_RGB || m_rle_code == BMP_BITFIELDS)) ||
               (m_bpp == 4 && m_rle_code == BMP_RLE4) ||
               (m_bpp == 8 && m_rle_code == BMP_RLE8))) 
            {
                m_iscolor = true;
                result = true;

                if( m_bpp <= 8 )
                {
                    memset( m_palette, 0, sizeof(m_palette[0])*256);
                    m_strm.GetBytes( m_palette, (clrused == 0? 1<<m_bpp : clrused)*4 );
                    m_iscolor = _fi_IsColorPalette( m_palette, m_bpp );
                } 
                else if( m_bpp == 16 && m_rle_code == BMP_BITFIELDS )
                {
                    int redmask = m_strm.GetDWord();
                    int greenmask = m_strm.GetDWord();
                    int bluemask = m_strm.GetDWord();

                    if( bluemask == 0x1f && greenmask == 0x3e0 && redmask == 0x7c00 )
                        m_bpp = 15;
                    else if( bluemask == 0x1f && greenmask == 0x7e0 && redmask == 0xf800 )
                        ;
                    else
                        result = false;
                }
                else if( m_bpp == 16 && m_rle_code == BMP_RGB )
                    m_bpp = 15;
            }
        }
        else if( size == 12 )
        {
            m_width  = m_strm.GetWord();
            m_height = m_strm.GetWord();
            m_bpp    = m_strm.GetDWord() >> 16;
            m_rle_code = BMP_RGB;

            if( m_width > 0 && m_height > 0 &&
               (m_bpp == 1 || m_bpp == 4 || m_bpp == 8 ||
                m_bpp == 24 || m_bpp == 32 )) 
            {
                if( m_bpp <= 8 )
                {
                    uchar buffer[256*3];
                    int j, clrused = 1 << m_bpp;
                    m_strm.GetBytes( buffer, clrused*3 );
                    for( j = 0; j < clrused; j++ )
                    {
                        m_palette[j].b = buffer[3*j+0];
                        m_palette[j].g = buffer[3*j+1];
                        m_palette[j].r = buffer[3*j+2];
                    }
                }
                result = true;
            }
        }
    }

    if( !result )
    {
        m_offset = -1;
        m_width = m_height = -1;
        m_strm.Close();
    }
    return result;
}


bool  BmpReader::ReadData( uchar* data, int step, int color )
{
    const  int buffer_size = 1 << 12;
    uchar  buffer[buffer_size];
    uchar  bgr_buffer[buffer_size];
    uchar  gray_palette[256];
    bool   result = false;
    uchar* src = buffer;
    uchar* bgr = bgr_buffer;
    int  src_pitch = ((m_width*(m_bpp != 15 ? m_bpp : 16) + 7)/8 + 3) & -4;
    int  nch = color ? 3 : 1;
    int  width3 = m_width*nch;
    int  y;

    if( m_offset < 0 || !m_strm.IsOpened())
        return false;
    
    data += (m_height - 1)*step;
    step = -step;

    if( (m_bpp != 24 || !color) && src_pitch+32 > buffer_size )
        src = new uchar[src_pitch+32];

    if( !color )
    {
        if( m_bpp <= 8 )
        {
            _fi_CvtPaletteToGray( m_palette, gray_palette, 1 << m_bpp );
        }
        if( m_width*3 + 32 > buffer_size ) bgr = new uchar[m_width*3 + 32];
    }
    
   // if( setjmp( m_strm.JmpBuf()) == 0 )
    {
        m_strm.SetPos( m_offset );
        
        switch( m_bpp )
        {
        /************************* 1 BPP ************************/
        case 1:
            for( y = 0; y < m_height; y++, data += step )
            {
                m_strm.GetBytes( src, src_pitch );
                _fi_FillColorRow1( color ? data : bgr, src, m_width, m_palette );
                if( !color )
                    _fi_icvCvt_BGR2Gray_8u_C3C1R( bgr, 0, data, 0, Size2i(m_width,1) );
            }
            result = true;
            break;
        
        /************************* 4 BPP ************************/
        case 4:
            if( m_rle_code == BMP_RGB )
            {
                for( y = 0; y < m_height; y++, data += step )
                {
                    m_strm.GetBytes( src, src_pitch );
                    if( color )
                        _fi_FillColorRow4( data, src, m_width, m_palette );
                    else
                        _fi_FillGrayRow4( data, src, m_width, gray_palette );
                }
                result = true;
            }
            else if( m_rle_code == BMP_RLE4 ) // rle4 compression
            {
                uchar* line_end = data + width3;
                y = 0;

                for(;;)
                {
                    int code = m_strm.GetWord();
                    int len = code & 255;
                    code >>= 8;
                    if( len != 0 ) // encoded mode
                    {
                        PaletteEntry clr[2];
                        uchar gray_clr[2];
                        int t = 0;
                        
                        clr[0] = m_palette[code >> 4];
                        clr[1] = m_palette[code & 15];
                        gray_clr[0] = gray_palette[code >> 4];
                        gray_clr[1] = gray_palette[code & 15];

                        uchar* end = data + len*nch;
                        if( end > line_end ) goto decode_rle4_bad;
                        do
                        {
                            if( color )
                                WRITE_PIX( data, clr[t] );
                            else
                                *data = gray_clr[t];
                            t ^= 1;
                        }
                        while( (data += nch) < end );
                    }
                    else if( code > 2 ) // absolute mode
                    {
                        if( data + code*nch > line_end ) goto decode_rle4_bad;
                        m_strm.GetBytes( src, (((code + 1)>>1) + 1) & -2 );
                        if( color )
                            data = _fi_FillColorRow4( data, src, code, m_palette );
                        else
                            data = _fi_FillGrayRow4( data, src, code, gray_palette );
                    }
                    else
                    {
                        int x_shift3 = (int)(line_end - data);
                        int y_shift = m_height - y;

                        if( code == 2 )
                        {
                            x_shift3 = m_strm.GetByte()*nch;
                            y_shift = m_strm.GetByte();
                        }

                        len = x_shift3 + (y_shift * width3) & ((code == 0) - 1);

                        if( color )
                            data = _fi_FillUniColor( data, line_end, step, width3, 
                                                 y, m_height, x_shift3,
                                                 m_palette[0] );
                        else
                            data = _fi_FillUniGray( data, line_end, step, width3, 
                                                y, m_height, x_shift3,
                                                gray_palette[0] );

                        if( y >= m_height )
                            break;
                    }
                }

                result = true;
decode_rle4_bad: ;
            }
            break;

        /************************* 8 BPP ************************/
        case 8:
            if( m_rle_code == BMP_RGB )
            {
                for( y = 0; y < m_height; y++, data += step )
                {
                    m_strm.GetBytes( src, src_pitch );
                    if( color )
                        _fi_FillColorRow8( data, src, m_width, m_palette );
                    else
                        _fi_FillGrayRow8( data, src, m_width, gray_palette );
                }
                result = true;
            }
            else if( m_rle_code == BMP_RLE8 ) // rle8 compression
            {
                uchar* line_end = data + width3;
                int line_end_flag = 0;
                y = 0;

                for(;;)
                {
                    int code = m_strm.GetWord();
                    int len = code & 255;
                    code >>= 8;
                    if( len != 0 ) // encoded mode
                    {
                        int prev_y = y;
                        len *= nch;
                        
                        if( data + len > line_end )
                            goto decode_rle8_bad;

                        if( color )
                            data = _fi_FillUniColor( data, line_end, step, width3, 
                                                 y, m_height, len,
                                                 m_palette[code] );
                        else
                            data = _fi_FillUniGray( data, line_end, step, width3, 
                                                y, m_height, len,
                                                gray_palette[code] );

                        line_end_flag = y - prev_y;
                    }
                    else if( code > 2 ) // absolute mode
                    {
                        int prev_y = y;
                        int code3 = code*nch;
                        
                        if( data + code3 > line_end )
                            goto decode_rle8_bad;
                        m_strm.GetBytes( src, (code + 1) & -2 );
                        if( color )
                            data = _fi_FillColorRow8( data, src, code, m_palette );
                        else
                            data = _fi_FillGrayRow8( data, src, code, gray_palette );

                        line_end_flag = y - prev_y;
                    }
                    else
                    {
                        int x_shift3 = (int)(line_end - data);
                        int y_shift = m_height - y;
                        
                        if( code || !line_end_flag || x_shift3 < width3 )
                        {
                            if( code == 2 )
                            {
                                x_shift3 = m_strm.GetByte()*nch;
                                y_shift = m_strm.GetByte();
                            }

                            x_shift3 += (y_shift * width3) & ((code == 0) - 1);

                            if( y >= m_height )
                                break;

                            if( color )
                                data = _fi_FillUniColor( data, line_end, step, width3, 
                                                     y, m_height, x_shift3,
                                                     m_palette[0] );
                            else
                                data = _fi_FillUniGray( data, line_end, step, width3, 
                                                    y, m_height, x_shift3,
                                                    gray_palette[0] );

                            if( y >= m_height )
                                break;
                        }

                        line_end_flag = 0;
                    }
                }

                result = true;
decode_rle8_bad: ;
            }
            break;
        /************************* 15 BPP ************************/
        case 15:
            for( y = 0; y < m_height; y++, data += step )
            {
                m_strm.GetBytes( src, src_pitch );
                if( !color )
                    _fi_icvCvt_BGR5552Gray_8u_C2C1R( src, 0, data, 0, Size2i(m_width,1) );
                else
                    _fi_icvCvt_BGR5552BGR_8u_C2C3R( src, 0, data, 0, Size2i(m_width,1) );
            }
            result = true;
            break;
        /************************* 16 BPP ************************/
        case 16:
            for( y = 0; y < m_height; y++, data += step )
            {
                m_strm.GetBytes( src, src_pitch );
                if( !color )
                    _fi_icvCvt_BGR5652Gray_8u_C2C1R( src, 0, data, 0, Size2i(m_width,1) );
                else
                    _fi_icvCvt_BGR5652BGR_8u_C2C3R( src, 0, data, 0, Size2i(m_width,1) );
            }
            result = true;
            break;
        /************************* 24 BPP ************************/
        case 24:
            for( y = 0; y < m_height; y++, data += step )
            {
                m_strm.GetBytes( color ? data : src, src_pitch );
                if( !color )
                    _fi_icvCvt_BGR2Gray_8u_C3C1R( src, 0, data, 0, Size2i(m_width,1) );
            }
            result = true;
            break;
        /************************* 32 BPP ************************/
        case 32:
            for( y = 0; y < m_height; y++, data += step )
            {
                if( !color )
				{
					m_strm.GetBytes( src, src_pitch );
					_fi_icvCvt_BGRA2Gray_8u_C4C1R( src, 0, data, 0, Size2i(m_width,1) );
				}
				else
					m_strm.GetBytes(data,src_pitch);
         //       else
           //         icvCvt_BGRA2BGR_8u_C4C3R( src, 0, data, 0, Size2i(m_width,1) );
            }
            result = true;
            break;
        default:
            assert(0);
        }
    }

    if( src != buffer ) delete[] src;
    if( bgr != bgr_buffer ) delete[] bgr;
    return result;
}


//////////////////////////////////////////////////////////////////////////////////////////

BmpWriter::BmpWriter( const char* filename ) : ImageWriter( filename )
{
}


BmpWriter::~BmpWriter()
{
}


bool  BmpWriter::WriteImage( const uchar* data, int step,
                                  int width, int height, int channels )
{
    bool result = false;
    int fileStep = (width*channels + 3) & -4;
    uchar zeropad[] = "\0\0\0\0";

    assert( data && width > 0 && height > 0 && step >= fileStep);
    
    if( m_strm.Open( m_filename ) )
    {
        int  bitmapHeaderSize = 40;
        int  paletteSize = channels > 1 ? 0 : 1024;
        int  headerSize = 14 /* fileheader */ + bitmapHeaderSize + paletteSize;
        PaletteEntry palette[256];
        
        // write signature 'BM'
        m_strm.PutBytes( fmtSignBmp, (int)strlen(fmtSignBmp) );

        // write file header
        m_strm.PutDWord( fileStep*height + headerSize ); // file size
        m_strm.PutDWord( 0 );
        m_strm.PutDWord( headerSize );

        // write bitmap header
        m_strm.PutDWord( bitmapHeaderSize );
        m_strm.PutDWord( width );
        m_strm.PutDWord( height );
        m_strm.PutWord( 1 );
        m_strm.PutWord( channels << 3 );
        m_strm.PutDWord( BMP_RGB );
        m_strm.PutDWord( 0 );
        m_strm.PutDWord( 0 );
        m_strm.PutDWord( 0 );
        m_strm.PutDWord( 0 );
        m_strm.PutDWord( 0 );

        if( channels == 1 )
        {
            _fi_FillGrayPalette( palette, 8 );
            m_strm.PutBytes( palette, sizeof(palette));
        }

        width *= channels;
        data += step*(height - 1);
        for( ; height--; data -= step )
        {
            m_strm.PutBytes( data, width );
            if( fileStep > width )
                m_strm.PutBytes( zeropad, fileStep - width );
        }

        m_strm.Close();
        result = true;
    }
    return result;
}


