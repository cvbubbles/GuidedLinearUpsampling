
#pragma once

#include"def.h"
#include"opencv2/highgui.hpp"
#include"imgset.h"

_CVX_BEG


//_CVX_API int sendAppCommand(const std::string &wndTitle, const std::string &cmd, bool waitFinished=true);

//_CVX_API void setKeyboardCallback(const std::string &wndName, KeyboardCallBack callBack, void *userData);

enum{
	KEY_TAB=9,
	KEY_ESCAPE=27,
	KEY_BACK=8,
};

enum
{
	CVKEY_ESCAPE=27,
	CVKEY_F1 = 0x700000,
	CVKEY_F2 = CVKEY_F1 + 0x010000,
	CVKEY_F3 = CVKEY_F1 + 0x020000,
	CVKEY_F4 = CVKEY_F1 + 0x030000,
	CVKEY_F5 = CVKEY_F1 + 0x040000,
	CVKEY_F6 = CVKEY_F1 + 0x050000,
	CVKEY_F7 = CVKEY_F1 + 0x060000,
	CVKEY_F8 = CVKEY_F1 + 0x070000,
	CVKEY_F9 = CVKEY_F1 + 0x080000,
	CVKEY_F10 = CVKEY_F1 + 0x090000,
	CVKEY_F11 = CVKEY_F1 + 0x0a0000,
	CVKEY_F12 = CVKEY_F1 + 0x0b0000,
};
// exitCode==27 (ESCAPE)
// exitCode==-1 (any key)
_CVX_API int cvxWaitKey(int exitCode = CVKEY_ESCAPE);

union CVEventData
{
	void *ptr=nullptr;
	int   ival;
public:
	CVEventData()
	{}
	CVEventData(const void *_ptr)
		:ptr((void*)_ptr)
	{}
	CVEventData(int _ival)
		:ival(_ival)
	{}
};

class _CVX_API CVEventHandler
{
public:
	//return 0 if processed
	//virtual int  onKeyboard(int code, int flags);
	
	//return 0 if processed
	//virtual int onMouse(int event, int x, int y, int flags);

	//virtual void onTrackbar(int pos);

	virtual int onEvent(int evt, int param1, int param2, CVEventData data);

	virtual ~CVEventHandler() throw();
};

typedef std::shared_ptr<CVEventHandler> CVEventHandlerPtr;

//make type exactly as int..
static const int EVENT_NULL = -1;

enum
{
	//mouse event : param1=x, param2=y, (int)data=flags

	//param1=code
	EVENT_KEYBOARD=20,

	//trackbar pos changed : param1=pos
	EVENT_TRACKBAR_POS,

	//the showed imageset is changed 
	//param1=<ID of the new imageset>, param2=<ID of the old imageset>,
	EVENT_IMAGESET_CHANGED,

	//the showed frame is changed
	//param1=<pos of the new frame>, param2=<pos of the old frame>
	EVENT_FRAME_CHANGED,

	EVENT_APP_COMMAND,

	//drag-drop files
	//data.ptr pointed to a std::vector<std::string> object containing the file pathes.
	EVENT_DRAGDROP, 

	EVENT_USER=100,
};

template<typename _EventOpT>
class CVEventHandlerEx
	:public CVEventHandler
{
	_EventOpT eventOp;

	template<typename _T>
	int _call(int event, int param1, int param2, CVEventData data, _T*)
	{
		return eventOp(event, param1, param2, data);
	}
	int _call(int event, int param1, int param2, CVEventData data, void*)
	{
		eventOp(event, param1, param2, data);
		return event;
	}
public:
	CVEventHandlerEx(const _EventOpT &op = _EventOpT())
		:eventOp(op)
	{}
	virtual int onEvent(int evt, int param1, int param2, CVEventData data)
	{
		return _call(evt, param1, param2, data, (decltype(eventOp(0, 0, 0, 0))*)0);
	}
};

template<typename _EventOpT>
inline CVEventHandlerPtr makeEventHandler(const _EventOpT &op)
{
	return CVEventHandlerPtr(new CVEventHandlerEx<_EventOpT>(op));
}

class _CVX_API ImageFunctor
{
public:
	virtual void pro(Mat &img);

	virtual ~ImageFunctor() throw();
};

typedef std::shared_ptr<ImageFunctor> ImageFunctorPtr;

template<typename _OpT>
class ImageFunctorT
	:public ImageFunctor
{
	_OpT  op;
public:
	ImageFunctorT(const _OpT &_op = _OpT())
		:op(_op)
	{}
	virtual void pro(Mat &img)
	{
		op(img);
	}
};

template<typename _OpT>
inline ImageFunctorPtr makeImageFunctor(const _OpT &op)
{
	return ImageFunctorPtr(new ImageFunctorT<_OpT>(op));
}

//
//template<typename _MouseOp>
//class CVMouseCallback
//	:public CVEventHandler
//{
//	_MouseOp    mouseOp;
//
//	template<typename _T>
//	int _call(int event, int x, int y, int flags, _T*)
//	{
//		return mouseOp(event, x, y, flags);
//	}
//	int _call(int event, int x, int y, int flags, void*)
//	{
//		mouseOp(event, x, y, flags);
//		return event;
//	}
//public:
//	CVMouseCallback(const _MouseOp &op = _MouseOp())
//		:mouseOp(op)
//	{
//	}
//	virtual int onMouse(int event, int x, int y, int flags)
//	{
//		return _call(event, x, y, flags, (decltype(mouseOp(0,0,0,0))*)0);
//	}
//};
//
//template<typename _KeyboardOp>
//class CVKeyboardCallback
//	:public CVEventHandler
//{
//	_KeyboardOp keyboardOp;
//
//	template<typename _T>
//	int _call(int code, int flags, _T*)
//	{
//		return keyboardOp(code, flags);
//	}
//	int _call(int code, int flags, void*)
//	{
//		keyboardOp(code, flags);
//		return code;
//	}
//public:
//	CVKeyboardCallback(const _KeyboardOp &op = _KeyboardOp())
//		:keyboardOp(op)
//	{
//	}
//	virtual int onKeyboard(int code, int flags)
//	{
//		return _call(code, flags,(decltype(keyboardOp(0,0))*)0 );
//	}
//};
//
//template<typename _TrackbarOp>
//class CVTrackbarCallback
//	:public CVEventHandler
//{
//	_TrackbarOp trackbarOp;
//public:
//	CVTrackbarCallback(const _TrackbarOp &op = _TrackbarOp())
//		:trackbarOp(op)
//	{}
//	virtual void onTrackbar(int pos)
//	{
//		trackbarOp(pos);
//	}
//};

class _CVX_API CVTrackbar
{
public:
	virtual void setEventHandler(CVEventHandlerPtr callBack, const std::string &name="") =0;

	virtual void sendEvent(int evt, int param1, int param2, CVEventData data) = 0;

	virtual int  getPos() = 0;

	virtual void setPos(int pos) = 0;

	virtual void setMax(int maxPos) = 0;

	virtual ~CVTrackbar() throw();
public:
	template<typename _OpT>
	void setEventHandler(const _OpT &op, const std::string &name="")
	{
		this->setEventHandler(makeEventHandler(op), name);
	}
};

#define CVGUI_DEFAULT_DUMP_FILE _TX("_guimat.mdat")

_CVX_API bool saveMatData(const string_t &file, const cv::Mat &mat);

_CVX_API cv::Mat loadMatData(const string_t &file);

_CVX_API void saveAndCopy(ImageSetPtr is, int gifDelayTime);

class _CVX_API CVWindow
{
protected:
	ImageSetPtr		imgSet;
	int				isID = -1;
	std::string		isName;
	size_t			framePos;
	ImageFunctorPtr		functor;

public:
	virtual void setWindowTitle(const std::string &title) = 0;

	virtual void setUserText(const std::string &text) = 0;

	virtual void show(ImageSetPtr is, int initPos=0, int isID=-1, const std::string &isName="") = 0;

	virtual void showAt(size_t pos) = 0;

	virtual void showNext() = 0;

	virtual void showPrev() = 0;

	ImageSetPtr getImageSet(){	return imgSet;	}

	int         getImageSetID() { return isID; }

	const std::string& getImageSetName() { return isName; }

	int         getFramePos() { return (int)framePos;}

	void  setFunctor(ImageFunctorPtr _functor, bool update = true)
	{
		functor = _functor;
		if (update)
			this->showAt(framePos);
	}

	virtual const Mat* getCurrentImage() = 0;

	virtual void setEventHandler(CVEventHandlerPtr callBack, const std::string &name="") = 0;

	virtual void sendEvent(int evt, int param1, int param2, CVEventData data) = 0;

	virtual CVTrackbar* createTrackbar(const std::string &trackbarName, int maxPos) = 0;

	virtual CVTrackbar* getTrackbar(const std::string &trackbarName) = 0;

	virtual ~CVWindow() throw();
public:
	int gifDelayTime = 50;
	bool numKeySwitch = true;
	bool echoPixel = true;
	bool showRichTitle = true;
public:
	template<typename _OpT>
	void setEventHandler(const _OpT &op, const std::string &name="")
	{
		this->setEventHandler(makeEventHandler(op), name);
	}
	
	void setNavigator(int keyPrev='d', int keyNext='f', int keySave='p', int mouseNext=EVENT_RBUTTONUP, bool numKeySwitch=true);

	void show(const Mat imgs[], int count, int initPos=0, const std::string title[]=NULL, int tcount=0)
	{
		auto is = ISImages::createEx(imgs, count);
		if (title)
			is->setTitle(title, tcount);
		this->show(is, initPos);
	}
	void show(const std::vector<Mat> &imgs, int initPos = 0, const std::vector<std::string> &title = std::vector<std::string>())
	{
		this->show(&imgs[0], (int)imgs.size(), initPos, title.empty() ? NULL : &title[0], (int)title.size());
	}
	void show(const Mat &img, const std::string *title = NULL)
	{
		this->show(&img, 1, 0, title, 1);
	}
};

_CVX_API void clearWindows();

_CVX_API CVWindow* getWindow(const std::string &wndName, bool createIfNotExist=false, bool partialMatching=false);

//set functor for all windows
_CVX_API void setWindowFunctor(ImageFunctorPtr functor, bool update=true);

template<typename _OpT>
inline void setEventHandler(const std::string &wndName, const _OpT &op)
{
	if (CVWindow *wnd = getWindow(wndName))
		wnd->setEventHandler(op);
}

template<typename _OpT>
inline CVTrackbar* createTrackbar(const std::string &trackbarName, const std::string &wndName, int maxPos, const _OpT &op)
{
	CVTrackbar *tb = NULL;
	if (CVWindow *wnd = getWindow(wndName))
	{
		if (tb = wnd->createTrackbar(trackbarName, maxPos))
			tb->setEventHandler(op);
	}
	return tb;
}

template<typename _OpT>
inline void setTrackbarEventHandler(const std::string &trackbarName, const std::string &wndName, const _OpT &op)
{
	if (CVWindow *wnd = getWindow(wndName))
	{
		if (CVTrackbar *tb = wnd->getTrackbar(trackbarName))
			tb->setEventHandler(op);
	}
}

//show an image set (a list of images)
_CVX_API CVWindow* imshowx(const std::string &wndName, ImageSetPtr is, int gifDelayTime = 50);

_CVX_API CVWindow* imshowx(const std::string &wndName, const Mat imgs[], int count, int gifDelayTime = 50, const std::string title[] = NULL, int tcount = 0);


inline CVWindow* imshowx(const std::string &wndName, const std::vector<Mat> &imgs, int gifDelayTime = 50, const std::vector<std::string> &title = std::vector<std::string>())
{
	return imshowx(wndName, &imgs[0], (int)imgs.size(), gifDelayTime, title.empty() ? NULL : &title[0]);
}

inline CVWindow* imshowx(const std::string &wndName, const Mat &img, const std::string *title=NULL)
{
	return imshowx(wndName, &img, 1, 0, title, 0);
}

//show multiple image sets
_CVX_API CVWindow* imshowx(const std::string &wndName, ImageSetPtr is[], int count, const std::string isName[] = NULL, int keyPrev = '-', int keyNext = '=', int keySave='p', bool numKeySwitch=true, int mouseNext = EVENT_RBUTTONUP);


_CVX_END

