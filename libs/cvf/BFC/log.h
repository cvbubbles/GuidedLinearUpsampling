#pragma once

#include"BFC/def.h"
#include<stdio.h>
#include<string>
#include<ostream>

_FF_BEG

#if 0
class Log;

class _BFCS_API LogChannel
{
	friend class Log;

	FILE *_fp;
	int   _flags;

	enum {
		_DISABLED=0x01
	};

	void _setFP(FILE *fp);
public:
	LogChannel();
	
	void enable(bool bEnable = true);

	void disable()
	{
		this->enable(false);
	}

	bool enabled() const
	{
		return (_flags&_DISABLED)==0;
	}
	operator bool() const
	{
		return enabled() && _fp;
	}
	void vlog(const char *fmt, va_list vl);

	void operator()(const char *fmt, ...);
};
#endif

class _BFCS_API LogStream
	:public std::ostream
{
public:
	LogStream();

	void open(FILE *stdstream);

	void vprint(const char *fmt, va_list vl);

	LogStream& operator()(const char *fmt, ...);
};

enum ChannelType
{
	CInfo, CWarning, CError, CDev, NLogChannels
};

class _BFCS_API Log
{
	LogStream *_channels;
public:
	Log();

	~Log();

	Log(const Log&) = delete;
	Log& operator=(const Log&) = delete;

	void initialize();

	LogStream& operator[](int channel)
	{
		return _channels[channel];
	}
	//Log& operator()(int channel, const char *fmt, ...);

	LogStream& operator()(const char *fmt, ...);

	template<typename _ValT>
	friend LogStream& operator<<(Log &os, const _ValT &val)
	{
		os[CInfo] << val;
		return os[CInfo];
	}

	//Log& etbeg();


	//Log& tend(const char *fmt=nullptr);
};

extern _BFCS_API Log  LOG;

_BFCS_API char* vformats(const char *fmt, va_list vl);

_BFCS_API char* formats(const char *fmt, ...);

_BFCS_API char* logfmt(const char *fmt, ...);

_BFCS_API double elapsed();

//#define LOGetb() double etbeg=ff::elapsed();
//#define LOGetu() etbeg=ff::elapsed();

inline double loget()
{
	return elapsed();
}
//inline double loget(double beg)
//{
//	return elapsed() - beg;
//}
//#define logetd() loget(etbeg)

inline double logfps(double beg)
{
	//return logfmt("fps=%.2lf", 1.0 / (elapsed() - beg));
	return 1.0 / (elapsed() - beg);
}
//#define logfps() _logfps(etbeg)

inline char* _logfl(const char *file, int line)
{
	return logfmt("location=%s(line %d)", file, line);
}
#define logfl()  _logfl(__FILE__,__LINE__)

class _BFCS_API IntFlags
{
	class CImpl;
	CImpl *ptr;
public:
	IntFlags();

	~IntFlags();
	IntFlags(const IntFlags&) = delete;
	IntFlags& operator=(const IntFlags&) = delete;

	void setDefault(int defaultValue);

	void setAllAsDefault(bool allAsDefault = true);

	void set(const std::string &label, int value);

	int get(const std::string &label);

	struct Flag
	{
		std::string  label;
		int   value;
	public:
		Flag(std::string &_label, int _value)
			:value(_value)
		{
			label.swap(_label);
		}
		operator bool() const
		{
			return value != 0;
		}
		operator int() const
		{
			return value;
		}
	};

	Flag operator[](std::string label) 
	{
		int val = this->get(label.c_str());
		return Flag(label, val);
	}
};

extern _BFCS_API IntFlags LOG_FLAGS;

_FF_END

