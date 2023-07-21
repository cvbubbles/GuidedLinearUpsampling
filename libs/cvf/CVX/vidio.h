#pragma once

#include"def.h"
#include"opencv2/highgui.hpp"
#include"BFC/stdf.h"
#include"BFC/portable.h"

_CVX_BEG


class VideoWriterEx
	:public cv::VideoWriter
{
	std::string _file;
	std::string _curFile;
	bool  _autoFileID;
	int _fourcc;
	double _fps;
	int   _state;
public:
	void set(const std::string &file, double fps, int fourcc = CV_FOURCC('M', 'J', 'P', 'G'))
	{
		_file = file;
		_fourcc = fourcc;
		_fps = fps;
		_state = 0;

		_autoFileID = ff::StrFormat(_file.c_str(), 1) != ff::StrFormat(_file.c_str(), 2);
	}
	bool writeEx(const Mat &img, int waitCode);

	const std::string& getCurFile() const
	{
		return _curFile;
	}
};


inline bool VideoWriterEx::writeEx(const Mat &img, int waitCode)
{
	bool written = false;

	if (_state == 0 && waitCode == 'b')
	{
		std::string file = _file;
		if (_autoFileID)
		{
			int i = 1;
			for (;; ++i)
			{
				file = ff::StrFormat(_file.c_str(), i);
				if (!ff::pathExist(file))
				{
					break;
				}
			}
		}

		if (!this->open(file, _fourcc, _fps, img.size()))
		{
			printf("can't open %s\n", file.c_str());
			return false;
		}

		printf("start capture to %s\n", file.c_str());
		_state = 1;
		_curFile = file;
	}
	else if (_state == 1)
	{
		this->write(img);
		written = true;

		if (waitCode == 'e')
		{
			printf("end capture\n");
			this->release();
			_state = 0;
		}
	}
	return written;
}

_CVX_END

