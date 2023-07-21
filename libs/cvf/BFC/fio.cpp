
#include"BFC/fio.h"
#include"BFC/err.h"
#include"BFC/mem.h"
#include"BFC/stdf.h"
#include"BFC/autores.h"
#include"BFC/portable.h"

#include<vector>
#include<list>
#ifdef _WIN32
#include<io.h>
#else
#include<sys/stat.h>
#include <unistd.h>
#include <sys/types.h>
#endif
#include<stdio.h>
#include<limits.h>
//#include<tchar.h>
#include<fstream>

//#include<windows.h>
//#pragma warning(disable:4996)

_FF_BEG

typedef long fpos_t;

static const int MAX_NAME=256, HEADER_SIZE=256, MAGIC_SIZE=8;

static char g_bf_magic[MAGIC_SIZE+1]="blklist\0";


#pragma pack(1)

struct FBlock
{
public:
	int			m_id;
	fpos_t		m_beg, m_end; 
public:
	FBlock(int id=0, fpos_t beg=0, fpos_t end=0)
		:m_id(id),m_beg(beg),m_end(end)
	{
	}
	int Size() const
	{
		return int(m_end-m_beg);
	}
};

struct FInfo
{
public:
	int        m_id;
	char		m_name[MAX_NAME];
};

typedef std::list<FBlock>  _FBListT;
typedef std::list<FInfo>   _FIListT;

struct GBlock
{
	fpos_t     m_list_beg, m_list_end;  
};

#pragma pack()


template<typename _ListT>
static bool _load_list(FILE *fp, _ListT &blkList)
{
	bool ok=true;
	uint count=0;

	if(fread(&count,sizeof(count),1,fp)==1)
	{
		blkList.resize(count);

		typename _ListT::iterator itr(blkList.begin());

		for(uint i=0; i<count; ++i, ++itr)
		{
			if(fread(&*itr,sizeof(*itr),1,fp)!=1)
			{
				i=count;
				ok=false;
			}
		}
	}
	else
		ok=false;

	return ok;
}

template<typename _ListT>
static bool _save_list(FILE *fp, _ListT &blkList)
{
	bool ok=true;
	uint count=(uint)blkList.size();

	if(fwrite(&count,sizeof(count),1,fp)==1)
	{
		typename _ListT::const_iterator itr(blkList.begin());

		for(uint i=0; i<count; ++i, ++itr)
		{
			if(fwrite(&*itr,sizeof(*itr),1,fp)!=1)
			{
				i=count;
				ok=false;
			}
		}
	}
	else
		ok=false;

	return ok;
}

static long getFileSize(FILE *fp)
{
	long size = 0;

	if (fp)
	{
		long pos = ftell(fp);
		fseek(fp, 0, SEEK_END);
		size = ftell(fp);
		fseek(fp, pos, SEEK_SET);
	}
	return size;
}

class DiskSwapFile::_CImp
{
	std::string	m_file;
	FILE       *m_fp;
	
	char        m_magic[MAGIC_SIZE];
	char        m_header[HEADER_SIZE];
	GBlock      m_gblk;

	_FIListT    m_infList;
	_FBListT    m_freeList;
	std::vector<FBlock>		m_blkList;

public:

	_CImp();

	void  Init()
	{
		memcpy(m_magic,g_bf_magic,MAGIC_SIZE);
		memset(m_header,0,sizeof(m_header));
		m_gblk.m_list_beg=m_gblk.m_list_end=0;
	}

	bool  OpenX(const std::string &file, bool bnew);

	void  Open(const std::string &file, bool bnew)
	{
	/*	_CImp temp;

		if(temp.OpenX(file,bnew))
		{
			this->Swap(temp);
		}*/
		this->OpenX(file,bnew);
	}

	size_t  _ListSize() const
	{
		size_t sz=3*sizeof(uint);

		sz+=m_infList.size()*sizeof(m_infList.front());
		sz+=m_freeList.size()*sizeof(m_freeList.front());
		sz+=m_blkList.size()*sizeof(m_blkList.front());

		return sz;
	}

	size_t _HeaderSize() const
	{
		return sizeof(m_magic)+sizeof(m_header)+sizeof(m_gblk);
	}

	void  Save();

	void Close()
	{
		if(m_fp)
		{
			this->Save();

			fclose(m_fp);

			m_blkList.clear();
			m_infList.clear();
			m_freeList.clear();
			m_fp=NULL;
		}
	}

	void _AddFreeBlock(const FBlock &blk)
	{
		if(blk.m_end>blk.m_beg)
		{
			_FBListT::iterator itr(m_freeList.begin());

			for(;itr!=m_freeList.end();++itr)
			{
				if(itr->m_end==blk.m_beg)
				{
					itr->m_end=blk.m_end;
					break;
				}
			}

			if(itr==m_freeList.end())
				m_freeList.push_back(blk);
		}
	}

	int  _NewBlock()
	{
		int id=-1;

		for(size_t i=0; i<m_blkList.size(); ++i)
		{
			if(m_blkList[i].m_id<0)
			{
				id=int(i);
				m_blkList[i]=FBlock(id,0,0);
				break;
			}
		}

		if(id<0)
		{
			id=(int)m_blkList.size();
			m_blkList.push_back(FBlock(id));
		}

		return id;
	}

	bool _AllocBlockFreeList(int size, FBlock &blk)
	{
		bool rv=false;

		{//try free list
			_FBListT::iterator itr(m_freeList.begin()), mitr(m_freeList.end());
			int   dmin=INT_MAX;

			for(;itr!=m_freeList.end(); ++itr)
			{
				int d=itr->Size()-size;

				if(d>=0)
				{
					if(d<dmin)
					{
						dmin=d;
						mitr=itr;
					}

					if(d==0)
						break;
				}
			}

			if(mitr!=m_freeList.end())
			{
				blk=FBlock(0,mitr->m_beg,mitr->m_beg+size);

				if(mitr->Size()==size)
					m_freeList.erase(mitr);
				else
					mitr->m_beg+=size;

				rv=true;
			}
		}
		return rv;
	}

	long FileSize() 
	{
		return getFileSize(m_fp);
	}

	void ResizeFile(long newSize)
	{
#ifndef _WIN32
		int fd = fileno(m_fp);
		if (ftruncate(fd, newSize) != 0)
			FF_EXCEPTION(ERR_FILE_OP_FAILED, "");
#else

		int fd = _fileno(m_fp);
		if (chsize(fd, newSize) != 0)
			FF_EXCEPTION(ERR_FILE_OP_FAILED, "");
#endif

#if 1
		/*int fd=_fileno(m_fp);

		if(_chsize(fd,newSize)!=0)
		{
			FF_EXCEPTION(ERR_FILE_OP_FAILED,m_file.c_str());
		}*/
#else
		long curSize=this->FileSize(), dsz=newSize-curSize;

		if(dsz>0)
		{
			char *buf=new char[dsz];
			fseek(m_fp,0,SEEK_END);

			fwrite(buf,dsz,1,m_fp);

			delete[]buf;
		}
#endif
	}

	FBlock _AllocBlock(int size)
	{
		FBlock blk;

		if(!_AllocBlockFreeList(size,blk))
		{
			long fsz=this->FileSize();

			this->ResizeFile(fsz+size);

			blk.m_beg=fsz;
			blk.m_end=fsz+size;
		}

		return blk;
	}

	void _ResizeBlock(int id, int newSize)
	{
		if(size_t(id)<m_blkList.size())
		{
			FBlock &blk(m_blkList[id]);

			assert(blk.m_id==id);
			int curSize=blk.Size();

			if(curSize>newSize)
			{
			//	m_freeList.push_back(FBlock(-1,blk.m_beg+newSize,blk.m_end));
				this->_AddFreeBlock(FBlock(-1,blk.m_beg+newSize,blk.m_end));

				blk.m_end=blk.m_beg+newSize;
			}
			else
				if(curSize<newSize)
				{
					FBlock nblk(this->_AllocBlock(newSize));

		//			m_freeList.push_back(blk);
					this->_AddFreeBlock(blk);

					nblk.m_id=blk.m_id;
					blk=nblk;
				}
		}
	}

	void SaveBlock(const void *buf, int size, int *id)
	{
		if(buf&&id)
		{
			if(*id<0)
				*id=this->_NewBlock();

			this->_ResizeBlock(*id,size);

			fseek(m_fp,m_blkList[*id].m_beg,SEEK_SET);

			if(fwrite(buf,size,1,m_fp)!=1)
			{
				FF_EXCEPTION(ERR_FILE_WRITE_FAILED,m_file.c_str());
			}

			fflush(m_fp);
		}
	}

	_FIListT::iterator _FindInf(int id) 
	{
		_FIListT::iterator itr(m_infList.begin());

		for(;itr!=m_infList.end();++itr)
		{
			if(itr->m_id==id)
				break;
		}
		return itr;
	}

	/*char*  _ConvertName(const char *name, ff_mem &mem)
	{
		size_t len=_tcslen(name)+1;

		if(len>MAX_NAME)
		{
			name+=MAX_NAME-len;
			len=MAX_NAME;
		}

#ifndef _UNICODE
		
		assert(sizeof(char)==1);

		return ff_a2w(name,mem);
#else
		assert(sizeof(char)==2);
	
		return (char*)name;
#endif
	}*/

	int  _IsValidID(int id) const
	{
		return size_t(id)<m_blkList.size() && m_blkList[id].m_id>=0;
	}

	int  _IDFromName(const char *wname)
	{
		int id=-1;

		for(_FIListT::const_iterator itr(m_infList.begin()); itr!=m_infList.end(); ++itr)
		{
			if(strcmp(wname,itr->m_name)==0)
			{
				id=itr->m_id;
				break;
			}
		}

		return id;
	}

	int  SetBlockName(int id, const char *name, bool allowMultiple)
	{
		int rv=0;

		if(this->_IsValidID(id))
		{
			//ff_mem mem;
			//char *wname=this->_ConvertName(name,mem);
			const char *wname = name;

			if(!allowMultiple)
			{
				int idx=this->_IDFromName(wname);

				if(idx>=0)
					return idx==id? 1:0;
			}

			_FIListT::iterator itr(this->_FindInf(id));

			if(itr==m_infList.end())
			{
				m_infList.resize(m_infList.size()+1);
				m_infList.back().m_id=id;
				itr=--(m_infList.end());
			}

			strcpy(itr->m_name,wname);
			rv=1;
		}
		return rv;
	}

	int  IDFromName(const char *name)
	{
		//ff_mem mem;
		//char *wname=this->_ConvertName(name,mem);

		return this->_IDFromName(name);
	}

	int  QueryBlockSize(int id)
	{
		int sz=0;

		if(this->_IsValidID(id))
		{
			sz=m_blkList[id].Size();
		}

		return sz;
	}

	int LoadBlock(void *buf, int size, int id)
	{
		int rsz=0;

		if(this->_IsValidID(id))
		{
			assert(m_blkList[id].m_id==id);

			FBlock blk(m_blkList[id]);

			rsz=blk.Size();

			if(size>=rsz)
			{
				fseek(m_fp,blk.m_beg,SEEK_SET);

				if(fread(buf,rsz,1,m_fp)!=1)
				{
					FF_EXCEPTION(ERR_FILE_READ_FAILED,m_file.c_str());
				}
			}
		}

		return rsz;
	}

	bool DeleteBlock(int id)
	{
		bool rv=false;

		if(this->_IsValidID(id))
		{
			_FIListT::iterator itr(this->_FindInf(id));

			if(itr!=m_infList.end())
				m_infList.erase(itr);

			m_blkList[id].m_id=-1;
	//		m_freeList.push_back(m_blkList[id]);
			this->_AddFreeBlock(m_blkList[id]);

			rv=true;
		}
		return rv;
	}

	~_CImp()
	{
		this->Close();
	}
};

DiskSwapFile::_CImp::_CImp()
{
	m_fp=NULL;
}

bool DiskSwapFile::_CImp::OpenX(const std::string &file, bool bnew)
{
	if(!m_fp)
	{
		if(pathExist(file))
		{
			m_fp=_tfopen(file.c_str(),("rb+"));

			if(fread(m_magic,MAGIC_SIZE,1,m_fp)!=1 ||
				memcmp(m_magic,g_bf_magic,MAGIC_SIZE)!=0 ||
				fread(m_header,HEADER_SIZE,1,m_fp)!=1 ||
				fread(&m_gblk,sizeof(m_gblk),1,m_fp)!=1)
			{
				FF_EXCEPTION(ERR_INVALID_FORMAT,file.c_str());
			}

			if(fseek(m_fp,m_gblk.m_list_beg,SEEK_SET)!=0||
				!_load_list(m_fp,m_infList) ||
				!_load_list(m_fp,m_freeList) ||
				!_load_list(m_fp,m_blkList)
				)
			{
				FF_EXCEPTION(ERR_FILE_READ_FAILED,file.c_str());
			}
		}
		else
		{
			if(bnew)
			{
				if(m_fp=_tfopen(file.c_str(),("wb+")))
				{
					this->ResizeFile((long)this->_HeaderSize());

					this->Init();
				}
			}
			else
				return false;
		}

		if(!m_fp)
			FF_EXCEPTION(ERR_FILE_OPEN_FAILED,file.c_str());

		m_file=file;

		return true;
	}

	return false;
}

void DiskSwapFile::_CImp::Save()
{
	if(m_fp)
	{
		size_t listSize=this->_ListSize(), curListSize=size_t(m_gblk.m_list_end-m_gblk.m_list_beg);

		fpos_t listBeg=m_gblk.m_list_beg;

		if(curListSize<listSize)
		{
			fseek(m_fp,0,SEEK_END);
			listBeg=ftell(m_fp);
		}
		else
		{
			fseek(m_fp,listBeg,SEEK_SET);
		}

		if(!_save_list(m_fp,m_infList) || 
			!_save_list(m_fp,m_freeList) ||
			!_save_list(m_fp,m_blkList) )
		{
			FF_EXCEPTION(ERR_FILE_WRITE_FAILED,m_file.c_str());
		}

		if(curListSize<listSize)
		{
			m_gblk.m_list_beg=listBeg;
			m_gblk.m_list_end=ftell(m_fp);
		}

		fseek(m_fp,0,SEEK_SET);

		if(fwrite(m_magic,sizeof(m_magic),1,m_fp)!=1 ||
			fwrite(m_header,sizeof(m_header),1,m_fp)!=1 ||
			fwrite(&m_gblk,sizeof(m_gblk),1,m_fp)!=1
			)
		{
			FF_EXCEPTION(ERR_FILE_WRITE_FAILED,m_file.c_str());
		}

		fflush(m_fp);
	}
}


DiskSwapFile::DiskSwapFile()
:m_imp(new _CImp)
{
}

DiskSwapFile::~DiskSwapFile()
{
	delete m_imp;
}

void DiskSwapFile::Open(const std::string &file, bool bnew)
{
	m_imp->Open(file,bnew);
}

void DiskSwapFile::Save()
{
	m_imp->Save();
}

void DiskSwapFile::Close()
{
	m_imp->Close();
}

void DiskSwapFile::SaveBlock(const void *buf, int size, int *id)
{
	m_imp->SaveBlock(buf,size,id);
}

void DiskSwapFile::SetBlockName(int id, const char *name, bool allowMultiple)
{
	m_imp->SetBlockName(id,name,allowMultiple);
}

int  DiskSwapFile::IDFromName(const char *name)
{
	return m_imp->IDFromName(name);
}

int  DiskSwapFile::QueryBlockSize(int id)
{
	return m_imp->QueryBlockSize(id);
}

int  DiskSwapFile::LoadBlock(void *buf, int size, int id)
{
	return m_imp->LoadBlock(buf,size,id);
}

bool DiskSwapFile::DeleteBlock(int id)
{
	return m_imp->DeleteBlock(id);
}

//=============================================================================================

#define _NSF_MAGIC		"mstream\0"
#define _NSF_VERSION	1

#pragma pack(1)

enum
{
	_NSF_TRANSFORMED=0x01
};

struct _NSFHeadBase
{
	char   m_magic[8];
	
	uint32  m_flag;

	uint32  m_version;

	uint32  m_raw_size; //size before transformation
};

struct _NSFHead
	:public _NSFHeadBase
{
	char   m_reserved[256-sizeof(_NSFHeadBase)];
};

#pragma pack()

static void _init_nsf_head(_NSFHead &head)
{
	memset(&head,0,sizeof(head));

	memcpy(head.m_magic,_NSF_MAGIC,sizeof(head.m_magic));
	head.m_version=_NSF_VERSION;
}

typedef std::string _nstring;

struct _NSFStream
{
	_nstring    m_name;

	ff::BMStream   m_stream;

public:
	_NSFStream(const _nstring &name=_nstring())
		:m_name(name)
	{
	}

public:
	DEFINE_BFS_IO_2(_NSFStream,m_name,m_stream);
};

class NamedStreamFile::_CImp
{
public:
	NamedStreamFile  *m_site;

	char					m_magic_value[8]; //user defined magic value
	std::string				&m_file;  //use reference to support swap by memory, otherwise may trigger assertion failed in debug mode
	ff::AutoPtr<ff::IBFStream>	m_is; //file-ptr to read body data

	_NSFHead				 m_head;		   //common file head
	std::vector<char>		&m_user_head;   //data of user head
	std::list<_NSFStream>   &m_streamList; 

public:
	
	_CImp(NamedStreamFile *site, const char magic[])
		:m_site(site), m_file(*new std::string), m_user_head(*new std::vector<char>), m_streamList(*new std::list<_NSFStream>)
	{
		memset(m_magic_value,0,sizeof(m_magic_value));
		if(!magic)
			magic=_NSF_MAGIC;
		strncpy(m_magic_value,magic, __min(strlen(magic),sizeof(m_magic_value)) );
	}

	~_CImp()
	{
		delete &m_streamList;
		delete &m_user_head;
		delete &m_file;
	}

	void Swap(_CImp &right)
	{
		ff::SwapObjectMemory(*this,right);
	}

	void _TryLoadBody()
	{
		if(m_is)
		{
			assert(m_streamList.empty());

			ff::AutoPtr<IBFStream> fis(m_is);
			m_is=ff::AutoPtr<IBFStream>(NULL); //you have only one chance to try load

			IBFStream::PosType fpos=fis->Tell();
			long fsize=fis->Size()-fpos;//_filelength(fid)-sizeof(m_head);

			if(fsize>0)
			{
				AutoArrayPtr<char> buf(new char[fsize]);

				fis->Read(buf,fsize,1);

				if(m_head.m_flag&_NSF_TRANSFORMED)
				{//de-compress
					AutoArrayPtr<char> dbuf(new char[m_head.m_raw_size]);
					size_t dsize=m_head.m_raw_size;

					int ec=m_site->TransformData(buf,fsize,dbuf,dsize,-1);

					if(ec>0)
					{
						buf=dbuf;
						fsize=(long)dsize;
					}
					else
					{
						if(ec<0)
							FF_EXCEPTION(ERR_UNKNOWN,"");
					}
				}

				IBMStream mis;
				mis.SetBuffer(buf,fsize,false);

				mis>>m_streamList;
			}
		}
	}

	void DoLoad(const std::string &file, bool bnew)
	{
		if(!bnew || ff::pathExist(file))
		{
			ff::AutoPtr<IBFStream> is(new IBFStream(file));

			is->Read(&m_head,sizeof(m_head),1);

			if(strncmp(m_head.m_magic,m_magic_value,sizeof(m_head.m_magic))!=0)
			{
				FF_EXCEPTION(ERR_INVALID_FORMAT,"");
			}

			std::vector<char> user_head;
			(*is)>>user_head;
			m_user_head.swap(user_head);

			m_is=is; //save to load body
		}

		m_file=file;
	}

	void Load(const std::string &file, bool bnew)
	{
		_CImp temp(m_site,m_magic_value);

		temp.DoLoad(file,bnew);

		this->Swap(temp);
	}

	std::list<_NSFStream>::iterator DoGetStream(const _nstring &name, bool bnew)
	{
		this->_TryLoadBody();

		std::list<_NSFStream>::iterator itr(m_streamList.begin());

		for(; itr!=m_streamList.end(); ++itr)
		{
		//	if(itr->m_name==name)
			if(stricmp(itr->m_name.c_str(),name.c_str())==0)
				break;
		}

		if(itr==m_streamList.end())
		{
			if(bnew)
			{
				m_streamList.push_back(_NSFStream(name));
				--itr; //pointer to the last elem.
			}
		}

		return itr;
	}

	BMStream* GetStream(const _nstring &name, bool bnew)
	{
		std::list<_NSFStream>::iterator itr(this->DoGetStream(name,bnew));

		return itr==m_streamList.end()? NULL : &itr->m_stream;
	}

	bool  DeleteStream(const _nstring &name)
	{
		std::list<_NSFStream>::iterator itr(this->DoGetStream(name,false));

		if(itr!=m_streamList.end())
		{
			m_streamList.erase(itr);
			return true;
		}

		return false;
	}

	void  Save(const char *file, int flag)
	{
		this->_TryLoadBody();

		if(!file)
		{
			assert(!m_file.empty());
			file=m_file.c_str();
		}

		OBMStream os;
		os<<m_streamList;

		void  *data=(void*)os.Buffer();
		AutoArrayPtr<char> buf;
		size_t dsize=(size_t)os.Size();

		_NSFHead  head;
		
		_init_nsf_head(head);
		memcpy(head.m_magic,m_magic_value,sizeof(head.m_magic));

		head.m_raw_size=(uint32)dsize;

		if(flag&NamedStreamFile::SAVE_TRANSFORM)
		{
			buf=AutoArrayPtr<char>(new char[dsize]);
			int ec=m_site->TransformData(data,os.Size(),buf,dsize,1);

			if(ec>0)
			{
				head.m_flag|=_NSF_TRANSFORMED;
				data=buf;
			}
			else
				if(ec<0)
					FF_EXCEPTION(ERR_UNKNOWN,"");
		}

		std::string dir(GetDirectory(file));

		if(!dir.empty() && !pathExist(dir))
		{ //create dir.
			makeDirectory(dir);
		}

		OBFStream fos(file);

		{
			fos.Write(&head,sizeof(head),1);
			fos<<m_user_head;
			fos.Write(data,dsize,1);
		}
	}

	void ListStreams(std::vector<_nstring> &vname)
	{
		this->_TryLoadBody();

		for(std::list<_NSFStream>::iterator itr(m_streamList.begin()); itr!=m_streamList.end(); ++itr)
		{
			vname.push_back(itr->m_name);
		}
	}
	int ListStreams(char *vstrs[], int nmax)
	{
		this->_TryLoadBody();

		int n = 0;
		if (vstrs &&nmax > 0)
		{
			for (std::list<_NSFStream>::iterator itr(m_streamList.begin()); itr != m_streamList.end(); ++itr)
			{
				vstrs[n++] = (char*)itr->m_name.c_str();
				if (n >= nmax)
					break;
			}
		}
		return n;
	}

	int  Size()
	{
		this->_TryLoadBody();

		return (int)m_streamList.size();
	}

	const void* GetUserHead(int &size)
	{
		void *data=NULL;
		if(!m_user_head.empty())
		{
			data=&m_user_head[0];
			size=(int)m_user_head.size();
		}
		return data;
	}

	void SetUserHead(const void *data, int size)
	{
		m_user_head.resize(size);
		if(data&&size>0)
		{
			memcpy(&m_user_head[0],data,size);
		}
	}

};



NamedStreamFile::NamedStreamFile(const char magic[])
{
	m_imp=new NamedStreamFile::_CImp(this, magic);
}

NamedStreamFile::~NamedStreamFile()
{
	delete m_imp;
}

void NamedStreamFile::Load(const std::string &file, bool bnew)
{
	m_imp->Load(file,bnew);
}
const std::string& NamedStreamFile::GetFile()
{
	return m_imp->m_file;
}
const void* NamedStreamFile::GetUserHead(int &size)
{
	return m_imp->GetUserHead(size);
}

int NamedStreamFile::GetUserHead(void *buf, int buf_size)
{
	int head_size;
	const void *head=this->GetUserHead(head_size);

	int dsize=__min(head_size,buf_size);
	memcpy(buf,head,dsize);

	return dsize;
}

void NamedStreamFile::SetUserHead(const void *data, int size)
{
	m_imp->SetUserHead(data,size);
}

BMStream* NamedStreamFile::GetStream(const _nstring &name, bool bnew)
{
	return m_imp->GetStream(name,bnew);
}

void NamedStreamFile::DeleteStream(const _nstring &name)
{
	m_imp->DeleteStream(name);
}

void NamedStreamFile::Save(const char *file, int flag)
{
	m_imp->Save(file,flag);
}

void NamedStreamFile::ListStreams(std::vector<_nstring> &vname)
{
	m_imp->ListStreams(vname);
}
int NamedStreamFile::ListStreams(char *vstrs[], int nmax)
{
	return m_imp->ListStreams(vstrs, nmax);
}
int NamedStreamFile::Size()
{
	return m_imp->Size();
}

int  NamedStreamFile::TransformData(const void *src, size_t isize, void *dest, size_t &dsize, int dir)
{
	return 0;
}

//===========================================================================================================

//=============================================================================================

#define _NSS_MAGIC		"nstream\0"
#define _NSS_VERSION	1

#pragma pack(1)


struct _NSSHeadBase
{
	char   m_magic[8];

	uint32  m_flag;

	uint32  m_version;

	uint32  m_raw_size; //size before transformation
};

struct _NSSHead
	:public _NSSHeadBase
{
	char   m_reserved[256 - sizeof(_NSSHeadBase)];
};

#pragma pack()

static void _init_NSS_head(_NSSHead &head)
{
	memset(&head, 0, sizeof(head));

	memcpy(head.m_magic, _NSS_MAGIC, sizeof(head.m_magic));
	head.m_version = _NSS_VERSION;
}

typedef std::string _nstring;

struct _NSSStream
{
	_nstring    m_name;
	NamedStreamSet::StreamPtr  m_streamPtr;
public:
	_NSSStream(const _nstring &name = _nstring())
		:m_name(name)
	{
	}
};

class NamedStreamSet::_CImp
{
public:
	NamedStreamSet  *m_site;

	char					m_magic_value[8]; //user defined magic value
	std::string				&m_file;  //use reference to support swap by memory, otherwise may trigger assertion failed in debug mode
	std::string             &m_dataDir;
	//ff::AutoPtr<ff::IBFStream>	m_is; //file-ptr to read body data

	_NSSHead				 m_head;		   //common file head
	std::vector<char>		&m_user_head;   //data of user head
	std::list<_NSSStream>   &m_streamList;

public:

	_CImp(NamedStreamSet *site, const char magic[])
		:m_site(site), m_file(*new std::string), m_dataDir(*new std::string), m_user_head(*new std::vector<char>), m_streamList(*new std::list<_NSSStream>)
	{
		memset(m_magic_value, 0, sizeof(m_magic_value));
		if (!magic)
			magic = _NSS_MAGIC;
		strncpy(m_magic_value, magic, __min(strlen(magic), sizeof(m_magic_value)));
	}

	~_CImp()
	{
		delete &m_streamList;
		delete &m_user_head;
		delete &m_file;
		delete &m_dataDir;
	}

	void Swap(_CImp &right)
	{
		ff::SwapObjectMemory(*this, right);
	}
	
	void DoLoad(const std::string &file, bool bnew)
	{
		if (!bnew || ff::pathExist(file)) //read data if file exist
		{
			ff::AutoPtr<IBFStream> is(new IBFStream(file));

			is->Read(&m_head, sizeof(m_head), 1);

			if (strncmp(m_head.m_magic, m_magic_value, sizeof(m_head.m_magic)) != 0)
			{
				FF_EXCEPTION(ERR_INVALID_FORMAT, "");
			}

			std::vector<char> user_head;
			(*is) >> user_head;
			m_user_head.swap(user_head);

			//m_is = is; //save to load body
		}

		m_file = file;
		m_dataDir = file + "[nss]/";
		if (!ff::pathExist(m_dataDir) && bnew) //create data dir.
		{
			ff::makeDirectory(m_dataDir);
		}

		if (!ff::pathExist(file) && bnew) //create the main file
			this->Save(m_file.c_str(), 0);
	}

	void Load(const std::string &file, bool bnew)
	{
		_CImp temp(m_site, m_magic_value);

		temp.DoLoad(file, bnew);

		this->Swap(temp);
	}

	std::string GetStreamFile(const std::string &name)
	{
		return m_dataDir + name;
	}

#if 0
	std::list<_NSSStream>::iterator DoGetStream(const _nstring &name, bool bnew, bool findOnly)
	{
		std::list<_NSSStream>::iterator itr(m_streamList.begin());

		for (; itr != m_streamList.end(); ++itr)
		{
			//	if(itr->m_name==name)
			if (stricmp(itr->m_name.c_str(), name.c_str()) == 0)
			{
				FFAssert1(itr->m_streamPtr->IsOpen());
				break;
			}
		}

		if (findOnly)
			return itr;

		if (itr == m_streamList.end())
		{
			std::string streamFile = this->GetStreamFile(name);

			if (bnew && !ff::pathExist(streamFile))
			{
				ff::OBFStream os(streamFile); //create the file
			}
			
			StreamPtr ptr(new BFStream(streamFile, false)); // set True here will clear the exist file contents

			m_streamList.push_back(_NSSStream(name));
			--itr; //pointer to the last elem.

			itr->m_streamPtr = ptr;
		}

		return itr;
	}

	StreamPtr GetStream(const _nstring &name, bool bnew)
	{
		std::list<_NSSStream>::iterator itr(this->DoGetStream(name, bnew, false));

		return itr == m_streamList.end() ? StreamPtr(NULL) : itr->m_streamPtr;
	}

	bool  DeleteStream(const _nstring &name)
	{
		std::list<_NSSStream>::iterator itr(this->DoGetStream(name, false, true));

		if (itr != m_streamList.end())
		{
			if (itr->m_streamPtr.use_count() > 1)
				FF_EXCEPTION1("The stream is in use");

			m_streamList.erase(itr);
		}

		std::string streamFile = this->GetStreamFile(name);
		if (ff::pathExist(streamFile))
		{
			ff::DeleteFileEx(streamFile);
			return true;
		}

		return false;
	}

	void _cleanBuffer()
	{
		for (auto itr = m_streamList.begin(); itr != m_streamList.end(); )
		{
			if (!itr->m_streamPtr||itr->m_streamPtr.use_count() <= 1)
			{
				auto titr = itr;
				++itr;
				m_streamList.erase(titr);
			}
			else
			{
				itr->m_streamPtr->Flush();

				++itr;
			}
		}
	}

	void  Save(const char *file, int flag)
	{
		if (!file)
		{
			assert(!m_file.empty());
			file = m_file.c_str();
		}

		_NSSHead  head;

		_init_NSS_head(head);
		memcpy(head.m_magic, m_magic_value, sizeof(head.m_magic));

		head.m_raw_size = (uint32)0;

		std::string dir(GetDirectory(file));

		if (!dir.empty() && !pathExist(dir))
		{ //create dir.
			makeDirectory(dir);
		}

		OBFStream fos(file);

		{
			fos.Write(&head, sizeof(head), 1);
			fos << m_user_head;
		}
		this->_cleanBuffer();
	}
#else
		StreamPtr DoGetStream(const _nstring &name, bool bnew)
		{
			std::string streamFile = this->GetStreamFile(name);

			if (bnew && !ff::pathExist(streamFile))
			{
				ff::OBFStream os(streamFile); //create the file
			}

			return StreamPtr(new BFStream(streamFile, false)); // set True here will clear the exist file contents
		}

		StreamPtr GetStream(const _nstring &name, bool bnew)
		{
			return this->DoGetStream(name, bnew);
		}

		bool  DeleteStream(const _nstring &name)
		{
			std::string streamFile = this->GetStreamFile(name);
			if (ff::pathExist(streamFile))
			{
				ff::DeleteFileEx(streamFile);
				return true;
			}

			return false;
		}

		void  Save(const char *file, int flag)
		{
			if (m_file.empty())
				return;

			if (!file)
				file = m_file.c_str();

			_NSSHead  head;

			_init_NSS_head(head);
			memcpy(head.m_magic, m_magic_value, sizeof(head.m_magic));

			head.m_raw_size = (uint32)0;

			std::string dir(GetDirectory(file));

			if (!dir.empty() && !pathExist(dir))
			{ //create dir.
				makeDirectory(dir);
			}

			OBFStream fos(file);

			{
				fos.Write(&head, sizeof(head), 1);
				fos << m_user_head;
			}
		}
#endif

	void ListStreams(std::vector<_nstring> &vname, bool listBuffered)
	{
		std::vector<std::string> files;

		if (listBuffered)
		{
			for (auto &v : this->m_streamList)
				files.push_back(v.m_name);
		}
		else
			ff::listFiles(m_dataDir, files, false, false);

		vname.swap(files);
	}

	const void* GetUserHead(int &size)
	{
		void *data = NULL;
		if (!m_user_head.empty())
		{
			data = &m_user_head[0];
			size = (int)m_user_head.size();
		}
		return data;
	}

	void SetUserHead(const void *data, int size)
	{
		m_user_head.resize(size);
		if (data&&size>0)
		{
			memcpy(&m_user_head[0], data, size);
		}
	}

};



NamedStreamSet::NamedStreamSet(const char magic[])
{
	m_imp = new NamedStreamSet::_CImp(this, magic);
}

NamedStreamSet::~NamedStreamSet()
{
	delete m_imp;
}

void NamedStreamSet::Load(const std::string &file, bool bnew)
{
	m_imp->Load(file, bnew);
}
const std::string& NamedStreamSet::GetFile()
{
	return m_imp->m_file;
}
const void* NamedStreamSet::GetUserHead(int &size)
{
	return m_imp->GetUserHead(size);
}

void NamedStreamSet::SetUserHead(const void *data, int size)
{
	m_imp->SetUserHead(data, size);
}

NamedStreamSet::StreamPtr NamedStreamSet::GetStream(const _nstring &name, bool bnew)
{
	return m_imp->GetStream(name, bnew);
}
std::string NamedStreamSet::GetStreamFile(const _nstring &name)
{
	return m_imp->GetStreamFile(name);
}

void NamedStreamSet::DeleteStream(const _nstring &name)
{
	m_imp->DeleteStream(name);
}

void NamedStreamSet::Save(const char *file, int flag)
{
	m_imp->Save(file, flag);
}

void NamedStreamSet::ListStreams(std::vector<_nstring> &vname/*, bool listBuffered*/)
{
	m_imp->ListStreams(vname, false);
}
int NamedStreamSet::nBuffered()
{
	return (int)m_imp->m_streamList.size();
}

//============================================================================================================

#define _PACKED_FILE_MAGIC  "packedf\0"

PackedFile::PackedFile(const char magic[])
	:NamedStreamFile(magic? magic : _PACKED_FILE_MAGIC)
{
}

//int PackedFile::SetHead(const void *head, int size)
//{
//	int ec=-1;
//	ff::BMStream *oms=this->GetStream(L"d:head",true);
//
//	if(oms)
//	{
//		oms->Write(head,size,1);
//		ec=0;
//	}
//
//	return ec;
//}
//
//int PackedFile::GetHead(void *head, int size)
//{
//	int ec=-1;
//
//	ff::IBMStream *ims=this->GetStream(L"d:head",false);
//
//	if(ims)
//	{
//		ims->Read(head,size,1);
//		ec=0;
//	}
//
//	return ec;
//}

int PackedFile::AddFiles(const std::vector<std::string> & vfiles, const std::string &dir, const std::vector<std::string> *vnames)
{
	int ec=0;

	const std::vector<std::string> &streamName(vnames && vnames->size()==vfiles.size()? *vnames : vfiles);

	for(size_t i=0; i<vfiles.size(); ++i)
	{
		std::string filex(dir+("\\")+vfiles[i]);

		ff::AutoFilePtr fp(_tfopen(filex.c_str(),("rb")));

		if(!fp)
		{
			ec=-1; break;
		}

		long fsize=getFileSize(fp);
		ff::AutoArrayPtr<char> buf(new char[fsize]);

		if(fread(buf,fsize,1,fp)!=1)
		{
			ec=-2; break;
		}

		ff::BMStream *oms=this->GetStream("f:"+streamName[i],true);
		if(oms)
		{
			oms->Resize(0); //clear 

			oms->Write(buf,fsize,1);
		}
		else
		{
			ec=-3; break;
		}
	}

	return ec;
}

int PackedFile::Unpack(const std::string &dir, const std::string *backup_dir)
{
	std::vector<std::string> vname;
	int ec=0;

	try
	{
		this->ListStreams(vname);
	}
	catch(...)
	{
		ec=-1;
	}

	if(ec==0)
	{
		for(size_t i=0; i<vname.size(); ++i)
		{
			if(vname[i].size()>2 && vname[i][0]=='f' && vname[i][1]==':') // if is "f:..."
			{
				std::string fname(vname[i].c_str()+2);

				ff::BMStream *ims=this->GetStream(vname[i],false);

				if(ims)
				{
					std::string filex(ff::CatDirectory(dir,fname));

					std::string dirx(ff::GetDirectory(filex));

					if(!dirx.empty())
						ff::makeDirectory(dirx);

					if(backup_dir && ff::pathExist(filex))
					{
						std::string bfile(*backup_dir+ff::GetFileName(filex));

						if(!ff::pathExist(*backup_dir))
							ff::makeDirectory(*backup_dir);

						ff::copyFile(filex.c_str(),bfile.c_str(),true);
					}

					ff::AutoFilePtr fp(fopen(filex.c_str(),"wb"));

					if(!fp ||
						fwrite(ims->Buffer(),ims->Size(),1,fp)!=1
						)
						ec=-2;
				}
			}
		}
	}

	return ec;
}

//====================================================================


static int _load_text_blocks(istream_t &is, std::vector<TextBlock> &vblk)
{
	int n=0;
	bool bFound=false;
	
	if(is)
	{
		std::string lnstr;

		std::list<TextBlock>  lblk;
		
		std::string  text;
		text.reserve(64*1024);

		while(std::getline(is,lnstr,('\n')))
		{
			const char *px=lnstr.c_str(), *pend=px+lnstr.size();

			px=SkipSpace(px,pend);

			if(px==pend || px[0]==('/')&&px[1]==('/')) //if is empty or commented
				continue;

			if(!bFound)
			{
				if(px[0]==('$')&&px[1]==('{'))
				{
					while(_istspace(pend[-1]) )
						--pend;

					assert(px+2<=pend);

					lblk.push_back(TextBlock());
					lblk.back().m_title.append(px+2,pend);
					text.resize(0);

					bFound=true;
				}
			}
			else
			{
				if(px[0]==('$')&&px[1]==('}'))
				{
					lblk.back().m_text.append(text.begin(),text.end());

					++n;
					bFound=false;
				}
				else
				{
			//		if(!lnstr.empty()&&*lnstr.rbegin()=='\r') //to handle binary data.
			//			lnstr.resize(lnstr.size()-1);

					text.append(px,pend);
					text.push_back(('\n'));
				}
			}
		}

		size_t i=vblk.size();
		vblk.resize(vblk.size()+lblk.size());

		for(std::list<TextBlock>::iterator itr(lblk.begin()); itr!=lblk.end(); ++itr)
		{
			vblk[i].m_title.swap(itr->m_title);
			vblk[i].m_text.swap(itr->m_text);
			++i;
		}
	}

	return n;
}

int _BFC_API LoadTextBlocks(const std::string &file, std::vector<TextBlock> &vblk)
{
	ifstream_t is(file.c_str());

	if(!is)
		return -1;

	return _load_text_blocks(is,vblk);
}

//==========================================================================================
#if 0
//return the number of characters
int _get_line(FILE *fp, char buf[])
{
	int n=0, ch=0;
	
	while((ch=_fgettc(fp))!=_TEOF)
	{
		if(ch==('\n'))
			break;
		else
			buf[n++]=(char)ch;
	}

	buf[n]=0;

	return ch==_TEOF&&n==0? _TEOF : n;
}

//lines can be connected by '/'
int _get_line_ex(FILE *fp, char buf[])
{
	int nc=0;

	while(true)
	{
		int nx=_get_line(fp,buf+nc);

		nc+=nx;

		if(nx>0 && buf[nc-1]==('/'))
			--nc;
		else
			break;
	}

	return nc;
}

int _load_text_lines(const char *file, std::vector<std::string> &vline)
{
	FILE *fp=_tfopen(file,("r"));

	if(!fp)
		return -1;

	char buf[1024*64];
	int nl=0; //number of lines
	int nc=0; //number of characters

	while((nc=_get_line_ex(fp,buf))!=_TEOF)
	{
		const char *px=buf, *pe=px+nc;

		px=SkipSpace(px,pe);

		if(px!=pe && !(px[0]==('/')&&px[1]==('/')) ) //not empty and not commented
		{
			vline.push_back(std::string(px,pe));
			++nl;
		}
	}

	fclose(fp);

	return nl;
}


int _BFC_API LoadTextLines(const std::string &file, std::vector<std::string> &vlines)
{
	return _load_text_lines(file.c_str(),vlines);
}




int _BFC_API SplitText(const std::string &text, std::vector<std::string> &vsplit, char split_char)
{
	int n=0;
	const char *px=text.c_str(), *pe=px+text.size();
	const char *pb=NULL;

	do
	{
		if(px!=pe && *px!=split_char)
		{
			if(!pb)
				pb=px;
		}
		else
		{
			if(pb && pb!=px)
			{
				std::string val(_ignore_blank_sides(pb,px));
				if(!val.empty())
				{
					vsplit.push_back(val);
					++n;
				}
			}
			pb=NULL;
		}

		++px;
	}
	while(px<=pe);


	return n;
}

#endif
//============================================================================================

enum
{
	_HTB_START=1,
	_HTB_END,
};

enum
{
	_HTBF_LEAF=0x01,
	_HTBF_LOADED=0x02,
};

struct HTBTag
{
	int			 m_type;
	std::string  m_name;
	std::string  m_value;
	char*		 m_pointer;
	int			 m_link;
	int			 m_flag;
public:
	HTBTag()
		:m_type(0),m_pointer(NULL),m_link(0),m_flag(0)
	{
	}
	bool is_leaf() const
	{
		return m_flag&_HTBF_LEAF? true:false;
	}
	bool is_loaded() const
	{
		return m_flag&_HTBF_LOADED? true:false;
	}
};

class HTBFileImpl
{
public:
	std::string m_file;
	std::string m_raw;

	std::vector<HTBTag>		m_vtag;
public:

	void _SearchBlocks();

	int  _MatchBlocks();

	void _Clear()
	{
		m_raw.clear();
		m_vtag.clear();
		m_file.clear();
	}

	int Load(const std::string &file);

	int  _GetBlock(int istart, int iend, const std::string &name);

	const std::string* GetBlock(const std::string &str);

};

void HTBFileImpl::_SearchBlocks()
{
	m_vtag.clear();
	m_vtag.reserve(1024);

	if(m_raw.empty())
		return;

	char *px=&m_raw[0], *pe=px+m_raw.size();
	char *pb=NULL;

	while(px!=pe)
	{
		if(!pb)
		{
			if(*px==('<') && px[-1]!=('%') ) //m_raw[0] is the space pending with load
				pb=px;
		}
		else
		{
			if(*px==('>'))
			{
				char *pbx=SkipSpace(pb+1,px);

				m_vtag.resize(m_vtag.size()+1);

				HTBTag &tx(m_vtag.back());

				if(*pbx==('/'))
				{
					tx.m_type=_HTB_END;
					tx.m_name.append(pbx+1,px);
					tx.m_pointer=pb;
				}
				else
				{
					tx.m_type=_HTB_START;
					tx.m_name.append(pbx,px);
					tx.m_pointer=px+1;
				}

				pb=NULL;
			}
		}

		++px;
	}
}

static void _translate_special_char(std::string &str)
{
	std::string dval(str.size(),(' '));
	size_t nd=0;

	const char *px=str.c_str(), *pe=px+str.size();

	for(; px!=pe; ++px)
	{
		if(*px==('%') && px+1!=pe)
		{
			switch(px[1])
			{
			case ('<'):
				dval[nd++]=('<');
				break;
			default:
				dval[nd++]=('%');
				dval[nd++]=px[1];
			}

			++px;
		}
		else
			dval[nd++]=*px;
	}

	dval.resize(nd);
	str.swap(dval);
}

static std::string _htb_ignore_blank_sides(const char *beg, const char *end)
{
	//ignore spaces
	beg = SkipSpace(beg, end);

	while (beg != end && _istspace(int(end[-1])))
	{
		--end;
	}

	return std::string(beg, end);
}

int HTBFileImpl::_MatchBlocks()
{
	for(size_t i=0; i<m_vtag.size(); ++i)
	{
		if(m_vtag[i].m_type==_HTB_START)
		{
			size_t j=i+1;

			for(; j<m_vtag.size(); ++j)
			{
				if(m_vtag[j].m_type==_HTB_END && m_vtag[j].m_name==m_vtag[i].m_name)
				{
					m_vtag[i].m_link=(int)j;
					m_vtag[j].m_link=(int)i;
					break;
				}
			}

			if(j==m_vtag.size())
			{
				printf("\nerror: unclosed block %s",m_vtag[i].m_name.c_str());
				return -1;
			}

			if(j==i+1) //Ò¶ï¿½ï¿½ï¿?
			{
				m_vtag[i].m_value=_htb_ignore_blank_sides(m_vtag[i].m_pointer,m_vtag[j].m_pointer);
				_translate_special_char(m_vtag[i].m_value);

				m_vtag[i].m_flag|=_HTBF_LEAF|_HTBF_LOADED;
			}
		}
	}

	return 0;
}

int HTBFileImpl::Load(const std::string &file)
{
	std::vector<std::string> vlines;
	int nl=LoadTextLines(file,vlines);

	if(nl<0)
	{
	//	FVT_EXCEPTION(FVT_EC_FILE_OPEN_FAILED,file.c_str());
		return -1;
	}

	m_raw.clear();
	m_raw.push_back((' ')); //pending a space, for easy the implementation of _SearchBlock

	for(size_t i=0; i<vlines.size(); ++i)
	{
		m_raw+=vlines[i];
		m_raw+=("\n");
	}

	this->_SearchBlocks();

	int ec=this->_MatchBlocks();
	if(ec<0)
		this->_Clear();
	else
	{
		m_file=ff::getFullPath(file);
	}

	return ec;
}

const char* _get_next_tag(const char *px, std::string &val)
{
	const char *pb=px;

	while(*px && *px!=('/') && *px!=('\\') && *px!=('.') )
		++px;

	val=std::string(pb,px);

	return *px? px+1 : px;
}

int HTBFileImpl::_GetBlock(int istart, int iend, const std::string &name)
{
	assert(istart<=iend && (size_t)iend<=m_vtag.size());

	for(int i=istart; i<iend; ++i)
	{
		if(m_vtag[i].m_type==_HTB_START)
		{
			if(m_vtag[i].m_name==name)
				return i;
			else
				i=m_vtag[i].m_link; //skip to the end
		}
	}

	return -1;
}

const std::string* HTBFileImpl::GetBlock(const std::string &str)
{
	int istart=-1, iend=(int)m_vtag.size();
	const char *px=str.c_str();

	while(true)
	{
		std::string tag;
		px=_get_next_tag(px,tag);

		if(tag.empty())
			break;

		int i=this->_GetBlock(istart+1,iend,tag);

		if(i<0)
			return NULL;

		istart=i;
		iend=m_vtag[i].m_link;

		if(iend==istart+1) //is leaf
			break;
	}

	if(istart<0)
		return NULL;

	if(!m_vtag[istart].is_loaded())
	{
		m_vtag[istart].m_value=_htb_ignore_blank_sides(m_vtag[istart].m_pointer, m_vtag[iend].m_pointer);
		m_vtag[istart].m_flag|=_HTBF_LOADED;
	}

	return &m_vtag[istart].m_value;
}


class HTBFile::_CImp
	:public HTBFileImpl
{
};

HTBFile::HTBFile()
:m_imp(new _CImp)
{
}

HTBFile::~HTBFile()
{
	delete m_imp;
}

int HTBFile::Load(const std::string &file)
{
	return m_imp->Load(file);
}

const std::string* HTBFile::GetBlock(const std::string &path) const
{
	return m_imp->GetBlock(path);
}

const std::string& HTBFile::GetFile() const
{
	return m_imp->m_file;
}


_FF_END


