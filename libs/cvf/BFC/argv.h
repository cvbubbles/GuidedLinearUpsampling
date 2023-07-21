

#pragma once


#include"ffdef.h"
#include<string>
#include<vector>

_FF_BEG



enum
{
	ASF_NOT_FOUND=0x01
};

class _BFC_API IArgSet
{
	bool  m_bAllowExcept;
public:
	IArgSet();

	//query the value of the varible @var.
	//this is a pure virtual function that must be implemented by varible manager classes.
	virtual const string_t* query(const char_t *var, int count = 1, int pos = 0, int *flag = NULL){ return NULL; }

	virtual bool contain(const char_t *var);

	//call this function if any error occur.
	void onFailed(const char_t *var);

	//call this function if an exception should be raised.
	void onException(const char_t *var);

	//whether allow exception raised from this class. 
	//by default @OnFailed call @OnException to raise a standard exception derived from std::exception, its behavior can be changed
	//by calling this function with @bAllow be false.
	void allowException(bool bAllow);

	//get the value of a varible with different types.
	//@var: the name of the varible.
	//@val: the array to store the varible.
	//@count: number of elements to get, with this argument array can be retrieved easily.
	//		e.g. to read -color 255 255 255, please call as: ->get("color", &val[0], 3, 0);
	//@pos : the start position, with this argument the varibles can be defined to be structure.
	//		e.g. to specify a set of strings whose number is not fixed, you can use the following command:
	//			-str 5 str0 str1 str2 str3 str4
	//			 then use the following code to get:
	//		{
	//			int   count=pvar->Get<int>("str");
	//			string_t *pstr=new string_t[count];
	//			pvar->get("str",pstr,count,2);
	//		}
	//@return value: the number of elements read successfully.
	//note that if exception is allowed and the required number of elements are not all read successfully, exceptions would be raised
	//before the function return, in which case you need not to check the return value to handle error.
	int get(const char_t *var, bool *val, int count=1, int pos=0);

	int get(const char_t *var, char_t *val, int count=1, int pos=0);

	int get(const char_t *var, int *val, int count=1, int pos=0);

	int get(const char_t *var, float *val, int count=1, int pos=0);

	int get(const char_t *var, double *val, int count=1, int pos=0);

	int get(const char_t *var, string_t *val, int count=1, int pos=0);

	int get(const char_t *var, unsigned char *val, int count=1, int pos=0);

	//convenient function to read varibles with only one element.
	//this function always raise exception when read failed.
	template<typename _ValT>
	_ValT get(const char_t *var, int pos=0)
	{
		_ValT val;
		if(this->get(var,&val,1,pos)!=1)
			this->onException(var);
		return val;
	}
	//convenient function to read varibles with default value.
	//this function would never raise exception, and the specified default value will be returned if failed to read from command line.
	template<typename _ValT>
	_ValT _getd(const char_t *var,const _ValT &defaultVal=_ValT(), int pos=0)
	{
		bool bae=m_bAllowExcept;
		this->allowException(false);

		_ValT val;
		int nr=this->get(var,&val,1,pos);
		
		this->allowException(bae);
		return nr==1? val:defaultVal;
	}
	bool _getd(const char_t *var, const bool defaultVal = false, int pos = 0)
	{
		return this->contain(var) ? this->get<bool>(var, pos) : defaultVal;
	}
	template<typename _ValT>
	_ValT getd(const char_t *var, const _ValT &defaultVal = _ValT(), int pos = 0)
	{
		return _getd(var, defaultVal, pos);
	}
	
	
#if 0
	//the vector can be represented as a structure, with the first element be the number of elements to read.
	//return value: negative if failed.
	template<typename _ValT,typename _AllocT>
	int GetVector(const char_t *var, std::vector<_ValT,_AllocT> &vec)
	{
		std::vector<_ValT,_AllocT> temp;
		int count=0;

		if(this->get(var,&count,1,0)==1)
		{
			if(count>0)
			{
				temp.resize(count);

				if(this->get(var,&temp[0],count,1)==count)
					vec.swap(temp);
				else
					count=-1;
			}
			else
				vec.resize(0);
		}
		else
			count=-1;

		if(count<0)
			this->OnFailed(var);

		return count;
	}
#else

	//need not to specify the number of elements
	template<typename _ValT,typename _AllocT>
	int getv(const char_t *var, std::vector<_ValT,_AllocT> &vec)
	{
		std::vector<_ValT,_AllocT> temp;

		bool bae=m_bAllowExcept;
		this->allowException(false);

		_ValT vx;

		for(int i=0; ; ++i)
		{
			if(this->get(var,&vx,1,i)==1)
			{
				temp.push_back(vx);
			}
			else
				break;
		}

		vec.swap(temp);
		
		this->allowException(bae);

		return (int)vec.size();
	}
	template<typename _ValT>
	std::vector<_ValT> getv(const char_t *var)
	{
		std::vector<_ValT> vec;
		this->getv(var, vec);
		return vec;
	}
	//convenient function to read a c-style array.
	template<int _N, typename _ValT>
	int getv(const char_t *var, _ValT(&val)[_N], int pos = 0)
	{
		return this->get(var, val, _N, pos);
	}
#endif
};

/*
to combine multiple arg-sets
*/
class _BFC_API IArgSetList
	:public IArgSet
{
protected:
	IArgSet  *m_pNext;
public:
	IArgSetList();

	virtual void setNext(IArgSet *pNext);
};

//argument set used with command line.
class _BFC_API ArgSet
	:public IArgSetList
{
	class _CImp;

	_CImp  *m_pImp;
public:
	class ArgInfo
	{
	public:
		string_t  m_name;
		string_t  m_strVals;
		std::vector<string_t>  m_vals;
		string_t  m_comment;
	protected:
		bool m_bStrValOutdated = true;
	};
public:
	static string_t loadConfigFile(const string_t &file, int flags = 0);
public:
	
//	VFX_DECLARE_COPY_FUNC(ArgSet);

	ArgSet();

	ArgSet(const string_t &args, bool fromFile=false);

	ArgSet(int argc, char_t *argv[]);

	ArgSet(const string_t name_val[], int count);

	~ArgSet();

//	const vfxArgInf* Find(const char_t *var);

	//get the number of argument.
	int  size() const;

	void clear();

	//set from a command line string
	//if @fromFile is true, args is the path to a config file, which then would loaded by loadConfigFile
	//exist contents will be cleared first
	void setArgs(const string_t &args, bool fromFile=false);

	//set from command line arguments
	//exist contents will be cleared first
	void  setArgs(int argc, char_t *argv[], bool ignoreFirst=true);

	//set args without clear exist contents.
	//@name_val : array of name and val pairs (the elements should be 2*count)
	void setArgs(const string_t name_val[], int count, bool bCreateIfNotExist = true);

	//set a specific arg, val may contain multiple values, e.g. setArg("a","3 7 6"), then "a" will be an array of {3,7,6}.
	void  setArg(const string_t &name, const string_t &val, bool bCreateIfNotExist=true);

	//set a specific arg pos, val must be a single value, e.g. setArg("a","3 7 6") is invalid
	void setArg(const string_t &name, int pos, const string_t &val, bool bCreateIfNotExist=true);

	const ArgInfo* getArg(const string_t &name) const;

	const std::vector<ArgInfo>& getArgList() const;

	const string_t& getArgStr();

public:
	virtual const string_t* query(const char_t *var, int count=1, int pos=0,int *flag=NULL);
};

//for compatibility only
typedef ArgSet CommandArgSet;

_FF_END



