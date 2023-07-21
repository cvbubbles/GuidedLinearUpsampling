
#pragma once

#include"ffdef.h"
#include"BFC/bfstream.h"
#include<string>
#include<vector>
#include<memory>

_FF_BEG

class _BFC_API DiskSwapFile
{
	class _CImp;

	_CImp *m_imp;
public:

	DiskSwapFile();

	~DiskSwapFile();

	void Open(const std::string &file, bool bnew=true);

	void Save();

	void Close();

	void SaveBlock(const void *buf, int size, int *id);

	void SetBlockName(int id, const char *name,bool allowMultiple=true);

	int  IDFromName(const char *name);

	int  QueryBlockSize(int id);

	int  LoadBlock(void *buf, int size, int id);

	bool DeleteBlock(int id);
};

class _BFC_API  NamedStreamFile
{
	class _CImp;

	_CImp  *m_imp;
public:
	
	NamedStreamFile(const char magic[]=NULL);

	~NamedStreamFile();

	void  Load(const std::string &file, bool bnew=true);

	const std::string &GetFile();

	const void* GetUserHead(int &size);

	//return size copied into @buf
	int   GetUserHead(void *buf, int buf_size);

	void  SetUserHead(const void *head, int size);

	BMStream *GetStream(const std::string &name, bool bnew=true);

	void  DeleteStream(const std::string &name);

	enum
	{
		SAVE_TRANSFORM=0x01,
	};
	void  Save(const char *file=NULL, int flag=0);

	void  ListStreams(std::vector<std::string> &vname);

	int  ListStreams(char *vstrs[], int nmax);

	int   Size();

	/*transform (e.g. compress) data before saving data to file or load from file

	@dir   : >0 compress (called when save to file)£»<0 decompress (called when load from file)

	@RV    : >0 success£» =0 no action;¡¡<0 error 
	*/
	virtual int TransformData(const void *src, size_t isize, void *dest, size_t &dsize, int dir);
};


class _BFC_API  NamedStreamSet
{
	class _CImp;

	_CImp  *m_imp;
public:
	typedef std::shared_ptr<BFStream>  StreamPtr;
public:

	NamedStreamSet(const char magic[] = NULL);

	~NamedStreamSet();

	void  Load(const std::string &file, bool bnew = true);

	const std::string &GetFile();

	const void* GetUserHead(int &size);

	void  SetUserHead(const void *head, int size);

	StreamPtr GetStream(const std::string &name, bool bnew = true);

	std::string GetStreamFile(const std::string &name);

	void  DeleteStream(const std::string &name);

	void  Save(const char *file = NULL, int flag = 0);

	void  ListStreams(std::vector<std::string> &vname);

	int   nBuffered();
};




class _BFC_API PackedFile
	:public NamedStreamFile
{
public:
	PackedFile(const char magic[]=NULL);

	//@vnames : if is not null, used as the name of each file stream; othersie @vfiles is used
	int AddFiles(const std::vector<std::string> & vfiles, const std::string &dir, const std::vector<std::string> *vnames=NULL);

	int Unpack(const std::string &dir, const std::string *backup_dir = NULL);
};

class _BFC_API  TextBlock
{
public:
	std::string		m_title;

	std::string     m_text;
};

//return the number of blocks
//return -1 if failed
/*
${title
text
//comment
$}
*/
int _BFC_API LoadTextBlocks(const std::string &file, std::vector<TextBlock> &vblk);

#if 0
//return -1 if failed
/*
abc...\ (connected)
cdef 
//commented
*/
int _BFC_API LoadTextLines(const std::string &file, std::vector<std::string> &vlines);


int _BFC_API SplitText(const std::string &text, std::vector<std::string> &vsplit, char split_char);
#endif

/*HTB=Hierachical Text Blocks

<tag1>
 <tag11> the value of tag11 </tag11>
 <tag12>
 //comments :...
 // % : special char.
  <tag121> the value %<> of tag121 </tag121>
 </tag12>
</tag1>

<tag2>
 the value of tag2 
</tag2>

*/
class _BFC_API HTBFile
{
	class _CImp;

	_CImp  *m_imp;
public:
	HTBFile();

	~HTBFile();

	int Load(const std::string &file);

	const std::string *GetBlock(const std::string &path) const;

	const std::string& GetFile() const;
};



_FF_END




