
#include"BFC/argv.h"
#include"BFC/autores.h"

#include"BFC/err.h"

#include<stdio.h>
#include<functional>
#include<assert.h>
#include"auto_tchar.h"

#define _CH_CVT _TX('%')

_FF_BEG

//=============================================================================================================

static char_t _cvt_special_char(char_t ch)
{
	char_t rch=ch;

	switch(ch)
	{
	case _TX('\"'):
	case _CH_CVT:
	case _TX('$'):
	case _TX('-'): 
	case _TX('/'):
		break;
	case _TX('n'):
		rch=_TX('\n');
		break;
	case _TX('t'):
		rch=_TX('\t');
		break;
	default:
//		FVT_WARNING("error_unknown_special_char",string_t(&ch,&ch+1));
		;
	}
	return rch;
}

static char_t *_skip_space(const char_t *pbeg,const char_t *pend)
{
	while(pbeg<pend&&_istspace((unsigned short)*pbeg))
		++pbeg;

	return (char_t*)pbeg;
}


static char_t *_skip_non_space(const char_t *pbeg,const char_t *pend)
{
	while(pbeg<pend&&!_istspace((unsigned short)*pbeg))
		++pbeg;

	return (char_t*)pbeg;
}

static char_t *_skip_dquotes(const char_t *pbeg, const char_t *pend)
{
	assert(pbeg<pend&&*pbeg==_TX('\"'));

	while(++pbeg!=pend)
	{
		if(*pbeg==_TX('\"')&&pbeg[-1]!=_CH_CVT)
			break;
	}
	if(pbeg==pend)
	{
	//	vfxReportError("error_unexpected_end_of_string",string_t(pbeg,pend));
	}
	return (char_t*)pbeg;
}

template<typename _OpT>
static char_t* _skip_non_str_until_ex(const char_t *pbeg, const char_t *pend, _OpT op)
{
	bool quate=false;
	const char_t *px=pbeg;

	while(px<pend&&(quate||!op(*px)))
	{
		if(*px==_TX('"'))
		{//if encounter a string enclosed with double quotation mark
			if(px==pbeg||px[-1]!=_CH_CVT)
				quate=!quate;
		}
		++px;
	}
	if(quate)
	{
//		vfxReportError("error_unexpected_end_of_string",string_t(pbeg,pend));
	}
	return (char_t*)px;
}

static char_t *_skip_non_str_non_space(const char_t *pbeg,const char_t *pend)
{
	return _skip_non_str_until_ex(pbeg,pend,_istspace);
}
static char_t *_skip_non_str_until(const char_t *pbeg,const char_t *pend,char_t until)
{
	//return _skip_non_str_until_ex(pbeg,pend,std::bind1st(std::equal_to<char_t>(),until));
	return _skip_non_str_until_ex(pbeg, pend, [until](char_t c) {
		return c == until;
		});
}
static char_t* _skip_until(const char_t *pbeg, const char_t *pend, char_t until)
{
	const char_t *px = pbeg;
	for (; px < pend && *px != until; ++px);
	return (char_t*)px;
}

static void _get_str_val(const char_t *pbeg, const char_t *px, string_t &str)
{
	const char_t *pwb=pbeg;

	for(;pwb!=px;++pwb)
	{
		if(*pwb==_TX('\"')&&(pwb==pbeg||pwb[-1]!=_CH_CVT))
			continue;
		else
			str.push_back(*pwb);
	}
}

static void _pro_arg(const char_t *pwb, const char_t *px, int nq, std::vector<string_t> &vstr)
{
	if(nq==0)
		vstr.push_back(string_t(pwb,px));
	else
	{
		vstr.push_back(string_t());
		vstr.back().reserve(px-pwb);

		_get_str_val(pwb,px,vstr.back());
	}
}


static void _read_strings(const char_t *pbeg, const char_t *pend, std::vector<string_t> &vstr)
{
	if(pbeg&&pend>pbeg)
	{
		int nq=0;
		const char_t *px=pbeg,*pwb=NULL;

		while(px!=pend)
		{
			if(!_istspace((ushort)*px))
			{
				if(!pwb)
				{
					pwb=px; nq=0;
				}

				if(*px==_TX('\"')&&px!=pbeg&&px[-1]!=_CH_CVT)
					++nq;
			}
			else
			{
				if(pwb&&(nq&1)==0)
				{
					_pro_arg(pwb,px,nq,vstr);
					pwb=NULL;
				}
			}
			++px;
		}

		if(pwb)
			_pro_arg(pwb,px,nq,vstr);
	}
}


template<typename _ValT>
static int _parse_arg(const string_t pps[], const char_t *fmt, _ValT *pval, int count)
{//for generic type
	int i=0;

	for(;i<count;++i)
	{
		if(_stscanf(pps[i].c_str(),fmt,pval+i)!=1)
			break;
	}
	return i;
}

static int _parse_arg(const string_t pps[],const char_t *fmt, string_t *pval, int count)
{//for string
	for(int i=0;i<count;++i)
		pval[i]=pps[i];
	return count;
}
static int _parse_arg(const string_t pps[],const char_t *fmt, char_t *pval, int count)
{//for char_t, need not to be separated by blank
	const char_t *ps=pps[0].c_str();
	int i=0;

	for(;i<count&&ps[i]&&!_istspace((ushort)ps[i]);++i)
	{
		pval[i]=ps[i];
	}
	return i;
}

static int _parse_arg(const string_t pps[], const char_t *fmt, bool *pval, int count)
{//for bool
	for(int i=0;i<count;++i)
		pval[i]=true; //default to be true

	if(pps&&!pps[0].empty())
	{
		const char_t *ps=pps[0].c_str();

		for(int i=0;i<count&&ps[i];++i)
		{//'+' for true, and '-' for false
			if(ps[i]==_TX('-'))
				pval[i]=false;
			else
				if(ps[i]!=_TX('+'))
				{
			//		vfxReportError("error_unknown_bool_char",ps);
					return i;
				}
		}	
	}
	
	return count;
}

template<typename _ValT>
static int _parse_arg_ex(IArgSet *pas, int request, const char_t *fmt, const char_t *var, _ValT *pval, int count,int pos)
{
	const string_t *pps=pas->query(var,request,pos);
	int nread=0;

	if(!pps&&count>0) //if not found
		pas->onFailed(var);
	else
	{
		nread=_parse_arg(pps,fmt,pval,count);
		if(nread!=count)
			pas->onFailed(var);
	}
	return nread;
}

IArgSet::IArgSet()
:m_bAllowExcept(true)
{
}
bool IArgSet::contain(const char_t *var)
{
	int flag=0;
	this->query(var,1,0,&flag);

	return (flag&ASF_NOT_FOUND)==0;
}
void IArgSet::allowException(bool bAllow)
{
	m_bAllowExcept=bAllow;
}
int IArgSet::get(const char_t *var, bool *val, int count, int pos )
{
	return _parse_arg(this->query(var,1,pos),_TX(""),val,count);
}
int IArgSet::get(const char_t *var, char_t *val, int count, int pos )
{
	return _parse_arg_ex(this,1,_TX(""),var,val,count,pos);
}
int IArgSet::get(const char_t *var, int *val, int count, int pos )
{
	return _parse_arg_ex(this,count,_TX("%d"),var,val,count,pos);
}
int IArgSet::get(const char_t *var, float *val, int count, int pos )
{
	return _parse_arg_ex(this,count,_TX("%f"),var,val,count,pos);
}
int IArgSet::get(const char_t *var, double *val, int count, int pos )
{
	return _parse_arg_ex(this,count,_TX("%lf"),var,val,count,pos);
}
int IArgSet::get(const char_t *var, string_t *val, int count, int pos )
{
	return _parse_arg_ex(this,count,_TX(""),var,val,count,pos);
}

template<typename _SrcT, typename _DestT>
static int _argset_cast_get(IArgSet *parg, const char_t *var, _DestT *val, int count, int pos)
{
	AutoArrayPtr<_SrcT> psrc(new _SrcT[count]);
	int nr=parg->get(var,psrc,count,pos);

	for(int i=0;i<nr;++i)
		val[i]=(_DestT)psrc[i];

	return nr;
}

int IArgSet::get(const char_t *var, unsigned char *val, int count, int pos )
{
	return _argset_cast_get<int>(this,var,val,count,pos);
}

void IArgSet::onFailed(const char_t *var)
{
	if(m_bAllowExcept)
		this->onException(var);
}
void IArgSet::onException(const char_t *var)
{
	FF_EXCEPTION(ERR_INVALID_ARGUMENT,var);
}
//========================================================================
IArgSetList::IArgSetList()
:m_pNext(NULL)
{
}
void IArgSetList::setNext(IArgSet *pNext)
{
	m_pNext=pNext;
}
//========================================================================

static void _cvt_cmd_string(char_t *pbeg, char_t *pend)
{//convert the command-line input
	char_t *prx=pbeg, *pwx=pbeg;

	while(prx<pend)
	{
		if(*prx==_CH_CVT&&prx+1!=pend)
		{//a double quotation mark that is not the beginning of a string
			++prx;
			*pwx=_cvt_special_char(*prx);
		}
		else
			if(*prx==_TX('"'))
			{//ignore this double quotation mark
				++prx; continue;
			}
			else
				*pwx=*prx;

		++pwx;
		++prx;
	}
	*pwx=_TX('\0');
}

static void _parse_arg_vals(char_t *pbeg,char_t *pend, std::vector<string_t> &vals)
{
	assert(pbeg==pend||_istspace((ushort)pend[-1]));

	char_t *px=pbeg;

	while(px<pend)
	{
		char_t *pbx=_skip_space(px,pend);
		char_t *pex=_skip_non_str_non_space(pbx,pend);

		if(pbx!=pex)
		{
			_cvt_cmd_string(pbx,pex);
			//idx.push_back(pbx);
			vals.push_back(pbx);
			px=pex+1;
		}
		else
		{
			assert(pex==pend);

			px=pend;
		}
	}
}

void removeEndingSpace(string_t &str)
{
	if (!str.empty())
	{
		const char_t *pb = str.c_str(), *pe = pb + str.size();
		while (pb != pe && _istspace(*pb))
			++pb;
		while (pe != pb && _istspace(pe[-1]))
			--pe;
		if (int(pe - pb) != str.size())
			str = string_t(pb, pe);
	}
}

struct _ArgItem
	:ArgSet::ArgInfo
{
	//do not hold any data member
public:
	void ParseValue()
	{
		m_vals.clear();
		removeEndingSpace(m_strVals);

		if (!m_strVals.empty())
		{
			string_t str(m_strVals);

			if (!_istspace((ushort)*str.rbegin()))
			{//append a blank in order to reserve space for '\0' of the last string.
				str.push_back(_TX(' '));
			}
			_parse_arg_vals(&str[0], &str[0] + str.size(), m_vals);

			if (m_vals.empty())
				m_strVals.clear();
		}

		m_bStrValOutdated = false;
	}
	void setArgs(int pos, const string_t &val)
	{
		if(uint(pos)<m_vals.size())
			m_vals[pos] = val;
		else
		{//fill all non-exist pos with val
			size_t i = m_vals.size();
			m_vals.resize(pos + 1);
			for (; i < m_vals.size(); ++i)
				m_vals[i] = val;
		}
		m_bStrValOutdated = true;
	}
};

static char_t *_skip_arg_name(const char_t *pbeg, const char_t *pend)
{
	while (pbeg<pend && !(_istspace((unsigned short)*pbeg)||*pbeg=='<')) //not space or begin of comment
		++pbeg;

	return (char_t*)pbeg;
}

static void _split_arg(const char_t *pbeg, const char_t *pend, std::vector<_ArgItem> &vArg, bool bAddHead)
{
	const char_t *px=pbeg,*pbx=NULL;
	int nhead=0; //number of anonymous varibles, which must be placed at the start of command-line
	int ni=0;    //number of named varibles, which can be placed anywhere after anonymous varibles
	int nq=0;   //number of quotes

	while(px<pend)
	{	
		if(
			(*px==_TX('-')&&px+1!=pend&&_istalpha((ushort)px[1])
#ifdef _MSC_VER
			||*px==_TX('/')
#endif
			)&&(px==pbeg||_istspace((ushort)px[-1]))
			)
		{//the beginning of a named varible, either the following two cases:
		 // 1) '-' followed by alphabetic letters ('-' followed by number may be an negative number)
		 // 2) '/' followed by any character (for windows only, for linux it may be a path as /mnt/...

			if((nq&1)==0) //if not in double quotes
			{
				if(pbx)
				{//save the value of previous varible
					vArg.back().m_strVals.append(pbx,px);
				}

				//search the end of the varible name
				//px+2 : *(px+1) is always taken as the var name, we skip this check in order to use "/<" for image-set-writer in vfx
				pbx=_skip_arg_name(px+2,pend); 
				
				vArg.push_back(_ArgItem());
				vArg.back().m_name=string_t(px+1,pbx);

				if (*pbx == '<')//begin of comment
				{
					px = pbx;
					pbx = _skip_until(pbx, pend, '>');
					vArg.back().m_comment = string_t(px + 1, pbx);
					if (*pbx == '>')
						++pbx;
				}

				px=pbx;
				++ni; 
			}
		}
		else
			if(ni==0&&bAddHead&&!_istspace((ushort)*px))
			{//the beginning of an anonymous varible
				const char_t *px0=px;
				px=_skip_non_str_non_space(px,pend);

				char_t buf[16];
				_stprintf(buf,_TX("#%d"),nhead); //make an name for the varible

				vArg.push_back(_ArgItem());
				vArg.back().m_name=buf;
				vArg.back().m_strVals =string_t(px0,px);
				++nhead;
			}
			else
			{
				if(*px==_TX('\"')&&(px==pbeg||px[-1]!=_CH_CVT))
				{
					++nq;
				}
			}
		++px;
	}
	
	if(pbx&&ni>0)
	{//save the value of the last varible
		vArg.back().m_strVals.append(pbx,pend);
	}
}

class ArgSet::_CImp
{
	typedef std::vector<_ArgItem>::iterator _ItrT;
public:
	std::vector<_ArgItem>  m_vArg;
	
	string_t m_argStr;
	bool     m_strOutdated = true;
//	int m_first_named;
public:
	_CImp()
	{
	}
	void clear()
	{
		m_vArg.clear();
	}
	void setArgs(const string_t &arg)
	{
		this->clear();

		if(!arg.empty())
		{
			_split_arg(&arg[0],&arg[0]+arg.size(),m_vArg,true);

			for (auto &a : m_vArg)
				a.ParseValue();
		}
		m_strOutdated = true;
	}

	_ItrT _find(const char_t *var)
	{
		_ItrT end(m_vArg.end()),itr(m_vArg.begin());

		for(;itr!=end;++itr)
		{
			if(_tcscmp(itr->m_name.c_str(),var)==0)
				break;
		}
		return itr;
	}

	//@bfound : whether the varible is found, note that the return value may be NULL even when the varible exists,
	// which may be for empty value or invalid @count or @pos
	const string_t* query(const char_t *var, int count, int pos, bool &bfound)
	{
		static char_t nbuf[16];

		_ItrT itr(m_vArg.end());

		if(var && var[0]==_TX('#') && pos!=0)
		{//if @pos is specified for #0 #1 #2 ...
			int ii=0;
			if(_stscanf(var+1,_TX("%d"),&ii)==1)
			{
		//		if(ii+pos<m_first_named) //in the range of anonymous
				{
					_stprintf(nbuf,_TX("#%d"),ii+pos); //add pos to the index, in order to support GetVector(...) of #0 #1 #2 ...
					itr=this->_find(nbuf);
					pos=0;
				}
			}
		}
		else
			itr=this->_find(var);

		const string_t *pr=NULL;
		
		if(itr!=m_vArg.end())
		{
			//itr->ParseValue();
			
			if(pos+count<=(int)itr->m_vals.size())
				pr=&itr->m_vals[pos];

			bfound=true;
		}
		else
			bfound=false;

		return pr;
	}

	_ArgItem* Find(const char_t *var)
	{
		_ItrT itr(this->_find(var));

		return  itr==m_vArg.end()? NULL:&*itr;
	}
	_ArgItem* _add_arg(const string_t &name, const string_t &val)
	{
		m_vArg.push_back(_ArgItem());
		m_vArg.back().m_name = name;
		m_vArg.back().m_strVals = val;
		m_vArg.back().ParseValue();
		return &m_vArg.back();
	}
	void  setArg(const string_t &name, const string_t &val, bool bCreate)
	{
		_ArgItem* arg = Find(name.c_str());
		if (!arg)
		{
			if(!bCreate)
				FF_EXCEPTION(ERR_INVALID_ARGUMENT, "argument not exist");
			else
				this->_add_arg(name, val);
		}
		else
		{
			arg->m_strVals = val;
			arg->ParseValue();
		}
		m_strOutdated = true;
	}

	void setArg(const string_t &name, int pos, const string_t &val, bool bCreate)
	{
		_ArgItem* arg = Find(name.c_str());
		if (!arg)
		{
			if (!bCreate)
				FF_EXCEPTION(ERR_INVALID_ARGUMENT, "argument not exist");
			else
			{
				arg = this->_add_arg(name, val);
				if (pos != 0)
					arg->setArgs(pos, val);
			}
		}
		else
			arg->setArgs(pos, val);

		m_strOutdated = true;
	}
	  
	const string_t& GetArgStr()
	{
		if (m_strOutdated)
		{
			m_argStr.clear();
			for (auto &e : m_vArg)
			{
				m_argStr += _TX('-')+e.m_name + _TX(' ') + e.m_strVals + _TX(' ');
			}
			m_strOutdated = false;
		}
		return m_argStr;
	}
};

//VFX_IMPLEMENT_COPY_FUNC(ArgSet);

string_t ArgSet::loadConfigFile(const string_t &file, int flags)
{
	FILE *fp = _tfopen(file.c_str(), _TX("r"));
	if (!fp)
	{
		FF_EXCEPTION(ERR_FILE_OPEN_FAILED, file.c_str());
	}
	string_t config;
	char_t line[1024];
	while (_fgetts(line, sizeof(line)/sizeof(line[0]), fp))
	{
		char_t *px = line;
		while (*px && _istspace(*px)) ++px; //ignore leading spaces

		size_t size = _tcslen(px);

		if (!(size==0 || size >= 2 && px[0] == _TX('/')&&px[1] == _TX('/'))) //not empty and not commented
			config += line + _TX(' ');
	}
	return config;
}

ArgSet::ArgSet()
:m_pImp(new _CImp)
{
}

ArgSet::ArgSet(const string_t &args, bool fromFile)
	: m_pImp(new _CImp)
{
	this->setArgs(args, fromFile);
}

ArgSet::ArgSet(int argc, char_t *argv[])
	: m_pImp(new _CImp)
{
	this->setArgs(argc, argv);
}
ArgSet::ArgSet(const string_t name_val[], int count)
{
	this->setArgs(name_val, count, true);
}

ArgSet::~ArgSet()
{
	delete m_pImp;
}
void ArgSet::setArgs(const string_t &args, bool fromFile)
{
	if(!fromFile)
		m_pImp->setArgs(args);
	else
	{
		string_t cfg(loadConfigFile(args));
		m_pImp->setArgs(cfg);
	}
}

void ArgSet::setArgs(int argc, char_t *argv[], bool ignoreFirst)
{
	string_t val;

	for(int i= ignoreFirst? 1:0; i<argc; ++i)
	{
		if (char_t *p = argv[i])
		{
			if (p[0] == _TX('-') || p[0] == _TX('/'))
				val += argv[i];
			else
			{
				val += _TX('\"');
				val += argv[i];
				val += _TX('\"');
			}

			val.push_back(_TX(' '));
		}
	}

	this->setArgs(val);
}

void  ArgSet::setArg(const string_t &name, const string_t &val, bool bCreateIfNotExist)
{
	m_pImp->setArg(name, val, bCreateIfNotExist);
}

void ArgSet::setArg(const string_t &name, int pos, const string_t &val, bool bCreateIfNotExist)
{
	m_pImp->setArg(name, pos, val, bCreateIfNotExist);
}

void ArgSet::setArgs(const string_t name_val[], int count, bool bCreateIfNotExist)
{
	for (int i = 0; i < count; ++i)
		this->setArg(name_val[i * 2], name_val[i * 2 + 1], bCreateIfNotExist);
}

const string_t* ArgSet::query(const char_t *var, int count, int pos,int *flag)
{
	char_t buf[32];
	if(!var)
	{
		_stprintf(buf,_TX("#%d"),pos);
		var=buf;
		pos=0;
	}

	bool bfound;
	const string_t* pr=m_pImp->query(var,count,pos,bfound);

	if (!bfound)
	{
		if (m_pNext)
			pr = m_pNext->query(var, count, pos, flag);
		else
		{
			if (flag)
				*flag |= ASF_NOT_FOUND;
		}
	}
	return pr;
}
int ArgSet::size() const
{
	return (int)m_pImp->m_vArg.size();
}
void ArgSet::clear()
{
	m_pImp->clear();
	this->setNext(NULL);
}
const ArgSet::ArgInfo* ArgSet::getArg(const string_t &name) const
{
	return m_pImp->Find(name.c_str());
}
const std::vector<ArgSet::ArgInfo>& ArgSet::getArgList() const
{
	return *(std::vector<ArgInfo>*)(&m_pImp->m_vArg);
}
 
const string_t& ArgSet::getArgStr()
{
	return m_pImp->GetArgStr();
}


_FF_END

