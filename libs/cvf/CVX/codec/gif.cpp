
#include <memory.h>
#include<assert.h>
#include<stdlib.h>
#include<stdio.h>

#include"ioimpl.h"

#include "gif.h"

static void _iCopyLine(const uchar (*pMap)[3],const uchar* pi,const int width,const int trans,
					   uchar* po,int pw)
{
	if(pw==3)
	{
		for(int xi=0;xi<width;++xi,po+=3,++pi)
		{
			po[0]=pMap[*pi][2];
			po[1]=pMap[*pi][1];
			po[2]=pMap[*pi][0];
		}
	}
	else
	{
		assert(pw==4);
		for(int xi=0;xi<width;++xi,po+=4,++pi)
		{
			po[0]=pMap[*pi][2];
			po[1]=pMap[*pi][1];
			po[2]=pMap[*pi][0];
			po[3]=*pi==trans? 0:255;
		}
	}
}
static const int _gifPassBeg[]={0,4,2,1};
static const int _gifPassItv[]={8,8,4,2};

static void _iCopyGifData(const uchar (*pMap)[3],const int interlaced,const uchar* pin,const int width,const int height,const int trans,
						  uchar* po,const int step,int pw)
{
	if(interlaced==0)
	{
		for(int yi=0;yi<height;++yi,po+=step,pin+=width)
			_iCopyLine(pMap,pin,width,trans,po,pw);
	}
	else
	{
		const uchar* pie=pin+width*height;
		for(int pi=0;pi<4;++pi)
		{
			const int itv=_gifPassItv[pi];
			const int pstep=width*itv,ostep=step*itv;
			const uchar* pix=pin+width*_gifPassBeg[pi];
			uchar* pox=po+step*_gifPassBeg[pi];
			for(;pix<pie;pix+=pstep,pox+=ostep)
				_iCopyLine(pMap,pix,width,trans,pox,pw);
		}
	}
}
static bool _iReadGifData(const GIFStream* pGif,const GIFData* pData,
						  uchar* pOut,int step,int pw,uchar* pTrans)
{
	assert(pw==3||pw==4);
	assert(pData->type==gif_image);
	pOut+=pData->y*step+pData->x*pw;
	const int width=pData->width,height=pData->height;
	const uchar* pi=pData->data.image.data;
	const uchar (*pMap)[3]=pData->data.image.cmapSize>0? &pData->data.image.cmapData[0]:
		pGif->cmapSize>0? &pGif->cmapData[0]:NULL;
	if(pMap)
	{
		_iCopyGifData(pMap,pData->data.image.interlaced,pData->data.image.data,pData->width,
			pData->height,pData->info.transparent,pOut,step,pw);
		if(pTrans)
		{
			const int ti=pData->info.transparent;
			if(ti>=0)
			{
				if(pData->data.image.interlaced)
				{
					pTrans[0]=pMap[2][ti];
					pTrans[1]=pMap[1][ti];
					pTrans[2]=pMap[0][ti];
				}
				else
				{
					pTrans[0]=pMap[ti][2];
					pTrans[1]=pMap[ti][1];
					pTrans[2]=pMap[ti][0];
				}
				pTrans[3]=255;
			}
			else
				pTrans[0]=pTrans[1]=pTrans[2]=pTrans[3]=0;
		}
		return true;
	}
	return false;
}
static void _iReleaseGIFStream(GIFStream* &pGif)
{
	if(pGif)
	{
		GIFData* pData=pGif->data;
		while(pData)
		{
			switch(pData->type)
			{
			case gif_image:
				free(pData->data.image.data);
				break;
			case gif_text:
				free(pData->data.text.text);
				break;
			case gif_comment:
				free(pData->data.comment.text);
				break;
			default:
				assert(false);
			}
			GIFData* pNext=pData->next;
			free(pData);
			pData=pNext;
		}
		free(pGif);
		pGif=NULL;
	}
}

 GIFStream*	GIFRead(const char * file)
 {
	FILE		*fp = fopen(file, ("rb"));
	GIFStream	*stream = NULL;

	if (fp != NULL) 
	{
		stream = GIFReadFP(fp);
		fclose(fp);
	}
	return stream;
 }

#include"opencv2/core.hpp"
#include"CVX/codec.h"
#include"CVX/core.h"

 _CVX_BEG

	 int	GIFWrite(const char *file, GIFStream *stream, int optimize);
 
 int readGIF(const std::string &file, std::vector<Mat> &frames, bool readAlpha,uchar pTransparent[4])
 {
	 frames.clear();

	 GIFStream* pGif=GIFRead(file.c_str());
	 if(pGif)
	 {
		 GIFData* pData=pGif->data;
		 while(pData)
		 {
			 Mat frame(pGif->height, pGif->width, readAlpha ? CV_8UC4 : CV_8UC3);

			 if(pData->type==gif_image&&
				 _iReadGifData(pGif,pData,frame.data,frame.step,frame.channels(),pTransparent)
				 )
			 {
				 frames.push_back(frame);
			 }
			 pData=pData->next;
		 }

		 GIFWrite("out2.gif", pGif, 1);

		 _iReleaseGIFStream(pGif);
	 }

	 return (int)frames.size();
 }


 static void _iGetRGB(const uchar* pi, int width, int height, int step, int pw, uchar* pb, uchar* pg, uchar* pr)
 {
	 for(int yi=0;yi<height;++yi,pi+=step)
	 {
		 const uchar* pix=pi;
		 for(int xi=0;xi<width;++xi,pix+=pw,++pb,++pg,++pr)
		 {
			 *pb=pix[0],*pg=pix[1],*pr=pix[2];
		 }
	 }
 }
 static int _iFindTransparent(const uchar (*pMap)[3],const int size,const uchar pTrans[3])
 {
	 if (!pTrans)
		 return -1;

	 int iMin=-1;
	 int ssd=255*255*4;
	 for(int i=0;i<size;++i)
	 {
		 int ssdx=0;
		 for(int j=0;j<3;++j)
		 {
			 int d=int(pMap[i][j])-pTrans[j];
			 ssdx+=d*d;
		 }
		 if(ssdx<ssd)
			 iMin=i,ssd=ssdx;
	 }
	 return iMin; 
 }


 int	GIFWrite(const char *file, GIFStream *stream, int optimize)
 {
	 if (stream != NULL) {
		 FILE	*fp = fopen(file, ("wb"));

		 if (fp != NULL) {
			 int	s = GIFWriteFP(fp, stream, optimize);
			 fclose(fp);
			 return s;
		 }
	 }
	 return 1;
 }


 void writeGIF(const std::string &file,const std::vector<Mat> &frames, int delayTime, const uchar pTransparent[3])
 {
	 auto copyData = [&frames](uchar *&data, int &dcn, int &np)
	 {
		 int icn = 0;
		 np = 0;
		 for (size_t i = 0; i < frames.size(); ++i)
		 {
			 if (frames[i].channels()>icn)
				 icn = frames[i].channels();
			 np += frames[i].cols*frames[i].rows;
		 }
		 dcn = icn <= 2 ? 1 : 3;

		 data = new uchar[np*dcn];
		 uchar *ptr = data;
		 for (size_t i = 0; i < frames.size(); ++i)
		 {
			 Mat img(frames[i]);
			 if (img.channels() != dcn)
				 convertBGRChannels(frames[i], img, dcn);
			 memcpy_2d(img.data, img.cols*dcn, img.rows, img.step, ptr, img.cols*dcn);
			 ptr += dcn*img.cols*img.rows;
		 }
	 };

	 if (file.empty() || frames.empty())
		 return;

	 int width = frames[0].cols, height = frames[0].rows;

	 int mapSize = 0;
	 GIFStream* pGif = NULL;

	 try{

		 pGif=(GIFStream*)malloc(sizeof(GIFStream));
		 memset(pGif,0,sizeof(*pGif));
		 pGif->aspectRatio=0;
		 pGif->background=-1;
		 pGif->width=width;
		 pGif->height=height;
		 pGif->colorResolution=15;
		 
		 uchar  *imgData=NULL;
		 int dcn = 0, np=0;
		 copyData(imgData, dcn, np);
		 uchar *pdata = new uchar[np];

		// mapSize=QuantizeImageMedian(imgData,width,height*frames.size(),width*dcn, dcn, pdata,&pGif->cmapData[0],GIF_MAXCOLORS, QIM_SWAP_RB);
		 mapSize = QuantizeImageMedian(imgData, np, 1, np*dcn, dcn, pdata, &pGif->cmapData[0], GIF_MAXCOLORS, QIM_SWAP_RB);
		 
		 GIFData head, *last = &head;
		 head.next = NULL;

		 if(mapSize>0)
		 {
			 pGif->cmapSize = pGif->colorMapSize = 256;
			 int transparent = _iFindTransparent(&pGif->cmapData[0], GIF_MAXCOLORS, pTransparent);
			 uchar *p = pdata;

			 for (size_t i = 0; i < frames.size(); ++i)
			 {
				 const Mat &img(frames[i]);
				 GIFData* pBlk = (GIFData*)malloc(sizeof(GIFData));
				 memset(pBlk, 0, sizeof(GIFData));

				 pBlk->info.transparent = transparent;
				 pBlk->info.delayTime = delayTime;
				 pBlk->info.disposal = gif_keep_disposal;
				 

				 pBlk->width = img.cols, pBlk->height = img.rows;
				 pBlk->type = gif_image;
				 
				 pBlk->data.image.data = p;
				 p += img.cols*img.rows;

				 last->next = pBlk;
				 last = pBlk;
			 }
		 }

		 pGif->data = head.next;
		 if (GIFWrite(file.c_str(), pGif, 1))
			 mapSize = 0;

		 last = head.next;
		 while (last)
		 {
			 last->data.image.data = NULL;
			 last = last->next;
		 }

		 delete[]pdata;
		 delete[]imgData;

		 _iReleaseGIFStream(pGif);

	 }
	 catch(...)
	 {
		 _iReleaseGIFStream(pGif);
		 throw;
	 }
 }


 _CVX_END


