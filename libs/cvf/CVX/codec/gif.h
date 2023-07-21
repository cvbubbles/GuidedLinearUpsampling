
#pragma once

#ifdef __cplusplus
extern "C"{
#endif

#define GIF_MAXCOLORS	256

typedef enum {
	gif_image, gif_comment, gif_text
} GIFStreamType;

typedef enum {
	gif_no_disposal = 0, gif_keep_disposal = 1, 
	gif_color_restore = 2, gif_image_restore = 3
} GIFDisposalType;

typedef struct {
	int		transparent;	/* transparency index */
	int		delayTime;	/* Time in 1/100 of a second */
	int		inputFlag;	/* wait for input after display */
	GIFDisposalType	disposal;
} GIF89info;

typedef struct GIFData {
	GIF89info	info;
	int		x, y;
	int		width, height;
	GIFStreamType	type;
	union {
		struct {
			int		cmapSize;
			unsigned char	cmapData[GIF_MAXCOLORS][3];
			unsigned char	*data;
			int		interlaced;
		} image;
		struct {
			int	fg, bg;
			int	cellWidth, cellHeight;
			int	len;
			char	*text;
		} text;
		struct {
			int	len;
			char	*text;
		} comment;
	} data;

	struct GIFData	*next;
} GIFData;

typedef struct {
	int		width, height;

	int		colorResolution;
	int		colorMapSize;
	int		cmapSize;
	unsigned char	cmapData[GIF_MAXCOLORS][3];

	int		background;
	int		aspectRatio;

	GIFData		*data;
} GIFStream;

GIFStream*  GIFReadFP(FILE *);

int		GIFWriteFP(FILE *, GIFStream *, int);


#ifdef __cplusplus
}//
#endif

