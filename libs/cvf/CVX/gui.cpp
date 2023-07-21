#include"gui.h"
#include<list>
#include<sstream>
#include"opencv2/imgcodecs.hpp"
#include"opencv2/highgui/highgui_c.h"

#include"BFC/portable.h"
#include"CVX/codec.h"
#include"BFC/stdf.h"
#include"BFC/autores.h"
#include"BFC/err.h"

#ifdef _MSC_VER
#include<Windows.h>

typedef  LRESULT(CALLBACK *WndProcT)(
	_In_ HWND   hwnd,
	_In_ UINT   uMsg,
	_In_ WPARAM wParam,
	_In_ LPARAM lParam
	);

#endif

#define _CMD_DIR_NAME _TX("__cvxcmd\\")
#define _WND_LEADING_CHAR '.'




#if 0 //def _MSC_VER
#include<Windows.h>

struct WndFindData
{
	std::string title;
	std::vector<HWND>  vwnd;
};

BOOL CALLBACK cmdEnumWindowsProc(_In_ HWND   hwnd, _In_ LPARAM lParam)
{
	char text[256];
	int n = ::GetWindowTextA(hwnd, text, sizeof(text)/sizeof(text[0]));
	text[n] = 0;
	
	if (text[0] == _WND_LEADING_CHAR) //mark cvx windows
	{
		WndFindData *fd = (WndFindData*)lParam;
		if (fd && n >= (int)fd->title.size() && 
			//strnicmp(text, fd->title.c_str(), fd->title.size()) == 0
			strstr(text, fd->title.c_str())!=NULL
			)
		{
			fd->vwnd.push_back(hwnd);
		}
	}
	return TRUE;
}

std::string cmdGetWindowName(HWND hwnd)
{
	char text[256];
	int n = ::GetWindowTextA(hwnd, text, sizeof(text) / sizeof(text[0]));
	text[n] = 0;

	char *px=text,*end = std::find(text, text + n, '<');
	if (*px == _WND_LEADING_CHAR)
		++px;
	return std::string(px, end);
}

int _sendAppCommand(const std::string &_wndTitle, const std::string &cmd, bool waitFinished)
{
	HWND hwnd = NULL;
	{
		WndFindData 	fd;
		fd.title = _wndTitle;
		HDESK hdesk = GetThreadDesktop(GetCurrentThreadId());
		EnumWindows(cmdEnumWindowsProc, (LPARAM)&fd);
		if (fd.vwnd.size() != 1)
			return -1;
		hwnd = fd.vwnd.front();
	}
	std::string wndTitle = cmdGetWindowName(hwnd);

	string_t wdir(ff::CatDirectory(ff::getTempPath(), _CMD_DIR_NAME));
	if (!ff::pathExist(wdir))
		CreateDirectory(wdir.c_str(), NULL);

	string_t lockFile(wdir + _TX("lock._")), cmdFile(wdir + _TX("cmd._")), stateFile(wdir + _TX("state._"));
	
	FILE *flock = NULL;
	while (!(flock = _tfopen(lockFile.c_str(), _TX("wb"))))
		Sleep(50);
	ff::AutoFilePtr _dflock(flock);

	ff::DeleteFileEx(stateFile);
	if (ff::pathExist(stateFile))
		return -2;

	{
		FILE *fcmd = _tfopen(cmdFile.c_str(), _TX("w"));
		if (!fcmd)
			return -3;
		fprintf(fcmd, "%s\n%s\n", wndTitle.c_str(), cmd.c_str());
		fclose(fcmd);
	}
	::PostMessage(hwnd, WM_KEYDOWN, VK_F11, 0x001E0001);
	::PostMessage(hwnd, WM_KEYUP, VK_F11, 0x001E0001);

	int state = 0;
	if (waitFinished)
	{
		FILE *fstate = NULL;
		while (!(fstate = _tfopen(stateFile.c_str(), _TX("rb"))))
			Sleep(50);
		if (fread(&state, sizeof(state), 1, fstate) != 1)
			state = -1;
		fclose(fstate);
	}
	return state;
}

#else

int _sendAppCommand(const std::string &_wndTitle, const std::string &cmd, bool waitFinished)
{
	printf("sendAppCommand supported only on windows.\n");
	return -1;
}
#endif

int _sendWndCommand()
{
	string_t wdir(ff::CatDirectory(ff::getTempPath(), _CMD_DIR_NAME));

	string_t cmdFile(wdir + _TX("cmd._")), stateFile(wdir + _TX("state._"));
	char name1[256], name[256], cmd[2048];
	FILE *fcmd = _tfopen(cmdFile.c_str(), _TX("r"));
	if (!fcmd)
		return -1;
	ff::AutoFilePtr _dfcmd(fcmd);
	if (fgets(name1, sizeof(name1), fcmd) && fgets(cmd, sizeof(cmd), fcmd))
	{
		sscanf(name1, "%s", name);
		cv::CVWindow *wnd = cv::getWindow(name, false, true);
		if (wnd)
		{
			string_t cmdt(ff::X2TS(cmd));
			wnd->sendEvent(cv::EVENT_APP_COMMAND, 0, 0, (void*)cmdt.c_str());

			FILE *fstate = _tfopen(stateFile.c_str(), _TX("wb"));
			if (fstate)
			{
				int state = 1;
				fwrite(&state, sizeof(state), 1, fstate);
				fclose(fstate);
			}
		}
	}
	return 0;
}

_CVX_BEG

int sendAppCommand(const std::string &wndTitle, const std::string &cmd, bool waitFinished)
{
	CVWindow *wnd = getWindow(wndTitle, false, true);
	if (wnd) //if find window in the app itself
	{
		string_t cmdt(ff::X2TS(cmd));
		wnd->sendEvent(EVENT_APP_COMMAND, 0, 0, (void*)cmdt.c_str());
		return 0;
	}
	//send to other apps
	return _sendAppCommand(wndTitle, cmd, waitFinished);
}

void saveAndCopy(const std::vector<Mat> &imgs, int gifDelayTime)
{
	string_t path = ff::getTempPath();

	if (imgs.size() == 1)
	{
		path += _TX("\\imshow.png");
		cv::imwrite(ff::X2MBS(path), imgs[0]);
	}
	else if (imgs.size() > 1)
	{
		path += _TX("\\imshow.gif");
		cv::writeGIF(ff::X2MBS(path), imgs, gifDelayTime);
	}
	ff::copyFilesToClipboard(&path, 1);
}

void saveAndCopy(ImageSetPtr is, int gifDelayTime)
{
	std::vector<Mat> imgs;
	for (int i = 0; i < is->nFrames(); ++i)
	{
		Mat img;
		if (!is->read(img, i))
			break;
		imgs.push_back(img);
	}

	saveAndCopy(imgs, gifDelayTime);
}

typedef int(*KeyboardCallBack)(int code, int flags, void *userData);

struct WndKBData
{
	std::string name;
	void       *handle;
	KeyboardCallBack kbCallBack;
	void  *userData;
};

static std::list<WndKBData>  gWndKBList;
static std::string WND_NAME_ALL = "";
static void* activeWndHandle = NULL;

template<typename _DataT>
typename std::list<_DataT>::iterator _findWnd(const std::string &wndName, std::list<_DataT> &wndList, bool create = false)
{
	auto itr = wndList.begin();
	for (; itr != wndList.end(); ++itr)
		if (stricmp(itr->name.c_str(),wndName.c_str())==0)
			break;
	if (create && itr == wndList.end())
	{
		wndList.push_back(_DataT());
		--itr;
		itr->name = wndName;
	//	cv::namedWindow(wndName);
	}
	return itr;
}

static void getFocusMouseCB(int event, int x, int y, int flag, void * data)
{
	activeWndHandle = data;
}

static void* _getCVWindowHandle(const char* name)
{
//#if OPENCV_VER<400||OPENCV_VER>1000&&OPENCV_VER<4000
	return cvGetWindowHandle(name);
//#else
//	CV_Assert(false);
//	return nullptr;
//#endif
}

void setKeyboardCallback(const std::string &wndName, KeyboardCallBack callBack, void *userData)
{
	auto itr = _findWnd(wndName, gWndKBList, true);
	
	itr->kbCallBack = callBack;
	itr->userData = userData;
	itr->handle = _getCVWindowHandle(wndName.c_str());

	cv::setMouseCallback(wndName, getFocusMouseCB, itr->handle);
}

int cvxWaitKey(int exitCode)
{
	int c = 0;
	while (true)
	{
		c = waitKey(0);

		if (c < 0)
			break; //there is no window

		if ((c & 0xFFFF) != 0)
			c &= 0xFFFF;

		if (c == CVKEY_F11)
		{
			_sendWndCommand();
		}

		for (auto itr = gWndKBList.begin(); itr != gWndKBList.end(); ++itr)
		{
			//if ((itr->handle == hwnd || ff::getParentWindow(itr->handle)==hwnd)
			if (itr->handle==activeWndHandle
				&& itr->kbCallBack)
			{
				c=itr->kbCallBack(c, 0, itr->userData);
				break;
			}
		}

		if (c != 0)
		{
			auto itr = _findWnd(WND_NAME_ALL, gWndKBList);
			if (itr != gWndKBList.end() && itr->kbCallBack)
				c = itr->kbCallBack(c, 0, itr->userData);
		}

		if (c != 0 && (exitCode==c || exitCode<0) )
			break;
	}
	return c;
}

#define _MAT_DATA_MAGIC 0xABCD1234

_CVX_API bool saveMatData(const string_t &file, const cv::Mat &mat)
{
	bool ok = false;
	FILE *fp = _tfopen(file.c_str(), _TX("wb"));
	if (fp)
	{
		cv::Mat matd = mat.clone();

		int head[] = { (int)_MAT_DATA_MAGIC, matd.cols, matd.rows, matd.type(), (int)matd.step};
		if (fwrite(head, sizeof(head), 1, fp) == 1 && fwrite(matd.data, matd.step*matd.rows, 1, fp) == 1)
			ok = true;
		fclose(fp);
	}
	return ok;
}

_CVX_API cv::Mat loadMatData(const string_t &file)
{
	cv::Mat mat;
	FILE *fp = _tfopen(file.c_str(), _TX("rb"));
	if (fp)
	{
		int head[5];
		if (fread(head, sizeof(head), 1, fp) == 1 && head[0]==_MAT_DATA_MAGIC)
		{
			int rows = head[2], cols = head[1], type = head[3], step = head[4];
			std::unique_ptr<uchar[]> dbuf(new uchar[step*rows]);
			if (fread(dbuf.get(), step*rows, 1, fp) == 1)
			{
				mat = cv::Mat(rows, cols, type, dbuf.get(), step);
				mat = mat.clone();
			}
		}
		fclose(fp);
	}
	return mat;
}

//int CVEventHandler::onKeyboard(int code, int flags)
//{
//	return code;
//}
//int CVEventHandler::onMouse(int event, int x, int y, int flags)
//{
//	return event;
//}
//void CVEventHandler::onTrackbar(int pos)
//{
//}
int CVEventHandler::onEvent(int event, int param1, int param2, CVEventData data)
{
	return event;
}

CVEventHandler::~CVEventHandler() throw()
{}

void ImageFunctor::pro(Mat &img)
{
}
ImageFunctor::~ImageFunctor() throw()
{}


CVWindow::~CVWindow() throw()
{}

CVTrackbar::~CVTrackbar() throw()
{}

#ifdef _WIN32

struct _CVXWindowData
{
	void *handle;
	CVWindow *cvw;
};

template<typename _DataT>
typename std::list<_DataT>::iterator _findWnd(void *handle, std::list<_DataT> &wndList, bool create = false)
{
	auto itr = wndList.begin();
	for (; itr != wndList.end(); ++itr)
		if (handle == itr->handle)
			break;
	if (create && itr == wndList.end())
	{
		wndList.push_back(_DataT());
		--itr;
		itr->handle = handle;
	}
	return itr;
}

static std::list<_CVXWindowData> g_wndData;

int dragQueryFiles(HDROP hdrop, std::vector<std::string> &vfiles)
{
	CHAR buf[MAX_PATH * 2];

	UINT count = ::DragQueryFileA(hdrop, 0xFFFFFFFF, NULL, 0);

	for (UINT i = 0; i<count; ++i)
	{
		UINT nc = ::DragQueryFileA(hdrop, i, buf, MAX_PATH * 2);
		buf[nc] = '\0';

		vfiles.push_back(buf);
	}

	return count;
}

static WndProcT  g_cvWndProc = NULL;

LRESULT CALLBACK cvxWindowProc(
	_In_ HWND   hwnd,
	_In_ UINT   uMsg,
	_In_ WPARAM wParam,
	_In_ LPARAM lParam
)
{
	if (uMsg == WM_DROPFILES)
	{
		HDROP hdrop = (HDROP)wParam;
		std::vector<std::string> vfiles;
		dragQueryFiles(hdrop, vfiles);
		DragFinish(hdrop);

		if (!vfiles.empty())
		{
			auto itr = _findWnd(hwnd, g_wndData);
			if (itr != g_wndData.end())
				itr->cvw->sendEvent(EVENT_DRAGDROP, 0, 0, CVEventData(&vfiles));
		}

	}
	return g_cvWndProc(hwnd, uMsg, wParam, lParam);
}

static void _setWindowProc(void *handle, CVWindow *cvw)
{
	HWND hwnd = (HWND)handle;
	LONG_PTR p=::GetWindowLongPtr(hwnd, GWLP_WNDPROC);
	auto itr = _findWnd(handle, g_wndData, true);
	itr->cvw = cvw;
	auto cvWndProc = reinterpret_cast<WndProcT>(p);

	if (g_cvWndProc && cvWndProc != g_cvWndProc)
		FF_EXCEPTION(ERR_GENERIC, "invalid window proc");

	g_cvWndProc = cvWndProc;

	::SetWindowLongPtr(hwnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(cvxWindowProc));
	::DragAcceptFiles(hwnd, TRUE);
}
#endif



struct CVWindowImpl
	:public CVWindow
{
	struct EventInfo
	{
		std::string			name;
		CVEventHandlerPtr	ptr;
	public:
		EventInfo()
		{
		}
		EventInfo(const std::string &_name, CVEventHandlerPtr _ptr)
			:name(_name), ptr(_ptr)
		{}
	};

	static std::vector<EventInfo>::iterator _findEventHandler(std::vector<EventInfo> &vec, const std::string &name)
	{
		if (name.empty())
			return vec.end();
		else
		{
			auto itr = vec.begin();
			for (; itr != vec.end(); ++itr)
				if (itr->name == name)
					break;
			return itr;
		}
	}
	static void _setEventHandler(std::vector<EventInfo> &vec, CVEventHandlerPtr callBack, const std::string &name)
	{
		auto itr = _findEventHandler(vec, name);
		if (itr==vec.end())
			vec.push_back(EventInfo(name, callBack));
		else
			itr->ptr = callBack;
	}

	struct Trackbar
		:public CVTrackbar
	{
		CVWindowImpl   *wnd = NULL;
		std::string name;
		std::vector<EventInfo> tbOps;
	public:
		virtual void setEventHandler(CVEventHandlerPtr callBack, const std::string &name)
		{
			_setEventHandler(tbOps, callBack, name);
		}
		virtual void sendEvent(int evt, int param1, int param2, CVEventData data)
		{
			if (!data.ptr)
				data.ptr = &name;

			for (auto itr = tbOps.rbegin(); itr != tbOps.rend(); ++itr)
			{
				CVEventHandlerPtr p = itr->ptr;
				if (p && EVENT_NULL == p->onEvent(evt, param1, param2, data))
					return;
			}
			//send to window handler
			if (evt != EVENT_NULL && wnd)
				wnd->sendEvent(evt, param1, param2, data);

		}
		virtual int  getPos()
		{
			return getTrackbarPos(name, wnd->name);
		}
		virtual void setPos(int pos)
		{
			setTrackbarPos(name, wnd->name, pos);
		}
		virtual void setMax(int maxPos)
		{
			setTrackbarMax(name, wnd->name, maxPos);
		}
	};

public:
	std::string name, userText;
	

	std::vector<EventInfo> eventHandler;
	std::list<Trackbar>  tbList;

	
	Mat                       curImg;
	bool                      bSetCallback = true;

private:
	std::list<Trackbar>::iterator _findTrackbar(const std::string &name, bool create = false)
	{
		auto itr = tbList.begin();
		for (; itr != tbList.end(); ++itr)
		{
			if (itr->name == name)
				break;
		}
		if (itr == tbList.end() && create)
		{
			tbList.push_back(Trackbar());
			--itr;

			itr->name = name;
			itr->wnd = this;
		}
		return itr;
	}
	void _sendEvent(int evt, int param1, int param2, CVEventData data)
	{
		//reverse iteration : let callback added later have higher priority
		for (auto itr = eventHandler.rbegin(); itr != eventHandler.rend(); ++itr)
		{
			CVEventHandlerPtr p = itr->ptr;
			if (p && EVENT_NULL == p->onEvent(evt, param1, param2, data))
				break;
		}
	}
public:
	~CVWindowImpl()
	{
		this->clearCallBack();
	}
	void updateTitle()
	{
		if (!this->showRichTitle)
			return;

		std::string frameName = ff::X2MBS(imgSet->frameName((int)framePos));
		char pos[1024], buf[1024];
		std::string head(_WND_LEADING_CHAR + name + (!isName.empty() ? ":" + isName : ""));

		pos[0] = 0;
		if (imgSet->nFrames()>1)
			sprintf(pos, "<%d/%d><%s>", (int)framePos + 1, imgSet->nFrames(), frameName.c_str());

		sprintf(buf, "%s%s<%dx%dx%d>%s", head.c_str(),pos, curImg.cols, curImg.rows, curImg.channels(), userText.empty() ? "" : ("<"+userText+">").c_str());

		this->setWindowTitle(buf);
	}
	virtual void setUserText(const std::string &text)
	{
		userText = text;
		this->updateTitle();
	}
	virtual void show(ImageSetPtr is, int initPos, int isID, const std::string &isName)
	{
		int old = this->isID;

		imgSet = is;
		this->isID = isID;
		//isID<0 means this is the unique IS, so isName should be empty() and will not be shown.
		this->isName = isName.empty()&&isID>=0? ff::StrFormat("%03d",isID).c_str() : isName;
		this->showAt(initPos);

		this->sendEvent(EVENT_IMAGESET_CHANGED, isID, old, this);
	}

	virtual void setWindowTitle(const std::string &title)
	{
		cv::setWindowTitle(name, title);
	}

	/*virtual void setFrameTitle(const std::string title[], int count)
	{
		imgTitle.clear();
		imgTitle.assign(title, title + count);
		this->showAt(framePos);
	}*/

	static int cvxKeyboardCallback(int code, int flags, void *data)
	{
		if (CVWindowImpl *d = (CVWindowImpl*)data)
		{
			d->_sendEvent(EVENT_KEYBOARD, code, flags, d);
		}
		return code;
	}

	static void cvxMouseCallback(int event, int x, int y, int flags, void *data)
	{
		if (CVWindowImpl *d = (CVWindowImpl*)data)
		{
			//set acitve window
			activeWndHandle = _getCVWindowHandle(d->name.c_str());

			d->_sendEvent(event, x, y, CVEventData(flags));
		}
	}
	void setCallBack()
	{
		if (bSetCallback)
		{
			cv::setKeyboardCallback(name, cvxKeyboardCallback, this);
			cv::setMouseCallback(name, cvxMouseCallback, this);
#ifdef _WIN32
			void *handle = _getCVWindowHandle(name.c_str());
			_setWindowProc(handle, this);
#endif

			bSetCallback = false;
		}
	}
	void clearCallBack()
	{
		cv::setKeyboardCallback(name, NULL, NULL);
		cv::setMouseCallback(name, NULL, NULL);
	}

	virtual void showAt(size_t pos)
	{
		if (pos < (size_t)imgSet->nFrames())
		{
			if (imgSet->read(curImg, pos))
			{
				int old = (int)framePos;

				if (!functor)
					imshow(name, curImg);
				else
				{
					Mat dimg = curImg.clone();
					functor->pro(dimg);
					imshow(name, dimg);
				}
				framePos = pos;

				this->updateTitle();

				if (bSetCallback)
					this->setCallBack();

				this->sendEvent(EVENT_FRAME_CHANGED, (int)framePos, old, this);
			}
		}
	}

	virtual void showNext()
	{
		size_t newPos = (framePos + 1) % imgSet->nFrames();
		this->showAt(newPos);
	}

	virtual void showPrev()
	{
		size_t newPos = framePos == 0 ? imgSet->nFrames() - 1 : (framePos - 1) % imgSet->nFrames();

		this->showAt(newPos);
	}

	virtual const Mat* getCurrentImage()
	{
		return framePos < (size_t)imgSet->nFrames()? &curImg : NULL;
	}

	virtual void setEventHandler(CVEventHandlerPtr callBack, const std::string &name)
	{
		_setEventHandler(eventHandler, callBack, name);
	}
	virtual void sendEvent(int evt, int param1, int param2, CVEventData data)
	{
		_sendEvent(evt, param1, param2, data);
	}

	
	static void cvxTrackbarCallback(int pos, void *data)
	{
		if (Trackbar *d = (Trackbar*)data)
		{
			d->sendEvent(EVENT_TRACKBAR_POS, pos, 0, d);
		}
	}

	virtual CVTrackbar* createTrackbar(const std::string &trackbarName, int maxPos)
	{
		auto itr = _findTrackbar(trackbarName, true);
		cv::createTrackbar(trackbarName, this->name, NULL, maxPos, cvxTrackbarCallback, &*itr);
		return &*itr;
	}

	virtual CVTrackbar* getTrackbar(const std::string &trackbarName)
	{
		auto itr = _findTrackbar(trackbarName, false);
		return itr == tbList.end() ? NULL : &*itr;
	}
};

//template<typename _PrintT, typename _T>
//void _imshowPrintT(int x, int y, const _T *p, int cn, const char *fmt)
//{
//	printf("\r(%d,%d)=(", x, y);
//	for (int i = 0; i < cn - 1; ++i)
//	{
//		printf(fmt, _PrintT(p[i]));
//		printf(",");
//	}
//	printf(fmt, _PrintT(p[cn - 1]));
//	printf(")\t\t");
//}
//void _imshowPrint(int x, int y, const void *p, int depth, int cn)
//{
//	if (depth == CV_8U)
//		_imshowPrintT<int>(x, y, (uchar*)p, cn, "%d");
//	else if (depth == CV_8S)
//		_imshowPrintT<int>(x, y, (char*)p, cn, "%d");
//	if (depth == CV_16S)
//		_imshowPrintT<int>(x, y, (short*)p, cn, "%d");
//	if (depth == CV_16U)
//		_imshowPrintT<int>(x, y, (ushort*)p, cn, "%d");
//	else if (depth == CV_32S)
//		_imshowPrintT<int>(x, y, (int*)p, cn, "%d");
//	else if (depth == CV_32F)
//		_imshowPrintT<float>(x, y, (float*)p, cn, "%f");
//	else if (depth == CV_64F)
//		_imshowPrintT<double>(x, y, (double*)p, cn, "%lf");
//}

template<typename _PrintT, typename _T>
std::string _imshowPrintT(int x, int y, const _T *p, int cn)
{
	std::ostringstream os;
	os << x << ',' << y << ">=<";
	for (int i = 0; i < cn - 1; ++i)
		os << _PrintT(p[i]) << ',';
	os << _PrintT(p[cn - 1]);
	return os.str();
}
std::string _imshowStrPrint(int x, int y, const void *p, int depth, int cn)
{
	std::string str;
	if (depth == CV_8U)
		str=_imshowPrintT<int>(x, y, (uchar*)p, cn);
	else if (depth == CV_8S)
		str=_imshowPrintT<int>(x, y, (char*)p, cn);
	if (depth == CV_16S)
		str=_imshowPrintT<int>(x, y, (short*)p, cn);
	if (depth == CV_16U)
		str = _imshowPrintT<int>(x, y, (ushort*)p, cn);
	else if (depth == CV_32S)
		str = _imshowPrintT<int>(x, y, (int*)p, cn);
	else if (depth == CV_32F)
		str = _imshowPrintT<float>(x, y, (float*)p, cn);
	else if (depth == CV_64F)
		str = _imshowPrintT<double>(x, y, (double*)p, cn);
	return str;
}


void _proCVXCommand(CVWindow *wnd, const char_t *cmdstr)
{
	ff::CommandArgSet arg;
	arg.setArgs(cmdstr);

	string_t cmd(arg.get<string_t>(_TX("#0")));
	if (cmd == _TX("dump"))
	{
		const cv::Mat *img = wnd->getCurrentImage();
		if (img)
		{
			string_t file(arg.get<string_t>(_TX("#1")));
			saveMatData(file, *img);
		}
	}
}

void CVWindow::setNavigator(int keyPrev, int keyNext, int keySave, int mouseNext, bool numKeySwitch)
{
	this->numKeySwitch = numKeySwitch;

	auto *wnd = this;
	this->setEventHandler([wnd, mouseNext, keyPrev, keyNext, keySave](int evt, int param1, int param2, CVEventData data){
		if (evt == mouseNext)
			wnd->showNext();

		if (evt == EVENT_MOUSEMOVE)
		{
			const int x = param1, y = param2;
			const long flags = (long)data.ival;
			const Mat *cur=NULL;
			if (wnd->echoPixel && (cur = wnd->getCurrentImage()) )
			{
				if (uint(x) < uint(cur->cols) && uint(y) < uint(cur->rows))
				{
					std::string str=_imshowStrPrint(x, y, cur->ptr(y, x), cur->depth(), cur->channels());
					wnd->setUserText(str);
				}
			}
			if (flags&EVENT_FLAG_CTRLKEY)
			{
				setWindowFunctor(makeImageFunctor([x, y](Mat &img){
					drawCross(img, Point(x, y), 5, img.channels()==1? Scalar(230) : Scalar(0, 255, 255));
				}));
			}
		}

		if (evt == EVENT_KEYBOARD)
		{
			int code = tolower(param1);

			if (wnd->numKeySwitch && code >= '1'&&code <= '9')
				wnd->showAt(code - '1');

			if (code == tolower(keyNext))
				wnd->showNext();
			else if (code == tolower(keyPrev))
				wnd->showPrev();
			else if (code == tolower(keySave))
			{
				if(false) //save the whole seqence as GIF
					saveAndCopy(wnd->getImageSet(), wnd->gifDelayTime);
				else 
				{//save only the current as PNG
					std::vector<Mat> vimg;
					if (auto* pcur = wnd->getImageSet()->read())
					{
						vimg.push_back(*pcur);
						saveAndCopy(vimg, 0);
					}
				}
			}
		}

		if (evt == EVENT_APP_COMMAND)
		{
			_proCVXCommand(wnd, (char*)data.ptr);
		}
	},"navigator");

}

std::list<CVWindowImpl>  &gWndList(*new std::list<CVWindowImpl>); //use new to alloc: to prevent the "call pure virtual function" error on Linux when program exit

_CVX_API CVWindow* getWindow(const std::string &wndName, bool createIfNotExist, bool partialMatching)
{
	auto itr = _findWnd(wndName, gWndList, createIfNotExist);

	//std::list<CVWindowImpl> *plist = &gWndList;

	//if (itr == gWndList.end() && partialMatching)
	//{
	//	std::vector<decltype(itr)> vr;
	//	for (itr = gWndList.begin(); itr != gWndList.end(); ++itr)
	//	{
	//		if (itr->name.size() >= wndName.size() && strnicmp(itr->name.c_str(), wndName.c_str(), wndName.size()) == 0)
	//			vr.push_back(itr);
	//	}
	//	if (vr.size() == 1) //if find unique
	//		itr = vr.front();
	//}

	return itr == gWndList.end() ? NULL : &*itr;
}

_CVX_API void setWindowFunctor(ImageFunctorPtr functor, bool update)
{
	for (auto &wnd : gWndList)
	{
		wnd.setFunctor(functor, update);
	}
}

_CVX_API CVWindow* imshowx(const std::string &wndName, ImageSetPtr is, int gifDelayTime)
{
	CVWindow *wnd = NULL;
	if (is)
	{
		auto itr = _findWnd(wndName, gWndList, true);
		if (itr != gWndList.end())
		{
			wnd = &*itr;
			wnd->show(is, 0);
			wnd->setNavigator();

			wnd->gifDelayTime = gifDelayTime;
		}
	}
	return wnd;
}

_CVX_API CVWindow* imshowx(const std::string &wndName, const Mat imgs[], int count, int gifDelayTime, const std::string title[], int tcount)
{
	auto is = ISImages::createEx(imgs, count);
	if (title)
		is->setTitle(title, tcount);

	return imshowx(wndName, is, gifDelayTime);
}

CVWindow* imshowx(const std::string &wndName, ImageSetPtr is[], int count, const std::string isName[], int keyPrev, int keyNext, int keySave, bool numKeySwitch, int mouseNext)
{
	CVWindow* wnd = imshowx(wndName, is[0]);

	std::vector<ImageSetPtr> vis(is, is + count);
	std::vector<std::string> vlabels;
	if (isName)
		vlabels.assign(isName, isName + count);
	else
	{//use ID as label
		for (int i = 0; i < count; ++i)
			vlabels.push_back(ff::StrFormat("%03d", i));
	}

	int cid = 0;
	wnd->setEventHandler([wnd, vis, vlabels, count, cid, keyPrev, keyNext, keySave, numKeySwitch, mouseNext](int evt, int code, int, CVEventData) mutable{
		int ret = evt;

		if (evt == EVENT_KEYBOARD)
		{
			int nid = cid;
			if (numKeySwitch && code >= '1' && code <= '9')
			{
				int id = code - '1';
				if (id < count)
					nid = id;
			}
			if (code == keyPrev)
				nid = cid == 0 ? count - 1 : cid - 1;

			if (code == keyNext)
				nid = (cid + 1) % count;

			if (nid != cid)
			{
				cid = nid;
				wnd->show(vis[cid], wnd->getFramePos(), cid, vlabels[cid]);
				ret = EVENT_NULL; //return 0 to prevent further processing of keys
			}
			if (code == keySave)
			{
				std::vector<Mat> vimgs;
				for (int i = 0; i < count; ++i)
				{
					Mat im;
					if (vis[i]->read(im, wnd->getFramePos()))
						vimgs.push_back(im);
				}
				saveAndCopy(vimgs, wnd->gifDelayTime);
				ret = EVENT_NULL;
			}
		}
		else if (evt == mouseNext)
		{
			cid = (cid + 1) % count;
			wnd->show(vis[cid], wnd->getFramePos(), cid, vlabels[cid]);
			ret = EVENT_NULL;
		}

		return ret;
	}, "imshowxHandler");

	return wnd;
}


_CVX_END

