
#include"BFC/thread.h"
#include<thread>
#include<mutex>
#include<list>
#include<condition_variable>

#ifdef _WIN32
#include<Windows.h>
#endif

_FF_BEG

class Thread::CImpl
{
	typedef Thread::Msg Msg;
	typedef Thread::MsgData MsgData;
public:
	std::thread      *threadPtr=nullptr;
	//std::thread::id  threadID;
	std::list<Msg*>  msgList;
	std::mutex		 msgMutex;
	std::condition_variable waitCV;
	bool             toBeExit=false;
	bool             isIdle=true;
public:
	CImpl()
	{
		threadPtr = new std::thread(threadProc, this);
	}
	~CImpl()
	{
		//this->waitAll();
		toBeExit = true;
		threadPtr->join();
		//threadPtr->detach();
		delete threadPtr;
	}
	bool setPriority(int priority)
	{
		bool ok = false;
		if (threadPtr)
		{
#ifdef _WIN32
			ok=::SetThreadPriority((HANDLE)threadPtr->native_handle(), priority)==TRUE;
#else
			ok=true;
#endif
		}
		return ok;
	}
	void _threadProc();

	static void threadProc(CImpl *site)
	{
		site->_threadProc();
	}

	void waitAll()
	{
		std::mutex  waitMutex;
		std::unique_lock<std::mutex> lock(waitMutex);
		waitCV.wait(lock);
	}

	void post(Msg *msg, bool wait)
	{
		//if posted from the GL thread itself and required to wait finish, execute directly without post event
		if (wait && std::this_thread::get_id() == threadPtr->get_id())
		{
			MsgData data;
			data.nRemainingMessages = this->nRemainingMessages();

			msg->exec(data);
			msg->release();
		}
		else
		{
			{
				std::lock_guard<std::mutex> lock(msgMutex);

				msgList.push_back(msg);
			}
			if (wait)
				this->waitAll();
		}
	}
	int nRemainingMessages()
	{
		std::lock_guard<std::mutex> lock(msgMutex);
		return (int)msgList.size();
	}
};

void Thread::CImpl::_threadProc()
{
	//threadID = std::this_thread::get_id();

	MsgData data;

	while (true)
	{
		Msg *msg = nullptr;
		
		data.nRemainingMessages = 0;
		{
			std::lock_guard<std::mutex> lock(msgMutex);

			if (!msgList.empty())
			{
				msg = msgList.front();
				msgList.pop_front();
				data.nRemainingMessages = (int)msgList.size();
			}
		}

		if (!msg)
		{
			isIdle = true;
			waitCV.notify_all();
			if (toBeExit)
				break;
			else
				std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}
		else
		{
			isIdle = false;
			msg->exec(data);
			msg->release();
		}
	}
}

Thread::Thread()
	:ptr(new CImpl)
{}
Thread::~Thread()
{
	delete ptr;
}
void Thread::post(Msg *msg, bool waitFinish)
{
	ptr->post(msg, waitFinish);
}
void Thread::waitAll()
{
	ptr->waitAll();
}

bool Thread::setPriority(int priority)
{
	return ptr->setPriority(priority);
}

int  Thread::nRemainingMessages()
{
	return ptr->nRemainingMessages();
}
bool Thread::isIdle()
{
	return ptr->isIdle;
}

_FF_END

