
#ifndef _FF_BFC_BFSTREAM_H
#define _FF_BFC_BFSTREAM_H

#include<stdio.h>
#include<string>
#include<map>
#include<deque>
#include<list>
#include<vector>

#include"BFC/ctc.h"
#include"BFC/err.h"

_FF_BEG

typedef uint32 bfs_size_t;

class _BFC_API BStreamBase
{
public:
	typedef long PosType,_PosT;
};

class _BFC_API BFileStreamBuffer
	:public BStreamBase
{
public:
	//tell current pos.
	PosType Tell() const
	{
		return ftell(_fp);
	}
	//move file pointer.
	void Seek(PosType offset,int origin);
	//whether is at the end-of-file.
	bool IsEOF() const
	{
		return feof(_fp)!=0;
	}
	bool IsGood() const
	{
		return _fp!=NULL&&!ferror(_fp);
	}
	bool IsOpen() const
	{
		return _fp!=0;
	}
	void Flush();
	
	void Close();
	
	operator bool() const
	{
		return IsGood();
	}
	//size of the file in bytes.
	long Size() const;

	void Resize(long size) ;

	void Swap(BFileStreamBuffer &right);

	int  Get()
	{
		return fgetc(_fp);
	}
	bool Empty() const
	{
		return this->Size()<=0;
	}
public:

	 BFileStreamBuffer();

	~BFileStreamBuffer();

protected:
	void _open(const string_t& file,const char_t * mode);

	void _read(void* buf,size_t sz,size_t count);
	
	void _write(const void* buf,size_t sz,size_t count);

private:
	BFileStreamBuffer(const BFileStreamBuffer&);
	BFileStreamBuffer& operator=(const BFileStreamBuffer&);

protected:
	FILE		*_fp;
	string_t	 _file;
};



class _BFC_API BMemoryStreamBuffer
	:public BStreamBase
{
public:
	//tell current pos.
	PosType Tell() const
	{
		return PosType(m_pcur-m_pbuf);
	}
	//move file pointer.
	void Seek(PosType offset,int origin);
	//whether is at the end-of-file.
	bool IsEOF() const
	{
		return m_pcur==m_peof;
	}
	bool IsGood() const
	{
		return m_pcur&&m_pcur<=m_peof;
	}
	bool IsOpen() const
	{
		return m_pcur!=NULL;
	}
	void Flush()
	{
	}
	
	void Close();

	void Clear();
	
	operator bool() const
	{
		return IsGood();
	}
	//size of the file in bytes.
	long Size() const ;

	void Resize(long size) ;

	void ReallocBuffer(long size);

	void Swap(BMemoryStreamBuffer &right);

	void SetBuffer(void *pbuf, long size, bool release);

	const char* Buffer() const
	{
		return m_pbuf;
	}
	void SetBlockSize(long size)
	{
		m_block_size=size;
	}
	int Get() 
	{
		return m_pcur<m_peof? *m_pcur++:EOF;
	}
	bool Empty() const
	{
		return this->Size()<=0;
	}
public:
	
	~BMemoryStreamBuffer();

	BMemoryStreamBuffer();

	BMemoryStreamBuffer(const BMemoryStreamBuffer &right);

	BMemoryStreamBuffer& operator=(const BMemoryStreamBuffer &right);

protected:
	void _realloc(long size);

	void _incsize(long inc);

	void _resize(long size)
	{
		m_peof=m_pbuf+size;
		if(m_pcur>m_peof)
			m_pcur=m_peof;
	}

	void _read(void* buf,size_t sz,size_t count);
	
	void _write(const void* buf,size_t sz,size_t count);
protected:
	char		*m_pbuf,*m_pcur,*m_peof,*m_pend;
	char		*m_rbuf;
	long		 m_block_size;
};

template<typename _BufferT>
class IBStream
	:virtual public _BufferT
{
	typedef _BufferT _MyBaseT;
public:
	void Read(void *buf,size_t size,size_t count)
	{
		_MyBaseT::_read(buf,size,count);
	}
	void ReadAt(void *buf,BStreamBase::PosType pos,size_t size,size_t count)
	{
		typename _MyBaseT::PosType cur=this->Tell();
		this->Seek(pos,SEEK_SET);
		this->Read(buf,size,count);
		this->Seek(cur,SEEK_SET);
	}
	//read a c-style string.
	//@_max=>The size of @buf.
	//@RV=>The size of the string stored in file, the terminating null character is not counted in.
	//(@RV>@_max)=>@buf is not large enough, the string is chopped.
	//(@buf==0&&_max==0)=>Get the size of the string.
	size_t ReadStr(char *buf,size_t _max)
	{
		assert(buf||_max==0);

		char* ptr=buf;
		size_t count=0;
		for(;count<_max;++count,++ptr)
			if((*ptr=this->Get())=='\0'||*ptr==EOF)
				break;
		if(count==_max)
		{
			if(buf)
				*(ptr-1)='\0';
			int ch;
			//to count the size of the string.
			while((ch=this->Get())&&ch!=EOF)
				++count;
			if(ch==EOF)
				FF_EXCEPTION(ERR_FILE_READ_FAILED,"");
		}
		else
			if(*ptr==EOF)
				FF_EXCEPTION(ERR_FILE_READ_FAILED,"");

		return count;
	}

	size_t ReadStrAt(char* buf,size_t _max,BStreamBase::PosType pos)
	{
		BStreamBase::PosType cur=this->Tell();
		this->Seek(pos,SEEK_SET);
		size_t rsz=this->ReadStr(buf,_max);
		this->Seek(cur,SEEK_SET);
		return rsz;
	}

	//read a c-style string.
	void ReadStr(std::string& str)
	{
		typename _MyBaseT::PosType  pos(this->Tell());
		size_t sz=this->ReadStr(0,0)+1;
		if(sz>1)
		{
			str.resize(sz);
			this->ReadStrAt(&str[0],sz,pos);
			str.resize(sz-1);
		}
		else
			str.resize(0);
	}
	
	void ReadStrAt(std::string& str,BStreamBase::PosType pos)
	{
		BStreamBase::PosType cur=this->Tell();
		this->Seek(pos,SEEK_SET);
		this->ReadStr(str);
		this->Seek(cur,SEEK_SET);
	}

	template<typename _T>
	_T Read() 
	{
		_T v;
		this->Read(&v,sizeof(_T),1);
		return v;
	}

	void ReadBlock(BMemoryStreamBuffer &buf)
	{
		this->_read_block(buf,this->Read<bfs_size_t>());
	}
	void Load(BMemoryStreamBuffer &buf)
	{
		this->Seek(0,SEEK_SET);
		this->_read_block(buf,this->Size());
	}

	bool HeadMatched(const std::string& headStr)
	{
		typename _MyBaseT::PosType cur = this->Tell();
		std::string str;
		bfs_size_t headSize = sizeof(bfs_size_t) + headStr.size();
		bfs_size_t size;

		bool rv = false;
		if (this->Size() < headSize)
			goto _EXIT;

		this->Seek(0, SEEK_SET);

		this->Read(&size,sizeof(size),1);
		if (size != headStr.size())
			goto _EXIT;

		if (headStr.empty())
		{
			rv = true;
			goto _EXIT;
		}

		str.resize(size);
		this->Read(&str[0], size, 1);
		if (str != headStr)
			goto _EXIT;
		rv = true;
		
	_EXIT:
		this->Seek(cur, SEEK_SET);
		return rv;
	}

protected:
	void _read_block(BMemoryStreamBuffer &buf, long size)
	{
		if(size>0)
		{
			char *pbuf=new char[size];
			this->Read(pbuf,size,1);
			buf.SetBuffer(pbuf,size,true);
		}
		else
			buf.SetBuffer(NULL,0,true);
	}
};

class _BFC_API IBMStream
	:public IBStream<BMemoryStreamBuffer>
{
};

class _BFC_API IBFStream
	:public IBStream<BFileStreamBuffer>
{
public:
	IBFStream();
	IBFStream(const string_t& fn);

	void Open(const string_t& fn);

protected:
	void _read_block(BMemoryStreamBuffer &buf, long size);
};




template<typename _IBST,typename _ValT>
inline void _ibfs_dist_unsafe(_IBST &is,_ValT &val,FlagType<true>)
{
//	is.Read(&val,sizeof(_ValT),1);
#ifndef __APPLE__
	CTCAssert(false);
#endif
    
    
}

template<typename _IBST,typename _ValT>
inline void _ibfs_dist_unsafe(_IBST &is,_ValT &val,FlagType<false>)
{
	is.Read(&val,sizeof(_ValT),1);
}

template<typename _IBST,typename _ValT>
inline void BSRead(_IBST &is,_ValT &val)
{
	_ibfs_dist_unsafe(is,val,FlagType<!IsMemcpy<_ValT>::Yes>());
}

template<typename _ValT,int _N,typename _IBST>
inline void _ibfs_read_array(_IBST &is,_ValT *pData)
{
	if(IsMemcpy<_ValT>::Yes)
		is.Read(pData,sizeof(_ValT),_N);
	else
	{
		for(int i=0;i<_N;++i)
			is>>pData[i];
	}
}
template<typename _IBST,typename _ValT,int _N>
inline void BSRead(_IBST &is,_ValT (&arr)[_N])
{
	_ibfs_read_array<_ValT,_N>(is,&arr[0]);
}
template<typename _IBST,typename _ValT,int _N>
inline void BSRead(_IBST &is,Array<_ValT,_N> &vec)
{
	_ibfs_read_array<_ValT,_N>(is,&vec[0]);
}
template<typename _IBST,typename _ValT,int _N>
inline void BSRead(_IBST &is,Vector<_ValT,_N> &vec)
{
	_ibfs_read_array<_ValT,_N>(is,&vec[0]);
}

template<typename _IBST,typename _CtrT>
inline void _ibfs_read_ctr(_IBST& is,_CtrT& ctr)
{
	bfs_size_t size = is.template Read<bfs_size_t>();
	ctr.resize(size);
	for(typename _CtrT::iterator itr(ctr.begin());itr!=ctr.end();++itr)
		is>>*itr;
}
template<typename _IBST,typename _ValT,typename _AllocT>
inline void BSRead(_IBST &is,std::vector<_ValT,_AllocT> &val)
{
	if(IsMemcpy<_ValT>::Yes)
	{
		bfs_size_t size = is.template Read<bfs_size_t>();
		val.resize(size);
		if(size!=0)
			is.Read(&val[0],sizeof(_ValT),size);
	}
	else
		_ibfs_read_ctr(is,val);
}
template<typename _IBST,typename _MapT>
inline void _ibfs_read_map(_IBST &is,_MapT &val)
{
	bfs_size_t size(is.template Read<bfs_size_t>());

	typename _MapT::key_type key;
	typedef typename _MapT::value_type::second_type _DataT;
	_DataT data;
	
	for (bfs_size_t i = 0; i<size; ++i)
	{
		is>>key;
		std::pair<typename _MapT::iterator,bool> pr(val.insert(typename _MapT::value_type(key,_DataT())));
		if(pr.second)
			is>>pr.first->second;
		else
			is>>data;
	}
}
template<typename _IBST,typename _KeyT,typename _ValT,typename _PrT,typename _AllocT>
inline void BSRead(_IBST &is,std::map<_KeyT,_ValT,_PrT,_AllocT> &val)
{
	_ibfs_read_map(is,val);
}

template<typename _IBST,typename _ValT,typename _AllocT>
inline void BSRead(_IBST &is,std::list<_ValT,_AllocT> &val)
{
	_ibfs_read_ctr(is,val);
}
template<typename _IBST,typename _ValT,typename _AllocT>
inline void BSRead(_IBST &is,std::deque<_ValT,_AllocT> &val)
{
	_ibfs_read_ctr(is,val);
}
template<typename _IBST,typename _CharT,typename _TraitsT,typename _AllocT>
inline void BSRead(_IBST &is,std::basic_string<_CharT,_TraitsT,_AllocT> &str)
{
	_ibfs_read_ctr(is,str);
}
template<typename _IBST,typename _FirstT,typename _SecondT>
inline void BSRead(_IBST &is,std::pair<_FirstT,_SecondT>& pr)
{
	is>>pr.first>>pr.second;
}

#ifndef _FVT_COMPATIBLE_BSTREAM

template<typename _BufferT,typename _T>
inline IBStream<_BufferT>& operator>>(IBStream<_BufferT>& is,_T& val)
{
	BSRead(is,val);	
	return is;
}

#else

template<typename _T>
inline IBFStream& operator>>(IBFStream& is,_T& val)
{
	BSRead(is,val);	
	return is;
}

#endif

//#if 0
template<typename _BufferT>
class OBStream
	:public virtual _BufferT
{
	typedef _BufferT _MyBaseT;
public:

	void Write(const void* buf,size_t size,size_t count)
	{
		_MyBaseT::_write(buf,size,count);
	}
	void WriteAt(const void *buf,BStreamBase::PosType pos,size_t size,size_t count)
	{
		BStreamBase::PosType cur=this->Tell();
		this->Seek(pos,SEEK_SET);
		this->Write(buf,size,count);
		this->Seek(cur,SEEK_SET);
	}

	void WriteStr(const char* str)
	{
		this->Write(str,1,strlen(str)+1);
	}
	void WriteStrAt(const char* str,BStreamBase::PosType pos)
	{
		this->WriteAt(str,pos,1,strlen(str)+1);
	}
	void WriteStr(const std::string& str)
	{
		this->WriteStr(str.c_str());
	}
	void WriteStrAt(const std::string& str,BStreamBase::PosType pos)
	{
		this->WriteStrAt(str.c_str(),pos);
	}
	void WriteBlock(const BMemoryStreamBuffer &buf)
	{
		long size=buf.Size();
		(*this)<<(bfs_size_t)size;
		this->Dump(buf);
	}
	void Dump(const BMemoryStreamBuffer &buf)
	{
		long size=buf.Size();
		if(size>0)
			this->Write(buf.Buffer(),size,1);
	}
};

class _BFC_API OBFStream
	:public OBStream<BFileStreamBuffer>
{
public:
	OBFStream();

	OBFStream(const string_t& FileName,bool bNew=true);
	//@bNew=>whether to empty the existing file and whether to create new file.
	// The file must exist if @bNew is false, else the function would fail.
	void Open(const string_t& FileName,bool bNew=true);
};

class _BFC_API OBMStream
	:public OBStream<BMemoryStreamBuffer>
{
};

template<typename _OBST,typename _ValT>
inline void 
#if _MSC_VER>=1400
__declspec(deprecated("Unsafe call to the output operator(<<) of _OBST!"))
#endif
_obfs_dist_unsafe(_OBST &os,const _ValT &val,FlagType<true>)
{
//	os.Write(&val,sizeof(val),1);
    
#ifndef __APPLE__
    CTCAssert(false);
#endif
}

template<typename _OBST,typename _ValT>
inline void _obfs_dist_unsafe(_OBST &os,const _ValT &val,FlagType<false>)
{
	os.Write(&val,sizeof(val),1);
}

template<typename _OBST,typename _ValT>
inline void BSWrite(_OBST &os,const _ValT &val)
{
	_obfs_dist_unsafe(os,val,FlagType<!IsMemcpy<_ValT>::Yes>());
}

template<typename _ValT,int _N,typename _OBST>
inline void _obfs_write_array(_OBST &os,const _ValT *pData)
{
	if(IsMemcpy<_ValT>::Yes)
		os.Write(pData,sizeof(_ValT),_N);
	else
	{
		for(int i=0;i<_N;++i)
			os<<pData[i];
	}
}
template<typename _OBST,typename _ValT,int _N>
inline void BSWrite(_OBST &os,const _ValT (&arr)[_N])
{
	_obfs_write_array<_ValT,_N>(os,&arr[0]);
}
template<typename _OBST,typename _ValT,int _N>
inline void BSWrite(_OBST &os,const Array<_ValT,_N> &vec)
{
	_obfs_write_array<_ValT,_N>(os,&vec[0]);
}
template<typename _OBST,typename _ValT,int _N>
inline void BSWrite(_OBST &os,const Vector<_ValT,_N> &vec)
{
	_obfs_write_array<_ValT,_N>(os,&vec[0]);
}

template<typename _OBST,typename _CtrT>
inline void _obfs_write_ctr(_OBST& os,const _CtrT& ctr)
{
	os << (bfs_size_t)ctr.size();
	for(typename _CtrT::const_iterator itr(ctr.begin());itr!=ctr.end();++itr)
		os<<*itr;
}
template<typename _OBST,typename _ValT,typename _AllocT>
inline void BSWrite(_OBST &os,const std::vector<_ValT,_AllocT> &val)
{
	if(IsMemcpy<_ValT>::Yes)
	{
		os << (bfs_size_t)val.size();
		if(!val.empty())
			os.Write(&val[0],sizeof(_ValT),val.size());
	}
	else
		_obfs_write_ctr(os,val);
}
template<typename _OBST,typename _ValT,typename _AllocT>
inline void BSWrite(_OBST &os,const std::list<_ValT,_AllocT> &val)
{
	_obfs_write_ctr(os,val);
}
template<typename _OBST,typename _ValT,typename _AllocT>
inline void BSWrite(_OBST &os,const std::deque<_ValT,_AllocT> &val)
{
	_obfs_write_ctr(os,val);
}
template<typename _OBST,typename _KeyT,typename _ValT,typename _PrT,typename _AllocT>
inline void BSWrite(_OBST &os,const std::map<_KeyT,_ValT,_PrT,_AllocT> &val)
{
	_obfs_write_ctr(os,val);
}
template<typename _OBST,typename _CharT,typename _TraitsT,typename _AllocT>
inline void BSWrite(_OBST &os,const std::basic_string<_CharT,_TraitsT,_AllocT> &str)
{
	_obfs_write_ctr(os,str);
}
template<typename _OBST,typename _FirstT,typename _SecondT>
inline void BSWrite(_OBST &os,const std::pair<_FirstT,_SecondT>& pr)
{
	os<<pr.first<<pr.second;
}

#ifndef _FVT_COMPATIBLE_BSTREAM

template<typename _BufferT,typename _T>
inline OBStream<_BufferT>& operator<<(OBStream<_BufferT>& os,const _T& val)
{
	BSWrite(os,val);	
	return os;
}

#else

template<typename _T>
inline OBFStream& operator<<(OBFStream& os,const _T& val)
{
	BSWrite(os,val);	
	return os;
}

#endif

class _BFC_API BFStream
	:public IBFStream,
	public OBFStream
{
public:
	BFStream();
	
	BFStream(const string_t& FileName,bool bNew=true);
	
	using OBFStream::Open;

	operator bool() const
	{
		return OBFStream::operator bool();
	}
};

class _BFC_API  BMStream
	:public IBMStream,
	public OBMStream
{
};

template<typename _IBST>
inline void BSRead(_IBST &is, IBMStream& buf)
{
	is.ReadBlock(buf);
}
template<typename _IBST>
inline void BSRead(_IBST &is, OBMStream& buf)
{
	is.ReadBlock(buf);
}
template<typename _IBST>
inline void BSRead(_IBST &is, BMStream& buf)
{
	is.ReadBlock(buf);
}

template<typename _OBST>
inline void BSWrite(_OBST &os, const IBMStream& buf)
{
	os.WriteBlock(buf);
}
template<typename _OBST>
inline void BSWrite(_OBST &os, const OBMStream& buf)
{
	os.WriteBlock(buf);
}
template<typename _OBST>
inline void BSWrite(_OBST &os, const BMStream& buf)
{
	os.WriteBlock(buf);
}

//copy contents in BFStream.
//size==0: copy all the contents from @ibeg to the end of @ibs.

size_t _BFC_API BFSCopy(IBFStream &ibs, BFStream::PosType ibeg,
	OBFStream &obs, BFStream::PosType obeg,
	size_t  size, size_t bufSize = 1024
	);
//#endif



#define BFSRead  BSRead
#define BFSWrite BSWrite

#define DEFINE_BFS_INPUT(class_name,in_seq) \
	template<typename _IBST> \
		friend void BSRead(_IBST &ibs,class_name &v) \
		{ \
			ibs>>in_seq; \
		} \
		static const char* GetInputSeq(){return #in_seq;}


#define DEFINE_BFS_OUTPUT(class_name,out_seq) \
		template<typename _OBST> \
		friend void BSWrite(_OBST &obs,const class_name &v) \
		{ \
			obs<<out_seq; \
		} \
		static const char* GetClassName() { return #class_name;} \
		static const char* GetOutputSeq(){return #out_seq;}

#define DEFINE_BFS_IO(class_name,in_seq,out_seq) \
	DEFINE_BFS_INPUT(class_name, in_seq) \
	DEFINE_BFS_OUTPUT(class_name, out_seq)


//input 
#define DEFINE_BFS_INPUT_1(class_name,m0) DEFINE_BFS_INPUT(class_name,v.m0)

#define DEFINE_BFS_INPUT_2(class_name,m0,m1) DEFINE_BFS_INPUT(class_name,v.m0>>v.m1)

#define DEFINE_BFS_INPUT_3(class_name,m0,m1,m2) DEFINE_BFS_INPUT(class_name,v.m0>>v.m1>>v.m2)

#define DEFINE_BFS_INPUT_4(class_name,m0,m1,m2,m3) DEFINE_BFS_INPUT(class_name,v.m0>>v.m1>>v.m2>>v.m3)

#define DEFINE_BFS_INPUT_5(class_name,m0,m1,m2,m3,m4) DEFINE_BFS_INPUT(class_name,v.m0>>v.m1>>v.m2>>v.m3>>v.m4)

#define DEFINE_BFS_INPUT_6(class_name,m0,m1,m2,m3,m4,m5) DEFINE_BFS_INPUT(class_name,\
				v.m0>>v.m1>>v.m2>>v.m3>>v.m4>>v.m5)

#define DEFINE_BFS_INPUT_7(class_name,m0,m1,m2,m3,m4,m5,m6) DEFINE_BFS_INPUT(class_name,\
				v.m0>>v.m1>>v.m2>>v.m3>>v.m4>>v.m5>>v.m6)

#define DEFINE_BFS_INPUT_8(class_name,m0,m1,m2,m3,m4,m5,m6,m7) DEFINE_BFS_INPUT(class_name,\
				v.m0>>v.m1>>v.m2>>v.m3>>v.m4>>v.m5>>v.m6>>v.m7)

#define DEFINE_BFS_INPUT_9(class_name,m0,m1,m2,m3,m4,m5,m6,m7,m8) DEFINE_BFS_INPUT(class_name,\
				v.m0>>v.m1>>v.m2>>v.m3>>v.m4>>v.m5>>v.m6>>v.m7>>v.m8)

#define DEFINE_BFS_INPUT_10(class_name,m0,m1,m2,m3,m4,m5,m6,m7,m8,m9) DEFINE_BFS_INPUT(class_name,\
				v.m0>>v.m1>>v.m2>>v.m3>>v.m4>>v.m5>>v.m6>>v.m7>>v.m8>>v.m9)

#define DEFINE_BFS_INPUT_11(class_name,m0,m1,m2,m3,m4,m5,m6,m7,m8,m9,m10) DEFINE_BFS_INPUT(class_name,\
				v.m0>>v.m1>>v.m2>>v.m3>>v.m4>>v.m5>>v.m6>>v.m7>>v.m8>>v.m9>>v.m10)

#define DEFINE_BFS_INPUT_12(class_name,m0,m1,m2,m3,m4,m5,m6,m7,m8,m9,m10,m11) DEFINE_BFS_INPUT(class_name,\
				v.m0>>v.m1>>v.m2>>v.m3>>v.m4>>v.m5>>v.m6>>v.m7>>v.m8>>v.m9>>v.m10>>v.m11)

#define DEFINE_BFS_INPUT_13(class_name,m0,m1,m2,m3,m4,m5,m6,m7,m8,m9,m10,m11,m12) DEFINE_BFS_INPUT(class_name,\
v.m0>>v.m1>>v.m2>>v.m3>>v.m4>>v.m5>>v.m6>>v.m7>>v.m8>>v.m9>>v.m10>>v.m11>>v.m12)

#define DEFINE_BFS_INPUT_14(class_name,m0,m1,m2,m3,m4,m5,m6,m7,m8,m9,m10,m11,m12,m13) DEFINE_BFS_INPUT(class_name,\
v.m0>>v.m1>>v.m2>>v.m3>>v.m4>>v.m5>>v.m6>>v.m7>>v.m8>>v.m9>>v.m10>>v.m11>>v.m12>>v.m13)

#define DEFINE_BFS_INPUT_15(class_name,m0,m1,m2,m3,m4,m5,m6,m7,m8,m9,m10,m11,m12,m13,m14) DEFINE_BFS_INPUT(class_name,\
v.m0>>v.m1>>v.m2>>v.m3>>v.m4>>v.m5>>v.m6>>v.m7>>v.m8>>v.m9>>v.m10>>v.m11>>v.m12>>v.m13>>v.m14)


//output

#define DEFINE_BFS_OUTPUT_1(class_name,m0) DEFINE_BFS_OUTPUT(class_name,v.m0)

#define DEFINE_BFS_OUTPUT_2(class_name,m0,m1) DEFINE_BFS_OUTPUT(class_name,v.m0<<v.m1)

#define DEFINE_BFS_OUTPUT_3(class_name,m0,m1,m2) DEFINE_BFS_OUTPUT(class_name,v.m0<<v.m1<<v.m2)

#define DEFINE_BFS_OUTPUT_4(class_name,m0,m1,m2,m3) DEFINE_BFS_OUTPUT(class_name,v.m0<<v.m1<<v.m2<<v.m3)

#define DEFINE_BFS_OUTPUT_5(class_name,m0,m1,m2,m3,m4) DEFINE_BFS_OUTPUT(class_name,v.m0<<v.m1<<v.m2<<v.m3<<v.m4)

#define DEFINE_BFS_OUTPUT_6(class_name,m0,m1,m2,m3,m4,m5) DEFINE_BFS_OUTPUT(class_name,\
				v.m0<<v.m1<<v.m2<<v.m3<<v.m4<<v.m5)

#define DEFINE_BFS_OUTPUT_7(class_name,m0,m1,m2,m3,m4,m5,m6) DEFINE_BFS_OUTPUT(class_name,\
				v.m0<<v.m1<<v.m2<<v.m3<<v.m4<<v.m5<<v.m6)

#define DEFINE_BFS_OUTPUT_8(class_name,m0,m1,m2,m3,m4,m5,m6,m7) DEFINE_BFS_OUTPUT(class_name,\
				v.m0<<v.m1<<v.m2<<v.m3<<v.m4<<v.m5<<v.m6<<v.m7)

#define DEFINE_BFS_OUTPUT_9(class_name,m0,m1,m2,m3,m4,m5,m6,m7,m8) DEFINE_BFS_OUTPUT(class_name,\
				v.m0<<v.m1<<v.m2<<v.m3<<v.m4<<v.m5<<v.m6<<v.m7<<v.m8)

#define DEFINE_BFS_OUTPUT_10(class_name,m0,m1,m2,m3,m4,m5,m6,m7,m8,m9) DEFINE_BFS_OUTPUT(class_name,\
				v.m0<<v.m1<<v.m2<<v.m3<<v.m4<<v.m5<<v.m6<<v.m7<<v.m8<<v.m9)

#define DEFINE_BFS_OUTPUT_11(class_name,m0,m1,m2,m3,m4,m5,m6,m7,m8,m9,m10) DEFINE_BFS_OUTPUT(class_name,\
				v.m0<<v.m1<<v.m2<<v.m3<<v.m4<<v.m5<<v.m6<<v.m7<<v.m8<<v.m9<<v.m10)

#define DEFINE_BFS_OUTPUT_12(class_name,m0,m1,m2,m3,m4,m5,m6,m7,m8,m9,m10,m11) DEFINE_BFS_OUTPUT(class_name,\
				v.m0<<v.m1<<v.m2<<v.m3<<v.m4<<v.m5<<v.m6<<v.m7<<v.m8<<v.m9<<v.m10<<v.m11)

#define DEFINE_BFS_OUTPUT_13(class_name,m0,m1,m2,m3,m4,m5,m6,m7,m8,m9,m10,m11,m12) DEFINE_BFS_OUTPUT(class_name,\
v.m0<<v.m1<<v.m2<<v.m3<<v.m4<<v.m5<<v.m6<<v.m7<<v.m8<<v.m9<<v.m10<<v.m11<<v.m12)

#define DEFINE_BFS_OUTPUT_14(class_name,m0,m1,m2,m3,m4,m5,m6,m7,m8,m9,m10,m11,m12,m13) DEFINE_BFS_OUTPUT(class_name,\
v.m0<<v.m1<<v.m2<<v.m3<<v.m4<<v.m5<<v.m6<<v.m7<<v.m8<<v.m9<<v.m10<<v.m11<<v.m12<<v.m13)

#define DEFINE_BFS_OUTPUT_15(class_name,m0,m1,m2,m3,m4,m5,m6,m7,m8,m9,m10,m11,m12,m13,m14) DEFINE_BFS_OUTPUT(class_name,\
v.m0<<v.m1<<v.m2<<v.m3<<v.m4<<v.m5<<v.m6<<v.m7<<v.m8<<v.m9<<v.m10<<v.m11<<v.m12<<v.m13<<v.m14)


#define DEFINE_BFS_IO_1(class_name,m0) DEFINE_BFS_INPUT_1(class_name,m0) DEFINE_BFS_OUTPUT_1(class_name,m0)

#define DEFINE_BFS_IO_2(class_name,m0,m1) DEFINE_BFS_INPUT_2(class_name,m0,m1)  DEFINE_BFS_OUTPUT_2(class_name,m0,m1)

#define DEFINE_BFS_IO_3(class_name,m0,m1,m2) DEFINE_BFS_INPUT_3(class_name,m0,m1,m2)  DEFINE_BFS_OUTPUT_3(class_name,m0,m1,m2)

#define DEFINE_BFS_IO_4(class_name,m0,m1,m2,m3) DEFINE_BFS_INPUT_4(class_name,m0,m1,m2,m3)  DEFINE_BFS_OUTPUT_4(class_name,m0,m1,m2,m3)

#define DEFINE_BFS_IO_5(class_name,m0,m1,m2,m3,m4) DEFINE_BFS_INPUT_5(class_name,m0,m1,m2,m3,m4)  DEFINE_BFS_OUTPUT_5(class_name,m0,m1,m2,m3,m4)

#define DEFINE_BFS_IO_6(class_name,m0,m1,m2,m3,m4,m5) DEFINE_BFS_INPUT_6(class_name,m0,m1,m2,m3,m4,m5)  DEFINE_BFS_OUTPUT_6(class_name,m0,m1,m2,m3,m4,m5)

#define DEFINE_BFS_IO_7(class_name,m0,m1,m2,m3,m4,m5,m6) DEFINE_BFS_INPUT_7(class_name,	m0,m1,m2,m3,m4,m5,m6)  DEFINE_BFS_OUTPUT_7(class_name,m0,m1,m2,m3,m4,m5,m6)

#define DEFINE_BFS_IO_8(class_name,m0,m1,m2,m3,m4,m5,m6,m7) DEFINE_BFS_INPUT_8(class_name,m0,m1,m2,m3,m4,m5,m6,m7)  DEFINE_BFS_OUTPUT_8(class_name,m0,m1,m2,m3,m4,m5,m6,m7)

#define DEFINE_BFS_IO_9(class_name,m0,m1,m2,m3,m4,m5,m6,m7,m8) DEFINE_BFS_INPUT_9(class_name,m0,m1,m2,m3,m4,m5,m6,m7,m8) \
															   DEFINE_BFS_OUTPUT_9(class_name,m0,m1,m2,m3,m4,m5,m6,m7,m8)

#define DEFINE_BFS_IO_10(class_name,m0,m1,m2,m3,m4,m5,m6,m7,m8,m9) DEFINE_BFS_INPUT_10(class_name, m0,m1,m2,m3,m4,m5,m6,m7,m8,m9)\
																   DEFINE_BFS_OUTPUT_10(class_name,m0,m1,m2,m3,m4,m5,m6,m7,m8,m9)

#define DEFINE_BFS_IO_11(class_name,m0,m1,m2,m3,m4,m5,m6,m7,m8,m9,m10) DEFINE_BFS_INPUT_11(class_name,m0,m1,m2,m3,m4,m5,m6,m7,m8,m9,m10) \
																		DEFINE_BFS_OUTPUT_11(class_name,m0,m1,m2,m3,m4,m5,m6,m7,m8,m9,m10)

#define DEFINE_BFS_IO_12(class_name,m0,m1,m2,m3,m4,m5,m6,m7,m8,m9,m10,m11) DEFINE_BFS_INPUT_12(class_name, m0,m1,m2,m3,m4,m5,m6,m7,m8,m9,m10,m11) \
																		   DEFINE_BFS_OUTPUT_12(class_name,m0,m1,m2,m3,m4,m5,m6,m7,m8,m9,m10,m11)

#define DEFINE_BFS_IO_13(class_name,m0,m1,m2,m3,m4,m5,m6,m7,m8,m9,m10,m11,m12) DEFINE_BFS_INPUT_13(class_name, m0,m1,m2,m3,m4,m5,m6,m7,m8,m9,m10,m11,m12) \
                                                                              DEFINE_BFS_OUTPUT_13(class_name,m0,m1,m2,m3,m4,m5,m6,m7,m8,m9,m10,m11,m12)

#define DEFINE_BFS_IO_14(class_name,m0,m1,m2,m3,m4,m5,m6,m7,m8,m9,m10,m11,m12,m13) DEFINE_BFS_INPUT_14(class_name, m0,m1,m2,m3,m4,m5,m6,m7,m8,m9,m10,m11,m12,m13) \
DEFINE_BFS_OUTPUT_14(class_name,m0,m1,m2,m3,m4,m5,m6,m7,m8,m9,m10,m11,m12,m13)

#define DEFINE_BFS_IO_15(class_name,m0,m1,m2,m3,m4,m5,m6,m7,m8,m9,m10,m11,m12,m13,m14) DEFINE_BFS_INPUT_15(class_name, m0,m1,m2,m3,m4,m5,m6,m7,m8,m9,m10,m11,m12,m13,m14) \
DEFINE_BFS_OUTPUT_15(class_name,m0,m1,m2,m3,m4,m5,m6,m7,m8,m9,m10,m11,m12,m13,m14)

class BFSFileSignature
{
private:
	struct Signature
	{
		std::string		className;
		std::string		signature;

		DEFINE_BFS_IO_2(Signature,className,signature)
	};

	std::vector<Signature>  vSig;

	DEFINE_BFS_IO_1(BFSFileSignature, vSig)
public:
	static std::string GetSeqSignature(const char *seq)
	{
		std::string desc;
		const char *px = seq;
		for (; *px != '\0'; ++px)
		{
			if (*px == '.')
			{
				if (!desc.empty())
					desc.push_back(',');

				while (isalnum(*++px) || *px == '_')
					desc.push_back(*px);

				if (*px == '\0')
					break;
			}
		}
		return desc;
	}
	template<typename _ClassT>
	void Add()
	{
		Signature sig;
		sig.className = _ClassT::GetClassName();
		sig.signature = GetSeqSignature(_ClassT::GetOutputSeq());

		vSig.push_back(sig);
	}
	const Signature * Get(const std::string &className)
	{
		for (auto &s : vSig)
		{
			if (s.className == className)
				return &s;
		}
		return NULL;
	}
	template<typename _ClassT>
	bool Check(std::string &msg)
	{
		bool ok = false;
		std::string className = _ClassT::GetClassName();
		const Signature *sig = this->Get(className);
		if (sig)
		{
			std::string isig = GetSeqSignature(_ClassT::GetInputSeq());
			if (sig->signature != isig)
				msg += "class " + className + ": " + isig + "!= required " + sig->signature + "\n";
			else
				ok = true;
		}
		else
			msg += "class " + className + ": signature not found";

		return ok;
	}
};



_FF_END


#endif

