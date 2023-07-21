#pragma once

#include"ffdef.h"

_FF_BEG

class _BFC_API Thread
{
	class CImpl;

	CImpl *ptr;
public:
	class MsgData
	{
	public:
		int  nRemainingMessages;
	};
	class Msg
	{
	public:
		virtual void exec(MsgData &data) = 0;

		virtual void release()
		{
			delete this;
		}

		virtual ~Msg()
		{}
	};

public:
	Thread();

	~Thread();

	Thread(const Thread&) = delete;
	Thread& operator=(const Thread&) = delete;

	void post(Msg *msg, bool waitFinish=false);

	void waitAll();

	bool setPriority(int priority);

	int  nRemainingMessages();

	bool isIdle();
	
private:
	template<typename _OpT>
	class _MsgX1
		:public Msg
	{
		_OpT _op;
	public:
		_MsgX1()
		{}
		_MsgX1(const _OpT &op)
			:_op(op)
		{}
		virtual void exec(MsgData &data)
		{
			_op(data);
		}
	};
	template<typename _OpT>
	class _MsgX0
		:public Msg
	{
		_OpT _op;
	public:
		_MsgX0()
		{}
		_MsgX0(const _OpT &op)
			:_op(op)
		{}
		virtual void exec(MsgData &data)
		{
			_op();
		}
	};
	template<typename _OpT>
	Msg* _makeMsg(const _OpT &op, void (_OpT::*)())
	{
		return new _MsgX0<_OpT>(op);
	}
	template<typename _OpT>
	Msg* _makeMsg(const _OpT &op, void (_OpT::*)() const)
	{
		return new _MsgX0<_OpT>(op);
	}
	template<typename _OpT>
	Msg* _makeMsg(const _OpT &op, void (_OpT::*)(MsgData &))
	{
		return new _MsgX1<_OpT>(op);
	}
	template<typename _OpT>
	Msg* _makeMsg(const _OpT &op, void (_OpT::*)(MsgData &) const)
	{
		return new _MsgX1<_OpT>(op);
	}
public:
	template<typename _OpT>
	void post(const _OpT &op, bool waitFinish = false)
	{
		this->post(_makeMsg(op, &_OpT::operator()), waitFinish);
	}
	template<typename _OpT>
	void call(const _OpT &op)
	{
		this->post(op, true);
	}
};



_FF_END

