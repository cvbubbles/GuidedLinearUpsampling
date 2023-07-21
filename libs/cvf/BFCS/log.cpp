
#include"BFC/log.h"
#include<stdarg.h>
#include<chrono>
#include<unordered_map>
#include<iostream>
#include"BFC/err.h"
#include<mutex>

_FF_BEG

#if 0
LogChannel::LogChannel()
	:_fp(nullptr),_flags(0)
{}
void LogChannel::_setFP(FILE *fp)
{
	_fp = fp;
}
void LogChannel::enable(bool bEnable)
{
	if (bEnable)
		_flags &= (~_DISABLED);
	else
		_flags |= _DISABLED;
}

void LogChannel::vlog(const char *fmt, va_list vl)
{
	if (_fp)
		vfprintf(_fp, fmt, vl);
}
void LogChannel::operator()(const char *fmt, ...)
{
	va_list varl;
	va_start(varl, fmt);

	vlog(fmt, varl);
}
#endif

LogStream::LogStream()
	:std::ostream(NULL)
{}
void LogStream::open(FILE *stdstream)
{
	decltype(std::cout.rdbuf()) pbuf = nullptr;
	if (stdstream == stdout)
		pbuf = std::cout.rdbuf();
	else if (stdstream == stderr)
		pbuf = std::cerr.rdbuf();
	if(!pbuf)
		FF_EXCEPTION1("invalid argument");

	this->set_rdbuf(pbuf);
	this->clear(std::ios::goodbit);
}
void LogStream::vprint(const char *fmt, va_list vl)
{
	(*this) << vformats(fmt, vl);
}
LogStream& LogStream::operator()(const char *fmt, ...)
{
	if (*this)
	{
		va_list varl;
		va_start(varl, fmt);

		(*this) << vformats(fmt, varl);
	}
	return *this;
}

Log::Log()
{
	_channels = new LogStream[NLogChannels];
	this->initialize();
}
Log::~Log()
{
	delete[]_channels;
}
void Log::initialize()
{
	_channels[CInfo].open(stdout);
	_channels[CWarning].open(stdout);
	_channels[CError].open(stderr);
	_channels[CDev].open(stdout);
}

//Log& Log::operator()(int channel, const char *fmt, ...)
//{
//	if ((*this)[channel])
//	{
//		va_list varl;
//		va_start(varl, fmt);
//
//		(*this)[channel].vlog(fmt, varl);
//	}
//	return *this;
//}
LogStream& Log::operator()(const char *fmt, ...)
{
	auto &os = (*this)[CInfo];
	if (os)
	{
		va_list varl;
		va_start(varl, fmt);

		os.vprint(fmt, varl);
	}
	return os;
}


//Log& Log::tend(const char *fmt)
//{
//	(*this)(fmt? fmt : "elapsed %.2fms", float(this->elapsed()));
//	return *this;
//}

_BFCS_API Log  LOG;


static auto  _tbeg = std::chrono::system_clock::now();
using namespace std;
using namespace chrono;

_BFCS_API double elapsed()
{
	auto now = std::chrono::system_clock::now();
	auto duration = duration_cast<microseconds>(now - _tbeg);
	double t = (double(duration.count()) / microseconds::period::den);
	return t;
}

struct FmtBuf
{
	char *buf=nullptr;
	int   size = 0;
	int   cur = 0;
	std::mutex  _mtx;
public:
	FmtBuf()
	{
		size = 100 * 1024;
		buf = new char[size];
		cur = 0;
	}
	~FmtBuf()
	{
		delete[]buf;
	}
	char* pro(const char *fmt, va_list vl)
	{
		std::lock_guard<std::mutex> _lock(_mtx);
		int bufSize = size - cur;
		int nw = vsnprintf(buf + cur, bufSize, fmt, vl);
		if (nw == bufSize)
		{
			cur = 0;
			bufSize = size;
			nw = vsnprintf(buf, bufSize, fmt, vl);
		}
		if (nw < 0)
		{
			buf[cur] = '\0';
			nw = 0;
		}
		char *p = buf + cur;
		cur += nw + 1;
		return p;
	}
};
static FmtBuf _fmtBuf;

_BFCS_API char* vformats(const char *fmt, va_list vl)
{
	return _fmtBuf.pro(fmt, vl);
}

_BFCS_API char* formats(const char *fmt, ...)
{
	va_list varl;
	va_start(varl, fmt);
	return vformats(fmt, varl);
}

_BFCS_API char* logfmt(const char *fmt, ...)
{
	va_list varl;
	va_start(varl, fmt);
	return vformats(fmt, varl);
}

class IntFlags::CImpl
{
public:
	typedef std::unordered_map<std::string, int> _MapT;

	_MapT  _valMap;

	struct PatternMap
	{
		std::string pattern;
		int        value;
	};
	std::vector<PatternMap>  _patMap;
	int   _defaultValue=0;
	bool  _allAsDefault = false;
public:
	void setDefault(int defaultValue, bool allAsDefault)
	{
		_defaultValue = defaultValue;
		_allAsDefault = allAsDefault;
	}
	void set(const std::string &label, int value)
	{
		if (!label.empty())
		{
			char last = label.back();
			if (last == '*')
			{
				std::string pat;
				if (label.size() >= 2) //if not "*"
					pat = std::string(&label[0], &label[label.size() - 2]);
				_patMap.push_back({ pat,value });
			}
			else
			{
				auto itr = _valMap.find(label);
				if (itr == _valMap.end())
				{
					_valMap.emplace(label, value);
				}
				else
					itr->second = value;
			}
		}
	}
	int get(const std::string &label)
	{
		if (_allAsDefault)
			return _defaultValue;

		{
			if (!_valMap.empty())
			{
				auto itr = _valMap.find(label);
				if (itr != _valMap.end())
					return itr->second;
			}

			if (!_patMap.empty())
			{
				for (auto itr = _patMap.rbegin(); itr != _patMap.rend(); ++itr)
				{
					auto &p = *itr;
					if (p.pattern.size() <= label.size() && strncmp(p.pattern.c_str(), label.c_str(), p.pattern.size()) == 0)
						return p.value;
				}
			}
		}

		return _defaultValue;
	}
};

IntFlags::IntFlags()
	:ptr(new CImpl)
{}
IntFlags::~IntFlags()
{
	delete ptr;
}
void IntFlags::setDefault(int defaultValue)
{
	ptr->_defaultValue = defaultValue;
}
void IntFlags::setAllAsDefault(bool allAsDefault)
{
	ptr->_allAsDefault = allAsDefault;
}
int IntFlags::get(const std::string &label)
{
	return ptr->get(label);
}

_BFCS_API IntFlags LOG_FLAGS;

void func()
{
	if (auto f = LOG_FLAGS["mask"])
	{
		auto v = f.label;
	}
}

_FF_END


