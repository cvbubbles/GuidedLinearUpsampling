/*
  Based upon a GIMP filter ("to-indexed.c"):
  "The GIMP -- an image manipulation program
   Copyright (C) 1995 Spencer Kimball and Peter Mattis"

  "This filter takes an rgb input image and creates a new
  indexed color image."

  median_cut, 2-pass, floyd no/yes
  char *out = to_indexed(input_image, num, dither, xsize, ysize, &colormap);

  Benny:
  v0.2 - Win95 GPF fixed.
  v0.3 - weeded out some functions.
*/

#if defined(_MSC_VER)&&defined(_DEBUG)

//#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>

#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <memory.h>


#include "fsdither.h" // tables:
/* 'range_array' 'floyd_steinberg_error1' 'floyd_steinberg_error2'
 'floyd_steinberg_error3' 'floyd_steinberg_error4'*/

#define MAXNUMCOLORS 256

/* dither type */
#define NODITHER 0
#define FSDITHER 1

#define PRECISION_R 	6
#define PRECISION_G 	6
//#define PRECISION_B 	6	// worse
#define PRECISION_B 	5       // ori

#if 0
#define R_SCALE
#define G_SCALE
#define B_SCALE
#else
/* scale RGB distances by *2,*3,*1 */
#define R_SCALE  	<< 1
#define G_SCALE  	* 3
#define B_SCALE
#endif


#define HIST_R_ELEMS 	(1<<PRECISION_R)
#define HIST_G_ELEMS 	(1<<PRECISION_G)
#define HIST_B_ELEMS 	(1<<PRECISION_B)

#define MR 		HIST_G_ELEMS*HIST_B_ELEMS
#define MG 		HIST_B_ELEMS

#define BITS_IN_SAMPLE 	8

#define R_SHIFT  	(BITS_IN_SAMPLE - PRECISION_R)
#define G_SHIFT  	(BITS_IN_SAMPLE - PRECISION_G)
#define B_SHIFT  	(BITS_IN_SAMPLE - PRECISION_B)

typedef struct _Color Color;
typedef struct _QuantizeObj QuantizeObj;
typedef void    (*Pass_Func) (QuantizeObj *, unsigned char *, unsigned char *, long, long);
//typedef void    (*Cleanup_Func) (QuantizeObj *);
typedef unsigned long ColorFreq;
typedef ColorFreq *Histogram;

struct _Color {
    int             red;
    int             green;
    int             blue;
};

struct _QuantizeObj {
    Pass_Func       first_pass;	/*  first pass over image data creates colormap  */
    Pass_Func       second_pass;	/*  second pass maps from image data to colormap */
//    Cleanup_Func    delete_func;	/*  function to clean up data associated with private */
//    int             bpp;	/*  bytes per pixel (grayscale vs rgb)           */
    int             desired_number_of_colors;	/*  Number of colors we will allow               */
    int             actual_number_of_colors;	/*  Number of colors actually needed             */
    Color           cmap[256];	/*  colormap created by quantization             */
    Histogram       histogram;	/*  holds the histogram                          */
};

typedef struct {
    /* The bounds of the box (inclusive); expressed as histogram indexes */
    int             Rmin, Rmax;
    int             Gmin, Gmax;
    int             Bmin, Bmax;
    /* The volume (actually 2-norm) of the box */
    int             volume;
    /* The number of nonzero histogram cells within this box */
    long            colorcount;
} box          , *boxptr;

typedef struct {
    long            ncolors;
    long            dither;
} Options;

void error(const char*)
{
}

/*
static void    *xmalloc(unsigned long   size)
{
    void           *p;
    p = malloc(size);
    if (!p)	error("could not xmallocate memory");
    return p;
}
*/

static void     xfree(void *p)
{
    if (p)
	free(p);
}

static void     zero_histogram_rgb(Histogram       histogram)
{
    int             r, g, b;
    for (r = 0; r < HIST_R_ELEMS; r++)
	for (g = 0; g < HIST_G_ELEMS; g++)
	    for (b = 0; b < HIST_B_ELEMS; b++)
		histogram[r * MR + g * MG + b] = 0;
}

static void     generate_histogram_rgb(Histogram histogram,unsigned char *src,long width,long height)
{
    int             num_elems;
    ColorFreq      *col;

    num_elems = width * height;
    zero_histogram_rgb(histogram);

    while (num_elems--) {
	col = &histogram[(src[0] >> R_SHIFT) * MR +
			 (src[1] >> G_SHIFT) * MG +
			 (src[2] >> B_SHIFT)];
	(*col)++;
	src += 3;
    }
}

static          boxptr
                find_biggest_color_pop(boxptr boxlist,int numboxes)
/* Find the splittable box with the largest color population */
/* Returns 0 if no splittable boxes remain */
{
    boxptr          boxp;
    int             i;
    long            maxc = 0;
    boxptr          which = 0;

    for (i = 0, boxp = boxlist; i < numboxes; i++, boxp++) {
	if (boxp->colorcount > maxc && boxp->volume > 0) {
	    which = boxp;
	    maxc = boxp->colorcount;
	}
    }

    return which;
}


static          boxptr
                find_biggest_volume(boxptr boxlist,int numboxes)
/* Find the splittable box with the largest (scaled) volume */
/* Returns 0 if no splittable boxes remain */
{
    boxptr          boxp;
    int             i;
    int             maxv = 0;
    boxptr          which = 0;

    for (i = 0, boxp = boxlist; i < numboxes; i++, boxp++) {
	if (boxp->volume > maxv) {
	    which = boxp;
	    maxv = boxp->volume;
	}
    }

    return which;
}


static void     update_box_rgb(Histogram histogram,boxptr boxp)
/* Shrink the min/max bounds of a box to enclose only nonzero elements, */
/* and recompute its volume and population */
{
    ColorFreq      *histp;
    int             R, G, B;
    int             Rmin, Rmax, Gmin, Gmax, Bmin, Bmax;
    int             dist0, dist1, dist2;
    long            ccount;

    Rmin = boxp->Rmin;
    Rmax = boxp->Rmax;
    Gmin = boxp->Gmin;
    Gmax = boxp->Gmax;
    Bmin = boxp->Bmin;
    Bmax = boxp->Bmax;

    if (Rmax > Rmin)
	for (R = Rmin; R <= Rmax; R++)
	    for (G = Gmin; G <= Gmax; G++) {
		histp = histogram + R * MR + G * MG + Bmin;
		for (B = Bmin; B <= Bmax; B++)
		    if (*histp++ != 0) {
			boxp->Rmin = Rmin = R;
			goto have_Rmin;
		    }
	    }
  have_Rmin:
    if (Rmax > Rmin)
	for (R = Rmax; R >= Rmin; R--)
	    for (G = Gmin; G <= Gmax; G++) {
		histp = histogram + R * MR + G * MG + Bmin;
		for (B = Bmin; B <= Bmax; B++)
		    if (*histp++ != 0) {
			boxp->Rmax = Rmax = R;
			goto have_Rmax;
		    }
	    }
  have_Rmax:
    if (Gmax > Gmin)
	for (G = Gmin; G <= Gmax; G++)
	    for (R = Rmin; R <= Rmax; R++) {
		histp = histogram + R * MR + G * MG + Bmin;
		for (B = Bmin; B <= Bmax; B++)
		    if (*histp++ != 0) {
			boxp->Gmin = Gmin = G;
			goto have_Gmin;
		    }
	    }
  have_Gmin:
    if (Gmax > Gmin)
	for (G = Gmax; G >= Gmin; G--)
	    for (R = Rmin; R <= Rmax; R++) {
		histp = histogram + R * MR + G * MG + Bmin;
		for (B = Bmin; B <= Bmax; B++)
		    if (*histp++ != 0) {
			boxp->Gmax = Gmax = G;
			goto have_Gmax;
		    }
	    }
  have_Gmax:
    if (Bmax > Bmin)
	for (B = Bmin; B <= Bmax; B++)
	    for (R = Rmin; R <= Rmax; R++) {
		histp = histogram + R * MR + Gmin * MG + B;
		for (G = Gmin; G <= Gmax; G++, histp += MG)
		    if (*histp != 0) {
			boxp->Bmin = Bmin = B;
			goto have_Bmin;
		    }
	    }
  have_Bmin:
    if (Bmax > Bmin)
	for (B = Bmax; B >= Bmin; B--)
	    for (R = Rmin; R <= Rmax; R++) {
		histp = histogram + R * MR + Gmin * MG + B;
		for (G = Gmin; G <= Gmax; G++, histp += MG)
		    if (*histp != 0) {
			boxp->Bmax = Bmax = B;
			goto have_Bmax;
		    }
	    }
  have_Bmax:

    /* Update box volume.
     * We use 2-norm rather than real volume here; this biases the method
     * against making long narrow boxes, and it has the side benefit that
     * a box is splittable iff norm > 0.
     * Since the differences are expressed in histogram-cell units,
     * we have to shift back to JSAMPLE units to get consistent distances;
     * after which, we scale according to the selected distance scale factors.
     */
    dist0 = ((Rmax - Rmin) << R_SHIFT) R_SCALE;
    dist1 = ((Gmax - Gmin) << G_SHIFT) G_SCALE;
    dist2 = ((Bmax - Bmin) << B_SHIFT) B_SCALE;
    boxp->volume = dist0 * dist0 + dist1 * dist1 + dist2 * dist2;

    /* Now scan remaining volume of box and compute population */
    ccount = 0;
    for (R = Rmin; R <= Rmax; R++)
	for (G = Gmin; G <= Gmax; G++) {
	    histp = histogram + R * MR + G * MG + Bmin;
	    for (B = Bmin; B <= Bmax; B++, histp++)
		if (*histp != 0) {
		    ccount++;
		}
	}

    boxp->colorcount = ccount;
}


static int      median_cut_rgb(Histogram histogram,boxptr boxlist,int numboxes,int desired_colors)
/* Repeatedly select and split the largest box until we have enough boxes */
{
    int             n, lb;
    int             R, G, B, cmax;
    boxptr          b1, b2;

    while (numboxes < desired_colors) {
	/* Select box to split.
	 * Current algorithm: by population for first half, then by volume.
	 */
	if (numboxes * 2 <= desired_colors) {
	    b1 = find_biggest_color_pop(boxlist, numboxes);
	} else {
	    b1 = find_biggest_volume(boxlist, numboxes);
	}

	if (b1 == 0)		/* no splittable boxes left! */
	    break;
	b2 = boxlist + numboxes;	/* where new box will go */
	/* Copy the color bounds to the new box. */
	b2->Rmax = b1->Rmax;
	b2->Gmax = b1->Gmax;
	b2->Bmax = b1->Bmax;
	b2->Rmin = b1->Rmin;
	b2->Gmin = b1->Gmin;
	b2->Bmin = b1->Bmin;
	/* Choose which axis to split the box on.
	 * Current algorithm: longest scaled axis.
	 * See notes in update_box about scaling distances.
	 */
	R = ((b1->Rmax - b1->Rmin) << R_SHIFT) R_SCALE;
	G = ((b1->Gmax - b1->Gmin) << G_SHIFT) G_SCALE;
	B = ((b1->Bmax - b1->Bmin) << B_SHIFT) B_SCALE;
	/* We want to break any ties in favor of green, then red, blue last.
	 */
	cmax = G;
	n = 1;
	if (R > cmax) {
	    cmax = R;
	    n = 0;
	}
	if (B > cmax) {
	    n = 2;
	}
	/* Choose split point along selected axis, and update box bounds.
	 * Current algorithm: split at halfway point.
	 * (Since the box has been shrunk to minimum volume,
	 * any split will produce two nonempty subboxes.)
	 * Note that lb value is max for lower box, so must be < old max.
	 */
	switch (n) {
	case 0:
	    lb = (b1->Rmax + b1->Rmin) / 2;
	    b1->Rmax = lb;
	    b2->Rmin = lb + 1;
	    break;
	case 1:
	    lb = (b1->Gmax + b1->Gmin) / 2;
	    b1->Gmax = lb;
	    b2->Gmin = lb + 1;
	    break;
	case 2:
	    lb = (b1->Bmax + b1->Bmin) / 2;
	    b1->Bmax = lb;
	    b2->Bmin = lb + 1;
	    break;
	}
	/* Update stats for boxes */
	update_box_rgb(histogram, b1);
	update_box_rgb(histogram, b2);
	numboxes++;
    }
    return numboxes;
}


static void     compute_color_rgb(QuantizeObj *quantobj,Histogram histogram,boxptr boxp,int icolor)
/* Compute representative color for a box, put it in colormap[icolor] */
{
    /* Current algorithm: mean weighted by pixels (not colors) */
    /* Note it is important to get the rounding correct! */
    ColorFreq      *histp;
    int             R, G, B;
    int             Rmin, Rmax;
    int             Gmin, Gmax;
    int             Bmin, Bmax;
    long            count;
    long            total = 0;
    long            Rtotal = 0;
    long            Gtotal = 0;
    long            Btotal = 0;

    Rmin = boxp->Rmin;
    Rmax = boxp->Rmax;
    Gmin = boxp->Gmin;
    Gmax = boxp->Gmax;
    Bmin = boxp->Bmin;
    Bmax = boxp->Bmax;

    for (R = Rmin; R <= Rmax; R++)
	for (G = Gmin; G <= Gmax; G++) {
	    histp = histogram + R * MR + G * MG + Bmin;
	    for (B = Bmin; B <= Bmax; B++) {
		if ((count = *histp++) != 0) {
		    total += count;
		    Rtotal += ((R << R_SHIFT) + ((1 << R_SHIFT) >> 1)) * count;
		    Gtotal += ((G << G_SHIFT) + ((1 << G_SHIFT) >> 1)) * count;
		    Btotal += ((B << B_SHIFT) + ((1 << B_SHIFT) >> 1)) * count;
		}
	    }
	}

    quantobj->cmap[icolor].red = (Rtotal + (total >> 1)) / total;
    quantobj->cmap[icolor].green = (Gtotal + (total >> 1)) / total;
    quantobj->cmap[icolor].blue = (Btotal + (total >> 1)) / total;
}


static void     select_colors_rgb(QuantizeObj *quantobj,Histogram histogram)
/* Master routine for color selection */
{
    boxptr          boxlist;
    int             numboxes;
    int             desired = quantobj->desired_number_of_colors;
    int             i;

    /* Allocate workspace for box list */
    boxlist = (boxptr) malloc(desired * sizeof(box));
    if (!boxlist) error("could not alloc mem");

    /* Initialize one box containing whole space */
    numboxes = 1;
    boxlist[0].Rmin = 0;
    boxlist[0].Rmax = (1 << PRECISION_R) - 1;
    boxlist[0].Gmin = 0;
    boxlist[0].Gmax = (1 << PRECISION_G) - 1;
    boxlist[0].Bmin = 0;
    boxlist[0].Bmax = (1 << PRECISION_B) - 1;
    /* Shrink it to actually-used volume and set its statistics */
    update_box_rgb(histogram, boxlist);
    /* Perform median-cut to produce final box list */
    numboxes = median_cut_rgb(histogram, boxlist, numboxes, desired);
    quantobj->actual_number_of_colors = numboxes;
    /* Compute the representative color for each box, fill colormap */
    for (i = 0; i < numboxes; i++)
	compute_color_rgb(quantobj, histogram, boxlist + i, i);
	
	free(boxlist);
}


/*
 * These routines are concerned with the time-critical task of mapping input
 * colors to the nearest color in the selected colormap.
 *
 * We re-use the histogram space as an "inverse color map", essentially a
 * cache for the results of nearest-color searches.  All colors within a
 * histogram cell will be mapped to the same colormap entry, namely the one
 * closest to the cell's center.  This may not be quite the closest entry to
 * the actual input color, but it's almost as good.  A zero in the cache
 * indicates we haven't found the nearest color for that cell yet; the array
 * is cleared to zeroes before starting the mapping pass.  When we find the
 * nearest color for a cell, its colormap index plus one is recorded in the
 * cache for future use.  The pass2 scanning routines call fill_inverse_cmap
 * when they need to use an unfilled entry in the cache.
 *
 * Our method of efficiently finding nearest colors is based on the "locally
 * sorted search" idea described by Heckbert and on the incremental distance
 * calculation described by Spencer W. Thomas in chapter III.1 of Graphics
 * Gems II (James Arvo, ed.  Academic Press, 1991).  Thomas points out that
 * the distances from a given colormap entry to each cell of the histogram can
 * be computed quickly using an incremental method: the differences between
 * distances to adjacent cells themselves differ by a constant.  This allows a
 * fairly fast implementation of the "brute force" approach of computing the
 * distance from every colormap entry to every histogram cell.  Unfortunately,
 * it needs a work array to hold the best-distance-so-far for each histogram
 * cell (because the inner loop has to be over cells, not colormap entries).
 * The work array elements have to be ints, so the work array would need
 * 256Kb at our recommended precision.  This is not feasible in DOS machines.

[ 256*1024/4 = 65,536 ]

 * To get around these problems, we apply Thomas' method to compute the
 * nearest colors for only the cells within a small subbox of the histogram.
 * The work array need be only as big as the subbox, so the memory usage
 * problem is solved.  Furthermore, we need not fill subboxes that are never
 * referenced in pass2; many images use only part of the color gamut, so a
 * fair amount of work is saved.  An additional advantage of this
 * approach is that we can apply Heckbert's locality criterion to quickly
 * eliminate colormap entries that are far away from the subbox; typically
 * three-fourths of the colormap entries are rejected by Heckbert's criterion,
 * and we need not compute their distances to individual cells in the subbox.
 * The speed of this approach is heavily influenced by the subbox size: too
 * small means too much overhead, too big loses because Heckbert's criterion
 * can't eliminate as many colormap entries.  Empirically the best subbox
 * size seems to be about 1/512th of the histogram (1/8th in each direction).
 *
 * Thomas' article also describes a refined method which is asymptotically
 * faster than the brute-force method, but it is also far more complex and
 * cannot efficiently be applied to small subboxes.  It is therefore not
 * useful for programs intended to be portable to DOS machines.  On machines
 * with plenty of memory, filling the whole histogram in one shot with Thomas'
 * refined method might be faster than the present code --- but then again,
 * it might not be any faster, and it's certainly more complicated.
 */

/* log2(histogram cells in update box) for each axis; this can be adjusted */
#define BOX_R_LOG  (PRECISION_R-3)
#define BOX_G_LOG  (PRECISION_G-3)
#define BOX_B_LOG  (PRECISION_B-3)

#define BOX_R_ELEMS  (1<<BOX_R_LOG)	/* # of hist cells in update box */
#define BOX_G_ELEMS  (1<<BOX_G_LOG)
#define BOX_B_ELEMS  (1<<BOX_B_LOG)

#define BOX_R_SHIFT  (R_SHIFT + BOX_R_LOG)
#define BOX_G_SHIFT  (G_SHIFT + BOX_G_LOG)
#define BOX_B_SHIFT  (B_SHIFT + BOX_B_LOG)

/*
 * The next three routines implement inverse colormap filling.  They could
 * all be folded into one big routine, but splitting them up this way saves
 * some stack space (the mindist[] and bestdist[] arrays need not coexist)
 * and may allow some compilers to produce better code by registerizing more
 * inner-loop variables.
 */

static int      find_nearby_colors(QuantizeObj *quantobj,int minR,int minG,int minB,int colorlist[])
/* Locate the colormap entries close enough to an update box to be candidates
 * for the nearest entry to some cell(s) in the update box.  The update box
 * is specified by the center coordinates of its first cell.  The number of
 * candidate colormap entries is returned, and their colormap indexes are
 * placed in colorlist[].
 * This routine uses Heckbert's "locally sorted search" criterion to select
 * the colors that need further consideration.
 */
{
    int             numcolors = quantobj->actual_number_of_colors;
    int             maxR, maxG, maxB;
    int             centerR, centerG, centerB;
    int             i, x, ncolors;
    int             minmaxdist, min_dist, max_dist, tdist;
    int             mindist[MAXNUMCOLORS];	/* min distance to colormap entry i */

    /* Compute true coordinates of update box's upper corner and center.
     * Actually we compute the coordinates of the center of the upper-corner
     * histogram cell, which are the upper bounds of the volume we care about.
     * Note that since ">>" rounds down, the "center" values may be closer to
     * min than to max; hence comparisons to them must be "<=", not "<".
     */
    maxR = minR + ((1 << BOX_R_SHIFT) - (1 << R_SHIFT));
    centerR = (minR + maxR) >> 1;
    maxG = minG + ((1 << BOX_G_SHIFT) - (1 << G_SHIFT));
    centerG = (minG + maxG) >> 1;
    maxB = minB + ((1 << BOX_B_SHIFT) - (1 << B_SHIFT));
    centerB = (minB + maxB) >> 1;

    /* For each color in colormap, find:
     *  1. its minimum squared-distance to any point in the update box
     *     (zero if color is within update box);
     *  2. its maximum squared-distance to any point in the update box.
     * Both of these can be found by considering only the corners of the box.
     * We save the minimum distance for each color in mindist[];
     * only the smallest maximum distance is of interest.
     */
    minmaxdist = 0x7FFFFFFFL;

    for (i = 0; i < numcolors; i++) {
	/* We compute the squared-R-distance term, then add in the other two. */
	x = quantobj->cmap[i].red;
	if (x < minR) {
	    tdist = (x - minR) R_SCALE;
	    min_dist = tdist * tdist;
	    tdist = (x - maxR) R_SCALE;
	    max_dist = tdist * tdist;
	} else if (x > maxR) {
	    tdist = (x - maxR) R_SCALE;
	    min_dist = tdist * tdist;
	    tdist = (x - minR) R_SCALE;
	    max_dist = tdist * tdist;
	} else {
	    /* within cell range so no contribution to min_dist */
	    min_dist = 0;
	    if (x <= centerR) {
		tdist = (x - maxR) R_SCALE;
		max_dist = tdist * tdist;
	    } else {
		tdist = (x - minR) R_SCALE;
		max_dist = tdist * tdist;
	    }
	}

	x = quantobj->cmap[i].green;
	if (x < minG) {
	    tdist = (x - minG) G_SCALE;
	    min_dist += tdist * tdist;
	    tdist = (x - maxG) G_SCALE;
	    max_dist += tdist * tdist;
	} else if (x > maxG) {
	    tdist = (x - maxG) G_SCALE;
	    min_dist += tdist * tdist;
	    tdist = (x - minG) G_SCALE;
	    max_dist += tdist * tdist;
	} else {
	    /* within cell range so no contribution to min_dist */
	    if (x <= centerG) {
		tdist = (x - maxG) G_SCALE;
		max_dist += tdist * tdist;
	    } else {
		tdist = (x - minG) G_SCALE;
		max_dist += tdist * tdist;
	    }
	}

	x = quantobj->cmap[i].blue;
	if (x < minB) {
	    tdist = (x - minB) B_SCALE;
	    min_dist += tdist * tdist;
	    tdist = (x - maxB) B_SCALE;
	    max_dist += tdist * tdist;
	} else if (x > maxB) {
	    tdist = (x - maxB) B_SCALE;
	    min_dist += tdist * tdist;
	    tdist = (x - minB) B_SCALE;
	    max_dist += tdist * tdist;
	} else {
	    /* within cell range so no contribution to min_dist */
	    if (x <= centerB) {
		tdist = (x - maxB) B_SCALE;
		max_dist += tdist * tdist;
	    } else {
		tdist = (x - minB) B_SCALE;
		max_dist += tdist * tdist;
	    }
	}

	mindist[i] = min_dist;	/* save away the results */
	if (max_dist < minmaxdist)
	    minmaxdist = max_dist;
    }

    /* Now we know that no cell in the update box is more than minmaxdist
     * away from some colormap entry.  Therefore, only colors that are
     * within minmaxdist of some part of the box need be considered.
     */
    ncolors = 0;
    for (i = 0; i < numcolors; i++) {
	if (mindist[i] <= minmaxdist)
	    colorlist[ncolors++] = i;
    }
    return ncolors;
}


static void     find_best_colors(QuantizeObj *quantobj, int minR, int minG, int minB, int numcolors, int colorlist[], int bestcolor[])
/* Find the closest colormap entry for each cell in the update box,
  given the list of candidate colors prepared by find_nearby_colors.
  Return the indexes of the closest entries in the bestcolor[] array.
  This routine uses Thomas' incremental distance calculation method to
  find the distance from a colormap entry to successive cells in the box.
 */
{
    int             iR, iG, iB;
    int             i, icolor;
    int            *bptr;	/* pointer into bestdist[] array */
    int            *cptr;	/* pointer into bestcolor[] array */
    int             dist0, dist1;	/* initial distance values */
    int             dist2;	/* current distance in inner loop */
    int             xx0, xx1;	/* distance increments */
    int             xx2;
    int             inR, inG, inB;	/* initial values for increments */

    /* This array holds the distance to the nearest-so-far color for each cell */
    int             bestdist[BOX_R_ELEMS * BOX_G_ELEMS * BOX_B_ELEMS];

    /* Initialize best-distance for each cell of the update box */
    bptr = bestdist;
    for (i = BOX_R_ELEMS * BOX_G_ELEMS * BOX_B_ELEMS - 1; i >= 0; i--)
	*bptr++ = 0x7FFFFFFFL;

    /* For each color selected by find_nearby_colors,
     * compute its distance to the center of each cell in the box.
     * If that's less than best-so-far, update best distance and color number.
     */

    /* Nominal steps between cell centers ("x" in Thomas article) */
#define STEP_R  ((1 << R_SHIFT) R_SCALE)
#define STEP_G  ((1 << G_SHIFT) G_SCALE)
#define STEP_B  ((1 << B_SHIFT) B_SCALE)

    for (i = 0; i < numcolors; i++) {
	icolor = colorlist[i];
	/* Compute (square of) distance from minR/G/B to this color */
	inR = (minR - quantobj->cmap[icolor].red) R_SCALE;
	dist0 = inR * inR;
	inG = (minG - quantobj->cmap[icolor].green) G_SCALE;
	dist0 += inG * inG;
	inB = (minB - quantobj->cmap[icolor].blue) B_SCALE;
	dist0 += inB * inB;
	/* Form the initial difference increments */
	inR = inR * (2 * STEP_R) + STEP_R * STEP_R;
	inG = inG * (2 * STEP_G) + STEP_G * STEP_G;
	inB = inB * (2 * STEP_B) + STEP_B * STEP_B;
	/* Now loop over all cells in box, updating distance per Thomas method */
	bptr = bestdist;
	cptr = bestcolor;
	xx0 = inR;
	for (iR = BOX_R_ELEMS - 1; iR >= 0; iR--) {
	    dist1 = dist0;
	    xx1 = inG;
	    for (iG = BOX_G_ELEMS - 1; iG >= 0; iG--) {
		dist2 = dist1;
		xx2 = inB;
		for (iB = BOX_B_ELEMS - 1; iB >= 0; iB--) {
		    if (dist2 < *bptr) {
			*bptr = dist2;
			*cptr = icolor;
		    }
		    dist2 += xx2;
		    xx2 += 2 * STEP_B * STEP_B;
		    bptr++;
		    cptr++;
		}
		dist1 += xx1;
		xx1 += 2 * STEP_G * STEP_G;
	    }
	    dist0 += xx0;
	    xx0 += 2 * STEP_R * STEP_R;
	}
    }
}

static void     fill_inverse_cmap_rgb(QuantizeObj *quantobj,Histogram histogram,int R,int G,int B)
/* Fill the inverse-colormap entries in the update box that contains
 histogram cell R/G/B.  (Only that one cell MUST be filled, but
 we can fill as many others as we wish.) */
{
    int             minR, minG, minB;	/* lower left corner of update box */
    int             iR, iG, iB;
    int            *cptr;	/* pointer into bestcolor[] array */
    ColorFreq      *cachep;	/* pointer into main cache array */
    /* This array lists the candidate colormap indexes. */
    int             colorlist[MAXNUMCOLORS];
    int             numcolors;	/* number of candidate colors */
    /* This array holds the actually closest colormap index for each cell. */
    int             bestcolor[BOX_R_ELEMS * BOX_G_ELEMS * BOX_B_ELEMS];

    /* Convert cell coordinates to update box ID */
    R >>= BOX_R_LOG;
    G >>= BOX_G_LOG;
    B >>= BOX_B_LOG;

    /* Compute true coordinates of update box's origin corner.
     * Actually we compute the coordinates of the center of the corner
     * histogram cell, which are the lower bounds of the volume we care about.
     */
    minR = (R << BOX_R_SHIFT) + ((1 << R_SHIFT) >> 1);
    minG = (G << BOX_G_SHIFT) + ((1 << G_SHIFT) >> 1);
    minB = (B << BOX_B_SHIFT) + ((1 << B_SHIFT) >> 1);

    /* Determine which colormap entries are close enough to be candidates
     * for the nearest entry to some cell in the update box.
     */
    numcolors = find_nearby_colors(quantobj, minR, minG, minB, colorlist);

    /* Determine the actually nearest colors. */
    find_best_colors(quantobj, minR, minG, minB, numcolors, colorlist,
		     bestcolor);

    /* Save the best color numbers (plus 1) in the main cache array */
    R <<= BOX_R_LOG;		/* convert ID back to base cell indexes */
    G <<= BOX_G_LOG;
    B <<= BOX_B_LOG;
    cptr = bestcolor;
    for (iR = 0; iR < BOX_R_ELEMS; iR++) {
	for (iG = 0; iG < BOX_G_ELEMS; iG++) {
	    cachep = &histogram[(R + iR) * MR + (G + iG) * MG + B];
	    for (iB = 0; iB < BOX_B_ELEMS; iB++) {
		*cachep++ = (*cptr++) + 1;
	    }
	}
    }
}

/*  This is pass 1  */
static void     median_cut_pass1_rgb(QuantizeObj *quantobj,unsigned char  * src,unsigned char  * dest,long width,long height)
{
    generate_histogram_rgb(quantobj->histogram, src, width, height);
    select_colors_rgb(quantobj, quantobj->histogram);
}


/* Map some rows of pixels to the output colormapped representation. */
static void     median_cut_pass2_no_dither_rgb(QuantizeObj *quantobj,unsigned char  * src,unsigned char  * dest,long width,long height)
/* This version performs no dithering */
{
    Histogram       histogram = quantobj->histogram;
    ColorFreq      *cachep;
    int             R, G, B;
    int             row, col;

    zero_histogram_rgb(histogram);
    for (row = 0; row < height; row++) {
	for (col = 0; col < width; col++) {
	    /* get pixel value and index into the cache */
	    R = (*src++) >> R_SHIFT;
	    G = (*src++) >> G_SHIFT;
	    B = (*src++) >> B_SHIFT;
	    cachep = &histogram[R * MR + G * MG + B];
	    /* If we have not seen this color before, find nearest colormap entry
	     and update the cache */
	    if (*cachep == 0)
		fill_inverse_cmap_rgb(quantobj, histogram, R, G, B);
	    /* Now emit the colormap index for this cell */
	    *dest++ =(unsigned char)(*cachep - 1);
	}

    }
}

/*
  Initialize the error-limiting transfer function (lookup table).
  The raw F-S error computation can potentially compute error values of up to
  +- MAXJSAMPLE.  But we want the maximum correction applied to a pixel to be
  much less, otherwise obviously wrong pixels will be created.  (Typical
  effects include weird fringes at color-area boundaries, isolated bright
  pixels in a dark area, etc.)  The standard advice for avoiding this problem
  is to ensure that the "corners" of the color cube are allocated as output
  colors; then repeated errors in the same direction cannot cause cascading
  error buildup.  However, that only prevents the error from getting
  completely out of hand; Aaron Giles reports that error limiting improves
  the results even with corner colors allocated.

  A simple clamping of the error values to about +- MAXJSAMPLE/8 works pretty
  well, but the smoother transfer function used below is even better.  Thanks
  to Aaron Giles for this idea.
 */

static int     *init_error_limit()
/* Allocate and fill in the error_limiter table */
{
    int            *table;
    int             in, out;

    table =(int*) malloc((255 * 2 + 1) * sizeof(int));
    if (!table) error("could not alloc mem");
    table += 255;		/* so we can index -255 ... +255 */

#define STEPSIZE 16

    /* Map errors 1:1 up to +- 16 */
    out = 0;
    for (in = 0; in < STEPSIZE; in++, out++) {
	table[in] = out;
	table[-in] = -out;
    }

    /* Map errors 1:2 up to +- 3*16 */
    for (; in < STEPSIZE * 3; in++, out += (in & 1) ? 0 : 1) {
	table[in] = out;
	table[-in] = -out;
    }

    /* Clamp the rest to final out value (which is 32) */
    for (; in <= 255; in++) {
	table[in] = out;
	table[-in] = -out;
    }

#undef STEPSIZE

    return table;
}


/* Map some rows of pixels to the output colormapped representation.
 Perform floyd-steinberg dithering. */
static void     median_cut_pass2_fs_dither_rgb(QuantizeObj *quantobj,unsigned char  * src,unsigned char  * dest,long width,long height)
{
    Histogram       histogram = quantobj->histogram;
    ColorFreq      *cachep;
    Color          *color;
    unsigned char  *src_row;
    unsigned char  *dest_row;
    int            *error_limiter;
    short          *fs_err1, *fs_err2;
    short          *fs_err3, *fs_err4;
    int            *red_n_row, *red_p_row;
    int            *grn_n_row, *grn_p_row;
    int            *blu_n_row, *blu_p_row;
    int            *rnr, *rpr;
    int            *gnr, *gpr;
    int            *bnr, *bpr;
    int            *tmp;
    int             r, g, b;
    int             re, ge, be;
    int             row, col;
    int             index;
    int             rowstride;
    int             step_dest, step_src;
    int             odd_row;
//    short          *range_limiter;
    unsigned char    *range_limiter;

    zero_histogram_rgb(histogram);

    error_limiter = init_error_limit();
    range_limiter = range_array + 256;

    red_n_row =(int*) malloc(sizeof(int) * (width + 2));
    if (!red_n_row) error("could not alloc mem");
    red_p_row = (int*)malloc(sizeof(int) * (width + 2));
    if (!red_p_row) error("could not alloc mem");
    grn_n_row = (int*)malloc(sizeof(int) * (width + 2));
    if (!grn_n_row) error("could not alloc mem");
    grn_p_row = (int*)malloc(sizeof(int) * (width + 2));
    if (!grn_p_row) error("could not alloc mem");
    blu_n_row = (int*)malloc(sizeof(int) * (width + 2));
    if (!blu_n_row) error("could not alloc mem");
    blu_p_row = (int*)malloc(sizeof(int) * (width + 2));
    if (!blu_p_row) error("could not alloc mem");

    memset(red_p_row, 0, sizeof(int) * (width + 2) );
    memset(grn_p_row, 0, sizeof(int) * (width + 2));
    memset(blu_p_row, 0, sizeof(int) * (width + 2));

    fs_err1 = floyd_steinberg_error1 + 511;
    fs_err2 = floyd_steinberg_error2 + 511;
    fs_err3 = floyd_steinberg_error3 + 511;
    fs_err4 = floyd_steinberg_error4 + 511;

    src_row = src;
    dest_row = dest;
    rowstride = width * 3;
    odd_row = 0;

    for (row = 0; row < height; row++) {
	src = src_row;
	dest = dest_row;

	src_row += rowstride;
	dest_row += width;

	rnr = red_n_row;
	gnr = grn_n_row;
	bnr = blu_n_row;
	rpr = red_p_row + 1;
	gpr = grn_p_row + 1;
	bpr = blu_p_row + 1;

	if (odd_row) {
	    step_dest = -1;
	    step_src = -3;

	    src += rowstride - 3;
	    dest += width - 1;

	    rnr += width + 1;
	    gnr += width + 1;
	    bnr += width + 1;
	    rpr += width;
	    gpr += width;
	    bpr += width;

	    *(rnr - 1) = *(gnr - 1) = *(bnr - 1) = 0;
	} else {
	    step_dest = 1;
	    step_src = 3;

	    *(rnr + 1) = *(gnr + 1) = *(bnr + 1) = 0;
	}

	*rnr = *gnr = *bnr = 0;

	for (col = 0; col < width; col++) {
	    r = range_limiter[src[0] + error_limiter[*rpr]];
	    g = range_limiter[src[1] + error_limiter[*gpr]];
	    b = range_limiter[src[2] + error_limiter[*bpr]];

	    re = r >> R_SHIFT;
	    ge = g >> G_SHIFT;
	    be = b >> B_SHIFT;

	    cachep = &histogram[re * MR + ge * MG + be];
	    /* If we have not seen this color before,
	    find nearest colormap entry
	    and update the cache */
	    if (*cachep == 0)
		fill_inverse_cmap_rgb(quantobj, histogram, re, ge, be);

	    index = *cachep - 1;
	    *dest = index;

	    color = &quantobj->cmap[index];
	    re = r - color->red;
	    ge = g - color->green;
	    be = b - color->blue;

	    if (odd_row) {
		*(--rpr) += fs_err1[re];
		*(--gpr) += fs_err1[ge];
		*(--bpr) += fs_err1[be];

		*rnr-- += fs_err2[re];
		*gnr-- += fs_err2[ge];
		*bnr-- += fs_err2[be];

		*rnr += fs_err3[re];
		*gnr += fs_err3[ge];
		*bnr += fs_err3[be];

		*(rnr - 1) = fs_err4[re];
		*(gnr - 1) = fs_err4[ge];
		*(bnr - 1) = fs_err4[be];
	    } else {
		*(++rpr) += fs_err1[re];
		*(++gpr) += fs_err1[ge];
		*(++bpr) += fs_err1[be];

		*rnr++ += fs_err2[re];
		*gnr++ += fs_err2[ge];
		*bnr++ += fs_err2[be];

		*rnr += fs_err3[re];
		*gnr += fs_err3[ge];
		*bnr += fs_err3[be];

		*(rnr + 1) = fs_err4[re];
		*(gnr + 1) = fs_err4[ge];
		*(bnr + 1) = fs_err4[be];
	    }

	    dest += step_dest;
	    src += step_src;
	}

	tmp = red_n_row;
	red_n_row = red_p_row;
	red_p_row = tmp;

	tmp = grn_n_row;
	grn_n_row = grn_p_row;
	grn_p_row = tmp;

	tmp = blu_n_row;
	blu_n_row = blu_p_row;
	blu_p_row = tmp;

	odd_row = !odd_row;

    }

    xfree(error_limiter - 255);
    xfree(red_n_row);
    xfree(red_p_row);
    xfree(grn_n_row);
    xfree(grn_p_row);
    xfree(blu_n_row);
    xfree(blu_p_row);
}

static QuantizeObj *initialize_median_cut(int num_colors, int dither_type)
{
    QuantizeObj    *quantobj;

    /* Initialize the data structures */
    quantobj =(QuantizeObj*) malloc(sizeof(QuantizeObj));
    if (!quantobj) error("could not alloc mem");

    quantobj->histogram =(Histogram) malloc(sizeof(ColorFreq) *
				      HIST_R_ELEMS *
				      HIST_G_ELEMS *
				      HIST_B_ELEMS);
    if (!quantobj->histogram) error("could not alloc mem");

    quantobj->desired_number_of_colors = num_colors;
    quantobj->first_pass = median_cut_pass1_rgb;

    switch (dither_type) {
    case NODITHER:
	    quantobj->second_pass = median_cut_pass2_no_dither_rgb;
	    break;
    case FSDITHER:
	    quantobj->second_pass = median_cut_pass2_fs_dither_rgb;
	    break;
    }

    return quantobj;
}

///* Externs: */
//extern unsigned char            *Picture256;
//extern unsigned char 		 QuantizedPalette[768];
//
//void to_indexed(char*input,long ncolors,long dither,int width,int height)
//{
//    QuantizeObj    *quantobj;
//    int             i, j;
//
//    quantobj = initialize_median_cut(ncolors, dither ? FSDITHER : NODITHER);
//    (*quantobj->first_pass)  (quantobj, input, Picture256, width, height);
//    (*quantobj->second_pass) (quantobj, input, Picture256, width, height);
//    for (i = 0, j = 0; i < quantobj->actual_number_of_colors; i++) {
//#if 0
//// rgb
//	QuantizedPalette[j++] = quantobj->cmap[i].red;
//	QuantizedPalette[j++] = quantobj->cmap[i].green;
//	QuantizedPalette[j++] = quantobj->cmap[i].blue;
//#else
//// bgr
//	QuantizedPalette[j++] = quantobj->cmap[i].blue;
//	QuantizedPalette[j++] = quantobj->cmap[i].green;
//	QuantizedPalette[j++] = quantobj->cmap[i].red;
//#endif
//    }
//    xfree(quantobj->histogram);
//    xfree(quantobj);
//}
///*-*/

#include"ioimpl.h"
#include<assert.h>


static void  _QuantCopy(const void* pIn,const int width,const int height,const int istep,const int ipw,
			  void* pOut,const int ostep,const int opw)
{
	assert(opw<=ipw);
	uchar* pi=(uchar*)pIn,*po=(uchar*)pOut;
	if(ipw==opw)
	{
		for(int yi=0;yi<height;++yi,po+=ostep,pi+=istep)
			memcpy(po,pi,width*opw);
	}
	else
	{
		for(int yi=0;yi<height;++yi,po+=ostep,pi+=istep)
		{
			uchar* pix=pi,*pox=po;
			for(int xi=0;xi<width;++xi,pix+=ipw,pox+=opw)
				memcpy(pox,pix,opw);
		}
	}
}

static uchar* _PreQuantize(const uchar *pData,const int width,const int height,const int step,const int cn,
						   uchar* pIndex,uchar (*pMap)[3],const int mapSize)
{
	uchar* prv=(uchar*)pData;
	
	if(cn==1||cn==2)
	{
		_QuantCopy(pData,width,height,step,cn,pIndex,width,1);
		for(int i=0;i<mapSize;++i)
			pMap[i][0]=pMap[i][1]=pMap[i][2]=uchar(i);
		prv=NULL;
	}
	else
		if((cn==3&&width*3!=step)||cn==4)
		{
			prv=new uchar[3*width*height];
			_QuantCopy(pData,width,height,step,cn,prv,width*3,3);
		}
	return prv;
}

int QuantizeImageMedian(const uchar *pData,const int width,const int height,const int step,const int cn,
				   uchar* pIndex,uchar (*pMap)[3],const int mapSize,int mode)
{
	int rv=mapSize;

	if(uchar* pIn=_PreQuantize(pData,width,height,step,cn,pIndex,pMap,mapSize))
	{
		QuantizeObj    *quantobj = initialize_median_cut(mapSize, mode&QIM_DITHER ? FSDITHER : NODITHER);
		(*quantobj->first_pass)  (quantobj,pIn, pIndex, width, height);
		(*quantobj->second_pass) (quantobj, pIn, pIndex, width, height);
		
		rv=quantobj->actual_number_of_colors;
		if(mode&QIM_SWAP_RB)
		{
			for (int i = 0; i < rv; i++) 
			{
				pMap[i][0] = quantobj->cmap[i].blue;
				pMap[i][1] = quantobj->cmap[i].green;
				pMap[i][2] = quantobj->cmap[i].red;
			}
		}
		else
		{
			for (int i = 0; i < rv; i++) 
			{
				pMap[i][0] = quantobj->cmap[i].red;
				pMap[i][1] = quantobj->cmap[i].green;
				pMap[i][2] = quantobj->cmap[i].blue;
			}
		}

		xfree(quantobj->histogram);
		xfree(quantobj);
		if(pIn!=pData)
			delete[]pIn;
	}
	return rv;
}



