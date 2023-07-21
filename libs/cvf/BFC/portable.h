
#pragma once

#include"ffdef.h"
#include<string>
#include<vector>
#include"auto_tchar.h"

_FF_BEG

_BFC_API bool  initPortable(int argc, char *argv[]);

_BFC_API string_t getEnvVar(const string_t &var);

_BFC_API bool setEnvVar(const string_t &var, const string_t &value);

_BFC_API bool  makeDirectory(const string_t &dirName);

_BFC_API bool copyFile(const string_t &src, const string_t &tar, bool overWrite = false);

_BFC_API string_t getExePath();

_BFC_API string_t  getCurrentDirectory();

_BFC_API void	setCurrentDirectory(const string_t& dir);

_BFC_API bool copyFilesToClipboard(const string_t files[], int count);

_BFC_API string_t getTempPath();

_BFC_API string_t  getTempFile(const string_t &preFix, const string_t &path = _TX(""), bool bUnique = true);

_BFC_API bool  pathExist(const string_t &path);

_BFC_API bool  isDirectory(const string_t &path);

_BFC_API string_t getFullPath(const string_t &path);

_BFC_API void  listFiles(const string_t &path, std::vector<string_t> &files, bool recursive = false, bool includeHidden=false);

_BFC_API void listSubDirectories(const string_t &path, std::vector<string_t> &dirs, bool recursive = false, bool includeHidden=false);

_BFC_API void removeDirectoryRecursively(const string_t &path);

_BFC_API void connectNetDrive(const std::string &remotePath, const std::string &localName, const std::string &userName, const std::string &passWD);

//============================================================================

enum
{
	THREAD_ACP = 3,
	ACP = 0,
	UTF8 = 65001,
	UTF7 = 65000
};

_BFC_API std::string& WCS2MBS(const wchar_t *wcs, std::string &acs, int code_page = THREAD_ACP);

_BFC_API std::wstring&  MBS2WCS(const char *acs, std::wstring &wcs, int code_page = THREAD_ACP);

inline const std::wstring X2WCS(const std::wstring &wcs, int _ = 0, int __ = 0, std::wstring _buf = std::wstring())
{
	return wcs;
}
inline const std::wstring X2WCS(const std::string &mbs, int i_code_page = THREAD_ACP, int _ = 0, std::wstring _buf = std::wstring())
{
	return MBS2WCS(mbs.c_str(), _buf, i_code_page);
}

inline const std::string X2MBS(const std::wstring &wcs, int _ = 0, int d_code_page = THREAD_ACP, std::string _buf = std::string())
{
	return WCS2MBS(wcs.c_str(), _buf, d_code_page);
}
inline const std::string X2MBS(const std::string &mbs, int i_code_page = THREAD_ACP, int d_code_page = THREAD_ACP, std::string _buf = std::string())
{
	return i_code_page == d_code_page ? mbs : X2MBS(X2WCS(mbs, i_code_page), 0, d_code_page, _buf);
}

template<typename _IStrT>
inline const string_t X2TS(const _IStrT &str, int i_code_page = THREAD_ACP, int d_code_page = THREAD_ACP, string_t _buf = string_t())
{
#ifdef _UNICODE
	return X2WCS(str, i_code_page, d_code_page, _buf);
#else
	return X2MBS(str, i_code_page, d_code_page, _buf);
#endif
}


_FF_END

