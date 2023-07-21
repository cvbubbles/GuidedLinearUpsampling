
#pragma once

#include"def.h"
#include"opencv2/core.hpp"

_CVX_BEG

struct GSPoint
{
	uint		m_dist;
	ushort		m_x; 
	ushort		m_y;
};

class GeoSeg
{
public:
	class  WorkPixel;
	
private:
	WorkPixel  *m_wpx;
	uint       *m_dist;
	int         m_width, m_height, m_stride;
public:
	GeoSeg();

	~GeoSeg();

	//set image and building the neighbor graph
	//@image should be CV_8UC3 or CV_8UC4 format
	//@icn : number of channels (3 or 4)
	int SetImage(const uchar *img, int width, int height, int istep, int icn, const int _RMEAN=-1, const float RSCALE=1.0f, const int SPATIAL_DIST=1);

	int SetL1(int width, int height);

	int GetProb(const uchar *mask, int mstep, float *prob, int stride);

	/* get geodesic distance to some seed points
	@mask : mask of seed points, mask[i] is a seed pixel if mask[i]==mv
	@dist : the result distant field

	both @mask and @dist must have the same size as the image setted with @SetImage.
	*/
	int GetDist(const uchar *mask, int mstep, uint *dist, int dstride, uchar mv);

	int GetDistWithSource(const uchar *mask, int mstep, GSPoint *dist, int dstride, uchar mv);
};

//compute geodesic distance to some seed points
//@mask : mask of seed points, mask[i] is a seed pixel if mask[i]==@mv
//@dist : result distance field
inline void geodesicDistanceTransform(const Mat &img, const Mat &mask, uchar mv, Mat &dist)
{
	assert(img.type() == CV_8UC3 || img.type() == CV_8UC4);
	assert(mask.type() == CV_8UC1);

	GeoSeg geos;
	geos.SetImage(img.data, img.cols, img.rows, img.step, img.channels());

	dist.create(img.size(), CV_32SC1);
	
	assert(dist.cols*sizeof(uint) == dist.step);
	geos.GetDist(mask.data, mask.step, dist.ptr<uint>(), dist.cols, mv);
}

_CVX_END



