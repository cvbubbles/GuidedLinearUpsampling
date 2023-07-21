
#include "iobase.h"
#include "bitstrm.h"
#include <assert.h>
#include <string.h>
#include <memory.h>
#include <ctype.h>

ImageReader::ImageReader( const char* filename )
{
	size_t slen=strlen(filename);
	m_filename=new char[slen+1];
	memcpy(m_filename,filename,slen+1);
    // strncpy( m_filename, filename, sizeof(m_filename) - 1 );
    //m_filename[sizeof(m_filename)-1] = '\0';
    m_width = m_height = 0;
    m_iscolor = false;
}


ImageReader::~ImageReader()
{
	delete[]m_filename;
	m_filename=NULL;
    Close();
}


void ImageReader::Close()
{
    m_width = m_height = 0;
    m_iscolor = false;
}


ImageWriter::ImageWriter( const char* filename )
{
	size_t slen=strlen(filename);
	m_filename=new char[slen+1];
	memcpy(m_filename,filename,slen+1);
}

ImageWriter::~ImageWriter()
{
	delete[]m_filename;
}

