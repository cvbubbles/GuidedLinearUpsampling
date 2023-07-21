
#include"BFC/object.h"
#include"BFC/factory.h"
#include<mutex>
 
_FF_BEG

void DObject::release()
{
	delete this;
}
DObject::~DObject()
{}

static FactoryEx *gFactory = nullptr;
int   DObject::registerType(CreateObjectFuncT createFunc, const char *group, const char *type, int flag)
{
	if (!gFactory)
		gFactory = new FactoryEx;

	gFactory->registerType(createFunc, group, type, flag != 0);

	return 0;
}

DObject* DObject::createObject(const char *group, const char *type)
{
	return gFactory ? gFactory->createObject<DObject>(group, type) : nullptr;
}


//==============================================================================

class Module::_CImpl
{
public:
	static std::shared_ptr<DObject> _makePtr(DObject *ptr) {
		return std::shared_ptr<DObject>(ptr, [](DObject *ptr) {ptr->release(); });
	}

	struct _DObject
	{
		std::string name;
		std::shared_ptr<DObject>  ptr;
	public:
		_DObject()
		{}
		_DObject(const char *_name, DObject *_ptr)
			:name(_name),
			ptr(_makePtr(_ptr))
		{}
	};
	std::list<_DObject>  objList;
	//std::mutex   _mutex;
	std::recursive_mutex _mutex;

	typedef std::list<_DObject>::iterator _ItrT;

	_ItrT findObject(const char *name)
	{
		auto itr = objList.begin();
		for (; itr != objList.end(); ++itr)
			if (strcmp(itr->name.c_str(), name) == 0)
				break;
		return itr;
	}
	DObject* getObject(Module *site, const char *name)
	{
		std::lock_guard<std::recursive_mutex> _lock(_mutex);

		auto itr = this->findObject(name);
		if (itr != objList.end())
			return itr->ptr.get();

		auto *ptr = site->_createObject(name);
		if (ptr)
			objList.push_back(_DObject(name, ptr));

		return ptr;
	}
	void setObject(const char *name, DObject *obj) {

		std::lock_guard<std::recursive_mutex> _lock(_mutex);

		auto itr = this->findObject(name);
		if (itr != objList.end())
			itr->ptr = _makePtr(obj);
		else
			objList.push_back(_DObject(name, obj));
	}
};

Module::Module()
	:iptr(new _CImpl)
{}
Module::~Module()
{
	delete iptr;
}

DObject* Module::getObject(const char *name)
{
	return iptr->getObject(this, name);
}
void     Module::setObject(const char *name, DObject *obj)
{
	iptr->setObject(name, obj);
}
void Module::setParams(const std::string &params)
{
}
DObject* Module::_createObject(const char *name)
{
	return nullptr;
}

_FF_END

