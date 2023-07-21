
#ifndef _FF_BFC_AUTORES_H
#define _FF_BFC_AUTORES_H

#include <utility>
#include <iterator>
#include<memory>


#include"ffdef.h"

_FF_BEG


//To help implement reference-counter in @AutoRes.
template<typename ValT>
class ReferenceCounter
{
	typedef typename  std::pair<int,ValT> _CounterT;
private:
	_CounterT *_pdata;

	void _inc() const
	{
		++_pdata->first;
	}
	void _dec() const
	{
		--_pdata->first;
	}
public:
	ReferenceCounter()
		:_pdata(new _CounterT(1,ValT()))
	{
	}
	ReferenceCounter(const ValT& val)
		:_pdata(new _CounterT(1,val))
	{
	}
	ReferenceCounter(const ReferenceCounter& right)
		:_pdata(right._pdata)
	{
		this->_inc();
	}
	ReferenceCounter& operator=(const ReferenceCounter& right)
	{
		if(_pdata!=right._pdata)
		{
			if(this->IsLast())
				delete _pdata;
			else
				this->_dec();
			_pdata=right._pdata;
			right._inc();
		}
		return *this;
	}
	//whether current reference is the last one.
	bool IsLast() const
	{
		return _pdata->first==1;
	}
	//get the value stored with the counter.
	const ValT& Value() const
	{
		return _pdata->second;
	}
	ValT& Value()
	{
		return _pdata->second;
	}
	//swap two counters.
	void Swap(ReferenceCounter& right)
	{
		std::swap(this->_pdata,right._pdata);
	}
	
	~ReferenceCounter() 
	{
		if(this->IsLast())
			delete _pdata;
		else
			this->_dec();
	}
};

 //auto-resource, which is the base to implement AutoPtr,AutoArrayPtr and AutoComPtr.
template<typename PtrT,typename RetPtrT,typename ReleaseOpT>
class AutoRes
{
	enum{_F_DETACHED=0x01};

	struct _ValT
	{
		PtrT			ptr;
		ReleaseOpT		rel_op;
		int				flag;
	public:
		_ValT()
			:ptr(PtrT()),flag(0)
		{
		}
		_ValT(const PtrT & _ptr,const ReleaseOpT & _rel_op, int _flag=0)
			:ptr(_ptr),rel_op(_rel_op),flag(_flag)
		{
		}
		void release()
		{
			if((flag&_F_DETACHED)==0)
				rel_op(ptr);
		}
	};

	typedef ReferenceCounter<_ValT>   _CounterT;

protected:
	_CounterT _counter;
	
	void _release()
	{ //release the referenced object with the stored operator.
		_counter.Value().release();
	}
public:
	AutoRes()
	{
	}
	explicit AutoRes(PtrT ptr,const ReleaseOpT& op=ReleaseOpT())
		:_counter(_ValT(ptr,op))
	{
	}
	AutoRes& operator=(const AutoRes& right)
	{
		if(this!=&right)
		{
			if(_counter.IsLast())
				this->_release();
			_counter=right._counter;
		}
		return *this;
	}
	RetPtrT operator->() const
	{
		return &*_counter.Value().ptr;
	}
	typename std::iterator_traits<RetPtrT>::reference operator*() const
	{
		return *_counter.Value().ptr;
	}
	void Swap(AutoRes& right)
	{
		_counter.Swap(right._counter);
	}
	//detach the referenced object from @*this.
	RetPtrT Detach()
	{
		_counter.Value().flag|=_F_DETACHED;
		return &*_counter.Value().ptr;
	}
	operator PtrT()
	{
		return _counter.Value().ptr;
	}
	~AutoRes() throw()
	{
		if(_counter.IsLast())
			this->_release();
	}
};
//operator to delete a single object.
class DeleteObj
{
public:
	template<typename _PtrT>
	void operator()(_PtrT ptr)
	{
		delete ptr;
	}
};

template<typename _ObjT>
class AutoPtr
	:public AutoRes<_ObjT*,_ObjT*,DeleteObj>
{
public:
	explicit AutoPtr(_ObjT* ptr=NULL)
		:AutoRes<_ObjT*,_ObjT*,DeleteObj>(ptr)
	{
	}
};

//operator to delete array of objects.
class DeleteArray
{
public:
	template<typename _PtrT>
	void operator()(_PtrT ptr)
	{
		delete[]ptr;
	}
};

template<typename _ObjT>
class AutoArrayPtr
	:public AutoRes<_ObjT*,_ObjT*,DeleteArray>
{
public:
	explicit AutoArrayPtr(_ObjT* ptr=NULL)
		:AutoRes<_ObjT*,_ObjT*,DeleteArray>(ptr)
	{
	}
	_ObjT& operator[](int i) const
	{
		return this->_counter.Value().ptr[i];
	}
};

class  OpCloseFile
{
public:
	void operator()(const FILE *fp)
	{
		if(fp)
			fclose((FILE*)fp);
	}
};

class AutoFilePtr
	:public AutoRes<FILE*,FILE*,OpCloseFile>
{
public:
	explicit AutoFilePtr(FILE *fp=NULL)
		:AutoRes<FILE*,FILE*,OpCloseFile>(fp)
	{
	}
};

//operator to release COM object.
class ReleaseObj
{
public:
	template<typename _PtrT>
	void operator()(_PtrT ptr)
	{
		if(ptr)
			ptr->Release();
	}
};

template<typename _ObjT>
class AutoComPtr
	:public AutoRes<_ObjT*,_ObjT*,ReleaseObj>
{
public:
	explicit AutoComPtr(_ObjT* ptr=NULL)
		:AutoRes<_ObjT*,_ObjT*,ReleaseObj>(ptr)
	{
	}
};

class DestroyObj
{
public:
	template<typename _PtrT>
	void operator()(_PtrT ptr)
	{
		if(ptr)
			ptr->Destroy();
	}
};

template<typename _ObjT>
class AutoDestroyPtr
	:public AutoRes<_ObjT*,_ObjT*,DestroyObj>
{
public:
	explicit AutoDestroyPtr(_ObjT* ptr=NULL)
		:AutoRes<_ObjT*,_ObjT*,DestroyObj>(ptr)
	{
	}
};


template<typename _ServerPtrT>
class ReleaseClientOp
{
private:
	_ServerPtrT m_pServer;
public:
	ReleaseClientOp(_ServerPtrT pServer=_ServerPtrT())
		:m_pServer(pServer)
	{
	}
	template<typename _ClientPtrT>
	void operator()(_ClientPtrT ptr)
	{
		if(m_pServer)
			m_pServer->ReleaseClient(ptr);
	}
};

template<typename _ServerPtrT,typename _ClientPtrT>
class AutoClientPtr
	:public AutoRes<_ClientPtrT,_ClientPtrT,ReleaseClientOp<_ServerPtrT> >
{
	typedef AutoRes<_ClientPtrT,_ClientPtrT,ReleaseClientOp<_ServerPtrT> > _MyBaseT;
public:
	explicit AutoClientPtr(_ServerPtrT pServer=_ServerPtrT(),_ClientPtrT pClient=_ClientPtrT())
		:_MyBaseT(pClient,ReleaseClientOp<_ServerPtrT>(pServer))
	{
	}
};

_FF_END


#endif


