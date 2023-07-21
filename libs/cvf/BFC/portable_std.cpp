#include"BFC/portable.h"
#include<experimental/filesystem>
#ifndef _WIN32
#include <libgen.h>
#include<unistd.h>
#endif

_FF_BEG

_BFC_API std::string& WCS2MBS(const wchar_t *wcs, std::string &mbs, int code_page)
{
	return mbs;
}

_BFC_API std::wstring&  MBS2WCS(const char *mbs, std::wstring &wcs, int code_page)
{
	return wcs;
}

_FF_END

//#define BOOST_NO_CXX11_SCOPED_ENUMS
//#include"boost/filesystem.hpp"
//#undef BOOST_NO_CXX11_SCOPED_ENUMS
//
//using namespace boost;
//using namespace filesystem;

using namespace std::experimental;
using namespace filesystem;

_FF_BEG

_BFC_API bool  makeDirectory(const string_t &dirName)
{
	//return boost::filesystem::create_directories(path(dirName));
	return filesystem::create_directories(path(dirName));
}

#define PATH_MAX 1024

_BFC_API bool copyFile(const string_t &src, const string_t &tar, bool overWrite)
{
	bool ok = true;
	try
	{
		ok=copy_file(path(src), path(tar), overWrite ? copy_options::overwrite_existing : copy_options::none);
	}
	catch (...)
	{
		ok = false;
	}


	return ok;
}

#ifndef _WIN32
_BFC_API string_t getExePath()
{
	char result[PATH_MAX];
	size_t count = readlink("/proc/self/exe", result, PATH_MAX);
	return std::string(result, (count > 0) ? count : 0);
}
#endif


_BFC_API string_t  getCurrentDirectory()
{
	return filesystem::current_path().string();
}
void	setCurrentDirectory(const string_t& dir)
{
	filesystem::current_path(path(dir));
}

_BFC_API bool copyFilesToClipboard(const string_t files[], int count)
{
	return false;
}

_BFC_API string_t getTempPath()
{
	return filesystem::temp_directory_path().string();
}

string_t  getTempFile(const string_t &preFix, const string_t &path, bool bUnique)
{
	return "";
}

_BFC_API bool  pathExist(const string_t &path)
{
	return filesystem::exists(filesystem::path(path));
}
_BFC_API bool  isDirectory(const string_t &path)
{
	return filesystem::is_directory(filesystem::path(path));
}
_BFC_API string_t getFullPath(const string_t &path)
{
	return filesystem::system_complete(filesystem::path(path)).string();
}

inline bool isHidden(const filesystem::path &p)
{
	return p.filename().string()[0] == '.';
}

_BFC_API void   listFiles(const string_t &path, std::vector<string_t> &files, bool recursive, bool includeHidden)
{
	std::vector<string_t> v;

	if (!recursive)
	{
		filesystem::directory_iterator itr(path), end;

		for (; itr != end; ++itr)
		{
			if (is_regular_file(itr->status()) && (includeHidden || !isHidden(*itr)))
				v.push_back(itr->path().filename().string());
		}
	}
	else
	{
		filesystem::recursive_directory_iterator itr(path), end;
		for (; itr != end; ++itr)
		{
			if (is_regular_file(itr->status()) && (includeHidden || !isHidden(*itr)))
				v.push_back(itr->path().relative_path().string());
		}
	}

	files.swap(v);
}

#if 1
_BFC_API void listSubDirectories(const string_t &path, std::vector<string_t> &dirs, bool recursive, bool includeHidden)
{
	std::vector<string_t> v;

	if (!recursive)
	{
		filesystem::directory_iterator itr(path), end;
		for (; itr != end; ++itr)
		{
			if (is_directory(*itr) && (includeHidden || !isHidden(*itr)))
				v.push_back(itr->path().filename().string());
		}
	}
	else
	{
		filesystem::recursive_directory_iterator itr(path), end;
		for (; itr != end; ++itr)
		{
			if (is_directory(*itr) && (includeHidden || !isHidden(*itr)))
				v.push_back(itr->path().relative_path().string());
		}
	}

	dirs.swap(v);
}
#endif

_BFC_API void removeDirectoryRecursively(const string_t &path)
{
	filesystem::remove_all(filesystem::path(path));
}




_FF_END

