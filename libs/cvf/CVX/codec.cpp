#include"codec.h"

#include"codec/readGIF.cpp"
#include"codec/writeGIF.cpp"
#include"codec/median.cpp"
#include"codec/gif.cpp"

#include"codec/bmpio.h"

_CVX_BEG

static bool _iReadImageData(ImageReader* pReader, cv::Mat& dest)
{
	assert(pReader);
	if (pReader->ReadHeader())
	{
		int width = pReader->GetWidth();
		int height = pReader->GetHeight();
		int cn = pReader->Channels();
		assert(cn>0 && cn <= 4);

		dest.create(height, width, CV_MAKETYPE(CV_8U, cn));
		if (pReader->ReadData(dest.data, dest.step, cn == 1 ? 0 : -1))
			return true;
	}
	return false;
}

cv::Mat readBMP(const std::string &file)
{
	cv::Mat dest;
	{
		BmpReader reader(file.c_str());
		if (!_iReadImageData(&reader, dest))
			throw file;
	}
	return dest;
}

void writeBMP(const std::string &file, const cv::Mat& img)
{
	if (!img.empty())
	{
		BmpWriter writer(file.c_str());
		if (!writer.WriteImage(img.data, img.step, img.cols, img.rows, img.channels()))
			throw file;
	}
}


_CVX_END


#include"codec/bitstrm.cpp"
#include"codec/bmpio.cpp"
#include"codec/utils.cpp"
#include"codec/iobase.cpp"

