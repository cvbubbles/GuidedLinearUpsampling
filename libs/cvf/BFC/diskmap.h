

#ifndef _FF_BFC_DISKMAP_H
#define _FF_BFC_DISKMAP_H

#include<vector>
#include<list>
#include<map>
#include<assert.h>

#include<iosfwd>

#include "bfc\bfstream.h"
#include "bfc\stdf.h"
#include "bfc\autores.h"
#include"bfc/portable.h"

_FF_BEG

template<typename _KeyT,typename _IdxT>
class DMIndexMapBase
{
public:
	typedef _KeyT	KeyType;
	typedef _IdxT	IndexType;

	static const IndexType INVALID_INDEX=(IndexType)-1;
};

template<typename _KeyT,typename _IdxT=int>
class DMIndexMap
	:public DMIndexMapBase<_KeyT,_IdxT>
{
	typedef typename std::map<_KeyT,IndexType>  _MapT;
	typedef typename _MapT::value_type _PairT;
	
	_MapT	m_map;
	std::vector<IndexType> m_free;
public:
	//functions that are not necessary for @DiskMap's implementation.

	void GetAllKeys(std::vector<_KeyT> &vKeys) const
	{
		vKeys.resize(0);
		vKeys.reserve(m_map.size());
		for(typename _MapT::const_iterator itr(m_map.begin());itr!=m_map.end();++itr)
			vKeys.push_back(itr->first);
	}
public:
	//functions that must be defined.

	void Swap(DMIndexMap &right)
	{
		m_map.swap(right.m_map);
		m_free.swap(right.m_free);
	}
	IndexType KeyToIndex(const KeyType & key) const
	{
		typename _MapT::const_iterator itr(m_map.find(key));
		return itr==m_map.end()? INVALID_INDEX:itr->second;
	}
	std::pair<IndexType,bool> New(const KeyType &key)
	{
		IndexType idx=0;
		
		std::pair<_MapT::iterator,bool> rp(m_map.insert(_PairT(key,idx)));
		if(!rp.second)
			idx=rp.first->second;
		else
		{
			if(!m_free.empty())
			{
				idx=m_free.back();
				m_free.pop_back();
			}
			else
				idx=(IndexType)m_map.size();
			rp.first->second=idx;
		}
		return std::pair<IndexType,bool>(idx,rp.second);
	}
	void	  Remove(const KeyType &key)
	{
		_MapT::iterator itr(m_map.find(key));
		if(itr!=m_map.end())
		{
			m_free.push_back(itr->second);
			m_map.erase(itr);
		}
	}
	void Save(OBFStream &obs,BFStream::PosType beg)
	{
		obs.Seek(beg,SEEK_SET);
		obs<<m_map<<m_free;
	}

	void Load(IBFStream &ibs,BFStream::PosType beg)
	{
		ibs.Seek(beg,SEEK_SET);
		ibs>>m_map>>m_free;
	}
};

template<typename _DiskMapT>
class DMDataOpBase
{
public:
	typedef typename _DiskMapT::KeyType KeyType;
	typedef typename _DiskMapT::IndexType IndexType;
	typedef _DiskMapT			MapType;
protected:
	_DiskMapT *m_pMap;
public:
	void SetMap(_DiskMapT *pMap)
	{
		m_pMap=pMap;
	}
	//return true if need to process this item.
	bool Start(const KeyType &key,IndexType idx)
	{
		return true;
	}
	//return true if need to save modified data.
	bool Process(typename _DiskMapT::ClientData &data)
	{
		return false;
	}
};


template<typename _DataT,typename _IdxMapT,typename _HeadDataT, typename _FileHead>
class DiskMap
{
	typedef DiskMap<_DataT,_IdxMapT,_HeadDataT,_FileHead> _MyT;
public:
	typedef _DataT	  DataType;
	typedef _IdxMapT IndexMapType;
	typedef _HeadDataT   HeadDataType;

	typedef typename _IdxMapT::KeyType KeyType;
	typedef typename _IdxMapT::IndexType IndexType;
public:
	static const IndexType INVALID_INDEX=_IdxMapT::INVALID_INDEX;

	class ClientData
	{
	public:
		_DataT		m_data;
		IndexType	m_nID;
		KeyType		m_key;
	public:
		ClientData()
			:m_nID(INVALID_INDEX)
		{
		}
	};

	typedef AutoClientPtr<_MyT*,_DataT*> AutoDataPtr;

public:
	//static const int CUSTOM_HEAD_SIZE=_HeadSize;
	static const int INVALID_FILE_POS=-1;
	typedef BFStream::PosType  _FilePosT;

	class FileBlock
	{
	public:
		_FilePosT m_beg;
		size_t	  m_size;
		int		  m_flag;
	public:
		FileBlock(_FilePosT beg=INVALID_FILE_POS,size_t size=0,int flag=0)
			:m_beg(beg),m_size(size),m_flag(flag)
		{
		}
		/*friend void BFSRead(IBFStream &ibs,FileBlock &val)
		{
			ibs.Read(&val,sizeof(val),1);
		}
		friend void BFSWrite(OBFStream &obs,const FileBlock &val)
		{
			obs.Write(&val,sizeof(val),1);
		}*/
		DEFINE_BFS_IO_3(FileBlock,m_beg,m_size,m_flag)
	};

	class FileStruct
	{
#define _TITLE_SIZE 256

	public:
		uchar	  m_title[_TITLE_SIZE];
	    _FileHead m_fhead;
		size_t	  m_size;
		FileBlock m_head;
		FileBlock m_map;
		FileBlock m_gblock;
		FileBlock m_free;
	public:
		FileStruct()
			:m_size(0)
		{
			InitTitle(m_title);
		}
		static bool IsValidTitle(const uchar pTitle[_TITLE_SIZE])
		{
			for(int i=0;i<_TITLE_SIZE;++i)
				if(pTitle[i]!=i)
					return false;
			return true;
		}
		static void InitTitle(uchar pTitle[_TITLE_SIZE])
		{
			for(int i=0;i<_TITLE_SIZE;++i)
				pTitle[i]=(uchar)i;
		}
#undef _TITLE_SIZE
	};

	enum{_DBF_MODIFIED=0x01};

	class DataBuf
	{
	public:
		ClientData  m_client;
		int			m_nRef;
		int			m_flag;
	public:
		DataBuf()
			:m_nRef(0),m_flag(0)
		{
		}
		bool Modified() const
		{
			return (m_flag&_DBF_MODIFIED)!=0;
		}
		
	};
	typedef std::list<DataBuf> _DataBufListT;

	class DataHeader
	{
	public:
		int						m_nID;
		KeyType					m_key;
		HeadDataType			m_data;
		FileBlock				m_block;
	public:
		DataHeader()
			:m_nID(INVALID_INDEX)
		{
		}
		DataHeader(int nID,const KeyType &key,const FileBlock &block)
			:m_nID(nID),m_key(key),m_block(block)
		{
		}
		bool IsValid() const
		{
			return m_nID!=INVALID_INDEX;
		}
		void SetInvalid()
		{
			m_nID=INVALID_INDEX;
		}
		/*friend void BFSRead(IBFStream &ibs,DataHeader &val)
		{
			ibs>>val.m_nID>>val.m_key>>val.m_data>>val.m_block;
		}
		friend void BFSWrite(OBFStream &obs,const DataHeader &val)
		{
			obs<<val.m_nID<<val.m_key<<val.m_data<<val.m_block;
		}*/
		DEFINE_BFS_IO_4(DataHeader,m_nID,m_key,m_data,m_block)
	};

	class HeaderItem
	{
	public:
		DataHeader				m_dataHead;
		typename _DataBufListT::iterator m_bufItr;
	public:
		HeaderItem()
		{
		}
		HeaderItem(typename _DataBufListT::iterator bufItr)
			:m_bufItr(bufItr)
		{
		}
	};
	typedef std::vector<HeaderItem> _HeaderListT;

	class  GBlock
	{
	public:
		std::string    m_name;
		BMStream  m_blk;
	public:
		DEFINE_BFS_IO_2(GBlock,m_name,m_blk)
	};

	typedef std::vector<GBlock>   _GBListT;

//	static const int N_DEF_TITLE_SIZE=512;
	static const int DEF_BLOCK_SIZE=1024*256;
	
	typedef std::vector<FileBlock>  _FreeBlockListT;
private:
	FileStruct      m_fs;
	_IdxMapT		m_idxMap;
	_HeaderListT    m_headList;
	_FreeBlockListT m_freeBlk;
	_GBListT        m_gblock;


	_DataBufListT   *m_pBufList;

	string_t     m_strFile;
	BFStream		m_BFS;
	double		    m_maxFree;
	
private:
	FileBlock _save_data(const _DataT &data)
	{
		m_BFS.Seek(0,SEEK_END);
		_FilePosT beg=m_BFS.Tell();
		m_BFS<<data;
		return FileBlock(beg,m_BFS.Tell()-beg);
	}

	bool _is_reffered(const HeaderItem &itm)
	{
		return itm.m_bufItr!=m_pBufList->end();
	}
	bool _on_err_release_ref(typename _DataBufListT::iterator bufItr)
	{//handle the error when release referenced object.

		FF_ERROR(ERR_INVALID_OP,"Try to release referenced data!");
		return true;
	}
	bool _release_buf_item(typename _DataBufListT::iterator bufItr)
	{
		if(bufItr!=m_pBufList->end())
		{
			if(bufItr->m_nRef>0)
			{
				if(!this->_on_err_release_ref(bufItr))
					return false;
			}
			HeaderItem *phi=&m_headList[bufItr->m_client.m_nID];
			if(bufItr->Modified())
			{
				m_freeBlk.push_back(phi->m_dataHead.m_block);
				phi->m_dataHead.m_block=this->_save_data(bufItr->m_client.m_data);
			}
			m_pBufList->erase(bufItr);
			phi->m_bufItr=m_pBufList->end();
		}
		return true;
	}
	bool _remove_data(int id)
	{
		if(size_t(id)<m_headList.size())
		{
			DataHeader *pdh=&m_headList[id].m_dataHead;
			if(pdh->IsValid())
			{
				assert(id==pdh->m_nID);
				m_freeBlk.push_back(pdh->m_block);
				this->_release_buf_item(m_headList[id].m_bufItr);
				m_headList[id].m_bufItr=m_pBufList->end();

				pdh->SetInvalid();
				m_idxMap.Remove(pdh->m_key);
				--m_fs.m_size;
				return true;
			}
		}

		return false;
	}
	std::pair<IndexType,bool> _add_data(const KeyType &key,const _DataT &data,const HeadDataType &headData,bool bOverwrite)
	{
		std::pair<IndexType,bool> ipr(m_idxMap.New(key));
		IndexType id=ipr.first;

		if(int(m_headList.size())<=id)
		{
			size_t sz0=m_headList.size();
			m_headList.resize(id+1);
			for(;(int)sz0<id+1;++sz0)
				m_headList[sz0].m_bufItr=m_pBufList->end();
		}
		DataHeader *pdh=&m_headList[id].m_dataHead;
		if(pdh->IsValid())
		{
			if(!bOverwrite)
				return ipr;
			else
			{
				this->_release_buf_item(m_headList[id].m_bufItr);
				m_freeBlk.push_back(pdh->m_block);
			}
		}
		else
			++m_fs.m_size;

		pdh->m_block=this->_save_data(data);
		pdh->m_nID=id;
		pdh->m_key=key;
		pdh->m_data=headData;
		m_headList[id].m_bufItr=m_pBufList->end();
		return ipr;
	}
	void _load_buf_item(HeaderItem *phi)
	{
		if(phi->m_bufItr==m_pBufList->end())
		{
			m_pBufList->resize(m_pBufList->size()+1);
			m_BFS.Seek(phi->m_dataHead.m_block.m_beg,SEEK_SET);
			m_BFS>>m_pBufList->back().m_client.m_data;
			m_pBufList->back().m_client.m_key=phi->m_dataHead.m_key;
			m_pBufList->back().m_client.m_nID=phi->m_dataHead.m_nID;
			m_pBufList->back().m_flag=0;
			m_pBufList->back().m_nRef=0;
			--phi->m_bufItr;
		}
	}

#if 0
public:
	void _check_buf_itr()
	{
		for(_HeaderListT::iterator itr(m_headList.begin());itr!=m_headList.end();++itr)
			if(itr->m_bufItr!=m_pBufList->end())
			{
				printf("NRef:%d.\n",itr->m_bufItr->m_nRef);
			}
	}
#endif

private:
	AutoDataPtr  _get_data(int id,bool bConst)
	{
//		this->_check_buf_itr();
		if(size_t(id)<m_headList.size())
		{
			_HeaderListT::iterator itr(m_headList.begin()+id);
			if(itr->m_dataHead.IsValid())
			{
				if(itr->m_bufItr==m_pBufList->end())
					this->_load_buf_item(&*itr);
				if(!bConst)
					itr->m_bufItr->m_flag|=_DBF_MODIFIED;
				++itr->m_bufItr->m_nRef;
				return AutoDataPtr(this,&itr->m_bufItr->m_client.m_data);
			}
		}
		return AutoDataPtr(NULL,NULL);
	}
	void _release_client(ClientData *pData)
	{
		DataBuf *pBuf((DataBuf*)pData);
		if(size_t(pData->m_nID)<m_headList.size())
		{
			assert(&*m_headList[pBuf->m_client.m_nID].m_bufItr==pBuf);
			if(--pBuf->m_nRef<=0)
				this->_release_buf_item(m_headList[pData->m_nID].m_bufItr);
		}
	}

	FileBlock _alloc_block(size_t blockSize)
	{
		m_BFS.Seek(0,SEEK_END);
		_FilePosT blockBeg=m_BFS.Tell();
		m_BFS.Resize(long(m_BFS.Size()+blockSize));

		return FileBlock(blockBeg,blockSize,0);
	}
	void   _block_assign(FileBlock &blk,IBFStream &src,_FilePosT beg,size_t size)
	{
		if(size>blk.m_size)
		{
			m_freeBlk.push_back(blk);
			blk=this->_alloc_block(size);
		}
		BFSCopy(src,beg,m_BFS,blk.m_beg,size);
	}
	void _read_header()
	{
		m_BFS.Seek(m_fs.m_head.m_beg,SEEK_SET);
		//read item number.
		_HeaderListT headTemp(m_BFS.Read<size_t>());
		//read item.
		for(_HeaderListT::iterator itr(headTemp.begin());itr!=headTemp.end();++itr)
		{
			m_BFS>>itr->m_dataHead;
			itr->m_bufItr=m_pBufList->end();
		}
		m_headList.swap(headTemp);
	}
	void _head_to_bfs(OBFStream &bfs) const
	{
		bfs.Resize(0);
		bfs<<m_headList.size();
		for(_HeaderListT::const_iterator itr(m_headList.begin());itr!=m_headList.end();++itr)
			bfs<<itr->m_dataHead;
	}
	void _save_header()
	{
		BFStream bfs(getTempFile(_TX("diskmap_temp_file")));
		_head_to_bfs(bfs);
		
		this->_block_assign(m_fs.m_head,bfs,0,bfs.Size());
	}
	void _read_map()
	{
		m_idxMap.Load(m_BFS,m_fs.m_map.m_beg);
	}
	void _save_map()
	{
		BFStream bfs(getTempFile(_TX("diskmap_temp_file")));
		m_idxMap.Save(bfs,0);
		this->_block_assign(m_fs.m_map,bfs,0,bfs.Size());
	}
	void _read_free()
	{
		m_BFS.Seek(m_fs.m_free.m_beg,SEEK_SET);
		m_BFS>>m_freeBlk;
	}
	void _save_free()
	{
		BFStream bfs(getTempFile(_TX("diskmap_temp_file")));
		bfs<<m_freeBlk;
		this->_block_assign(m_fs.m_free,bfs,0,bfs.Size());
	}
	void _read_gblock()
	{
		m_BFS.Seek(m_fs.m_gblock.m_beg,SEEK_SET);
		m_BFS>>m_gblock;
	}
	void _save_gblock()
	{
		BFStream bfs(ff::getTempFile(_TX("diskmap_temp_file")));
		bfs<<m_gblock;
		this->_block_assign(m_fs.m_gblock,bfs,0,bfs.Size());
	}
	void _load(const string_t &file)
	{
		if(!m_pBufList->empty())
			FF_EXCEPTION(ERR_INVALID_OP,"Try to release the referenced object!");

		m_BFS.Open(file,false);
		m_BFS.Seek(0,SEEK_SET);
		m_BFS.Read(&m_fs,sizeof(m_fs),1);
		if(FileStruct::IsValidTitle(m_fs.m_title))
		{
			_read_header();
			_read_map();
			_read_free();
			_read_gblock();
		}	
		else
			FF_EXCEPTION(ERR_INVALID_OP,"");

		m_strFile=file;
	}
	void _save()
	{
		if(m_BFS.IsOpen())
		{
			if(double(this->_get_free_size())/this->FileSize()>this->m_maxFree)
				this->Compact();

			m_BFS.Seek(0,SEEK_SET);

			assert(FileStruct::IsValidTitle(m_fs.m_title));

			m_BFS.Write(&m_fs,sizeof(m_fs),1);
			_save_header();
			_save_map();
			_save_free();
			_save_gblock();
		}
	}
	FileBlock _alloc_block(OBFStream &bfs,size_t blockSize)
	{
		bfs.Seek(0,SEEK_END);
		_FilePosT beg=bfs.Tell();
		bfs.Resize(long(beg+blockSize));
		if(blockSize>=sizeof(size_t))
			bfs<<size_t(0);
		return FileBlock(beg,blockSize,0);
	}
	void _init_file_struct(FileStruct &fs,size_t initHeadSize,size_t initMapSize,size_t initFreeSize,size_t initGBSize)
	{
		memset(&fs,0,sizeof(fs));
		FileStruct::InitTitle(fs.m_title);
		_FilePosT pos=sizeof(FileStruct);
		fs.m_map.m_beg=pos,fs.m_map.m_size=initMapSize;
		pos+=(_FilePosT)initMapSize;
		fs.m_head.m_beg=pos,fs.m_head.m_size=initHeadSize;
		pos+=(_FilePosT)initHeadSize;
		fs.m_free.m_beg=pos,fs.m_free.m_size=initFreeSize;
		pos+=(_FilePosT)initFreeSize;
		fs.m_gblock.m_beg=pos,fs.m_gblock.m_size=initGBSize;
		fs.m_size=m_fs.m_size;
	}
	void _create(const string_t &file,size_t initHeadSize,size_t initMapSize)
	{
		OBFStream bfs(file);
		FileStruct fs;
		bfs.Write(&fs,sizeof(fs),1);
		fs.m_map=this->_alloc_block(bfs,initMapSize);
		fs.m_head=this->_alloc_block(bfs,initHeadSize);
		fs.m_gblock=this->_alloc_block(bfs,DEF_BLOCK_SIZE);
		fs.m_free=this->_alloc_block(bfs,DEF_BLOCK_SIZE);
		bfs.Seek(0,SEEK_SET);
		bfs.Write(&fs,sizeof(fs),1);
	}
	void _compact_to(OBFStream &os)
	{
		if(m_BFS.IsOpen())
		{
			os.Seek(0,SEEK_SET);
			
			FileStruct newFS;
			this->_init_file_struct(newFS,m_fs.m_head.m_size,m_fs.m_map.m_size,m_fs.m_free.m_size,m_fs.m_gblock.m_size);	
			os.Write(&newFS,sizeof(m_fs),1);
			BFSCopy(m_BFS,m_fs.m_map.m_beg,os,newFS.m_map.m_beg,m_fs.m_map.m_size);
			BFSCopy(m_BFS,m_fs.m_head.m_beg,os,newFS.m_head.m_beg,m_fs.m_head.m_size);
			BFSCopy(m_BFS,m_fs.m_gblock.m_beg,os,newFS.m_gblock.m_beg,m_fs.m_gblock.m_size);
			BFSCopy(m_BFS,m_fs.m_free.m_beg,os,newFS.m_free.m_beg,m_fs.m_free.m_size);
			os.Seek(0,SEEK_END);

			_HeaderListT newHeadList(m_headList);
			
			for(size_t i=0;i<m_headList.size();++i)
			{
				if(m_headList[i].m_dataHead.IsValid())
				{
					newHeadList[i]=m_headList[i];
					FileBlock &newBlk(newHeadList[i].m_dataHead.m_block);
					os.Seek(0,SEEK_END);
					newBlk.m_beg=os.Tell();
					if(
						BFSCopy(m_BFS,m_headList[i].m_dataHead.m_block.m_beg,os,newBlk.m_beg,newBlk.m_size)!=newBlk.m_size
						)
						FF_EXCEPTION(ERR_FILE_WRITE_FAILED,"");
				}
			}
			m_headList.swap(newHeadList);
			m_freeBlk.clear();
			m_fs=newFS;
		}
	}
	void _compact()
	{
		string_t tempFile(m_strFile+_TX(".diskmap.compact.tmp"));
		OBFStream os(tempFile);
		this->_compact_to(os);
		m_BFS.Close();
		os.Close();
		DeleteFileEx(m_strFile);
		RenameFileEx(tempFile,m_strFile);
		m_BFS.Open(m_strFile,false);
	}
	size_t _get_free_size()
	{
		size_t sz=0;
		for(_FreeBlockListT::const_iterator itr(m_freeBlk.begin());itr!=m_freeBlk.end();++itr)
			sz+=itr->m_size;
		return sz;
	}
	void _print_data(std::ostream &os,IndexType idx,FlagType<true>)
	{
		AutoDataPtr ptr(this->GetDataEx(idx,true));
		if(ptr)
			os<<(*ptr);
	}
	void _print_data(std::ostream &os,IndexType idx,FlagType<false>)
	{
		os<<"...";
	}
	template<bool _BPrintData>
	void _print(std::ostream &os)
	{
		for(IndexType idx(this->IndexBegin());idx<this->IndexEnd();++idx)
		{
			if(!this->IsNullIndex(idx))
			{
				const KeyType key(this->IndexToKey(idx));
				os<<'<'<<key<<"><"<<idx<<">:";
				_print_data(os,idx,FlagType<_BPrintData>());
				os<<'\n';
			}
		}
	}
	template<typename _DataOpT>
	void _for_each_item(_DataOpT &op)
	{
		op.SetMap((typename _DataOpT::MapType*)this);
		for(_HeaderListT::iterator itr(m_headList.begin());itr!=m_headList.end();++itr)
		{
			if(itr->m_dataHead.IsValid()&&op.Start(itr->m_dataHead.m_key,itr->m_dataHead.m_nID))
			{
				AutoDataPtr ptr(this->GetDataEx(itr->m_dataHead.m_nID,false));
				if(ptr)
				{
					if(op.Process(*(ClientData*)(&*ptr)))
					{
						assert(itr->m_bufItr!=m_pBufList->end());
						itr->m_bufItr->m_flag|=_DBF_MODIFIED;
					}
				}
			}
		}
	}
public:
	void ReleaseClient(_DataT *pData)
	{
		this->_release_client((ClientData*)pData);
	}
private:
	DiskMap(const DiskMap&);
	DiskMap& operator=(const DiskMap&);
public:
	DiskMap()
	{
		m_pBufList=new _DataBufListT;
	}
	~DiskMap()
	{
		if(!m_pBufList->empty())
			FF_ERROR(ERR_INVALID_OP,"Try to destroy referenced map.");

		try
		{
			this->Save();
		}
		catch(...)
		{
		}
		delete m_pBufList;
	}
	void Swap(DiskMap &right)
	{
#if 1
		std::swap(m_fs,right.m_fs);
		m_idxMap.Swap(right.m_idxMap);
		m_headList.swap(right.m_headList);
		m_freeBlk.swap(right.m_freeBlk);
		m_gblock.swap(right.m_gblock);
	
	//	m_pBufList->swap(right.m_bufList);
		std::swap(m_pBufList,right.m_pBufList);

		m_strFile.swap(right.m_strFile);
		m_BFS.Swap(right.m_BFS);
		std::swap(m_maxFree,right.m_maxFree);
#else
		SwapObjectMemory(*this,right);
#endif
	}
	void Load(const string_t &strFile,
				size_t initHeadSize=DEF_BLOCK_SIZE,
				size_t initMapSize=DEF_BLOCK_SIZE,
				double maxFree=0.3
				)
	{
		if(!ff::pathExist(strFile))
			this->_create(strFile,initHeadSize,initMapSize);
#if 1
		DiskMap temp;
		temp._load(strFile);
//		temp._check_buf_itr();
//		this->_check_buf_itr();
		this->Swap(temp);
//		this->_check_buf_itr();
//		temp._check_buf_itr();
#else
		this->_load(strFile);
#endif
		m_maxFree=maxFree;
	}
	//get the size of the disk file.
	size_t FileSize()
	{
		return m_BFS.Size();
	}
	//get the number of data items.
	size_t Size() const
	{
		return m_fs.m_size;
	}
	void Save()
	{
		this->_save();
	}
	const IndexMapType& IndexMap() const
	{
		return m_idxMap;
	}
	//get index from key.
	IndexType  KeyToIndex(const KeyType &key)
	{
		return m_idxMap.KeyToIndex(key);
	}
	const KeyType& IndexToKey(IndexType idx)
	{
		if(this->IsNullIndex(idx))
			FF_ERROR(ERR_INVALID_ARGUMENT,"");
		
		return m_headList[idx].m_dataHead.m_key;
	}
	//whether the index is null. (corresponding to no data)
	bool	IsNullIndex(IndexType idx) const
	{
		return size_t(idx)>=m_headList.size()||!m_headList[idx].m_dataHead.IsValid();
	}
	//get the minimal index.
	IndexType IndexBegin() const
	{
		return 0;
	}
	//get the index end.
	IndexType IndexEnd() const
	{
		return (IndexType)m_headList.size();
	}
	//get data according to index.
	//@bConst:whether the data would be modified, if the data is accessed as const, its mofification
	//would not be save to disk. a data item may be referenced by both const and non-const auto-ptr,
	//in which case it is taken as non-const and any modification would be take effect to the disk file,
	//including those through const pointer.
	AutoDataPtr GetDataEx(IndexType id,bool bConst)
	{
		return this->_get_data(id,bConst);
	}
	AutoDataPtr GetData(const KeyType &key,bool bConst)
	{
		return this->GetDataEx(this->KeyToIndex(key),bConst);
	}
	HeadDataType& GetHeadDataEx(IndexType idx)
	{
		if(this->IsNullIndex(idx))
			FF_ERROR(ERR_INVALID_ARGUMENT,"");

		return m_headList[idx].m_dataHead.m_data;
	}
	HeadDataType& GetHeadData(const KeyType &key)
	{
		return this->GetHeadData(this->KeyToIndex(key));
	}
	//add data item to the set.
	//@bOverwrite: whether to overwrite the existing data with the same key.
	//@RV.first: index of the added data.
	//@RV.second: true if no overwritting occur, false otherwise.

	std::pair<IndexType,bool>  AddData(const KeyType &key,const _DataT &data,const HeadDataType& headData,bool bOverwrite)
	{
		return this->_add_data(key,data,headData,bOverwrite);
	}
	std::pair<IndexType,bool>  AddData(const KeyType &key,const _DataT &data,bool bOverwrite)
	{
		return this->_add_data(key,data,HeadDataType(),bOverwrite);
	}

	bool  RemoveEx(IndexType id)
	{
		return this->_remove_data(id);
	}
	bool Remove(const KeyType &key)
	{
		return this->RemoveEx(this->KeyToIndex(key));
	}
	void Compact()
	{
		this->_compact();
	}

	template<bool _BPrintData>
	void Print(std::ostream &os)
	{
		this->_print<_BPrintData>(os);
	}

	template<typename _DataOpT>
	void ForEachItem(_DataOpT &op)
	{
		this->_for_each_item(op);
	}

	_FileHead&  GetFileHeader()
	{
		return this->m_fs.m_fhead;
	}

	BMStream*  GetBlock(const std::string &name, bool bnew=true)
	{
		for(_GBListT::iterator itr(m_gblock.begin());itr!=m_gblock.end();++itr)
		{
			if(itr->m_name==name)
				return &itr->m_blk;
		}
		if(bnew)
		{
			m_gblock.push_back(GBlock());
			m_gblock.back().m_name=name;
			return &m_gblock.back().m_blk;
		}
		return NULL;
	}
};

_FF_END


#endif

