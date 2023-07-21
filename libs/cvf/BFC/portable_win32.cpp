
#include"portable.h"

#include<Windows.h>
#include<direct.h>
#include <Shellapi.h>
#include <Shlobj.h>

#include <WinNetWk.h>
#pragma comment(lib, "Mpr.lib")

#include"BFC/stdf.h"
#include"BFC/err.h"

#define DEV


_FF_BEG

_BFC_API bool  initPortable(int argc, char *argv[])
{
	return true;
}

string_t getEnvVar(const string_t &var)
{
	char_t buf[2048];
	int n = ::GetEnvironmentVariable(var.c_str(), buf, sizeof(buf)/sizeof(buf[0]));
	buf[n] = 0;
	return buf;
}
bool setEnvVar(const string_t &var, const string_t &value)
{
	return ::SetEnvironmentVariable(var.c_str(), value.c_str()) ? true : false;
}

bool  makeDirectory(const string_t &dirName)
{
	if (!dirName.empty() && !ff::pathExist(dirName))
	{
		if (_tmkdir(dirName.c_str()))
		{
			if (errno == ENOENT)
			{
				string_t fd(GetFatherDirectory(dirName));

				if (fd.empty())
					return false;
				else
					makeDirectory(fd);

				return makeDirectory(dirName);
			}
		}
		else
			return true;
	}
	return false;
}

_BFC_API bool copyFile(const string_t &src, const string_t &tar, bool overWrite)
{
	return ::CopyFile(src.c_str(), tar.c_str(), overWrite ? FALSE : TRUE)? true:false;
}

_BFC_API string_t getExePath()
{
	char_t buf[MAX_PATH];
	int npos=::GetModuleFileName(NULL, buf, MAX_PATH);
	buf[npos] = 0;
	return buf;
}

string_t  getCurrentDirectory()
{
	char_t buf[MAX_PATH];
	string_t cdir(buf, &buf[GetCurrentDirectory(MAX_PATH, buf)]);
	//GetCurrentDirectory(...) return a root directory end with '\', but
	//return other directoies without.
	return cdir.size()>3 ? cdir + _TX("\\") : cdir;
}
void	setCurrentDirectory(const string_t& dir)
{
	SetCurrentDirectory(dir.c_str());
}

bool copyToClipboard(const char_t *lpBuffer, UINT uBufLen)
{
	DROPFILES dropFiles;
	UINT uGblLen, uDropFilesLen;
	HGLOBAL hGblFiles;
	char *szData, *szFileList;

	uDropFilesLen = sizeof(DROPFILES);
	dropFiles.pFiles = uDropFilesLen;
	dropFiles.pt.x = 0;
	dropFiles.pt.y = 0;
	dropFiles.fNC = FALSE;
	dropFiles.fWide = TRUE;

	uGblLen = uDropFilesLen + uBufLen * 2 + 8;
	hGblFiles = GlobalAlloc(GMEM_ZEROINIT | GMEM_MOVEABLE | GMEM_DDESHARE, uGblLen);
	szData = (char*)GlobalLock(hGblFiles);
	memcpy(szData, (LPVOID)(&dropFiles), uDropFilesLen);
	szFileList = szData + uDropFilesLen;

#ifndef _UNICODE
	MultiByteToWideChar(CP_ACP, MB_COMPOSITE, lpBuffer, uBufLen, (WCHAR *)szFileList, uBufLen);
#else
	memcpy(szFileList, lpBuffer, uBufLen * 2);
#endif

	GlobalUnlock(hGblFiles);

	if (OpenClipboard(NULL))
	{
		EmptyClipboard();
		SetClipboardData(CF_HDROP, hGblFiles);
		CloseClipboard();
		return true;
	}
	return false;
}

_BFC_API bool copyFilesToClipboard(const string_t files[], int count)
{
	string_t str;
	for (int i = 0; i < count; ++i)
	{
		str += files[i];
		str.push_back(_TX('\0'));
	}
	return copyToClipboard(str.c_str(), str.size()+1);
}

_BFC_API string_t getTempPath()
{
	char_t buf[256];
	int n = ::GetTempPath(sizeof(buf)/sizeof(buf[0]), buf);
	buf[n] = 0;
	return string_t(buf);
}

string_t  getTempFile(const string_t &preFix, const string_t &path, bool bUnique)
{
	string_t pathx(path);
	if (pathx.empty())
		pathx = getTempPath();
	char_t buf[MAX_PATH * 2];
	if (!GetTempFileName(pathx.c_str(), preFix.c_str(), bUnique, buf))
		return _TX("");
	return buf;
}

//_BFC_API void* getFocusWindow()
//{
//	return ::GetFocus();
//}
//
//_BFC_API void* getParentWindow(void* hwnd)
//{
//	return ::GetParent((HWND)hwnd);
//}

_BFC_API bool  pathExist(const string_t &path)
{
	return ::GetFileAttributes(path.c_str()) != INVALID_FILE_ATTRIBUTES;
}

_BFC_API bool  isDirectory(const string_t &path)
{
	DWORD att = ::GetFileAttributes(path.c_str());
	return att != INVALID_FILE_ATTRIBUTES && (att&FILE_ATTRIBUTE_DIRECTORY) != 0;
}

_BFC_API string_t getFullPath(const string_t &path)
{
	char_t buf[1024];
	char_t *pPath = _tfullpath(buf, path.c_str(), sizeof(buf) / sizeof(char_t));
	return pPath ? pPath : _T("");
}

//===============================================

class _IForEachFileCallBack
{
protected:
	string_t m_strRoot;
public:
	//set root directory.
	virtual void OnSetRoot(const string_t &root);
	//enter directory.
	//@relativePath : path relative to root directory @m_strRoot.
	virtual bool OnEnterDir(const string_t &relativePath);

	virtual bool OnDirFound(const string_t &relativePath);

	virtual bool OnFileFound(const string_t &relativePath, const string_t &fileName, const WIN32_FIND_DATA *pFFD);

	virtual void OnLeaveDir(const string_t &relativePath);
};

void _IForEachFileCallBack::OnSetRoot(const string_t &root)
{
	m_strRoot = root;
}
bool _IForEachFileCallBack::OnEnterDir(const string_t &relativePath)
{
	return true;
}
bool _IForEachFileCallBack::OnDirFound(const string_t &relativePath)
{
	return true;
}
bool _IForEachFileCallBack::OnFileFound(const string_t &relativePath, const string_t &fileName, const WIN32_FIND_DATA *pFFD)
{
	return true;
}
void _IForEachFileCallBack::OnLeaveDir(const string_t &relativePath)
{
}

static bool _for_each_file_imp(const string_t &dir, const string_t &relativeDir, bool bRecursive, bool includeHidden, _IForEachFileCallBack *pOp)
{
	bool rv = false;
	if (pOp->OnEnterDir(relativeDir))
	{
		WIN32_FIND_DATA		ffd;
		HANDLE hFind = ::FindFirstFile((dir + _TX("*")).c_str(), &ffd);
		do
		{
			string_t fn(ffd.cFileName), ffn(dir + fn);

			if (includeHidden || (ffd.dwFileAttributes&FILE_ATTRIBUTE_HIDDEN) == 0)
			{
				if ((ffd.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY) != 0)
				{//is dir!
					if (fn != _TX(".") && fn != _TX(".."))
					{
						string_t subDir(relativeDir + fn + _TX("\\"));

						if (!pOp->OnDirFound(subDir))
							goto _FEF_END;

						if (bRecursive)
						{
							if (!_for_each_file_imp(ffn + _TX("\\"), subDir, bRecursive, includeHidden, pOp))
								goto _FEF_END;
						}
					}
				}
				else
				{
					if (!pOp->OnFileFound(relativeDir, fn, &ffd))
						goto _FEF_END;
				}
			}
		} while (::FindNextFile(hFind, &ffd));

		rv = true;
	_FEF_END:

		FindClose(hFind);
		pOp->OnLeaveDir(relativeDir);
	}
	return rv;
}

void ForEachFile(const string_t &dir, _IForEachFileCallBack *pOp, bool bRecursive, bool includeHidden)
{
	if (pOp)
	{
		string_t dirx(dir+_TX("\\"));

		pOp->OnSetRoot(dirx);

		if (isDirectory(dirx))
			_for_each_file_imp(dirx, _TX(""), bRecursive, includeHidden, pOp);
	}
}

//class _FileFindData
//	:public WIN32_FIND_DATA
//{
//public:
//	_FileFindData(const WIN32_FIND_DATA *pData = NULL)
//	{
//		if (!pData)
//			memset(this, 0, sizeof(*this));
//		else
//			memcpy(this, pData, sizeof(*this));
//	}
//
//	bool IsDir() const
//	{
//		return (dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY) != 0;
//	}
//};

class _FileListerOp
	:public _IForEachFileCallBack
{
public:
	std::vector<string_t> ffd;
	DWORD   excAtt;
public:
	_FileListerOp(DWORD ea)
		:excAtt(ea)
	{
	}
	virtual bool OnFileFound(const string_t &relativePath, const string_t &fileName, const WIN32_FIND_DATA *pFFD)
	{
		if (pFFD && (pFFD->dwFileAttributes&excAtt) == 0)
			ffd.push_back(relativePath+fileName);
		return true;
	}
};

_BFC_API void   listFiles(const string_t &path, std::vector<string_t> &files, bool recursive, bool includeHidden)
{
	_FileListerOp op(0);
	ForEachFile(path, &op, recursive,includeHidden);
	files.swap(op.ffd);
}

class _FolderListerOp
	:public _IForEachFileCallBack
{
public:
	std::vector<string_t> ffd;
public:
	virtual bool OnDirFound(const string_t &relativePath)
	{
		ffd.push_back(relativePath);
		return true;
	}
};

_BFC_API void listSubDirectories(const string_t &path, std::vector<string_t> &dirs, bool recursive, bool includeHidden)
{
	_FolderListerOp op;
	ForEachFile(path, &op, recursive, includeHidden);
	dirs.swap(op.ffd);
}

struct _RemoveDirOp
	:public _IForEachFileCallBack
{
public:
	bool OnFileFound(const string_t &path, const string_t &name, const WIN32_FIND_DATA *pFFD)
	{
		ff::DeleteFileEx(m_strRoot + path + name);
		return true;
	}
	void OnLeaveDir(const string_t &path)
	{
		::RemoveDirectory((m_strRoot + path).c_str());
	}
};

_BFC_API void removeDirectoryRecursively(const string_t &path)
{
	_RemoveDirOp op;
	ForEachFile(path, &op, true, true);
}

_BFC_API void connectNetDrive(const std::string &remotePath, const std::string &localName, const std::string &userName, const std::string &passWD)
{
	WNetCancelConnection2A(localName.c_str(), CONNECT_UPDATE_PROFILE, true);

	//string szPasswd = "fan123", szUserName = "fan"; //用户名和密码
	NETRESOURCEA net_Resource;
	memset(&net_Resource, 0, sizeof(net_Resource));
	net_Resource.dwDisplayType = RESOURCEDISPLAYTYPE_DIRECTORY;
	net_Resource.dwScope = RESOURCE_CONNECTED;
	net_Resource.dwType = RESOURCETYPE_ANY;
	net_Resource.dwUsage = 0;
	net_Resource.lpComment = (LPSTR)"";
	net_Resource.lpLocalName = (LPSTR)localName.c_str();  //映射成本地驱动器Z:
	net_Resource.lpProvider = NULL;
	net_Resource.lpRemoteName = (LPSTR)remotePath.c_str(); // \\servername\共享资源名
	DWORD dwFlags = CONNECT_UPDATE_PROFILE;
	DWORD dw = WNetAddConnection2A(&net_Resource, passWD.c_str(), userName.c_str(), dwFlags);
	if (dw != ERROR_SUCCESS)
		FF_EXCEPTION(ERR_WINAPI_FAILED,"");
}

_BFC_API std::string& WCS2MBS(const wchar_t *wcs, std::string &mbs, int code_page)
{
	size_t csize = wcslen(wcs);
	mbs.resize(2 * csize + 1);

	int dsize = WideCharToMultiByte(code_page, 0, wcs, csize, &mbs[0], (int)mbs.size(), NULL, NULL); //return 0 if failed
	mbs.resize(dsize);

	return mbs;
}

_BFC_API std::wstring&  MBS2WCS(const char *mbs, std::wstring &wcs, int code_page)
{
	size_t csize = strlen(mbs);
	wcs.resize(csize + 1);

	int dsize = MultiByteToWideChar(code_page, 0, mbs, csize, &wcs[0], (int)wcs.size());
	wcs.resize(dsize);

	return wcs;
}

_FF_END

