#pragma once

#include"BFC/def.h"
#include<memory>
#include<string>

_FF_BEG


typedef void* (*CreateObjectFuncT)();

//object type that supports dynamic-creation
class _BFCS_API DObject
{
public:
	//virtual void exec(const char *cmd, const char *args, void *param1 = nullptr, void *param2 = nullptr);

	virtual void release();

	virtual ~DObject();
public:
	static int   registerType(CreateObjectFuncT createFunc, const char *group, const char *type, int flag);

	static DObject* createObject(const char *group, const char *type);

	template<typename _T>
	static _T* createObjectT(const char *group, const char *type)
	{
		return (_T*)createObject(group, type);
	}
};

#define DEFINE_DTYPE2(_class,typeName) public:\
	virtual void release() {delete this;}\
	static const char* getTypeName() {return typeName;}

#define DEFINE_DTYPE(_class) DEFINE_DTYPE2(_class,#_class)


#define DEFINE_DGROUP2(_class,groupName) \
public:\
static std::shared_ptr<_class> create(const char *type = nullptr){\
	return std::shared_ptr<_class>(\
		createObjectT<_class>(groupName, type),\
		[](DObject *ptr) {ptr->release(); }\
	);}\
static const char* getGroupName() {return groupName;}

#define DEFINE_DGROUP(_class) DEFINE_DGROUP2(_class,#_class)


#define _FF_TOKENPASTE(x, y) x ## y
#define _FF_TOKENPASTE2(x, y) _FF_TOKENPASTE(x, y)
#define _FF_UNIQUE_NAME(x) _FF_TOKENPASTE2(x, __LINE__)


#define REGISTER_DTYPE2(_class, flag) namespace _FF_UNIQUE_NAME(_ff_regtype){namespace{ static void* createObject(){return new _class;} \
 static int _auto_register_ffobjs=ff::DObject::registerType(createObject, _class::getGroupName(), _class::getTypeName(), flag);}}
#define REGISTER_DTYPE(_class) REGISTER_DTYPE2(_class,0)

class _BFCS_API Module
	:public DObject
{
	class _CImpl;
	_CImpl  *iptr;
private:
	template<typename _ValT>
	struct _ValObject
		:public DObject
	{
		_ValT  val;
	};
public:
	Module();

	~Module();

	Module(const Module&) = delete;
	Module& operator=(const Module&) = delete;

	DObject*  getObject(const char *name);

	void     setObject(const char *name, DObject *obj);

	template<typename _T>
	_T* getObject(const char *name) {
		return reinterpret_cast<_T*>(this->getObject(name));
	}

	template<typename _T>
	_T* getObject() {
		return reinterpret_cast<_T*>(this->getObject(_T::getTypeName()));
	}

	template<typename _ValT>
	_ValT& getv(const char *name, bool createIfNotExist = false)
	{
		typedef _ValObject<_ValT> _T;
		_T *obj = this->getObject<_T>(name);
		if (!obj)
		{
			if (createIfNotExist)
			{
				obj = new _T;
				this->setObject(name, obj);
			}
			else
			{
				char msg[256];
				sprintf(msg, "Module: object is not exist, name=%s", name);
				throw std::invalid_argument(msg);
			}
		}
		return obj->val;
	}
	template<typename _ValT>
	void setv(const char *name, const _ValT &val)
	{
		this->getv<_ValT>(name, true) = val;
	}

	virtual void setParams(const std::string &params);
protected:
	//call by getObject to create new object
	virtual DObject* _createObject(const char *name);
};

_FF_END

