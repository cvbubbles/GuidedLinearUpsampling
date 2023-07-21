
#include"BFC/stdf.h"
#include"BFC/err.h"
#include"BFC/portable.h"
#include"BFC/cfg.h"
#include<stdarg.h>
#include<memory>
//#include<direct.h>

#pragma warning(disable:4996)

_FF_BEG 

 
/*
@ReviseFileName can apply following conversions to the file name @file:

1)"."		=>	@currDir.
2)".."		=>	father directory of @currDir.
3)".\\*"	=>	replace ".\\" with @currDir.
4)"..\\*"	=>  replace "..\\" with father directory of @currDir.
5)"*"		=>  @currDir+"*" if "*" is not a full path ( contain no disk descriptor in Windows.)
*/
//string_t _BFC_API ReviseFileName(const string_t& file,const string_t& currDir)
//{
//	string_t cdir(currDir),rfile(file);
//	if(*cdir.rbegin()!=_TX('\\'))
//		cdir.push_back(_TX('\\'));
//
//	if(rfile.empty())
//		rfile=cdir;
//	else if(rfile.size()==1)
//	{
//		if(rfile[0]==_TX('.'))
//			rfile=cdir;
//		else
//			rfile=cdir+rfile;
//	}
//	else if(rfile[1]!=':')
//	{//not a full name.
//		if(rfile[0]==_TX('.')&&rfile[1]==_TX('.'))
//		{
//			string_t fd(GetFatherDirectory(cdir));
//			
//			if(rfile.size()>2&&rfile[2]==_TX('\\'))
//				fd.append(rfile.begin()+3,rfile.end());
//			rfile.swap(fd);
//		}
//		else
//		{//.\....
//			string_t rf(cdir);
//			if(rfile[0]==_TX('.'))
//			{
//				if(rfile[1]==_TX('\\'))
//					rf.append(rfile.begin()+2,rfile.end());
//			}
//			else
//				rf.append(rfile.begin(),rfile.end());
//			rfile.swap(rf);
//		}
//	}
//	return rfile;
//}


inline bool _is_dir(const string_t& file)
{
	return !file.empty()&&IsDirChar(*file.rbegin());
}

string_t _BFC_API GetDirectory(const string_t& file)
{
	string_t::const_iterator itr(file.end());
	while(itr!=file.begin()&&!IsDirChar(*--itr));

	if(itr!=file.begin())
		return string_t(file.begin(),itr+1);
	return _TX("");
}
string_t _BFC_API GetFatherDirectory(const string_t& file)
{
	if(!file.empty())
	{
		string_t::const_iterator itr=file.end();
		if(_is_dir(file))
			--itr;
		while(itr!=file.begin()&&!IsDirChar(*--itr));
	
		if(itr!=file.begin())//include _TX('\\') in.
			return string_t(file.begin(),itr+1);
	}
	return _TX("");
}

string_t _BFC_API GetFileName(const string_t& file,bool bExt)
{
	if(!file.empty())
	{
		string_t::const_iterator end(file.end()),itr(end);
		if(_is_dir(file))
			--end,--itr;
		while(itr!=file.begin()&&!IsDirChar(*--itr));
		
		string_t fn(IsDirChar(*itr)? itr+1:itr,end);
		if(!bExt)
		{
			itr=fn.end();
			while(itr!=fn.begin()&&*--itr!=_TX('.'));
			
			if(!fn.empty()&&*itr==_TX('.'))
				fn=string_t((string_t::const_iterator)fn.begin(),itr);
		}
		return fn;
	}
	return _TX("");
}

string_t _BFC_API GetFileExtention(const string_t& file)
{
	string_t::const_iterator itr(file.end());
	while(itr!=file.begin())
	{
		--itr;
		if(*itr==_TX('.'))
			return string_t(++itr,file.end());
		else
			if(IsDirChar(*itr))
				return _TX("");
	}
	return _TX("");
}

int  _BFC_API RevisePath(string_t &dirName,int mode)
{
	int rv=0;
	if(mode&RP_PUSH_BACK_SLASH)
	{
		if(dirName.empty()||!IsDirChar(*dirName.rbegin()))
			dirName.push_back(_TX('/'));
		rv|=RP_PUSH_BACK_SLASH;
	}
	else
	{
		if(!dirName.empty()&&IsDirChar(*dirName.rbegin()))
			dirName.resize(dirName.size()-1);
		rv|=RP_POP_BACK_SLASH;
	}
	return rv;
}

string_t _BFC_API ReplacePathElem(const string_t &path,const string_t &value,int elemType)
{
	string_t rv;
	
	switch(elemType)
	{
	case RPE_DIRECTORY:
		{
			rv=value;
			RevisePath(rv, RP_PUSH_BACK_SLASH);
			rv+=GetFileName(path,true);
		}
		break;
	case RPE_FILE_NAME:
		rv=GetDirectory(path)+value+_TX(".")+GetFileExtention(path);
		break;
	case RPE_DOT:
		rv=GetDirectory(path)+GetFileName(path,false)+value+GetFileExtention(path);
		break;
	case RPE_FILE_EXTENTION:
		rv=GetDirectory(path)+GetFileName(path,false)+_TX(".")+value;
		break;
	default:
		FF_ERROR(ERR_INVALID_ARGUMENT,"");
	}
	return rv;
}

//bool  _BFC_API MakeDirectory(const string_t &dirName)
//{
//	if(!dirName.empty() && !ff::IsExistPath(dirName))
//	{
//		if(_tmkdir(dirName.c_str()))
//		{
//			if(errno==ENOENT)
//			{
//				string_t fd(GetFatherDirectory(dirName));
//
//				if(fd.empty())
//					return false;
//				else
//					MakeDirectory(fd);
//
//				return MakeDirectory(dirName);
//			}
//		}
//		else
//			return true;
//	}
//	return false;
//}
//bool _BFC_API RemoveDirectoryEx(const string_t &dirName)
//{
//	bool rv=false;
//	try
//	{
//		if(!dirName.empty())
//		{
//			LocalFileAPI::global.RemoveDirectoryRecursively(dirName.c_str());
//			rv=true;
//		}
//	}
//	catch(...)
//	{
//	}
//	return rv;
//}

bool _BFC_API  DeleteFileEx(const string_t &file)
{
	return _tremove(file.c_str())==0;
}
bool _BFC_API  RenameFileEx(const string_t &file,const string_t &newName)
{
	return _trename(file.c_str(),newName.c_str())==0;
}
bool _BFC_API ContainDirChar(const string_t &str)
{
	for(string_t::const_iterator itr(str.begin());itr!=str.end();++itr)
		if(IsDirChar(*itr))
			return true;
	return false;
}
//bool _BFC_API IsExistPath(const string_t &file)
//{
//	return pathExist(file);
//}
//bool _BFC_API IsDirectory(const string_t &path)
//{
//	return isDirectory(path);
//}

string_t   CatDirectory(const string_t &dir,const string_t &file)
{
	if(!dir.empty()&&!file.empty())
	{
		if(IsDirChar(*dir.rbegin()))
			return dir+file;
		else
			return dir+_TX("/")+file;
	}
	return dir+file;
}

void FilePath::setPath(const string_t &file)
{
	dir = GetDirectory(file);
	fileBase = GetFileName(file, false);
	fileExt = GetFileExtention(file);
}

//static string_t get_temp_path()
//{
//	char_t buf[MAX_PATH*2];
//	DWORD len=GetTempPath(MAX_PATH*2,buf);
//	
//	buf[len]=0;
//	return buf;
//}
//string_t  _BFC_API GetTempFile(const string_t &preFix,const string_t &path,bool bUnique)
//{
//	string_t pathx(path);
//	if(pathx.empty())
//		pathx=get_temp_path();
//	char_t buf[MAX_PATH*2];
//	if(!GetTempFileName(pathx.c_str(),preFix.c_str(),bUnique,buf))
//		FF_WINAPI_FAILED();
//	return buf;
//}

void _BFC_API  ConvertString(char_t *pbeg,char_t *pend,int type)
{
	if(pbeg&&pbeg<pend)
	{
		switch(type)
		{
		case CS_TO_LOWER:
			for(;pbeg!=pend;++pbeg)
			{
				if(*pbeg>=_TX('A')&&*pbeg<=_TX('Z'))
					*pbeg+=_TX('a')-_TX('A');
			}
			break;
		case CS_TO_UPPER:
			for(;pbeg!=pend;++pbeg)
			{
				if(*pbeg>=_TX('a')&&*pbeg<=_TX('z'))
					*pbeg-=_TX('a')-_TX('A');
			}
			break;
		}
	}
}
void  _BFC_API  ConvertString(string_t &str,int type)
{
	if(!str.empty())
		ConvertString(&str[0],&str[0]+str.size(),type);
}

char_t *SkipSpace(const char_t *pbeg,const char_t *pend)
{
	while(pbeg<pend&&_istspace((unsigned short)*pbeg))
		++pbeg;

	return (char_t*)pbeg;
}

char_t *SkipNonSpace(const char_t *pbeg,const char_t *pend)
{
	while(pbeg<pend&&!_istspace((unsigned short)*pbeg))
		++pbeg;
	return (char_t*)pbeg;
}

char_t* SkipUntil(const char_t *pbeg, const char_t *pend, char_t cv)
{
	while(pbeg<pend&&*pbeg!=cv)
		++pbeg;

	return (char_t*)pbeg;
}

int  StrICmp(const string_t &left,const string_t &right)
{
	int v=(left.empty()? 1:0)|(right.empty()? 2:0);

	if(v==0)
		v=_tcsicmp(VecMem(left),VecMem(right));
	else
		v=(v&1)-((v&2)>>1);
	return v;
}

char_t* ReadString(const char_t *pbeg,const char_t *pend, string_t &str)
{
	pbeg=SkipSpace(pbeg,pend);
	pend=SkipNonSpace(pbeg,pend);
	str=string_t(pbeg,pend);
	return (char_t*)pend;
}

void _BFC_API MemCopy2D(void* pDst,size_t dstStep,const void * pSrc,size_t srcStep,size_t rowBytes,size_t nRows)
{
	if(pDst&&pSrc)
	{				
		if(rowBytes==dstStep&&rowBytes==srcStep)
			memcpy(pDst,pSrc,rowBytes*nRows);
		else
		{
			char* pdst((char*)pDst);
			char* psrc((char*)pSrc);
			for(size_t i=0;i<nRows;++i,pdst+=dstStep,psrc+=srcStep)
				memcpy(pdst,psrc,rowBytes);
		}
	}
}

std::string  StrFormatV(const char *pfmt,va_list varl)
{
	const int _BUF_SIZE=4096;

	char buf[_BUF_SIZE];
	int nw=vsnprintf(buf,_BUF_SIZE,pfmt,varl);

	/*if(nw<0)
	{
		nw=_vscprintf(pfmt,varl);
		std::string rstr;
		rstr.resize(nw);
		vsnprintf(&rstr[0],nw,pfmt,varl);
		return rstr;
	}*/
	
	return std::string(buf,&buf[nw]);
}

std::string  StrFormat(const char *pfmt,...)
{
	va_list varl;
	va_start(varl,pfmt);

	return StrFormatV(pfmt,varl);
}

std::wstring  StrFormatV(const wchar_t *pfmt,va_list varl)
{
	const int _BUF_SIZE = 4096;

	wchar_t buf[_BUF_SIZE];
	int nw=vswprintf(buf,_BUF_SIZE,pfmt,varl);

	/*if(nw<0)
	{
		nw=_vscwprintf(pfmt,varl);
		std::wstring rstr;
		rstr.resize(nw);
		vsnwprintf(&rstr[0],nw,pfmt,varl);
		return rstr;
	}*/
	
	return std::wstring(buf,&buf[nw]);
}

std::wstring  StrFormat(const wchar_t *pfmt,...)
{
 
	va_list varl;
	va_start(varl,pfmt);

	return StrFormatV(pfmt,varl);
}

static string_t _ignore_blank_sides(const char_t *beg, const char_t *end)
{
	//ºöÂÔ¿ªÍ·½áÎ²µÄ¿Õ°××Ö·û
	beg = SkipSpace(beg, end);

	while (beg != end && _istspace(int(end[-1])))
	{
		--end;
	}

	return string_t(beg, end);
}


int _BFC_API SplitText(const string_t &text, std::vector<string_t> &vsplit, char_t split_char)
{
	int n = 0;
	const char_t *px = text.c_str(), *pe = px + text.size();
	const char_t *pb = NULL;

	do
	{
		if (px != pe && *px != split_char)
		{
			if (!pb)
				pb = px;
		}
		else
		{
			if (pb && pb != px)
			{
				string_t val(_ignore_blank_sides(pb, px));
				if (!val.empty())
				{
					vsplit.push_back(val);
					++n;
				}
			}
			pb = NULL;
		}

		++px;
	} while (px <= pe);


	return n;
}

//return the number of characters
int _get_line(FILE *fp, char_t buf[])
{
	int n = 0, ch = 0;

	while ((ch = _fgettc(fp)) != _TEOF)
	{
		if (ch == _T('\n'))
			break;
		else
			buf[n++] = (char_t)ch;
	}

	buf[n] = 0;

	return ch == _TEOF&&n == 0 ? _TEOF : n;
}

//lines can be connected by '/'
int _get_line_ex(FILE *fp, char_t buf[])
{
	int nc = 0;

	while (true)
	{
		int nx = _get_line(fp, buf + nc);

		nc += nx;

		if (nx>0 && buf[nc - 1] == _T('/'))
			--nc;
		else
			break;
	}

	return nc;
}

int _load_text_lines(const char_t *file, std::vector<string_t> &vline)
{
	FILE *fp = _tfopen(file, _T("r"));

	if (!fp)
		return -1;

	char_t buf[1024 * 64];
	int nl = 0; //number of lines
	int nc = 0; //number of characters

	while ((nc = _get_line_ex(fp, buf)) != _TEOF)
	{
		const char_t *px = buf, *pe = px + nc;

		px = SkipSpace(px, pe);

		if (px != pe && !(px[0] == _T('/') && px[1] == _T('/'))) //not empty and not commented
		{
			vline.push_back(string_t(px, pe));
			++nl;
		}
	}

	fclose(fp);

	return nl;
}


int _BFC_API LoadTextLines(const string_t &file, std::vector<string_t> &vlines)
{
	return _load_text_lines(file.c_str(), vlines);
}


_FF_END


