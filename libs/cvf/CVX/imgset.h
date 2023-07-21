
#pragma once

#include"def.h"
#include"CVX/core.h"
#include"BFC/argv.h"

_CVX_BEG


class ImageSet
{
public:
	virtual Size	size() = 0;
	//read current image without move to the next.
	//return NULL if end of image set reached.
	virtual const cv::Mat* read() = 0;

	bool read(cv::Mat &img, int pos = -1);

	cv::Mat read(int pos);

	template<typename _IOPT>
	bool read(cv::Mat &img, int pos, _IOPT op)
	{
		bool rv = this->read(img, pos);
		if (rv)
			op(img);
		return rv;
	}

	//move to the next.
	//return false if the new pos reach the end of image set..
	virtual bool     moveForward() = 0;
	//reset the read position to the beginning.
	virtual bool     seekBeg();

	//===================
	//random access functions
	//number of images in the image set.
	virtual int		nFrames() = 0;
	//get current pos.
	virtual int		 pos() = 0;
	//set current pos.
	virtual bool	 setPos(int pos) = 0;

	virtual string_t frameName(int pos);

	ImageSet();

	virtual ~ImageSet();

	virtual bool create(const string_t &config, ff::IArgSet *arg);

private:
	ImageSet(const ImageSet&);
	ImageSet& operator=(const ImageSet&);
};

typedef std::shared_ptr<ImageSet> ImageSetPtr;

class VideoCapture;

class ISVideoFile
	:public ImageSet
{
protected:
	VideoCapture	*m_pCap=NULL;

	Size			m_size;
	cv::Mat		   *m_pCurImg=NULL;
	int				m_nc=0;
public:
	ISVideoFile()
	{}

	ISVideoFile(const string_t &file, int channels = 0)
	{
		this->create(file, channels);
	}

	virtual bool create(const string_t &config, ff::IArgSet *arg);

	bool  create(const string_t &file, int channels=0);

	static std::shared_ptr<ISVideoFile> createEx(const string_t &file, int channels = 0);

	~ISVideoFile();

public:
	virtual Size	size();

	using ImageSet::read;
	virtual const cv::Mat* read();

	virtual bool     moveForward();

	virtual int		nFrames();

	virtual int		 pos();

	virtual bool		setPos(int pos);
};


class ISImageFiles
	:public ImageSet
{
protected:
	Size		m_size =Size(0,0);

	int			m_bufPos = -1;
	cv::Mat	m_bufImg;

	int			m_pos = -1;
	string_t		m_dir;
	std::vector<string_t>  m_vfiles;
	int			m_nc =0; //force number of channels
public:
	ISImageFiles()
	{}

	ISImageFiles(const string_t &config)
	{
		this->create(config, NULL);
	}

	virtual bool create(const string_t &config, ff::IArgSet *arg);

	static std::shared_ptr<ISImageFiles> createEx(const string_t &config);

	int		getFileIndex(const string_t &name);

public:
	virtual Size	size();

	using ImageSet::read;
	virtual const cv::Mat* read();

	virtual bool     moveForward();

	virtual int		nFrames();

	virtual int		 pos();

	virtual bool		setPos(int pos);

	virtual string_t frameName(int pos);
};

class ISImages
	:public ImageSet
{
protected:
	std::vector<Mat>  m_imList;
	std::vector<string_t>  m_title;
	size_t             m_pos;
public:
	ISImages()
		:m_pos(0)
	{}

	ISImages(const Mat &img)
	{
		this->setImages(img);
	}

	ISImages(const Mat imList[], int count)
	{
		this->setImages(imList, count);
	}

	template<typename _ItrT>
	ISImages(_ItrT beg, _ItrT end)
	{
		this->setImages(beg, end);
	}
	template<typename _ListT>
	ISImages(const _ListT &imgList)
	{
		this->setImages(imgList.begin(), imgList.end());
	}

	void setImages(const Mat &img);

	void setImages(const Mat imList[], int count);

	template<typename _ItrT>
	void setImages(_ItrT beg, _ItrT end)
	{
		std::vector<Mat> imList;
		for (; beg != end; ++beg)
			imList.push_back(*beg);
		m_imList.swap(imList);
		m_pos = 0;
	}

	void setTitle(const std::string title[], int count);

	void setTitle(const std::wstring title[], int count);

	static std::shared_ptr<ISImages> createEx(const Mat &img)
	{
		return std::shared_ptr<ISImages>(new ISImages(img));
	}
	static std::shared_ptr<ISImages> createEx(const Mat imList[], int count)
	{
		return std::shared_ptr<ISImages>(new ISImages(imList, count));
	}
	template<typename _ItrT>
	static std::shared_ptr<ISImages> createEx(_ItrT beg, _ItrT end)
	{
		return std::shared_ptr<ISImages>(new ISImages(beg, end));
	}
	template<typename _ListT>
	static std::shared_ptr<ISImages> createEx(const _ListT &imgList)
	{
		return std::shared_ptr<ISImages>(new ISImages(imgList));
	}
public:
	virtual Size	size();

	using ImageSet::read;
	virtual const cv::Mat* read();

	virtual bool     moveForward();

	virtual int		nFrames();

	virtual int		 pos();

	virtual bool	setPos(int pos);

	virtual string_t frameName(int pos);
};

class ISMaker
	:public ImageSet
{
public:
	class ISource
	{
	public:
		virtual bool get(Mat &img, int pos) = 0;

		virtual ~ISource() throw();
	};

	template<typename _OpT>
	class _SourceT
		:public ISource
	{
		_OpT op;
	public:
		_SourceT(const _OpT &_op=_OpT())
			:op(_op)
		{}
		virtual bool get(Mat &img, int pos)
		{
			return op(img, pos);
		}
	};
private:
	ISource *src = NULL;
	int      frames = 0;
	int      curPos = 0;
	Mat      img;

	void _setMaker(int _count, ISource *_src)
	{
		if (src)
			delete src;
		src = _src;
		frames = _count;
		this->setPos(0);
	}
public:
	template<typename _OpT>
	static std::shared_ptr<ISMaker> createEx(int frames, const _OpT &op)
	{
		return std::shared_ptr<ISMaker>(new ISMaker(frames, op));
	}
public:
	ISMaker()
	{}
	template<typename _OpT>
	ISMaker(int frames, const _OpT &op)
	{
		this->setMaker(frames, op);
	}
	~ISMaker() throw()
	{
		delete src;
	}
	template<typename _OpT>
	void setMaker(int frames, const _OpT &op)
	{
		this->_setMaker(frames, new _SourceT<_OpT>(op));
	}

	virtual Size	size();

	using ImageSet::read;
	virtual const cv::Mat* read();

	virtual bool     moveForward();

	virtual int		nFrames();

	virtual int		 pos();

	virtual bool	setPos(int pos);
};


ImageSetPtr createImageSet(const string_t &config);

//the arg-list should be end with -1
int readFrames(ImageSet *isr, ...);

int readFrames(const char_t *isrConfig, ...);


_CVX_END

