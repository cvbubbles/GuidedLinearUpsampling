#pragma once

#include<string>
#include<list>
#include"BFC/err.h"

_FF_BEG

template<typename _ObjectT>
class Factory
{
private:
	class ObjectType
	{
	public:
		std::string name;
		_ObjectT *obj;
		bool      isDefault;
	};

	std::list<ObjectType>  _objList;

	ObjectType* _getDefault()
	{
		ObjectType *p = nullptr;
		for (auto &v : _objList)
			if (v.isDefault)
			{
				p = &v; break;
			}
		return p;
	}
public:

	void registerType(_ObjectT *obj, const std::string &name, bool isDefault)
	{
		if (isDefault)
		{
			ObjectType *p = this->_getDefault();
			if (p)
			{
				printf("warning: default type has been set as %s when registering %s.", p->name.c_str(), name.c_str());
				isDefault = false;
			}
		}
		_objList.push_back({ name,obj, isDefault });
	}

	_ObjectT* create(const char *type)
	{
		_ObjectT *obj = nullptr;

		if (type)
		{
			for (auto &v : _objList)
				if (stricmp(v.name.c_str(), type) == 0)
				{
					obj = v.obj;
					break;
				}
		}
		else
		{
			auto *p = _getDefault();
			if (p)
				obj = p->obj;
		}
		if (obj)
			obj = reinterpret_cast<_ObjectT*>(obj->clone());

		if (!obj)
			FF_EXCEPTION(ERR_INVALID_ARGUMENT, (std::string("failed to create object of type ") + (type ? type : "DEFAULT")).c_str());

		return obj;
	}

	template<typename _T>
	_T* createObject(const char *type)
	{
		return reinterpret_cast<_T*>(this->create(type));
	}
};

typedef void* (*CreateFuncT)();

class Factory2
{
private:
	class ObjectType
	{
	public:
		std::string name;
		CreateFuncT createFunc;
		bool      isDefault;
	};

	std::list<ObjectType>  _objList;

	ObjectType* _getDefault()
	{
		ObjectType *p = nullptr;
		for (auto &v : _objList)
			if (v.isDefault)
			{
				p = &v; break;
			}
		return p;
	}
public:

	void registerType(CreateFuncT createFunc, const char *name, bool isDefault)
	{
		if (isDefault)
		{
			ObjectType *p = this->_getDefault();
			if (p)
			{
				printf("warning: default type has been set as %s when registering %s.", p->name.c_str(), name);
				isDefault = false;
			}
		}
		_objList.push_back({ std::string(name), createFunc, isDefault });
	}

	void* create(const char *type)
	{
		CreateFuncT create = nullptr;

		if (type)
		{
			for (auto &v : _objList)
				if (stricmp(v.name.c_str(), type) == 0)
				{
					create = v.createFunc;
					break;
				}
		}
		else
		{
			auto *p = _getDefault();
			if (p)
				create = p->createFunc;
		}

		void *obj = create ? create() : nullptr;

		//if (!obj)
		//	FF_EXCEPTION(ERR_INVALID_ARGUMENT, (std::string("failed to create object of type ") + (type ? type : "DEFAULT")).c_str());

		return obj;
	}

	template<typename _T>
	_T* createObject(const char *type)
	{
		return reinterpret_cast<_T*>(this->create(type));
	}
};

class FactoryEx
{
	struct DGroup
	{
		std::string name;
		Factory2    fact;
	};

	std::list<DGroup>  _fList;

	std::list<DGroup>::iterator _find(const char *groupName, bool createIfNotExist)
	{
		auto itr = _fList.begin();
		for (; itr != _fList.end(); ++itr) {
			if (strcmp(itr->name.c_str(), groupName) == 0)
				break;
		}
		if (itr == _fList.end() && createIfNotExist)
		{
			_fList.push_back(DGroup());
			--itr;
			itr->name = groupName;
		}
		return itr;
	}
public:
	Factory2* get(const char *group, bool createIfNotExist = true)
	{
		auto itr = this->_find(group, createIfNotExist);
		return itr != _fList.end() ? &itr->fact : nullptr;
	}
	void registerType(CreateFuncT createFunc, const char *group, const char *name, bool isDefault)
	{
		this->get(group, true)->registerType(createFunc, name, isDefault);
	}
	void* create(const char *group, const char *type)
	{
		auto *f = this->get(group, false);
		return f ? f->create(type) : nullptr;
	}
	template<typename _T>
	_T* createObject(const char *group, const char *type)
	{
		return reinterpret_cast<_T*>(this->create(group, type));
	}
};

_FF_END

