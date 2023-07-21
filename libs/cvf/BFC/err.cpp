

#include<cassert>
#include<string>

#include"BFC/err.h"
#include"BFC/autores.h"
#include"BFC/stdf.h"

#include"BFC/cfg.h"
#include"BFC/portable.h"

#ifdef _MSC_VER
#include<intrin.h>
#endif

_FF_BEG


CSException::CSException()
:m_err(NULL), m_msg(NULL)
{
}

static char* _str_clone(const char *str)
{
	char *ds=NULL;

	if(str)
	{
		size_t len=strlen(str);
		ds=new char[len+1];
		memcpy(ds,str,len+1);
	}

	return ds;
}

CSException::CSException(const char *err, const char *msg)
:m_err(NULL), m_msg(NULL),m_fmt(NULL)
{
	m_err=_str_clone(err);
	m_msg=_str_clone(msg);
}

CSException::~CSException() throw()
{
	delete[]m_err;
	delete[]m_msg;
	delete[]m_fmt;
}

const char* CSException::GetError() const
{
	return m_err;
}

const char* CSException::GetErrorMessage() const
{
	return m_msg;
}

const char* CSException::what() const throw()
{
	if (!m_fmt)
	{
		m_fmt = new char[(m_err? strlen(m_err) : 0) + (m_msg? strlen(m_msg):0)+10];
		if(m_err)
			strcpy(m_fmt, m_err);
		if (m_msg)
		{
			strcat(m_fmt, ":");
			strcat(m_fmt, m_msg);
		}
	}
	return m_fmt;
}


_FFS_API void  DefaultErrorHandler(int type, const char *err,const char *msg,const char* file,int line)
{
	switch(type)
	{
	case ERROR_CLASS_EXCEPTION:
		throw CSException(err,msg);
		break;
	case ERROR_CLASS_PROGRAM_ERROR:
		{
		printf("assert failed: expr=\"%s\"\n", msg);
#ifdef _MSC_VER
			__debugbreak(); //retry.
#else
		throw CSException(err, msg);
#endif
		}
		break;
	case ERROR_CLASS_WARNING:
		printf("program warning: err=%s, msg=%s\n", err? err : "null", msg? msg : "null");
		break;
	}
}

static ErrorHandler g_errorHandler=DefaultErrorHandler;

_FFS_API void SetErrorHandler(ErrorHandler handler)
{
	if(handler)
		g_errorHandler=handler;
	else
		g_errorHandler=DefaultErrorHandler;
}

_FFS_API ErrorHandler GetErrorHandler()
{
	return g_errorHandler;
}


_FF_END

using namespace ff;

_FFS_API void  _FF_HandleError(int type, const char *err,const std::string &msg,const char* file,int line)
{	
//#if _IS_WINDOWS()
//	ff::AutoArrayPtr<char> buf;
//
//
//	if(err==ERR_WINAPI_FAILED && (!msg||msg[0]=='\0') )
//	{ //get winapi error message
//		const int BUF_SIZE=1024;
//		buf=ff::AutoArrayPtr<char>(new char[BUF_SIZE]);
//
//		DWORD nsz=FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM,NULL,::GetLastError(),0,buf,BUF_SIZE,NULL);
//		buf[nsz]=0;
//
//		msg=buf;
//	}
//#endif
	//output error message in IDE output window
//	OutputDebugStringA(StrFormat("\n***************\nFVT %s:\n",type==FVT_ERROR_WARNING? "Warning":type==FVT_ERROR_EXCEPTION? "Exception":"Error").c_str());
//	OutputDebugStringA(StrFormat("%s(%d) : %s",file,line,msg.c_str()).c_str());
//	OutputDebugStringA("\n***************\n");

	_FF_NS(g_errorHandler)(type,err,msg.c_str(),file,line);
}

_FFS_API void  _FF_HandleError(int type, const char *err,const std::wstring &msg,const char* file,int line)
{
	std::string amsg(X2MBS(msg));
	
	_FF_HandleError(type,err,amsg,file,line);
}

_FFS_API   const char *ERR_GENERIC ="ERROR";
 _FFS_API  const char *ERR_UNKNOWN				="ERR_UNKNOWN";
 _FFS_API  const char *ERR_BAD_ALLOC				="ERR_BAD_ALLOC";
 _FFS_API  const char *ERR_ASSERT_FAILED			="ERR_ASSERT_FAILED";

 _FFS_API  const char *ERR_INVALID_ARGUMENT		="ERR_INVALID_ARGUMENT";
 _FFS_API  const char *ERR_INVALID_OP				="ERR_INVALID_OP";
 _FFS_API  const char *ERR_INVALID_FORMAT			="ERR_INVALID_FORMAT";

 _FFS_API  const char *ERR_NULL_POINTER			="ERR_NULL_POINTER";
 _FFS_API  const char *ERR_BUFFER_OVERFLOW		="ERR_BUFFER_OVERFLOW";

 _FFS_API  const char *ERR_FILE_OPEN_FAILED		="ERR_FILE_OPEN_FAILED";
 _FFS_API  const char *ERR_FILE_READ_FAILED		="ERR_FILE_READ_FAILED";
 _FFS_API  const char *ERR_FILE_WRITE_FAILED		="ERR_FILE_WRITE_FAILED";
 _FFS_API  const char *ERR_FILE_OP_FAILED			="ERR_FILE_OP_FAILED";

 _FFS_API  const char *ERR_OP_FAILED = "ERR_OP_FAILED";
 _FFS_API  const char *ERR_WINAPI_FAILED			="ERR_WINAPI_FAILED";

