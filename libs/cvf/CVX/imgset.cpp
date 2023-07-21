
#include"imgset.h"
#include"BFC/stdf.h"
#include"BFC/autores.h"
#include"BFC/err.h"
#include"BFC/auto_tchar.h"

#include<stdarg.h>
#include<algorithm>
#include<string.h>
#include"opencv2/highgui/highgui.hpp"
#include"opencv2/videoio.hpp"

#include"BFC/portable.h"

static bool _contain_sub_str(const string_t &str, const char_t *kw[], int nkw)
{
	for (int i = 0; i<nkw; ++i)
	{
		//	std::string::const_iterator  itr(std::find_first_of(str.begin(),str.end(),kw[i],kw[i]+strlen(kw[i])));

		if (std::search(str.begin(), str.end(), kw[i], kw[i] + _tcslen(kw[i])) != str.end())
			return true;
	}
	return false;
}

static bool _ContainSubStr(const string_t &str, const char_t *keyWords[], int nkw)
{
	return _contain_sub_str(str, keyWords, nkw);
}

_CVX_BEG

ImageSet::ImageSet()
{
}

bool ImageSet::read(cv::Mat &img, int pos)
{
	if (pos>=0 && !this->setPos(pos))
		return false;

	const cv::Mat *ptr = this->read();
	if (ptr)
	{ 
		ptr->copyTo(img);
		return true;
	}
	return false;
}

cv::Mat ImageSet::read(int pos)
{
	cv::Mat img;
	if (!this->read(img, pos))
		FF_EXCEPTION(ERR_OP_FAILED, "");
	return img;
}

ImageSet::~ImageSet()
{
}

bool ImageSet::create(const string_t &config, ff::IArgSet *arg)
{
	return true;
}

bool ImageSet::seekBeg()
{
	return this->nFrames()>0 ? this->setPos(0) : false;
}

string_t ImageSet::frameName(int pos)
{
	char_t buf[8];
	_stprintf(buf, _TX("%03d"), pos);
	return buf;
}

//==================================================================================


ISVideoFile::~ISVideoFile()
{
	delete m_pCurImg;
	delete m_pCap;
}

bool ISVideoFile::create(const string_t &config, ff::IArgSet *arg)
{
	return this->create(arg->get<string_t>(_T("#0")), arg->get<int>(_T("nc")));
}

bool  ISVideoFile::create(const string_t &file, int channels)
{
	bool ec = false;

	if (!m_pCap)
		m_pCap = new VideoCapture;
	if (!m_pCurImg)
		m_pCurImg = new Mat;

	if (m_pCap->open(ff::X2MBS(file)) && m_pCap->grab() )
	{
		m_size = Size((int)m_pCap->get(CAP_PROP_FRAME_WIDTH), (int)m_pCap->get(CAP_PROP_FRAME_HEIGHT));
		m_nc = channels;
		
		ec = true;
	}

	return ec;
}
std::shared_ptr<ISVideoFile> ISVideoFile::createEx(const string_t &file, int channels)
{
	auto *obj = new ISVideoFile;
	if (!obj->create(file, channels))
	{
		delete obj;
		obj = NULL;
	}
	return std::shared_ptr<ISVideoFile>(obj);
}
int ISVideoFile::nFrames()
{
	return (int)m_pCap->get(CAP_PROP_FRAME_COUNT);
}
Size ISVideoFile::size()
{
	return m_size;
}
const cv::Mat* ISVideoFile::read()
{
	return m_pCurImg&&!m_pCurImg->empty() ? m_pCurImg : NULL;
}

bool ISVideoFile::moveForward()
{
	return this->setPos(this->pos()+1);
}
int ISVideoFile::pos()
{
	return (int)m_pCap->get(CAP_PROP_POS_FRAMES);
}

bool ISVideoFile::setPos(int pos)
{
	if (m_pCap->set(CAP_PROP_POS_FRAMES, pos) && m_pCap->grab())
	{
		Mat T;
		if (m_pCap->retrieve(T))
		{
			if (m_nc > 0)
				convertBGRChannels(T, *m_pCurImg, m_nc);
			else
				*m_pCurImg = T;
		}
		else
			*m_pCurImg = Mat();
		return true;
	}
	return false;
}

//=====================================================================================

uint _get_file_id(const string_t &fileName)
{
	const char_t *px = fileName.c_str();
	const char_t *pe = px+fileName.size();

	while (px != pe && !_istdigit(*px)) ++px;

	uint id = uint(-1);

	if (px != pe)
	{
		const char_t *pb = px;
		while (px != pe && _istdigit(*px)) ++px;
		string_t sid(pb, px);
		id = _ttoi(sid.c_str());
	}

	return id;
}


template<typename _PairT>
struct less_first
{
public:
	bool operator()(const _PairT &left, const _PairT &right)
	{
		return left.first<right.first;
	}
};

template<typename _PairT>
struct less_second
{
public:
	bool operator()(const _PairT &left, const _PairT &right)
	{
		return left.second<right.second;
	}
};

int ListFiles(const string_t &path, const string_t &ext, std::vector<string_t> &flist)
{
	std::vector<string_t> files;
	ff::listFiles(path, files, false);

	typedef std::pair<string_t, uint>  _PairT;
	std::vector< _PairT>  vlist;

	bool compare_by_id = true;
	for (size_t i = 0; i < files.size(); ++i)
	{
		string_t iext(ff::GetFileExtention(files[i]));
		if (_tcsicmp(ext.c_str(), iext.c_str()) != 0)
			continue;

		uint id = 0;

		if (compare_by_id)
		{
			id = _get_file_id(files[i]);
			if (id == uint(-1)) //if failed
				compare_by_id = false;
		}

		vlist.push_back(_PairT(files[i], id));
	} 

	if (compare_by_id)
		std::sort(vlist.begin(), vlist.end(), less_second<_PairT>());
	else
		std::sort(vlist.begin(), vlist.end(), less_first<_PairT>());

	for (size_t i = 0; i<vlist.size(); ++i)
		flist.push_back(vlist[i].first);

	return 0;
}

bool ISImageFiles::create(const string_t &config, ff::IArgSet *arg=NULL)
{
	int ec = 0;

	ff::CommandArgSet _arg;
	if (!arg)
	{
		_arg.setArgs(config);
		arg = &_arg;
	}

	const char_t *kw = _T("*.");
	if (_ContainSubStr(config, &kw, 1))
	{//list files in the same dir
		string_t file(arg->get<string_t>(_T("#0")));
		string_t dir(ff::GetDirectory(file)), ext(ff::GetFileExtention(file));
		dir=ff::getFullPath(dir);

		ec = ListFiles(dir, ext, m_vfiles);
		m_dir = dir + _TX("\\");
	}
	else
	{//enum files in the config
		m_vfiles.clear();
		arg->allowException(false);
		string_t str;

		for (int i = 0;; ++i)
		{
			char_t buf[32];
			_stprintf(buf, _T("#%d"), i);
			if (arg->get(buf, &str) > 0)
				m_vfiles.push_back(str);
			else
				break;
		}
		m_dir = _T("");
		arg->allowException(true);
	}

	if (ec != 0)
		return false;

	m_nc = arg->getd<int>(_T("nc"),0);
	ec = -1;

	if (this->nFrames()>0)
	{
		const cv::Mat *first = NULL;

		if (this->setPos(0) && (first = this->read()))
		{
			m_size = first->size();
			ec = 0;
		}
	}
	else
		m_size=Size(0,0);

	return ec==0? true:false;
}

std::shared_ptr<ISImageFiles> ISImageFiles::createEx(const string_t &config)
{
	ISImageFiles *obj = new ISImageFiles;
	if (!obj->create(config, NULL))
	{
		delete obj;
		obj = NULL;
	}
	return std::shared_ptr<ISImageFiles>(obj);
}

int ISImageFiles::getFileIndex(const string_t &name)
{
	for (size_t i = 0; i<m_vfiles.size(); ++i)
	{
		if (_tcsicmp(name.c_str(), m_vfiles[i].c_str()) == 0)
			return (int)i;
	}

	return -1;
}

int ISImageFiles::nFrames()
{
	return (int)m_vfiles.size();
}
Size ISImageFiles::size()
{
	return m_size;
}
const cv::Mat* ISImageFiles::read()
{
	if (m_bufPos != m_pos)
	{
		bool ok = false;

		if (uint(m_pos)<uint(m_vfiles.size()))
		{
			try
			{
				Mat T = cv::imread(ff::X2MBS(m_dir + m_vfiles[m_pos]), -1);
				if (m_nc > 0)
					convertBGRChannels(T, m_bufImg, m_nc);
				else
					m_bufImg = T;

				m_bufPos = m_pos;
				ok = true;
			}
			catch (...)
			{
			}
		}

		if (!ok)
		{
			m_bufPos = -1;
		}
	}

	return m_bufPos == m_pos && uint(m_pos)<uint(m_vfiles.size()) ? &m_bufImg : NULL;

}
bool ISImageFiles::moveForward()
{
	return this->setPos(m_pos + 1) == 0 ? true : false;
}

int ISImageFiles::pos()
{
	return m_pos;
}

bool ISImageFiles::setPos(int pos)
{
	if (uint(pos)<uint(this->nFrames()))
	{
		m_pos = pos;
		return true;
	}
	return false;
}

string_t ISImageFiles::frameName(int pos)
{
	if (uint(pos)<uint(this->nFrames()))
		return m_vfiles[pos];

	return _TX("");
}

void ISImages::setImages(const Mat &img)
{
	m_imList.clear();
	m_imList.push_back(img);
	m_pos = 0;
}
void ISImages::setImages(const Mat imList[], int count)
{
	this->setImages(imList, imList + count);
}
void ISImages::setTitle(const std::string title[], int count)
{
	std::vector<string_t> t(count);
	for (int i = 0; i < count; ++i)
		t[i] = ff::X2TS(title[i]);
	m_title.swap(t);
}
void ISImages::setTitle(const std::wstring title[], int count)
{
	std::vector<string_t> t(count);
	for (int i = 0; i < count; ++i)
		t[i] = ff::X2TS(title[i]);
	m_title.swap(t);
}
int ISImages::nFrames()
{
	return (int)m_imList.size();
}
Size ISImages::size()
{
	return m_pos < m_imList.size() ? m_imList[m_pos].size() : Size(0, 0);
}
const cv::Mat* ISImages::read()
{
	return m_pos<m_imList.size()? &m_imList[m_pos] : NULL;

}
bool ISImages::moveForward()
{
	return this->setPos(m_pos + 1) == 0 ? true : false;
}

int ISImages::pos()
{
	return (int)m_pos;
}

bool ISImages::setPos(int pos)
{
	if (uint(pos)<uint(this->nFrames()))
	{
		m_pos = pos;
		return true;
	}
	return false;
}

string_t ISImages::frameName(int pos)
{
	return (uint)pos < uint(m_title.size()) ? m_title[pos] : this->ImageSet::frameName(pos);
}

ISMaker::ISource::~ISource() throw()
{
}
Size   ISMaker::size()
{
	return img.size();
}

const cv::Mat* ISMaker::read()
{
	return &img;
}

bool     ISMaker::moveForward()
{
	return this->setPos(curPos + 1);
}
int		ISMaker::nFrames()
{
	return frames;
}
int		 ISMaker::pos()
{
	return curPos;
}

bool	ISMaker::setPos(int pos)
{
	if (src&&src->get(img, pos))
	{
		curPos = pos;
		return true;
	}
	return false;
}

//=============================================


static const char_t *video_file_key_words[] = { _TX(".avi"), _TX(".mov"), _TX(".mkv"), _TX(".flv"), _TX(".wmv"), _TX(".rmvb"), _TX(".rm"), _TX(".mpg"), _TX(".mts"), _TX(".mp4")};

static const char_t *image_file_key_words[] = { _TX(".jpg"), _TX(".png"), _TX(".gif"), _TX(".bmp") };

enum
{
	ISR_UNKNOWN,
	ISR_VIDEO,
	ISR_IMAGES
};

static int _get_isr_type(const string_t &config, char_t ct)
{
	int type = ISR_UNKNOWN;

	switch (ct)
	{
	case _TX('v'):
		type = ISR_VIDEO;
		break;
	case _TX('f'):
		type = ISR_IMAGES;
		break;
	case _TX('*'):
	{
		if (_ContainSubStr(config, AL_ARR_BN(video_file_key_words)))
			type = ISR_VIDEO;
		else
			if (_ContainSubStr(config, AL_ARR_BN(image_file_key_words)))
				type = ISR_IMAGES;
	}
	}
	return type;
}

int  getISRType(const string_t &config, char_t ct)
{
	return _get_isr_type(config, ct);
}

ImageSet* createImageSet(const string_t &config, char_t ct, ff::IArgSet *arg)
{
	int type = getISRType(config, ct);
	ff::AutoPtr<ImageSet> pISRX(NULL);

	switch (type)
	{
	case ISR_IMAGES:
		pISRX = ff::AutoPtr<ImageSet>(new ISImageFiles);
		break;
	case ISR_VIDEO:
		pISRX = ff::AutoPtr<ImageSet>(new ISVideoFile);
		break;
	default:
		FF_EXCEPTION("unrecognized isr type","");
	}
	if (pISRX)
		pISRX->create(config, arg);

	return pISRX&&pISRX->create(config,arg)? pISRX.Detach() : NULL;
}

ImageSetPtr createImageSet(const string_t &config)
{
	using ff::CommandArgSet;
	static CommandArgSet defaultArgs;
	static bool initDefault = true;
	if (initDefault)
	{
		defaultArgs.setArgs(_TX("-t * -nc 0"));
		initDefault = false;
	}

	CommandArgSet args;
	args.setArgs(config);
	args.setNext(&defaultArgs);

	return ImageSetPtr(createImageSet(config, args.get<char_t>(_TX("t")), &args));
}

int readFramesV(ImageSet *isr, va_list varg)
{
	int n = 0;
	int idx;
	while (uint(idx = va_arg(varg, int)) < (uint)isr->nFrames())
	{
		Mat *pimg = va_arg(varg, Mat*);
		if (pimg)
		{
			isr->read(*pimg, idx);
			++n;
		}
	}
	va_end(varg);

	return n;
}

int readFrames(ImageSet *isr, ...)
{
	va_list varg;
	va_start(varg, isr);

	return readFramesV(isr,varg);
}

int readFrames(const char_t *isrConfig, ...)
{
	ImageSetPtr isr(createImageSet(isrConfig));
	if (isr)
	{
		va_list varl;
		va_start(varl, isrConfig);

		return readFramesV(isr.get(), varl);
	}
	return 0;
}


_CVX_END

