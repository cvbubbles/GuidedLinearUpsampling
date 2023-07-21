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
/* | Copyright 1993, David Koblas (koblas@netcom.com)                  | */
/* |                                                                   | */
/* | Permission to use, copy, modify, and to distribute this software  | */
/* | and its documentation for any purpose is hereby granted without   | */
/* | fee, provided that the above copyright notice appear in all       | */
/* | copies and that both that copyright notice and this permission    | */
/* | notice appear in supporting documentation.  There is no           | */
/* | representations about the suitability of this software for        | */
/* | any purpose.  this software is provided "as is" without express   | */
/* | or implied warranty.                                              | */
/* |                                                                   | */
/* +-------------------------------------------------------------------+ */

/* ppmtogif.c - read a portable pixmap and produce a GIF file
**
** Based on GIFENCOD by David Rowley <mgardi@watdscu.waterloo.edu>.A
** Lempel-Zim compression based on "compress".
**
** Copyright (C) 1989 by Jef Poskanzer.
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appear in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software is provided "as is" without express or
** implied warranty.
**
** The Graphics Interchange Format(c) is the Copyright property of
** CompuServe Incorporated.  GIF(sm) is a Service Mark property of
** CompuServe Incorporated.
*/

//#if defined(_MSC_VER)&&defined(_DEBUG)
//
//#define _CRTDBG_MAP_ALLOC
//#include <stdlib.h>
//#include <crtdbg.h>
//
//#endif

#include <stdio.h>
#include <stdlib.h>
#include "gif.h"

#define TRUE 1
#define FALSE 0

#define	PUTBYTE(v, fp)	putc(v, fp)
#define PUTWORD(v, fp)	do { 	\
				putc(((v) & 0xff), fp); \
				putc((((v) >> 8) & 0xff), fp); \
			} while (0)

/*
 * a code_int must be able to hold 2**BITS values of type int, and also -1
 */
typedef int             code_int;

typedef long int          count_int;

 void putImage(FILE *, int, int, int, int, unsigned char *);
 void putColorMap(FILE *, int, unsigned char [GIF_MAXCOLORS][3]);
 void putDataBlocks(FILE *fp, int, unsigned char *);
 void putGif89Info(FILE *, GIF89info *);

 void output ( code_int code );
 void cl_block ( void );
 void cl_hash ( count_int hsize );
 void char_init ( void );
 void char_out ( int c );
 void flush_char ( void );

/*
**
*/
struct cval { 
	int idx, cnt;
};

extern "C"  int cvalCMP(const void *a, const void *b)
{
	return ((cval*)b)->cnt -((cval*)a)->cnt;
}





#include <ctype.h>

/*
 * General DEFINEs
 */

#define BITS    12

#define HSIZE  5003            /* 80% occupancy */

#ifdef NO_UCHAR
 typedef char   char_type;
#else /*NO_UCHAR*/
 typedef        unsigned char   char_type;
#endif /*NO_UCHAR*/

#define ARGVAL() (*++(*argv) || (--argc && *++argv))

#define tab_prefixof(i) CodeTabOf(i)
#define tab_suffixof(i)        ((char_type*)(htab))[i]
#define de_stack               ((char_type*)&tab_suffixof((code_int)1<<BITS))

#define MAXCODE(n_bits)        	(((code_int) 1 << (n_bits)) - 1)

 #define HashTabOf(i)       htab[i]
#define CodeTabOf(i)    codetab[i]


 /*****************************************************************
 * TAG( output )
 *
 * Output the given code.
 * Inputs:
 *      code:   A n_bits-bit integer.  If == -1, then EOF.  This assumes
 *              that n_bits =< (long)wordsize - 1.
 * Outputs:
 *      Outputs code to the file.
 * Assumptions:
 *      Chars are 8 bits long.
 * Algorithm:
 *      Maintain a BITS character long buffer (so that 8 codes will
 * fit in it exactly).  Use the VAX insv instruction to insert each
 * code in turn.  When the buffer fills up empty it and start over.
 */

 const unsigned long masks[] = {  0x0000, 
				  0x0001, 0x0003, 0x0007, 0x000F,
                                  0x001F, 0x003F, 0x007F, 0x00FF,
                                  0x01FF, 0x03FF, 0x07FF, 0x0FFF,
                                  0x1FFF, 0x3FFF, 0x7FFF, 0xFFFF };


struct _GIFWriteImp
{
private:
 int n_bits;                      /* number of bits/code */
 int maxbits;                	/* user settable max # bits/code */
 code_int maxcode;                /* maximum code, given n_bits */
 code_int maxmaxcode; 		/* should NEVER generate this code */

 count_int htab [HSIZE];
 unsigned short codetab [HSIZE];

 code_int hsize;                 	/* for dynamic table sizing */

 unsigned long cur_accum;
 int cur_bits;

/*
 * To save much memory, we overlay the table used by compress() with those
 * used by decompress().  The tab_prefix table is the same size and type
 * as the codetab.  The tab_suffix table needs 2**BITS characters.  We
 * get this from the beginning of htab.  The output stack uses the rest
 * of htab, and contains characters.  There is plenty of room for any
 * possible stack (stack used to be 8000 characters).
 */

 code_int free_ent;              /* first unused entry */

/*
 * block compression parameters -- after all codes are used up,
 * and compression rate changes, start over.
 */
 int clear_flg;

 int offset;

/*
 * compress stdin to stdout
 *
 * Algorithm:  use open addressing double hashing (no chaining) on the
 * prefix code / next character combination.  We do a variant of Knuth's
 * algorithm D (vol. 3, sec. 6.4) along with G. Knott's relatively-prime
 * secondary probe.  Here, the modular division first probe is gives way
 * to a faster exclusive-or manipulation.  Also do block compression with
 * an adaptive reset, whereby the code table is cleared when the compression
 * ratio decreases, but after the table fills.  The variable-length output
 * codes are re-sized at this point, and a special CLEAR code is generated
 * for the decompressor.  Late addition:  construct the table according to
 * file size for noticeable speed improvement on small files.  Please direct
 * questions about this implementation to ames!jaw.
 */

 int g_init_bits;
 FILE* g_outfile;

 int ClearCode;
 int EOFCode;


/*
** Number of characters so far in this 'packet'
*/
 int a_count;

/*
** Define the storage for the packet accumulator
*/
 char accum[256];

public:


 int optimizeCMAP(GIFStream *stream)
{
	GIFData	*cur=NULL, *img=NULL;
	int	count = 0;

	for (cur = stream->data; cur != NULL; cur = cur->next) {
		if (cur->type == gif_image) {
			img = cur;
			count++;
		}
	}
	/*
	**  No images, no optimizations...
	**   or too many images...
	*/
	if (count == 0 || count > 1)
		return 0;
	
	/*
	**  One image, nice and simple...
	**    Insure there is a global colormap, and optimize the
	**    image too it.
	*/
	{
		int		size;
		unsigned char	*dp = img->data.image.data;
		unsigned char	*ep = dp + img->width * img->height;
		struct cval 	vals[256];
		int		i;
		unsigned char	tmap[256][3], rmap[256];


		if ((size = img->data.image.cmapSize) == 0)
			size = stream->cmapSize;

		for (i = 0; i < size; i++) {
			vals[i].idx = i;
			vals[i].cnt = 0;
		}
		for (dp = img->data.image.data, i = 0; dp < ep; i++, dp++)
			vals[*dp].cnt++;
		
		/*
		**  Quite, I'm doing a bubble sort... ACK!
		*/
		qsort(vals, size, sizeof(vals[0]), cvalCMP);

		for (i = 0; i < size; i++) 
			if (vals[i].idx != i)
				break;
		/*
		**  Already sorted, no change!
		*/
		if (i == size)
			return 1;
		for (i = 0; i < size; i++) 
			rmap[vals[i].idx] = i;

		/*
		**  Now reorder the colormap, and the image
		*/
		for (dp = img->data.image.data, i = 0; dp < ep; i++, dp++)
			*dp = rmap[*dp];
		if (img->info.transparent != -1)
			img->info.transparent = rmap[img->info.transparent];
		/*
		**  Toast the local colormap
		*/
		if (img->data.image.cmapSize != 0) {
			for (i = 0; i < size; i++) {
				stream->cmapData[i][0] = 
						img->data.image.cmapData[i][0];
				stream->cmapData[i][1] = 
						img->data.image.cmapData[i][1];
				stream->cmapData[i][2] = 
						img->data.image.cmapData[i][2];
			}
			img->data.image.cmapSize = 0;
			stream->cmapSize = size;
		}

		/*
		**  Now finally reorer the colormap
		*/
		for (i = 0; i < size; i++) {
			tmap[i][0] = stream->cmapData[i][0];
			tmap[i][1] = stream->cmapData[i][1];
			tmap[i][2] = stream->cmapData[i][2];
		}
		for (i = 0; i < size; i++) {
			stream->cmapData[rmap[i]][0] = tmap[i][0];
			stream->cmapData[rmap[i]][1] = tmap[i][1];
			stream->cmapData[rmap[i]][2] = tmap[i][2];
		}
	}
	return 1;
}


/*
**  Return the ceiling log of n 
*/
 int	binaryLog(int val)
{
	int	i;

	if (val == 0)
		return 0;

	for (i = 1; i <= 8; i++)
		if (val <= (1 << i))
			return i;
	return 8;
}

int	GIFWriteFP(FILE *fp, GIFStream *stream, int optimize)
{
	GIFData	*cur;
	int	flag = FALSE;
	int	c;
	int	globalBitsPP = 0;
	int	resolution;

	if (fp == NULL)
		return TRUE;
	if (stream == NULL)
		return FALSE;

	/*
	**  First find if this is a 87A or an 89A GIF image
	**    also, figure out the color resolution of the image.
	*/
	resolution = binaryLog(stream->cmapSize) - 1;
	for (cur = stream->data; !flag && cur != NULL; cur = cur->next) {
		if (cur->type == gif_text || cur->type == gif_comment) {
			flag = TRUE;
		} else if (cur->type == gif_image) {
			int	v = binaryLog(cur->data.image.cmapSize);

			if (v > resolution)
				resolution = v;

			/*
			**  Uses one of the 89 extensions.
			*/
			if (cur->info.transparent != -1 ||
			    cur->info.delayTime   != 0 ||
			    cur->info.inputFlag   != 0 ||
			    cur->info.disposal    != 0)
				flag = TRUE;
		}
	}
	/*
	**
	*/
	if (optimize) 
		optimize = optimizeCMAP(stream);

	fwrite(flag ? "GIF89a" : "GIF87a", 1, 6, fp);

	PUTWORD(stream->width,  fp);
	PUTWORD(stream->height, fp);

	/* 
	** assume 256 entry color resution, and non sorted colormap 
	*/
	c = ((resolution & 0x07) << 5) | 0x00;	
	if (stream->cmapSize != 0) {
		globalBitsPP = binaryLog(stream->cmapSize);
		c |= 0x80;
		c |= globalBitsPP - 1;
	}
	/*
	**  Is the global colormap optimized?
	*/
	if (optimize)
		c |= 0x08;
	PUTBYTE(c, fp);

	PUTBYTE(stream->background, fp);
	PUTBYTE(stream->aspectRatio, fp);

	putColorMap(fp, stream->cmapSize, stream->cmapData);

	//enable loop
	{
		static unsigned char byChar[19] = { 0x21, 0xFF, 0x0B, 'N', 'E', 'T', 'S', 'C', 'A', 'P', 'E', '2', '.', '0', 0x03, 0x01, 0x00, 0x00, 0x00 };
		fwrite(byChar, sizeof(byChar), 1, fp);
	}

	for (cur = stream->data; cur != NULL; cur = cur->next) {
		if (cur->type == gif_image) {
			int	bpp;

			putGif89Info(fp, &cur->info);

			PUTBYTE(0x2c, fp);
			PUTWORD(cur->x, fp);
			PUTWORD(cur->y, fp);
			PUTWORD(cur->width, fp);
			PUTWORD(cur->height, fp);

			c = cur->data.image.interlaced ? 0x40 : 0x00;
			if (cur->data.image.cmapSize != 0) {
				bpp = binaryLog(cur->data.image.cmapSize);
				c |= 0x80;
				c |= bpp;
			} else {
				bpp = globalBitsPP;
			}

			PUTBYTE(c, fp);

			putColorMap(fp, cur->data.image.cmapSize,
					cur->data.image.cmapData);

			putImage(fp, cur->data.image.interlaced, bpp,
					cur->width, cur->height, 
					cur->data.image.data);
		} else if (cur->type == gif_comment) {
			PUTBYTE('!', fp);
			PUTBYTE(0xfe, fp);
			putDataBlocks(fp, cur->data.comment.len, 
					(unsigned char*)cur->data.comment.text);
		} else if (cur->type == gif_text) {
			putGif89Info(fp, &cur->info);

			PUTBYTE('!', fp);
			PUTBYTE(0x01, fp);

			PUTWORD(cur->x, fp);
			PUTWORD(cur->y, fp);
			PUTWORD(cur->width, fp);
			PUTWORD(cur->height, fp);

			PUTBYTE(cur->data.text.cellWidth, fp);
			PUTBYTE(cur->data.text.cellHeight, fp);
			PUTBYTE(cur->data.text.fg, fp);
			PUTBYTE(cur->data.text.bg, fp);

			putDataBlocks(fp, cur->data.text.len, 
					(unsigned char*)cur->data.text.text);
		}
	}

	/*
	**  Write termination
	*/
	PUTBYTE(';', fp);

	return FALSE;
}
 void putColorMap(FILE *fp, int size, unsigned char data[GIF_MAXCOLORS][3])
{
	int	i;

	for (i = 0; i < size; i++) {
		PUTBYTE(data[i][0], fp);
		PUTBYTE(data[i][1], fp);
		PUTBYTE(data[i][2], fp);
	}
}

 void putDataBlocks(FILE *fp, int size, unsigned char *data)
{
	int	n;

	while (size > 0) {
		n = size > 255 ? 255 : size;

		PUTBYTE(n, fp);
		fwrite(data, 1, n, fp);
		data += n;
		size -= n;
	}
	PUTBYTE(0, fp);	/* End Block */
}

 void putGif89Info(FILE *fp, GIF89info *info)
{
	unsigned char	c;

	if (info->transparent == -1 &&
	    info->delayTime == 0 &&
	    info->inputFlag == 0 &&
	    info->disposal == 0)
		return;
	
	PUTBYTE('!', fp);
	PUTBYTE(0xf9, fp);
	PUTBYTE(4, fp);
	c = (info->inputFlag ? 0x02 : 0x00) |
	    ((info->disposal & 0x07) << 2) |
	    ((info->transparent != -1) ? 0x01 : 0x00);
	PUTBYTE(c, fp);
	PUTWORD(info->delayTime, fp);
	PUTBYTE(info->transparent, fp);

	/*
	**  End
	*/
	PUTBYTE(0, fp);
}


/***************************************************************************
 *
 *  GIFCOMPR.C       - GIF Image compression routines
 *
 *  Lempel-Ziv compression based on 'compress'.  GIF modifications by
 *  David Rowley (mgardi@watdcsu.waterloo.edu)
 *
 ***************************************************************************/


/*
 *
 * GIF Image compression - modified 'compress'
 *
 * Based on: compress.c - File compression ala IEEE Computer, June 1984.
 *
 * By Authors:  Spencer W. Thomas       (decvax!harpo!utah-cs!utah-gr!thomas)
 *              Jim McKie               (decvax!mcvax!jim)
 *              Steve Davies            (decvax!vax135!petsd!peora!srd)
 *              Ken Turkowski           (decvax!decwrl!turtlevax!ken)
 *              James A. Woods          (decvax!ihnp4!ames!jaw)
 *              Joe Orost               (decvax!vax135!petsd!joe)
 *
 */

 void putImage(FILE *fp, int interlaced, int bpp, int width, int height, 
			unsigned char *data)
{
//	unsigned char	*end = data + width * height;
	int		left = interlaced ? width : width * height;
	int		cury = 0, pass = 0;
	unsigned char	*dp = data;
	long		fcode;
	code_int	v, i, ent, disp, hsize_reg;
	int		c, hshift;
//	int		skip = 8;

	if (bpp <= 1) {
		g_init_bits = 3;
		PUTBYTE(2, fp);
	} else {
		g_init_bits = bpp + 1;
		PUTBYTE(bpp, fp);
	}

	/*
	** Set up the globals:  g_init_bits - initial number of bits
	**                      g_outfile   - pointer to output file
	*/
	g_outfile   = fp;

	/*
	** Set up the necessary values
	*/
	offset = 0;
	clear_flg = FALSE;
	maxbits = BITS;
	maxmaxcode = 1 << BITS;
	maxcode = MAXCODE(n_bits = g_init_bits);
	hsize = HSIZE;
	cur_accum = 0;
	cur_bits  = 0;

	ClearCode = (1 << (g_init_bits - 1));
	EOFCode   = ClearCode + 1;
	free_ent  = ClearCode + 2;

	char_init();	/* clear the output accumulator */

	hshift = 0;
	for (fcode = (long)hsize;  fcode < 65536; fcode *= 2)
		++hshift;
	hshift = 8 - hshift;                /* set hash code range bound */

	hsize_reg = hsize;
	cl_hash((count_int)hsize);            /* clear hash table */

	output((code_int)ClearCode);

	ent = *dp++;
	do {
again:
		/*
		**  Fetch the next pixel
		*/
		c = *dp++;
		if (--left == 0) {
			if (interlaced) {
				do {
					switch (pass) {
					case 0:
						cury += 8;
						if (cury >= height) {
							pass++;
							cury = 4;
						}
						break;
					case 1:
						cury += 8;
						if (cury >= height) {
							pass++;
							cury = 2;
						}
						break;
					case 2:
						cury += 4;
						if (cury >= height) {
							pass++;
							cury = 1;
						}
						break;
					case 3:
						cury += 2;
						break;
					}
				} while (pass < 3 && cury >= height);
				if (cury >= height)
					goto done;
				dp = data + cury * width;
				left = width;
				c = *dp++;
			} else {
				goto done;
			}
		}


		/*
		**  Now output it...
		*/

		fcode = (long) (((long) c << maxbits) + ent);

		i = (((code_int)c << hshift) ^ ent);	/* xor hashing */
		v = HashTabOf(i);

		if (v == fcode) {
			ent = CodeTabOf (i);
			goto again;
		} else if (v >= 0) {
			/* 
			** secondary hash (after G. Knott) 
			*/
			disp = hsize_reg - i;           
			if (i == 0)
				disp = 1;
			do {
				if ((i -= disp) < 0)
					i += hsize_reg;
			
				v = HashTabOf(i);
				if (v == fcode) {
					ent = CodeTabOf(i);
					goto again;
				}
			} while (v > 0);
		}

		output((code_int)ent);
		ent = c;
		if (free_ent < maxmaxcode) {
			CodeTabOf(i) = free_ent++; /* code -> hashtable */
			HashTabOf(i) = fcode;
		} else {
			cl_block();
		}
	} while (1);
done:
	/*
	** Put out the final code.
	**/
	output((code_int)ent);
	output((code_int)EOFCode);

	/*
	**  End block byte
	*/
	PUTBYTE(0x00, fp);
}


 void output(code_int code)
{
	cur_accum &= masks[ cur_bits ];

	if( cur_bits > 0 )
		cur_accum |= ((long)code << cur_bits);
	else
		cur_accum = code;

	cur_bits += n_bits;

	while( cur_bits >= 8 ) {
		char_out( (unsigned int)(cur_accum & 0xff) );
		cur_accum >>= 8;
		cur_bits -= 8;
	}

	/*
	** If the next entry is going to be too big for the code size,
	** then increase it, if possible.
	*/
	if (free_ent > maxcode || clear_flg) {
		if( clear_flg ) {
			maxcode = MAXCODE (n_bits = g_init_bits);
			clear_flg = FALSE;
		} else {
			++n_bits;
			if ( n_bits == maxbits )
				maxcode = maxmaxcode;
			else
				maxcode = MAXCODE(n_bits);
		}
        }

	if (code == EOFCode) {
		/*
		** At EOF, write the rest of the buffer.
		*/
		while (cur_bits > 0) {
			char_out((unsigned int)(cur_accum & 0xff));
			cur_accum >>= 8;
			cur_bits -= 8;
		}

		flush_char();

		fflush(g_outfile);
	}
}

/*
 * Clear out the hash table
 */
 void cl_block()
{

        cl_hash((count_int)hsize);
        free_ent = ClearCode + 2;
        clear_flg = TRUE;

        output((code_int)ClearCode);
}

 void cl_hash(count_int hsize)          /* reset code table */
{
	int	i;

	for (i = 0; i < hsize; i++) 
		htab[i] = -1;
}

/******************************************************************************
 *
 * GIF Specific routines
 *
 ******************************************************************************/



/*
** Set up the 'byte output' routine
*/
 void char_init()
{
        a_count = 0;
}

/*
** Add a character to the end of the current packet, and if it is 254
** characters, flush the packet to disk.
*/
 void char_out(int c)
{
        accum[a_count++] = c;
        if (a_count == 255)
                flush_char();
}

/*
** Flush the packet to disk, and reset the accumulator
*/
 void flush_char()
{
        if (a_count != 0) {
                PUTBYTE(a_count, g_outfile);
                fwrite(accum, 1, a_count, g_outfile);
                a_count = 0;
        }
}

}; //end of impl...;

int		GIFWriteFP(FILE * fp, GIFStream * pGif, int optimize)
{
	int fail=TRUE;
	
	try
	{
		_GIFWriteImp imp;
		fail=imp.GIFWriteFP(fp,pGif,optimize);
	}
	catch(...)
	{
		fail=TRUE;
	}
	return fail;
}

//int	GIFWrite(char *file, GIFStream *stream, int optimize)
//{
//	if (stream != NULL) {
//		FILE	*fp = fopen(file, "wb");
//
//		if (fp != NULL) {
//			int	s = GIFWriteFP(fp, stream, optimize);
//			fclose(fp);
//			return s;
//		}
//	}
//	return TRUE;
//}

