
#ifndef _FF_BFC_STDF_H
#define _FF_BFC_STDF_H

#include"ffdef.h"

#include<string>
#include<vector>

_FF_BEG

inline bool IsDirChar(char_t ch)
{
	return ch==_TX('\\')||ch==_TX('/');
}
/*
Retrive the directory from a file name.
If @file itself is a directory name the return value is the directory itself.
If @file contain no directory information, the return value is an empty string.

NOTE: a directory should always end with backslash("\").
*/
string_t _BFC_API GetDirectory(const string_t& file);

/*
Get the father directory of a file or directory.
If @file is a directory, the return value is its father directory, eg. "c:\windows\" => "c:\"
If @file is a file name, the return value is the father directory of its directory, eg. "c:\windows\file.txt" =>"c:\"

NOTE: a directory should always end with backslash("\").
*/
string_t _BFC_API GetFatherDirectory(const string_t& file);
  
/*
Get the name of a file (along with extension) or directory,
@bExt : whether to preserve the file extention.
eg.:
"c:\file.txt" => "file.txt"  if @bExt is true; "file" if @bExt is false.
"c:\windows\" => "windows"

NOTE: a directory should always end with backslash("\").

*/
string_t _BFC_API GetFileName(const string_t& file,bool bExt=true);

/*
Get extension of a file.
If @file is directory name or contain no extension, the return value is empty string.


NOTE: a directory should always end with backslash("\").

*/
string_t _BFC_API GetFileExtention(const string_t& file);



/*
Revise path name.
Return the mask which indicate the operations finished successfully.
*/
enum
{
	RP_PUSH_BACK_SLASH=0x01, //append '\\' if the path is not end with '\\'.
	RP_POP_BACK_SLASH=0x02,  //remove '\\' if the path is end with '\\'.
};
int  _BFC_API RevisePath(string_t &dirName,int mode=RP_PUSH_BACK_SLASH);

enum
{
	RPE_DIRECTORY=1,
	RPE_FILE_NAME,
	RPE_DOT,
	RPE_FILE_EXTENTION
};

string_t _BFC_API ReplacePathElem(const string_t &path,const string_t &value,int elemType);

//make a directory.
//return false if the directory is exist!
//bool  _BFC_API MakeDirectory(const string_t &dirName);

//bool _BFC_API RemoveDirectoryEx(const string_t &dirName);
//delete file.
//return false if failed!
bool _BFC_API  DeleteFileEx(const string_t &file);

bool _BFC_API  RenameFileEx(const string_t &file,const string_t &newName);

//whether the string contain slash or backslash
bool _BFC_API ContainDirChar(const string_t &str);

//bool _BFC_API IsExistPath(const string_t &path);
//
//bool _BFC_API IsDirectory(const string_t &path);

//concatenate dir name and file name to make a full path.
string_t _BFC_API  CatDirectory(const string_t &dir,const string_t &file);

class _BFC_API FilePath
{
public:
	string_t  dir;
	string_t  fileBase;
	string_t  fileExt;
public:
	FilePath() {}

	FilePath(const string_t &file)
	{
		this->setPath(file);
	}

	void setPath(const string_t &file);

	string_t fileName() const
	{
		return fileBase + _TX(".") + fileExt;
	}
	string_t fullPath() const
	{
		return CatDirectory(dir, fileName());
	}
};


//string_t  _BFC_API GetTempFile(const string_t &preFix,const string_t &path=_TX(""),bool bUnique=true);

enum
{
	CS_TO_UPPER,
	CS_TO_LOWER
};

void _BFC_API  ConvertString(char_t *pbeg,char_t *pend,int type=CS_TO_LOWER);

void  _BFC_API  ConvertString(string_t &str,int type=CS_TO_LOWER);

inline void str2lower(string_t &str)
{
	ConvertString(str, CS_TO_LOWER);
}
inline void str2upper(string_t &str)
{
	ConvertString(str, CS_TO_UPPER);
}

_BFC_API char_t* SkipSpace(const char_t *pbeg,const char_t *pend);

_BFC_API char_t* SkipNonSpace(const char_t *pbeg,const char_t *pend);

_BFC_API char_t* SkipUntil(const char_t *pbeg, const char_t *pend, char_t cv);

int _BFC_API StrICmp(const string_t &left,const string_t &right);

_BFC_API char_t* ReadString(const char_t *pbeg,const char_t *pend, string_t &str);


// e.g. to find the match parentheses  (...(..)...)
template<typename _ItrT,typename _ValT>
_ItrT FindMatch(_ItrT beg,_ItrT end,const _ValT &leftVal,const _ValT &rightVal,int startNLeft=0)
{
	int nLeft=startNLeft;

	for(;beg!=end;++beg)
	{
		if(*beg==leftVal)
			++nLeft;
		else
			if(*beg==rightVal)
				--nLeft;
		if(nLeft<=0)
			break;
	}
	return beg;
}

//e.g. find sub-string contain only the specified two characters.
template<typename _ItrT,typename _ValT>
_ItrT FindSub2(_ItrT beg,_ItrT end,const _ValT &val1,const _ValT &val2)
{
	for(;beg!=end;++beg)
	{
		if(*beg==val1)
		{
			if(++beg==end)
				break;
			else
				if(*beg==val2)
				{
					--beg;
					break;
				}
		}
	}
	return beg;
}


/*
Copy memory from one rectangular region to another.
@pDst		: beginning address of destination.
@dstStep	: distance of adjcent rows of @pDst in bytes.
@pSrc		: beginning address of source.
@srcStep	: distance of adjcent rows of @pSrc in bytes.
@rowBytes	: bytes to copy in each row.
@nRows		: number of rows to copy.
*/
void _BFC_API MemCopy2D(void* pDst,size_t dstStep,const void * pSrc,size_t srcStep,size_t rowBytes,size_t nRows);


_BFC_API  std::string  StrFormat(const char *pfmt,...);

_BFC_API  std::wstring  StrFormat(const wchar_t *pfmt,...);

//swap two objects by memory copy.
template<typename _ObjT>
inline void SwapObjectMemory(_ObjT& left,_ObjT& right)
{
	enum{_MSZ=sizeof(_ObjT)};
	char buf[_MSZ];
	memcpy(buf,&left,_MSZ);
	memcpy(&left,&right,_MSZ);
	memcpy(&right,buf,_MSZ);
}

template<typename _ValT,typename _MinT,typename _MaxT>
inline _ValT Truncate(_ValT val,_MinT _min,_MaxT _max)
{
	return val<_min? _min:val>_max? _max:val;
}
template<typename _PtrValT>
inline _PtrValT * ByteDiff(_PtrValT *ptr,int off)
{
	return (_PtrValT*)((char*)ptr+off);
}
template<typename _ValT,int _N>
inline int  ArrSize(_ValT (&arr)[_N])
{
	return _N;
}

template<typename _VectorT>
inline typename _VectorT::value_type* VecMem(_VectorT &vec,int pos=0)
{
	return (typename _VectorT::value_type*)(pos<(int)vec.size()? &vec[pos]:NULL);
}
template<typename _DestT,typename _VectorT>
inline _DestT* VecMemAs(_VectorT &vec,int pos=0)
{
	return (_DestT*)(VecMem(vec,pos));
}

//=================================================================================

#define ArrSize(arr) (sizeof(arr)/sizeof(arr[0]))

#define AL_ARR_BE(arr) &((arr)[0]),&((arr)[ArrSize(arr)])

#define AL_ARR_BN(arr) &((arr)[0]),ArrSize(arr)

_BFC_API int  SplitText(const string_t &text, std::vector<string_t> &vsplit, char_t split_char);

_BFC_API int LoadTextLines(const string_t &file, std::vector<string_t> &vlines);


_FF_END

#endif


