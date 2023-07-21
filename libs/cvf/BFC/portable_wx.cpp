
#ifdef _WIN32

#ifndef wxUSE_UNICODE
#define wxUSE_UNICODE 0 //Linux: use non-unicode version will introduce crash on program startup.
#endif

#define WXUSINGDLL

#endif

#ifdef _UNICODE
#define _WXUNICODE 1
#endif

#include"wx/wx.h"
#include"wx/stdpaths.h"
#include"wx/filename.h"
#include"wx/dir.h"
#include"wx/clipbrd.h"
//#include"wx/utils.h"
//wx will change the definition of _UNICODE
#ifndef _WXUNICODE
#undef _UNICODE
#endif

#include"portable.h"
#include"BFC/stdf.h"

_FF_BEG

_BFC_API bool initPortable(int argc, char *argv[])
{
	return wxInitialize(argc, argv);
}

char_t* wx_cstr(const wxString &str)
{
#ifdef _UNICODE
	return (char_t*)str.c_str().AsWChar();
#else
	return (char_t*)str.c_str().AsChar();
#endif
}

string_t getEnvVar(const string_t &var)
{
	return wxGetenv(var.c_str());
}
bool setEnvVar(const string_t &var, const string_t &value)
{
	return wxSetEnv(var, value);
}

bool  makeDirectory(const string_t &dirName)
{
	return wxFileName::Mkdir(dirName, wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL);
#if 0
	if (!dirName.empty() && !ff::pathExist(dirName))
	{
		//if (_tmkdir(dirName.c_str()))
		if (wxMkdir(dirName))
		{
			//if (errno == ENOENT)
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
#endif
}

_BFC_API bool copyFile(const string_t &src, const string_t &tar, bool overWrite)
{
	return wxCopyFile(src, tar, overWrite);
}

_BFC_API string_t getExePath()
{
	return wx_cstr(wxStandardPaths::Get().GetExecutablePath());
}

string_t  getCurrentDirectory()
{
	return wx_cstr(wxGetCwd());
}

void	setCurrentDirectory(const string_t& dir)
{
	wxSetWorkingDirectory(dir);
}

bool copyFilesToClipboard(const string_t files[], int count)
{
	bool ok = false;
	if (wxTheClipboard->Open())
	{
		wxFileDataObject *obj=new wxFileDataObject;
		for (int i = 0; i < count; ++i)
			obj->AddFile(files[i]);
		
		if (wxTheClipboard->SetData(obj))
			ok = true;
		wxTheClipboard->Close();
	}
	return ok;

	//return true; //supported only on windows
}

string_t getTempPath()
{
	return wx_cstr(wxStandardPaths::Get().GetTempDir());
}

//void* getFocusWindow()
//{
//	wxWindow *wnd = wxWindow::FindFocus();
//	return wnd? wnd->GetHandle() : NULL;
//}
//
//void* getParentWindow(void* hwnd)
//{
//	wxWindow wnd;
//	wnd.SetHWND((WXHWND)hwnd);
//	//wnd.AssociateHandle((WXWidget)hwnd);
//	wxWindow *parent = wnd.GetParent();
//	return parent ? parent->GetHandle() : NULL;
//}

bool  pathExist(const string_t &path)
{
    std::string pathstd=path;
    wxString wxpath(pathstd.c_str(), wxConvUTF8);
//    std::string path2;
//    
//    wxString mystring(wxT("HelloWorld"));
//    std::string stlstring = std::string(mystring.mb_str());
//    
////    path2=std::string(wxpath);
////    std::string path3;
////    path3=wxpath.ToStdString();
//    
////    auto tmp=wxpath.mb_str();
////    string_t path2=string_t(wxpath.mb_str());
//    
//	return wxFileName::Exists(wxpath);
    
    return wxFileName::Exists(path);
}

bool  isDirectory(const string_t &path)
{
	return wxDir::Exists(path);
}

string_t getFullPath(const string_t &path)
{
	wxFileName fn(path);
	fn.Normalize(wxPATH_NORM_ALL);
	return wx_cstr(fn.GetFullPath());
}

//===============================================

class IForEachFileCallBack
{
protected:
	wxString m_strRoot;
public:
	//set root directory.
	virtual void OnSetRoot(const wxString &root) { m_strRoot = root; }
	//enter directory.
	//@relativePath : path relative to root directory @m_strRoot.
	virtual bool OnEnterDir(const wxString &relativePath){ return true; }

	virtual bool OnDirFound(const wxString &relativePath){ return true; }

	virtual bool OnFileFound(const wxString &relativePath, const wxString &fileName){ return true; }

	virtual void OnLeaveDir(const wxString &relativePath){ }
};

static bool _for_each_file_imp(const wxString &dir, const wxString &relativeDir, bool bRecursive, IForEachFileCallBack *pOp, bool includeHidden)
{
	bool rv = false;
	if (pOp->OnEnterDir(relativeDir))
	{
		wxDir dirObj(dir);

		wxString fn;
		if (!dirObj.IsOpened() || !dirObj.GetFirst(&fn, wxEmptyString, wxDIR_FILES|wxDIR_DIRS|(includeHidden? wxDIR_HIDDEN:0)))
			return false;

		do
		{
			wxString ffn(dir + fn);

			if (wxDir::Exists(ffn))
			{//is dir!
				//if (fn != _TX(".") && fn != _TX(".."))
				{
					wxString subDir(relativeDir + fn + _TX("/"));

					if (!pOp->OnDirFound(subDir))
						goto _FEF_END;

					if (bRecursive)
					{
						if (!_for_each_file_imp(ffn + _TX("/"), subDir, bRecursive, pOp, includeHidden))
							goto _FEF_END;
					}
				}
			}
			else
			{
				if (!pOp->OnFileFound(relativeDir, fn))
					goto _FEF_END;
			}
		} while (dirObj.GetNext(&fn));

		rv = true;
	_FEF_END:

		pOp->OnLeaveDir(relativeDir);
	}
	return rv;
}

void ForEachFile(const wxString &dir, IForEachFileCallBack *pOp, bool bRecursive, bool includeHidden)
{
	if (pOp)
	{
		wxString dirx(dir + _TX("/"));

		pOp->OnSetRoot(dirx);

		if (wxDir::Exists(dirx))
			_for_each_file_imp(dirx, _TX(""), bRecursive, pOp, includeHidden);
	}
}

class FileListerOp
	:public IForEachFileCallBack
{
public:
	std::vector<string_t> ffd;
public:
	virtual bool OnFileFound(const wxString &relativePath, const wxString &fileName)
	{
		ffd.push_back(wx_cstr(relativePath + fileName));
		return true;
	}
};

void  listFiles(const string_t &path, std::vector<string_t> &files, bool recursive, bool includeHidden)
{
	FileListerOp op;
	ForEachFile(path, &op, recursive, includeHidden);
	files.swap(op.ffd);
}

class FolderListerOp
	:public IForEachFileCallBack
{
public:
	std::vector<string_t> ffd;
public:
	virtual bool OnDirFound(const wxString &relativePath)
	{
		ffd.push_back(wx_cstr(relativePath));
		return true;
	}
};

void listSubDirectories(const string_t &path, std::vector<string_t> &dirs, bool recursive, bool includeHidden)
{
	FolderListerOp op;
	ForEachFile(path, &op, recursive, includeHidden);
	dirs.swap(op.ffd);
}

_BFC_API void removeDirectoryRecursively(const string_t &path)
{
	wxFileName::Rmdir(path, wxPATH_RMDIR_RECURSIVE);
}


wxFontEncoding wx_get_encoding(int code_page)
{
	return code_page == ACP||code_page==THREAD_ACP ? wxFONTENCODING_SYSTEM : /*code_page == THREAD_ACP ? wxFONTENCODING_DEFAULT :*/ code_page == UTF7 ? 
	wxFONTENCODING_UTF7 : code_page == UTF8 ? wxFONTENCODING_UTF8 : wxFONTENCODING_DEFAULT;
}

std::string& WCS2MBS(const wchar_t *wcs, std::string &acs, int code_page)
{
	wxCSConv conv(wx_get_encoding(code_page));
	wxCharBuffer cs=conv.cWC2MB(wcs);
	return acs = cs.data();
}

std::wstring&  MBS2WCS(const char *acs, std::wstring &wcs, int code_page)
{
	wxCSConv conv(wx_get_encoding(code_page));
	wxWCharBuffer cs = conv.cMB2WC(acs);
	return wcs = cs.data();
}

_FF_END

