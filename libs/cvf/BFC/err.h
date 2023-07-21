
#ifndef _FF_BFC_ERR_H
#define _FF_BFC_ERR_H

#include<exception>
#include<assert.h>
#include<string>

#include"ffdef.h"

_FF_BEG

///////////////////////////////////////////////////////////////////////
//

//c-string exception

class _FFS_API CSException
	:public std::exception  
{
public:
	CSException();

	explicit CSException(const char *err, const char *msg);

	~CSException() throw();

	const char* GetError() const;

	const char* GetErrorMessage() const;

	virtual const char *what( ) const throw();

protected:
	const char *m_err, *m_msg;
	mutable char *m_fmt;
};

typedef void (*ErrorHandler)(int type, const char *err,const char *msg,const char* file,int line);

_FFS_API void  DefaultErrorHandler(int type, const char *err,const char *msg,const char* file,int line);

//use default handler if @handler is null
_FFS_API void SetErrorHandler(ErrorHandler handler);

_FFS_API ErrorHandler GetErrorHandler();


_FF_END

extern _FFS_API  const char *ERR_GENERIC;
extern _FFS_API  const char *ERR_UNKNOWN;
extern _FFS_API  const char *ERR_BAD_ALLOC;

extern _FFS_API  const char *ERR_ASSERT_FAILED;

extern _FFS_API  const char *ERR_INVALID_ARGUMENT;
extern _FFS_API  const char *ERR_INVALID_OP;
extern _FFS_API  const char *ERR_INVALID_FORMAT;

extern _FFS_API  const char *ERR_NULL_POINTER;
extern _FFS_API  const char *ERR_BUFFER_OVERFLOW;

extern _FFS_API  const char *ERR_FILE_OPEN_FAILED;
extern _FFS_API  const char *ERR_FILE_READ_FAILED;
extern _FFS_API  const char *ERR_FILE_WRITE_FAILED;
extern _FFS_API  const char *ERR_FILE_OP_FAILED;

extern _FFS_API  const char *ERR_OP_FAILED;
extern _FFS_API  const char *ERR_WINAPI_FAILED;


enum
{
	ERROR_CLASS_WARNING=1,
	ERROR_CLASS_EXCEPTION,
	ERROR_CLASS_PROGRAM_ERROR
};

_FFS_API void  _FF_HandleError(int type, const char *err,const std::string &msg,const char* file,int line);

_FFS_API void  _FF_HandleError(int type, const char *err,const std::wstring &msg,const char* file,int line);

#define _FF_HANDLE_ERROR(type,err,msg)  _FF_HandleError(type,err,msg,__FILE__,__LINE__)

#define FF_WARNING(err,msg) _FF_HANDLE_ERROR(ERROR_CLASS_WARNING,err,msg)

#define FF_EXCEPTION(err,msg)  _FF_HANDLE_ERROR(ERROR_CLASS_EXCEPTION,err,msg)

#define FF_EXCEPTION1(msg)  FF_EXCEPTION(ERR_GENERIC,msg)

#define FF_ERROR(err, msg)  _FF_HANDLE_ERROR(ERROR_CLASS_PROGRAM_ERROR,err,msg)

#define FF_WINAPI_FAILED()  FF_EXCEPTION(ERR_WINAPI_FAILED, (char*)0)


template<typename _ValT>
inline _ValT& _ff_verify_func(_ValT &val, const char *statement, const char* file,int line)
{
	if(!val)
		_FF_HandleError(ERROR_CLASS_EXCEPTION,ERR_ASSERT_FAILED,statement,file,line);
	
	return val;
}

//specialize for pointer
template<typename _ValT>
inline _ValT* _ff_verify_func(_ValT *ptr, const char *statement, const char* file,int line)
{
	if(!ptr)
		_FF_HandleError(ERROR_CLASS_EXCEPTION,ERR_ASSERT_FAILED,statement,file,line);
	
	return ptr;
}

#define verify(value)  _ff_verify_func((value),#value,__FILE__,__LINE__)


#define FFAssert1(expr)  if(!(expr)) { _FF_HandleError(ERROR_CLASS_PROGRAM_ERROR,ERR_ASSERT_FAILED,#expr,__FILE__,__LINE__); }

#define FFAssert(expr)  DEVX(FFAssert1(expr))
#define DEVAssert(expr) FFAssert(expr)

#endif

