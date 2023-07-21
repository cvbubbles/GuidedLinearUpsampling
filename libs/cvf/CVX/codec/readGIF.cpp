/*
**  Copyright 1994, Home Pages, Inc.
**
**    Please read the file COPYRIGHT for specific information.
**
**    Home Pages, Inc.
**    257 Castro St. Suite 219
**    Mountain View, CA 94041
**
**    Phone: 1 415 903 5353
**      Fax: 1 415 903 5345
**
**    EMail: support@homepages.com
** 
*/
 
/* +-------------------------------------------------------------------+ */
/* | Copyright 1990 - 1994, David Koblas. (koblas@netcom.com)          | */
/* |   Permission to use, copy, modify, and distribute this software   | */
/* |   and its documentation for any purpose and without fee is hereby | */
/* |   granted, provided that the above copyright notice appear in all | */
/* |   copies and that both that copyright notice and this permission  | */
/* |   notice appear in supporting documentation.  This software is    | */
/* |   provided "as is" without express or implied warranty.           | */
/* +-------------------------------------------------------------------+ */

//#if defined(_MSC_VER)&&defined(_DEBUG)
//
//#define _CRTDBG_MAP_ALLOC
//#include <stdlib.h>
//#include <crtdbg.h>
//
//#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>

#include "gif.h"

#define        TRUE    1
#define        FALSE   0

#define        MAX_LWZ_BITS            12

#define INTERLACE		0x40
#define LOCALCOLORMAP		0x80

#define BitSet(byte, bit)	(((byte) & (bit)) == (bit))
#define	ReadOK(file,buffer,len) (fread(buffer, len, 1, file) != 0)
#define MKINT(a,b)		(((b)<<8)|(a))

static void ReleaseFP(GIFStream*& pGif)
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
		
			}
			GIFData* pNext=pData->next;
			free(pData);
			pData=pNext;
		}
		free(pGif);
		pGif=NULL;
	}
}

/***************************************************************************
*
*  GIF_ERROR()    --  should not return
*  INFO_MSG() --  info message, can be ignored
*
***************************************************************************/

#if 0
#define INFO_MSG(fmt)	pm_message fmt 
#define GIF_ERROR(str)	pm_error(str)
#else
#if 0 
#define INFO_MSG(fmt)	
#define GIF_ERROR(str) 	do { RWSetMsg(str); longjmp(setjmp_buffer, 1); } while(0)
#else
#define INFO_MSG(fmt)	
#define GIF_ERROR(str) 	throw str;
#endif
#endif

/***************************************************************************/

#define _BS 1

struct _GIFReadImp
{
protected:
	int    verbose ;
	int    showComment,ZeroDataBlock;

	int		curbit, lastbit, get_done, last_byte;
	int		return_clear;
	int      stack[(1<<(MAX_LWZ_BITS))*2*_BS], *sp;
	int      code_size, set_code_size;
	int      max_code, max_code_size;
	int      clear_code, end_code;
	
	unsigned char    buf[280];
	int	table[2][(1<< MAX_LWZ_BITS)*_BS];
    int	firstcode, oldcode;

	std::vector<void*> vMem;

public:
	
	_GIFReadImp()
		:verbose(FALSE),showComment(FALSE),ZeroDataBlock(FALSE)
	{
	}

	void* _Malloc(size_t size)
	{
		if(size>1024*1024*100)
			throw std::bad_alloc();

		void* ptr=malloc(size);
		vMem.push_back(ptr);
		return ptr;
	}

#define GIF_NEW(x)			((x *)_Malloc(sizeof(x)))
	

GIFStream *GIFReadFP(FILE *fd)
{
	unsigned char   buf[256];
	unsigned char   c;
	GIFStream	*stream;
	GIFData		*cur, **end;
	GIF89info	info;
	int		resetInfo = TRUE;
	int		n;

	//_Malloc(20);

	try
	{
	if (fd == NULL)
		return NULL;

	/*if (setjmp(setjmp_buffer))
		goto out;*/

	if (! ReadOK(fd,buf,6)) 
	       GIF_ERROR("GIF_ERROR reading magic number" );

	if (strncmp((char*)buf,"GIF",3) != 0) 
	       GIF_ERROR("not a GIF file" );

	if ((strncmp((char*)buf + 3, "87a", 3) != 0) && 
	    (strncmp((char*)buf + 3, "89a", 3) != 0)) 
		GIF_ERROR("bad version number, not '87a' or '89a'" );

	if (! ReadOK(fd,buf,7))
		GIF_ERROR("failed to read screen descriptor");

	stream = GIF_NEW(GIFStream);
	memset(stream,0,sizeof(*stream));

	stream->width           = MKINT(buf[0], buf[1]);
	stream->height          = MKINT(buf[2], buf[3]);

	stream->cmapSize        = 2 << (buf[4] & 0x07);
	stream->colorMapSize    = stream->cmapSize;
	stream->colorResolution = ((int)(buf[4] & 0x70) >> 3) + 1;
	stream->background      = buf[5];
	stream->aspectRatio     = buf[6];

	stream->data            = NULL;

	end = &stream->data;

	/*
	**  Global colormap is present.
	*/
	if (BitSet(buf[4], LOCALCOLORMAP)) {
		if (readColorMap(fd, stream->cmapSize, stream->cmapData))
			GIF_ERROR("unable to get global colormap");
	} else {
		stream->cmapSize   = 0;
		stream->background = -1;
	}

	/*if (stream->aspectRatio != 0 && stream->aspectRatio != 49) 
	{
	       float   r=float((stream->aspectRatio + 15.0) / 64.0);
	       INFO_MSG(("warning - non-square pixels; to fix do a 'pnmscale -%cscale %g'",
		   r < 1.0 ? 'x' : 'y',
		   r < 1.0 ? 1.0 / r : r ));
	}*/

	while (ReadOK(fd, &c, 1) && c != ';') {
		if (resetInfo) {
			info.disposal    = gif_no_disposal;
			info.inputFlag   = 0;
			info.delayTime   = 0;
			info.transparent = -1;
			resetInfo = FALSE;
		}
		cur = NULL;

		if (c == '!') {		/* Extension */
			if (! ReadOK(fd,&c,1))
				GIF_ERROR("EOF / read GIF_ERROR on extention function code");
			if (c == 0xf9) {	/* graphic control */
				(void) GetDataBlock(fd, buf);
				info.disposal    = GIFDisposalType((buf[0] >> 2) & 0x7);
				info.inputFlag   = (buf[0] >> 1) & 0x1;
				info.delayTime   = MKINT(buf[1],buf[2]);
				if (BitSet(buf[0], 0x1))
					info.transparent = buf[3];

				while (GetDataBlock(fd,  buf) != 0)
					;
			} else if (c == 0xfe || c == 0x01) {
				int		len = 0;
				int		size = 256;
				char		*text = NULL;

				/* 
				**  Comment or Plain Text
				*/

				cur = GIF_NEW(GIFData);

				if (c == 0x01) {
					(void)GetDataBlock(fd, buf);
						
					cur->type   = gif_text;
					cur->info   = info;
					cur->x      = MKINT(buf[0],buf[1]);
					cur->y      = MKINT(buf[2],buf[3]);
					cur->width  = MKINT(buf[4],buf[5]);
					cur->height = MKINT(buf[6],buf[7]);

					cur->data.text.cellWidth  = buf[8];
					cur->data.text.cellHeight = buf[9];
					cur->data.text.fg         = buf[10];
					cur->data.text.bg         = buf[11];

					resetInfo = TRUE;
				} else {
					cur->type    = gif_comment;
				}

				//text = (char*)_Malloc(size);
				text=NULL;

				while ((n = GetDataBlock(fd, buf)) > 0) {
				//	if (n + len >= size) 
						//text = (char*)realloc(text, size += 256);
				//		text=(char*)_Malloc(size+=256);
				//	memcpy(text + len, buf, n);
				//	len += n;
				}

				if (c == 0x01) {
					cur->data.text.len  = len;
					cur->data.text.text = text;
				} else {
					cur->data.comment.len  = len;
					cur->data.comment.text = text;
				}
			} else {
				/*
				**  Unrecogonized extension, consume it.
				*/
				while (GetDataBlock(fd, buf) > 0)
					;
			}
		} else if (c == ',') {
			if (! ReadOK(fd,buf,9))
			       GIF_ERROR("couldn't read left/top/width/height");

			cur = GIF_NEW(GIFData);

			cur->type   = gif_image;
			cur->info   = info;
			cur->x      = MKINT(buf[0], buf[1]);
			cur->y      = MKINT(buf[2], buf[3]);
			cur->width  = MKINT(buf[4], buf[5]);
			cur->height = MKINT(buf[6], buf[7]);
			cur->data.image.cmapSize = 1 << ((buf[8] & 0x07) + 1);
			if (BitSet(buf[8], LOCALCOLORMAP)) {
				if (readColorMap(fd, cur->data.image.cmapSize, 
					             cur->data.image.cmapData))
					GIF_ERROR("unable to get local colormap");
			} else {
				cur->data.image.cmapSize = 0;

			}
			cur->data.image.data = (unsigned char *)_Malloc(cur->width * cur->height);
			cur->data.image.interlaced = BitSet(buf[8], INTERLACE);
			readImage(fd, BitSet(buf[8], INTERLACE), 
				cur->width, cur->height, cur->data.image.data);

			resetInfo = TRUE;
		} else {
			INFO_MSG(("bogus character 0x%02x, ignoring", (int)c));
		}

		if (cur != NULL) {
			*end = cur;
			end = &cur->next;
			cur->next = NULL;
		}
	}

	if (c != ';')
		GIF_ERROR("EOF / data stream" );

	}
	catch(...)
	{
	//	ReleaseFP(stream);
		for(size_t i=0;i<vMem.size();++i)
			free(vMem[i]);

		stream=NULL;
	}
	return stream;
}


 int readColorMap(FILE *fd, int size, 
			unsigned char data[GIF_MAXCOLORS][3])
{
	int             i;
	unsigned char   rgb[3 * GIF_MAXCOLORS];
	unsigned char	*cp = rgb;

	if (! ReadOK(fd, rgb, size * 3))
		return TRUE;

	for (i = 0; i < size; i++) {
		data[i][0] = *cp++;
		data[i][1] = *cp++;
		data[i][2] = *cp++;
	}

	return FALSE;
}

/*
**
*/
 int GetDataBlock(FILE *fd, unsigned char *buf)
{
       unsigned char   count;

       if (! ReadOK(fd,&count,1)) {
               INFO_MSG(("GIF_ERROR in getting DataBlock size"));
               return -1;
       }

       ZeroDataBlock = count == 0;

       if ((count != 0) && (! ReadOK(fd, buf, count))) {
               INFO_MSG(("GIF_ERROR in reading DataBlock"));
               return -1;
       }

       return count;
}

/*
**
**
*/

/*
**  Pulled out of nextCode
*/
	
 void initLWZ(int input_code_size)
{
//	 int	inited = FALSE;

	set_code_size = input_code_size;
	code_size     = set_code_size + 1;
	clear_code    = 1 << set_code_size ;
	end_code      = clear_code + 1;
	max_code_size = 2 * clear_code;
	max_code      = clear_code + 2;

	curbit = lastbit = 0;
	last_byte = 2;
	get_done = FALSE;

	return_clear = TRUE;

	sp = stack;
}


 int nextCode(FILE *fd, int code_size)
{
	 static const int maskTbl[16] = {
		0x0000, 0x0001, 0x0003, 0x0007, 
		0x000f, 0x001f, 0x003f, 0x007f,
		0x00ff, 0x01ff, 0x03ff, 0x07ff,
		0x0fff, 0x1fff, 0x3fff, 0x7fff,
	};
	int                     i, j, ret, end;

	if (return_clear) {
		return_clear = FALSE;
		return clear_code;
	}

	end = curbit + code_size;

	if (end >= lastbit) {
		int	count;

		if (get_done) {
			if (curbit >= lastbit)
				GIF_ERROR("ran off the end of my bits" );
			return -1;
		}
		buf[0] = buf[last_byte-2];
		buf[1] = buf[last_byte-1];

		if ((count = GetDataBlock(fd, &buf[2])) == 0)
			get_done = TRUE;

		last_byte = 2 + count;
		curbit = (curbit - lastbit) + 16;
		lastbit = (2+count)*8 ;

		end = curbit + code_size;
	}

	j = end / 8;
	i = curbit / 8;

	if(unsigned(i)>=sizeof(buf)||unsigned(i+2)>=sizeof(buf))
		GIF_ERROR("Invalid image data!");

        if (i == j) 
                ret = buf[i];
        else if (i + 1 == j) 
                ret = buf[i] | (buf[i+1] << 8);
        else 
                ret = buf[i] | (buf[i+1] << 8) | (buf[i+2] << 16);

        ret = (ret >> (curbit % 8)) & maskTbl[code_size];

	curbit += code_size;
	
	return ret;
}

#define readLWZ(fd) ((sp > stack) ? *--sp : nextLWZ(fd))

 int nextLWZ(FILE *fd)
{
       int		code, incode;
       register int	i;

       while ((code = nextCode(fd, code_size)) >= 0) {
               if (code == clear_code) {
                       for (i = 0; i < clear_code; ++i) {
                               table[0][i] = 0;
                               table[1][i] = i;
                       }
                       for (; i < (1<<MAX_LWZ_BITS); ++i)
                               table[0][i] = table[1][i] = 0;
                       code_size = set_code_size+1;
                       max_code_size = 2*clear_code;
                       max_code = clear_code+2;
                       sp = stack;
			do {
			       firstcode = oldcode = nextCode(fd, code_size);
			} while (firstcode == clear_code);

			return firstcode;
               }
	       if (code == end_code) {
                       int             count;
                       unsigned char   buf[260];

                       if (ZeroDataBlock)
                               return -2;

                       while ((count = GetDataBlock(fd, buf)) > 0)
                               ;

                       if (count != 0)
					   {
						   INFO_MSG(("missing EOD in data stream"));
					   }
                       return -2;
               }

               incode = code;

#define CHECK_SP(sp) {if(sp>=&stack[sizeof(stack)/sizeof(stack[0])]) GIF_ERROR("");}

               if (code >= max_code) {
				   CHECK_SP(sp);
                       *sp++ = firstcode;
                       code = oldcode;
               }

               while (code >= clear_code) {
				   CHECK_SP(sp);
                       *sp++ = table[1][code];
                       if (code == table[0][code])
                               GIF_ERROR("circular table entry BIG GIF_ERROR");
                       code = table[0][code];
               }

               *sp++ = firstcode = table[1][code];

               if ((code = max_code) <(1<<MAX_LWZ_BITS)) {
                       table[0][code] = oldcode;
                       table[1][code] = firstcode;
                       ++max_code;
                       if ((max_code >= max_code_size) &&
                               (max_code_size < (1<<MAX_LWZ_BITS))) {
                               max_code_size *= 2;
                               ++code_size;
                       }
               }

               oldcode = incode;

               if (sp > stack)
                       return *--sp;
       }
       return code;
}

 void readImage(FILE *fd, int interlace, int width, int height, 
			unsigned char *data)
{
       unsigned char	*dp, c;      
       int		v, xpos = 0, ypos = 0;//, pass = 0;

	/*
	**  Initialize the Compression routines
	*/
	//if (! ReadOK(fd,&c,1)||c!=8) 
	   //Shixian 2007-01-25, fix the bug
	   //can't load gif 
	   if(!ReadOK(fd,&c,1)||c>8)
		GIF_ERROR("EOF / read GIF_ERROR on image data" );

	initLWZ(c);

	if (verbose)
	{
		INFO_MSG(("reading %d by %d%s GIF image",
			width, height, interlace ? " interlaced" : ""));
	}

	if (interlace) {
		int	i;
		int	pass = 0, step = 8;

		for (i = 0; i < height; i++) {
			dp = &data[width * ypos];
			for (xpos = 0; xpos < width; xpos++) {
				if ((v = readLWZ(fd)) < 0)
					goto fini;

				*dp++ = v;
			}
			if ((ypos += step) >= height) {
				do {
					if (pass++ > 0)
						step /= 2;
					ypos = step / 2;
				} while (ypos > height);
			}
		}
	} else {
		dp = data;
		for (ypos = 0; ypos < height; ypos++) {
			for (xpos = 0; xpos < width; xpos++) {
				if ((v = readLWZ(fd)) < 0)
					goto fini;

				*dp++ = v;
			}
		}
	}

fini:
       if (readLWZ(fd) >= 0)
	   {
		   INFO_MSG(("too much input data, ignoring extra..."));
	   }

       return;
}

 };

 

 GIFStream*	GIFReadFP(FILE *fp)
 {
	 GIFStream* pGif=NULL;
	 try
	 {
		 _GIFReadImp imp;
		 pGif=imp.GIFReadFP(fp);
	 }
	 catch(...)
	 {
		 ReleaseFP(pGif);
		 pGif=NULL;
	 }
	 return pGif;
 }
 //GIFStream*	GIFRead(const char * file)
 //{
	//FILE		*fp = fopen(file, "rb");
	//GIFStream	*stream = NULL;

	//if (fp != NULL) 
	//{
	//	stream = GIFReadFP(fp);
	//	fclose(fp);
	//}
	//return stream;
 //}
