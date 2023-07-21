
#include<assert.h>

#ifdef _WIN32
#include<io.h>
#else
#include<sys/stat.h>
#include <unistd.h>
#include <sys/types.h>
#endif

#include"BFC/bfstream.h"
#include "BFC/autores.h"

#include"BFC/cfg.h"
#include"auto_tchar.h"


#pragma warning(disable:4996)

_FF_BEG

void BFileStreamBuffer::Seek(PosType offset,int origin)
{
	if(fseek(_fp,offset,origin)!=0)
		FF_EXCEPTION(ERR_FILE_OP_FAILED,"");
}

void BFileStreamBuffer::Flush()
{
	if(fflush(_fp)!=0)
		FF_EXCEPTION(ERR_FILE_OP_FAILED,"");
}

void BFileStreamBuffer::Close()
{
	if(_fp&&fclose(_fp)!=0)
		FF_EXCEPTION(ERR_FILE_OP_FAILED,"");
	_fp=NULL;
}

long BFileStreamBuffer::Size() const
{
	long size=0;

	if(_fp)
	{
		PosType pos=this->Tell();

		BFileStreamBuffer *_this=(BFileStreamBuffer*)this; //de-const
		_this->Flush();
		
		_this->Seek(0, SEEK_END);
		size=this->Tell();

		_this->Seek(pos,SEEK_SET);
	}
	return size;
}
void BFileStreamBuffer::Resize(long size)
{
	if(_fp)
	{
		this->Flush();
#ifndef _WIN32
        int fd=fileno(_fp);
        if(ftruncate(fd,size)!=0)
            FF_EXCEPTION(ERR_FILE_OP_FAILED,"");
#else
        
        int fd=_fileno(_fp);
		if(chsize(fd,size)!=0)
			FF_EXCEPTION(ERR_FILE_OP_FAILED,"");
#endif
	}
}
void BFileStreamBuffer::Swap(BFileStreamBuffer &right)
{
	std::swap(_fp,right._fp);
}
BFileStreamBuffer::~BFileStreamBuffer()
{
	this->Close();
}

BFileStreamBuffer::BFileStreamBuffer()
:_fp(NULL)
{
}
void BFileStreamBuffer::_open(const string_t& file,const char_t *mode)
{
	FILE *fpx=_tfopen(file.c_str(),mode);
	if(!fpx)
		FF_EXCEPTION(ERR_FILE_OPEN_FAILED,file.c_str());

	if(this->IsOpen())
		this->Close();

	_fp=fpx;
	_file=file;
}
void BFileStreamBuffer::_read(void *buf, size_t sz, size_t count)
{
	if(fread(buf,sz,count,_fp)!=count)
		FF_EXCEPTION(ERR_FILE_READ_FAILED,_file.c_str());
}
void BFileStreamBuffer::_write(const void *buf, size_t sz, size_t count)
{
	size_t ret=fwrite(buf,sz,count,_fp);
	if(ret!=count)
		FF_EXCEPTION(ERR_FILE_WRITE_FAILED,_file.c_str());
	//	FF_EXCEPTION(ERR_FILE_WRITE_FAILED, StrFormat(_TX("%s: size=%d, count=%d/%d"), _file.c_str(), (int)sz, (int)ret, (int)count).c_str() ) ;
}

//============================================================================
BMemoryStreamBuffer::BMemoryStreamBuffer()
{
	m_pbuf=m_pcur=m_peof=m_pend=m_rbuf=NULL;
	m_block_size=1024*10;
}
BMemoryStreamBuffer::~BMemoryStreamBuffer()
{
	this->Close();
}
BMemoryStreamBuffer::BMemoryStreamBuffer(const BMemoryStreamBuffer &right)
{
	m_block_size=right.m_block_size;
	
	if(right.Size()>0)
	{
		long size=right.Size();
		m_pbuf=new char[size];
		memcpy(m_pbuf,right.m_pbuf,size);
		m_pcur=m_rbuf=m_pbuf;
		m_peof=m_pend=m_pbuf+size;
	}
	else
		m_pbuf=m_pcur=m_peof=m_pend=m_rbuf=NULL;
}
BMemoryStreamBuffer& BMemoryStreamBuffer::operator =(const BMemoryStreamBuffer &right)
{
	if(this!=&right)
	{
		BMemoryStreamBuffer temp(right);
		this->Swap(temp);
	}
	return *this;
}

void BMemoryStreamBuffer::Seek(PosType offset,int origin)
{
	char *px=NULL;
	switch(origin)
	{
	case SEEK_END:
		px=m_peof+offset;
		break;
	case SEEK_CUR:
		px=m_pcur+offset;
		break;
	default:
		px=m_pbuf+offset;
	}
	if(!(px>=m_pbuf&&px<=m_peof))
		FF_EXCEPTION(ERR_FILE_OP_FAILED,"");
	else
		m_pcur=px;
}
void BMemoryStreamBuffer::Clear()
{
	delete[]m_rbuf;

	m_pbuf=m_pcur=m_peof=m_pend=m_rbuf=NULL;
}
void BMemoryStreamBuffer::Close()
{
	this->Clear();
}
long BMemoryStreamBuffer::Size() const
{
	return (long)(m_peof-m_pbuf);
}
void BMemoryStreamBuffer::Resize(long size)
{
	if(size>=0)
	{
		if(m_pbuf+size>m_pend)
		{
			this->_realloc(size+m_block_size);
		}
		
		this->_resize(size);
	}
}
void BMemoryStreamBuffer::ReallocBuffer(long size)
{
	this->_realloc(size);
}
void BMemoryStreamBuffer::Swap(BMemoryStreamBuffer &right)
{
	std::swap(m_pbuf,right.m_pbuf);
	std::swap(m_pcur,right.m_pcur);
	std::swap(m_peof,right.m_peof);
	std::swap(m_pend,right.m_pend);
	std::swap(m_rbuf,right.m_rbuf);
	std::swap(m_block_size,right.m_block_size);
}
void BMemoryStreamBuffer::SetBuffer(void *pbuf, long size, bool release)
{
	delete[]m_rbuf;
	
	m_pbuf=m_pcur=(char*)pbuf;
	m_peof=m_pend=m_pbuf+size;

	m_rbuf=release? m_pbuf:NULL;
}
void BMemoryStreamBuffer::_realloc(long size)
{
	if(size>this->Size())
	{
		char *pbuf=new char[size];
		
		if(m_peof!=m_pbuf)
			memcpy(pbuf,m_pbuf,this->Size());

		m_pend=pbuf+size;
		m_peof=pbuf+this->Size();
		m_pcur=pbuf+(m_pcur-m_pbuf);

		delete[]m_rbuf;
		m_pbuf=m_rbuf=pbuf;
	}
}
void BMemoryStreamBuffer::_incsize(long inc)
{
	inc = __max(inc, this->Size());
	this->_realloc(this->Size()+inc/*+m_block_size*/);
}
void BMemoryStreamBuffer::_write(const void* buf,size_t sz,size_t count)
{
	if(buf)
	{
		size_t csz=sz*count;

		if(m_pcur+csz>m_pend)
			this->_incsize((long)csz);
		memcpy(m_pcur,buf,csz);

		long new_size=(long)(m_pcur-m_pbuf+csz);
		if(new_size>this->Size())
			this->_resize(new_size);
		m_pcur+=csz;
	}
}
void BMemoryStreamBuffer::_read(void* buf,size_t sz,size_t count)
{
	if(buf)
	{
		long csz=(long)(sz*count);

		if(m_pcur+csz>m_peof)
			FF_EXCEPTION(ERR_FILE_READ_FAILED,"");
		else
		{
			memcpy(buf,m_pcur,csz);
			m_pcur+=csz;
		}
	}
}

//============================================================================

IBFStream::IBFStream()
{
}
IBFStream::IBFStream(const string_t& fn)
{
	this->Open(fn);
}
void IBFStream::Open(const string_t& fn)
{
	this->_open(fn,_TX("rb"));
}


OBFStream::OBFStream()
{
}
OBFStream::OBFStream(const string_t& fn,bool bNew)
{
	this->Open(fn,bNew);
}
void OBFStream::Open(const string_t& fn,bool bNew)
{
	if(bNew)
		this->_open(fn,_TX("wb+"));
	else
		this->_open(fn,_TX("rb+"));
}


BFStream::BFStream()
{
}
BFStream::BFStream(const string_t& fn,bool destroy)
{
	this->Open(fn,destroy);
}


size_t _BFC_API BFSCopy(IBFStream &ibs,BFStream::PosType ibeg,
					  OBFStream &obs,BFStream::PosType obeg,
					  size_t  size, size_t bufSize
					  )
{
	size_t isz=ibs.Size(),osz=obs.Size();
	if(size_t(ibeg)>isz||obeg<0)
		return 0;
	if(size==0) //copy all. 
		size=isz-ibeg; 
	else
		size=__min(size_t(isz-ibeg),size);

	if(obeg+size>osz)
		obs.Resize((long)(obeg+size));
	
	ibs.Seek(ibeg,SEEK_SET);
	obs.Seek(obeg,SEEK_SET);

	AutoArrayPtr<char> buf(new char[bufSize]);
	char *pb=&buf[0];

	for(size_t n=0;n<size;n+=bufSize)
	{
		size_t rsz=__min(bufSize,size-n);
		ibs.Read(buf,rsz,1);
		obs.Write(buf,rsz,1);
	}
	return size;
}

_FF_END


