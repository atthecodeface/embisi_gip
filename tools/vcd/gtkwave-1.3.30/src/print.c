/*
 * Copyright (c) Tony Bybell 1999-2001. 
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version. 
 */

/*
 * This module has been re-implemented by Udi Finkelstein. Since it is no
 * longer a PostScript-only module, it had been renamed "print.c". 
 *
 * Much of the code has been "C++"-ized in style, yet written in C. We use
 * classes, virtual functions, class members, and "this" pointers written in
 * C. 
 */

#include "currenttime.h"
#include "analyzer.h"
#include "aet.h"
#include "bsearch.h"
#include "alloca.h"
#include "debug.h"
#include "strace.h"
#include "print.h"

/*
 * Constants 
 */
static int      inch = 72;	/* 72 points/inch */

/**********************************************
 * this is essentially wavewindow.c's rendering
 * engine modified to produce postscript
 **********************************************/

#define WAVE_COURIER_SCALE_FAC 1.6	/* more or less is the correct
					 * pixel->ps scale mapping  */

static gdouble  ps_chwidth = 1.0;
static gdouble  ybound;
static int      pr_signal_fill_width;
static int      ps_nummaxchars;

char            ps_fullpage = 1;/* set by menu.c in the callback function */
int             ps_maxveclen = 66;	/* maximum vector length that is
					 * printable in signal window
					 * (includes = and base symbol) */
int             liney_max = 0;	/* will be the bottom of the marker */

/*
 * PostScript device specific operations 
 */
static void 
ps_header(pr_context * prc);
static void 
ps_trailer(pr_context * prc);
static void 
ps_signal_init(pr_context * prc);
static void 
ps_setgray(pr_context * prc, gdouble gray);
static void 
ps_draw_line(pr_context * prc, gdouble x1, gdouble y1,
	     gdouble x2, gdouble y2);
static void 
ps_draw_box(pr_context * prc, gdouble x1, gdouble y1, gdouble x2,
	    gdouble y2);
static void 
ps_draw_string(pr_context * prc, int x, int y, char *str,
	       int xsize, int ysize);

gtk_print_device ps_print_device = {
				    ps_header,
				    ps_trailer,
				    ps_signal_init,
				    ps_setgray,
				    ps_draw_line,
				    ps_draw_box,
				    ps_draw_string
};

/*
 * MIF device specific operations 
 */
static void 
mif_header(pr_context * prc);
static void 
mif_trailer(pr_context * prc);
static void 
mif_signal_init(pr_context * prc);
static void 
mif_setgray(pr_context * prc, gdouble gray);
static void 
mif_translate(pr_context * prc, gdouble x, gdouble y);
static void 
mif_draw_line(pr_context * prc, gdouble x1, gdouble y1,
	      gdouble x2, gdouble y2);
static void 
mif_draw_box(pr_context * prc, gdouble x1, gdouble y1,
	     gdouble x2, gdouble y2);
static void 
mif_draw_string(pr_context * prc, int x, int y, char *str,
		int xsize, int ysize);

gtk_print_device mif_print_device = {
				     mif_header,
				     mif_trailer,
				     mif_signal_init,
				     mif_setgray,
				     mif_draw_line,
				     mif_draw_box,
				     mif_draw_string
};

/**************************************************************************
 * Shorthand routins                                                      *
 *                                                                        *
 * These routines call the specific operations through the device pointer *
 **************************************************************************/
void
pr_header(pr_context * prc)
{
	(*prc->gpd->gpd_header) (prc);
}

void
pr_trailer(pr_context * prc)
{
	(*prc->gpd->gpd_trailer) (prc);
}

void
pr_signal_init(pr_context * prc)
{
	(*prc->gpd->gpd_signal_init) (prc);
}

void
pr_setgray(pr_context * prc, gdouble gray)
{
	(*prc->gpd->gpd_setgray) (prc, gray);
}

void
pr_draw_line(pr_context * prc, gdouble x1, gdouble y1, gdouble x2,
	     gdouble y2)
{
	(*prc->gpd->gpd_draw_line) (prc, x1, y1, x2, y2);
}

void
pr_draw_box(pr_context * prc, gdouble x1, gdouble y1, gdouble x2, gdouble y2)
{
	(*prc->gpd->gpd_draw_box) (prc, x1, y1, x2, y2);
}

void
pr_draw_string(pr_context * prc, int x, int y, char *str, int xsize,
	       int ysize)
{
	(*prc->gpd->gpd_draw_string) (prc, x, y, str, xsize, ysize);
}


/*************************************************************************
 *                    PostScript specific routines                       *
 *************************************************************************/

/*
 * Set current gray level, with 0.0 being white, and 1.0 being black. 
 */
static void
ps_setgray(pr_context * prc, gdouble gray)
{
	fprintf(prc->handle, "%f setgray\n", gray);
}

/*
 * Create a rectangular path 
 */
static void
ps_box(pr_context * prc, gdouble tx1, gdouble ty1, gdouble tx2, gdouble ty2)
{
	fprintf(prc->handle, "%f %f %f %f box\n", ty1, tx1, ty2, tx2);
}

/*
 * Draw a box 
 */
static void
ps_draw_box(pr_context * prc, gdouble x1, gdouble y1, gdouble x2, gdouble y2)
{
	gdouble         tx1, ty1, tx2, ty2;

	tx1 = x1 * prc->xscale;
	ty1 = y1 * prc->yscale;
	tx2 = x2 * prc->xscale;
	ty2 = y2 * prc->yscale;

	ps_box(prc, tx1, ty1, tx2, ty2);
	fprintf(prc->handle, "fill\n");
}

static void
ps_signal_init(pr_context * prc)
{

	fprintf(prc->handle, "grestore\n" "gsave\n");

	if (prc->fullpage) {
		ps_setgray(prc, 0.0);
		ps_box(prc, prc->MinX - 1, prc->MinY - 2, prc->MaxX + 1,
		       prc->MaxY + 2);
		fprintf(prc->handle, "stroke\n");
		ps_setgray(prc, 0.5);
		ps_box(prc, prc->MinX, prc->MinY - 1, prc->MaxX, prc->MaxY + 1);
		fprintf(prc->handle, "clip\n");
	}
	fprintf(prc->handle,
		"%d %d translate\n"
		"1 1 scale\n"
		"0.5 setlinewidth\n" "stroke\n", prc->MinY, prc->MinX);
}

static void
ps_header(pr_context * prc)
{
	gdouble         ps_skip;

	ps_skip = prc->MinX +
		(prc->MaxX -
	      prc->MinX) * (((gdouble) pr_signal_fill_width) / prc->xtotal);

	fprintf(prc->handle,
		"%%!PS-Adobe-2.0 EPSF-1.2\n"
		"%%%%BoundingBox: %d %d %d %d\n"
		"/box { %% stack: x1 y1 x2 y2\n"
		"\tnewpath\n"
		"\t2 copy moveto %% x2 y2\n"
		"\t3 index 1 index lineto %% x1 y2\n"
		"\t3 index 3 index lineto %% x1 y1\n"
		"\t1 index 3 index lineto %% x2 y1\n"
		"\tpop pop pop pop\n"
		"\tclosepath\n"
		"} def\n"
		"/l { %% stack: x1 y1 x2 y2\n"
		"\tnewpath moveto lineto closepath stroke\n"
		"} def\n",
	   prc->MinY, prc->MinX, (int) (prc->fullpage ? prc->MaxY : ybound),
		prc->MaxX);

	if (!prc->fullpage) {
		ps_box(prc, prc->MinX - 1, prc->MinY - 1, prc->MaxX, ybound);
		fprintf(prc->handle, "clip\n");
	}
	fprintf(prc->handle,
		"/Courier findfont\n"
		"10 scalefont\n"
		"setfont\n"
		"2 setlinecap\n"
		"gsave\n" "1 1 scale\n" "0.5 setlinewidth\n" "stroke\n");
	ps_setgray(prc, 0.75);
	ps_box(prc, ps_skip, prc->MinY - 1, prc->MaxX, prc->MaxY + 1);
	fprintf(prc->handle,
		"clip\n" "%d %f translate stroke\n", prc->MinY, ps_skip);
}


static void
ps_draw_line(pr_context * prc, gdouble x1, gdouble y1, gdouble x2,
	     gdouble y2)
{
	gdouble         tx1, ty1, tx2, ty2;

	if (x1 < -1.0)
		x1 = -1.0;
	if (x2 < -1.0)
		x2 = -1.0;
	if (x1 > 10000.0)
		x1 = 10000.0;
	if (x2 > 10000.0)
		x2 = 10000.0;

	tx1 = x1 * prc->xscale;
	ty1 = y1 * prc->yscale;
	tx2 = x2 * prc->xscale;
	ty2 = y2 * prc->yscale;

	fprintf(prc->handle, "%f %f %f %f l\n", ty1, tx1, ty2, tx2);
}


static void
ps_draw_string(pr_context * prc, int x, int y, char *str, int xsize,
	       int ysize)
{
	int             i;
	int             len;

	len = strlen(str);
	if (!len)
		return;

	fprintf(prc->handle, "save\n%f %f moveto\n%f %f scale\n90 rotate\n(",
		(y - 1) * prc->yscale, x * prc->xscale,
		(ysize * prc->yscale) * WAVE_COURIER_SCALE_FAC / 10.0,
		(xsize * prc->xscale) / len * WAVE_COURIER_SCALE_FAC / 10.0);

	for (i = 0; i < len; i++) {
		char            ch;

		ch = str[i];
		if (ch < 32) {
			ch = 32;/* fix out of range signed chars */
		} else if ((ch == '(') || (ch == ')') || (ch == '\\')) {
			fprintf(prc->handle, "\\");	/* escape parens or esc */
		}
		fprintf(prc->handle, "%c", ch);
	}

	fprintf(prc->handle, ") show\n" "restore\n");
}


static void
ps_trailer(pr_context * prc)
{
	fprintf(prc->handle, "grestore showpage\n");
}

/*************************************************************************
 *                    MIF specific routines                              *
 *************************************************************************/

/*
 * Generic maint functions missing in gcc 
 */
#ifndef _MSC_VER
static          gdouble
max(gdouble a, gdouble b)
{
	return (a > b ? a : b);
}

static          gdouble
min(gdouble a, gdouble b)
{
	return (a < b ? a : b);
}
#endif


/*
 * Set current gray level, with 0.0 being white, and 1.0 being black. 
 */
static void
mif_setgray(pr_context * prc, gdouble gray)
{
	prc->gray = gray;
}

/*
 * Set current gray level, with 0.0 being white, and 1.0 being black. 
 */
static void
mif_translate(pr_context * prc, gdouble x, gdouble y)
{
	prc->tr_x = x;
	prc->tr_y = y;
}

/*
 * Draw an empty box 
 */
static void
mif_box(pr_context * prc, gdouble x1, gdouble y1, gdouble x2, gdouble y2)
{
	fprintf(prc->handle,
		" <Rectangle\n"
		"   <GroupID 1>\n"
		"   <Fill 15>\n"
		"   <Pen 0>\n"
		"   <PenWidth 0.5 pt>\n"
		"   <Overprint No >\n"
		"   <ObTint %d%%>\n"
		"   <ShapeRect %d pt %d pt %d pt %d pt>\n"
		"   <BRect %d pt %d pt %d pt %d pt>\n"
		" > # end of Rectangle\n",
		(int) (10000 * (1 - prc->gray)),
		(int) (y1), (int) (x1),
		abs((int) (y2 - y1)), abs((int) (x2 - x1)),
		(int) (y1), (int) (x1),
		abs((int) (y2 - y1)), abs((int) (x2 - x1)));
}

/*
 * Draw a filled box 
 */

static void
mif_draw_box(pr_context * prc, gdouble x1, gdouble y1, gdouble x2,
	     gdouble y2)
{
	gdouble         tx1, ty1, tx2, ty2;
	int             rx, ry, rw, rh;

	tx1 = x1 * prc->xscale + prc->tr_x;
	ty1 = y1 * prc->yscale + prc->tr_y;
	tx2 = x2 * prc->xscale + prc->tr_x;
	ty2 = y2 * prc->yscale + prc->tr_y;

	/* The exprssion below is derived from: */
	/* rx = min((prc->PageX * inch - tx1), (prc->PageX * inch - tx2)) */
	rx = (int) (prc->PageX * inch - max(tx2, tx1));
	ry = (int) min(ty1, ty2);
	rw = abs((int) (tx2 - tx1));
	rh = abs((int) (ty2 - ty1));


	fprintf(prc->handle,
		" <Rectangle\n"
		"   <GroupID 1>\n"
		"   <Fill 0>\n"
		"   <Pen 0>\n"
		"   <PenWidth 0.5 pt>\n"
		"   <Overprint No >\n"
		"   <ObTint %d%%>\n"
		"   <ShapeRect %d pt %d pt %d pt %d pt>\n"
		"   <BRect %d pt %d pt %d pt %d pt>\n"
		" > # end of Rectangle\n",
	   (int) (10000 * (1 - prc->gray)), ry, rx, rh, rw, rx, ry, rh, rw);
}

static void
mif_signal_init(pr_context * prc)
{
	if (prc->fullpage) {
		mif_setgray(prc, 0.0);
		mif_box(prc, prc->MinX - 1, prc->MinY - 2, prc->MaxX + 1,
			prc->MaxY + 2);
		mif_setgray(prc, 0.5);
		mif_box(prc, prc->MinX, prc->MinY - 1, prc->MaxX, prc->MaxY + 1);
	}
	mif_translate(prc, prc->MinX, prc->MinY);
}

static void
mif_header(pr_context * prc)
{
	gdouble         modified_skip;
	gdouble         mif_skip;

	mif_skip =
		(prc->MaxX -
	      prc->MinX) * (((gdouble) pr_signal_fill_width) / prc->xtotal);
	modified_skip = prc->MinX + mif_skip;

	fprintf(prc->handle,
		"<MIFFile 5.00> # Generated by GTKWave\n"
	  "               # MIF support by Udi Finkelstein <udif@usa.net>\n"
		"<Page\n");
	mif_translate(prc, modified_skip, prc->MinY);
}


static void
mif_draw_line(pr_context * prc, gdouble x1, gdouble y1, gdouble x2,
	      gdouble y2)
{
	gdouble         tx1, ty1, tx2, ty2;

	if (x1 < -1.0)
		x1 = -1.0;
	if (x2 < -1.0)
		x2 = -1.0;
	if (x1 > 10000.0)
		x1 = 10000.0;
	if (x2 > 10000.0)
		x2 = 10000.0;

	tx1 = x1 * prc->xscale + prc->tr_x;
	ty1 = y1 * prc->yscale + prc->tr_y;
	tx2 = x2 * prc->xscale + prc->tr_x;
	ty2 = y2 * prc->yscale + prc->tr_y;

	fprintf(prc->handle,
		" <PolyLine\n"
		"   <GroupID 1>\n"
		"   <Pen 0>\n"
		"   <PenWidth 0.5 pt>\n"
		"   <ObTint %d%%>\n"
		"   <Overprint No >\n"
		"   <NumPoints 4>\n"
		"   <Point  %d pt %d pt>\n"
		"   <Point  %d pt %d pt>\n"
		" > # end of PolyLine\n",
		(int) (10000 * (1 - prc->gray)),
		(int) (ty1), (int) (prc->PageX * inch - tx1),
		(int) (ty2), (int) (prc->PageX * inch - tx2));
}


static void
mif_draw_string(pr_context * prc, int x, int y, char *str, int xsize,
		int ysize)
{
	int             len;
	gdouble         tx, ty;
	gdouble         stretchx, stretchy;

	if (x < -1.0)
		x = -1.0;
	if (x > 10000.0)
		x = 10000.0;

	tx = x * prc->xscale + prc->tr_x;
	ty = y * prc->yscale + prc->tr_y;

	len = strlen(str);
	if (!len)
		return;

	stretchy = (ysize * (1.52 * prc->yscale));
	stretchx =
		(xsize / (len * stretchy)) * prc->xscale * WAVE_COURIER_SCALE_FAC *
		100.00;

	fprintf(prc->handle,
		" <TextLine\n"
		"  <Angle 90.0>\n"
		"  <TLOrigin  %d pt %d pt>\n"
		"  <TLAlignment Left>\n"
		"  <Font\n"
		"   <FTag `'>\n"
		"   <FSize %3.0f pt>\n"
		"   <FPairKern Yes>\n"
		"   <FStretch %3.2f%%>\n"
		"   <FFamily `Courier'>\n"
		"   <FEncoding `FrameRoman'>\n"
		"   <FLocked No>\n"
		"  > # end of Font\n"
		"  <String `%s'>\n"
		" > # end of TextLine\n",
	     (int) (ty), (int) (prc->PageX * inch - tx), stretchy, stretchx,
		str);
}


static void
mif_trailer(pr_context * prc)
{
	fprintf(prc->handle,
		" <Group\n"
		"  <ID 1>\n"
	    "  <Overprint No>\n" " > # end of Group\n" "> # end of Page\n");
}

/**********************************************/

/*
 * Initialize print related constants 
 */
static void
pr_wave_init(pr_context * prc)
{
	int             wh = waveheight;
	int             yheight = 33 * fontheight;

	prc->MinX = prc->LM * inch;
	prc->MaxX = (prc->PageX - prc->RM) * inch;
	prc->MinY = prc->BM * inch;
	prc->MaxY = (prc->PageY - prc->TM) * inch;

	if (!prc->fullpage) {
		if (wh < 2 * fontheight)
			wh = 2 * fontheight;
	}
	yheight = (wh < yheight) ? yheight : wh;
	yheight = yheight - (yheight % fontheight);

	if (!prc->fullpage) {
		ybound = ((prc->MaxY - prc->MinY) / ((gdouble) yheight)) *
			((gdouble) (wh - (wh % fontheight))) + prc->MinY;
	}
	prc->xtotal = (gdouble) (wavewidth + pr_signal_fill_width);
	prc->xscale = ((prc->MaxX - prc->MinX) / prc->xtotal);
	prc->yscale = (prc->MaxY - prc->MinY) / ((gdouble) yheight);

}

static int
ps_MaxSignalLength(void)
{
	Trptr           t;
	int             len = 0, maxlen = 0, numchars = 0;
	int             vlen = 0;
	int             i, trwhich, trtarget, num_traces_displayable;
	GtkAdjustment  *sadj;
	char            sbuf[65];
	int             bufxlen;
	int             bufclen;

	ps_nummaxchars = 7;	/* allows a good spacing if 60 pixel default
				 * is used */

	sadj = GTK_ADJUSTMENT(wave_vslider);
	trtarget = (int) (sadj->value);

	t = traces.first;
	trwhich = 0;
	while (t) {
		if ((trwhich < trtarget) && (t->next)) {
			trwhich++;
			t = t->next;
		} else {
			break;
		}
	}

	num_traces_displayable = signalarea->allocation.height / fontheight;

	for (i = 0; (i < num_traces_displayable) && (t); i++) {

		if ((shift_timebase = t->shift)) {
			sbuf[0] = '(';
			reformat_time(sbuf + 1, t->shift, time_dimension);
			strcpy(sbuf + (bufclen = strlen(sbuf + 1) + 1), ")");
			bufclen++;
			bufxlen = gdk_string_measure(signalfont, sbuf);
		} else {
			bufxlen = bufclen = 0;
		}

		if (t->flags & TR_BLANK) {
			if (t->name) {
				len = gdk_string_measure(signalfont, t->name) + bufxlen;
				numchars = strlen(t->name) + bufclen;

				if (len > maxlen)
					maxlen = len;
				if (numchars > ps_nummaxchars)
					ps_nummaxchars = numchars;
			}
		} else if (t->name) {
			len = gdk_string_measure(signalfont, t->name) + bufxlen;
			numchars = strlen(t->name) + bufclen;
			if ((tims.marker != -1) && (!(t->flags & TR_EXCLUDE))) {
				t->asciitime = tims.marker;
				if (t->asciivalue)
					free_2(t->asciivalue);

				if (t->vector) {
					char           *str, *str2;
					vptr            v;

					v = bsearch_vector(t->n.vec, tims.marker);
					str = convert_ascii(t, v);
					if (str) {
						int             slen;

						str2 = (char *) malloc_2(strlen(str) + 2);
						*str2 = '=';
						strcpy(str2 + 1, str);
						free_2(str);

						t->asciivalue = str2;
						if ((slen = strlen(str2)) > ps_maxveclen) {
							str2[ps_maxveclen] = 0;
							str2[ps_maxveclen - 1] = '+';
							vlen = gdk_string_measure(signalfont, str2);
							numchars += ps_maxveclen;
						} else {
							vlen = gdk_string_measure(signalfont, str2);
							numchars += slen;
						}
					} else {
						vlen = 0;
						t->asciivalue = NULL;
					}
				} else {
					char           *str;
					hptr            hptr;

					if ((hptr = bsearch_node(t->n.nd, tims.marker))) {
						if (!t->n.nd->ext) {
							str = (char *) calloc_2(1, 3 * sizeof(char));
							str[0] = '=';
							if (t->flags & TR_INVERT) {
								str[1] = "1XZ0"[hptr->v.val];
							} else {
								str[1] = "0XZ1"[hptr->v.val];
							}
							t->asciivalue = str;
							vlen = gdk_string_measure(signalfont, str);
							numchars += 2;
						} else {
							char           *str2;

							if (hptr->flags & HIST_REAL) {
								if (!(hptr->flags & HIST_STRING)) {
									str =
										convert_ascii_real((double *) hptr->
												   v.vector);
								} else {
									str =
										convert_ascii_string((char *) hptr->
												     v.vector);
								}
							} else {
								str = convert_ascii_vec(t, hptr->v.vector);
							}
							if (str) {
								int             slen;

								str2 = (char *) malloc_2(strlen(str) + 2);
								*str2 = '=';
								strcpy(str2 + 1, str);
								free_2(str);

								t->asciivalue = str2;
								if ((slen = strlen(str2)) > ps_maxveclen) {
									str2[ps_maxveclen] = 0;
									str2[ps_maxveclen - 1] = '+';
									vlen =
										gdk_string_measure(signalfont, str2);
									numchars += ps_maxveclen;
								} else {
									vlen =
										gdk_string_measure(signalfont, str2);
									numchars += slen;
								}
							} else {
								vlen = 0;
								t->asciivalue = NULL;
							}
						}
					} else {
						vlen = 0;
						t->asciivalue = NULL;
					}
				}

				len += vlen;
			}
			if (len > maxlen)
				maxlen = len;
			if (numchars > ps_nummaxchars)
				ps_nummaxchars = numchars;
		}
		t = t->next;
	}

	maxlen += 6;		/* endcap padding */
	if (maxlen < 60)
		maxlen = 60;

	return maxlen;
}

/**********************************************/

static void
pr_renderhash(pr_context * prc, int x, TimeType tim)
{
	TimeType        rborder;
	int             hashoffset;
	int             fhminus2;
	int             rhs;

	fhminus2 = fontheight - 2;

	pr_setgray(prc, 0.75);

	pr_draw_line(prc, x, 0,
		     x, ((!timearray) && (display_grid)
			 && (enable_vert_grid)) ? liney_max : fhminus2);

	if (tim == tims.last)
		return;

	rborder = (tims.last - tims.start) * pxns;
	DEBUG(printf("Rborder: %lld, Wavewidth: %d\n", rborder, wavewidth));

	if (rborder > wavewidth)
		rborder = wavewidth;
	if ((rhs = x + pixelsperframe) > rborder)
		rhs = rborder;
	pr_draw_line(prc, x, wavecrosspiece, rhs, wavecrosspiece);

	x += (hashoffset = hashstep);

	while ((hashoffset < pixelsperframe) && (x <= rhs)) {
		pr_draw_line(prc, x, wavecrosspiece, x, fhminus2);

		hashoffset += hashstep;
		x += hashstep;
	}

}


static void 
pr_renderblackout(pr_context * prc)
{
gfloat pageinc;
TimeType lhs, rhs, lclip, rclip;
struct blackout_region_t *bt = blackout_regions;
                         
if(bt)
        {
        pageinc=(gfloat)(((gdouble)wavewidth)*nspx);
        lhs = tims.start;
        rhs = pageinc + lhs;
        
        while(bt)
                {
                if( ((bt->bstart <= lhs) && (bt->bend >= lhs)) || ((bt->bstart >= lhs) && (bt->bstart <= rhs)) )
                        {
                        lclip = bt->bstart; rclip = bt->bend;
                
                        if(lclip < lhs) lclip = lhs;
                                else if (lclip > rhs) lclip = rhs;

                        if(rclip < lhs) rclip = lhs;
                 
                        lclip -= lhs;
                        rclip -= lhs;
                        if(rclip>(wavewidth+1)) rclip = wavewidth+1;

			pr_setgray(prc, 0.80);
                        pr_draw_box(prc,
                                (((gdouble)lclip)*pxns), fontheight,
                                (((gdouble)(rclip))*pxns), waveheight-fontheight);
                        }
                 
                bt=bt->next;
                }
        }
}


static void
pr_rendertimes(pr_context * prc)
{
	TimeType        tim, rem;
	int             x, len;
	char            timebuff[32];

	pr_renderblackout(prc);

	tim = tims.start;
	tims.end = tims.start + (((gdouble) wavewidth) * nspx);

	/**********/
	if (timearray) {
		int             pos, pos2;
		TimeType       *t, tm;
		int             y = fontheight + 2;
		int             oldx = -1;

		pos = bsearch_timechain(tims.start);
top:
		if ((pos >= 0) && (pos < timearray_size)) {
			pr_setgray(prc, 0.90);
			t = timearray + pos;
			for (; pos < timearray_size; t++, pos++) {
				tm = *t;
				if (tm >= tims.start) {
					if (tm <= tims.end) {
						x = (tm - tims.start) * pxns;
						if (oldx == x) {
							pos2 =
								bsearch_timechain(tims.start +
										  (((gdouble) (x + 1)) * nspx));
							if (pos2 > pos) {
								pos = pos2;
								goto top;
							} else
								continue;
						}
						oldx = x;
						pr_draw_line(prc, x, y, x, liney_max);
					} else {
						break;
					}
				}
			}
		}
	}
	/**********/

	DEBUG(printf
	      ("Ruler Start time: " TTFormat ", Finish time: " TTFormat "\n",
	       tims.start, tims.end));

	x = 0;
	if (tim) {
		rem = tim % nsperframe;
		if (rem) {
			tim = tim - nsperframe - rem;
			x = -pixelsperframe - ((rem * pixelsperframe) / nsperframe);
		}
	}
	for (;;) {
		pr_renderhash(prc, x, tim);
		if (tim) {
			reformat_time(timebuff, time_trunc(tim), time_dimension);
		} else {
			strcpy(timebuff, "0");
		}

		len = gdk_string_measure(wavefont, timebuff);

		if ((prc->gpd == &ps_print_device)
		|| ((x - (len >> 1) >= 0) && (x + (len >> 1) < wavewidth))) {
			pr_setgray(prc, 0.0);
			pr_draw_string(prc,
				       x - (len >> 1), wavefont->ascent + 1,
				       timebuff, len, wavefont->ascent);
		}
		tim += nsperframe;
		x += pixelsperframe;
		if ((x >= wavewidth) || (tim > tims.last))
			break;
	}
}

/*************************************************/

static void
pr_draw_named_markers(pr_context * prc)
{
	gdouble         pixstep;
	gint            xl, y;
	int             i;
	TimeType        t;

	pixstep = ((gdouble) nsperframe) / ((gdouble) pixelsperframe);

	for (i = 0; i < 26; i++) {
		if ((t = named_markers[i]) != -1) {
			if ((t >= tims.start) && (t <= tims.last) && (t <= tims.end)) {
				xl = ((gdouble) (t - tims.start)) / pixstep;	/* snap to integer */
				if ((xl >= 0) && (xl < wavewidth)) {
					char            nbuff[2];
					int             xsize;
					nbuff[0] = 'A' + i;
					nbuff[1] = 0x00;

					pr_setgray(prc, 0.40);
					for (y = fontheight - 1; y <= liney_max - 5; y += 8) {
						pr_draw_line(prc, xl, y, xl, y + 5);
					}

					xsize = gdk_string_measure(wavefont, nbuff);
					pr_setgray(prc, 0.00);
					pr_draw_string(prc,
						       xl - (xsize >> 1) + 1,
						       fontheight - 1, nbuff,
						       xsize,
						       (prc->gpd ==
							&ps_print_device) ? wavefont->ascent /
						       2 : wavefont->ascent);
				}
			}
		}
	}
}


static void
pr_draw_marker(pr_context * prc)
{
	gdouble         pixstep;
	gint            xl;

	if (tims.marker != -1) {
		if ((tims.marker >= tims.start) && (tims.marker <= tims.last)
		    && (tims.marker <= tims.end)) {
			pixstep = ((gdouble) nsperframe) / ((gdouble) pixelsperframe);
			xl = ((gdouble) (tims.marker - tims.start)) / pixstep;	/* snap to integer */
			if ((xl >= 0) && (xl < wavewidth)) {
				pr_setgray(prc, 0.40);
				pr_draw_line(prc, xl, fontheight - 1, xl, liney_max);
			}
		}
	}
}

/*************************************************/

/*
 * draw single traces and use this for rendering the grid lines for
 * "excluded" traces 
 */
static void 
pr_draw_hptr_trace(pr_context * prc, Trptr t, hptr h, int which, int dodraw)
{
	TimeType        x0, x1, newtime;
	int             y0, y1, yu, liney;
	TimeType        tim, h2tim;
	hptr            h2, h3;
	char            hval, h2val;

	tims.start -= shift_timebase;
	tims.end -= shift_timebase;

	liney = ((which + 2) * fontheight) - 2;
	if (t && (t->flags & TR_INVERT)) {
		y0 = ((which + 1) * fontheight) + 2;
		y1 = liney - 2;
	} else {
		y1 = ((which + 1) * fontheight) + 2;
		y0 = liney - 2;
	}

	yu = (y0 + y1) / 2;

	if ((display_grid) && (enable_horiz_grid)) {
		pr_setgray(prc, 0.75);
		pr_draw_line(prc,
			     (tims.start < tims.first) ? (tims.first - tims.start) * pxns : 0, liney,
			     (tims.last <= tims.end) ? (tims.last - tims.start) * pxns : wavewidth - 1, liney);
	}
	pr_setgray(prc, 0.0);
	if ((h) && (tims.start == h->time)) {
		pr_draw_line(prc, 0, y0, 0, y1);
	}
	if (dodraw)
		for (;;) {
			if (!h)
				break;
			tim = (h->time);
			if ((tim > tims.end) || (tim > tims.last))
				break;

			x0 = (tim - tims.start) * pxns;
			if (x0 < -1) {
				x0 = -1;
			} else if (x0 > wavewidth) {
				break;
			}
			h2 = h->next;
			if (!h2)
				break;
			h2tim = tim = (h2->time);
			if (tim > tims.last)
				tim = tims.last;
			else if (tim > tims.end + 1)
				tim = tims.end + 1;
			x1 = (tim - tims.start) * pxns;
			if (x1 < -1) {
				x1 = -1;
			} else if (x1 > wavewidth) {
				x1 = wavewidth;
			}
			if (x0 != x1) {
				hval = h->v.val;
				h2val = h2->v.val;

				switch (hval) {
				case 0:	/* 0 */
					pr_draw_line(prc, x0, y0, x1, y0);

					if (h2tim <= tims.end)
						switch (h2val) {
						case 2:
							pr_draw_line(prc, x1, y0, x1, yu);
							break;
						default:
							pr_draw_line(prc, x1, y0, x1, y1);
							break;
						}
					break;

				case 1:	/* X */
					pr_setgray(prc, 0.70);
					pr_draw_box(prc, x0, y0, x1, y1);
					pr_setgray(prc, 0.0);

					if (x0 >= 0)
						pr_draw_line(prc, x0, y0, x0, y1);

					pr_draw_line(prc, x0, y0, x1, y0);
					pr_draw_line(prc, x0, y1, x1, y1);
					if (h2tim <= tims.end)
						pr_draw_line(prc, x1, y0, x1, y1);
					break;

				case 2:	/* Z */
					pr_draw_line(prc, x0, yu, x1, yu);
					if (h2tim <= tims.end)
						switch (h2val) {
						case 0:
							pr_draw_line(prc, x1, yu, x1, y0);
							break;
						case 3:
							pr_draw_line(prc, x1, yu, x1, y1);
							break;
						default:
							pr_draw_line(prc, x1, y0, x1, y1);
							break;
						}
					break;

				case 3:	/* 1 */
					pr_draw_line(prc, x0, y1, x1, y1);
					if (h2tim <= tims.end)
						switch (h2val) {
						case 0:
							pr_draw_line(prc, x1, y1, x1, y0);
							break;
						case 2:
							pr_draw_line(prc, x1, y1, x1, yu);
							break;
						default:
							pr_draw_line(prc, x1, y0, x1, y1);
							break;
						}
					/* fallthrough on break */

				default:
					break;
				}
			} else {
				pr_draw_line(prc, x1, y0, x1, y1);
				newtime = (((gdouble) (x1 + WAVE_OPT_SKIP)) * nspx) + tims.start + shift_timebase;	/* skip to next pixel */
				h3 = bsearch_node(t->n.nd, newtime);
				if (h3->time > h->time) {
					h = h3;
					continue;
				}
			}

			h = h->next;
		}

	tims.start += shift_timebase;
	tims.end += shift_timebase;
}


/*
 * draw hptr vectors (integer+real) 
 */
static void 
pr_draw_hptr_trace_vector(pr_context * prc, Trptr t, hptr h, int which)
{
	TimeType        x0, x1, newtime, width;
	int             y0, y1, yu, liney, ytext;
	TimeType        tim, h2tim;
	hptr            h2, h3;
	char           *ascii = NULL;
	int             pixlen, ysiz;
	int             type;
	int             lasttype = -1;

	tims.start -= shift_timebase;
	tims.end -= shift_timebase;

	liney = ((which + 2) * fontheight) - 2;
	y1 = ((which + 1) * fontheight) + 2;
	y0 = liney - 2;
	yu = (y0 + y1) / 2;
	ytext = yu - (wavefont->ascent / 2) + wavefont->ascent;

	ysiz = wavefont->ascent - 1;
	if (ysiz < 1)
		ysiz = 1;


	if ((display_grid) && (enable_horiz_grid)) {
		pr_setgray(prc, 0.75);
		pr_draw_line(prc,
			     (tims.start < tims.first) ? (tims.first - tims.start) * pxns : 0, liney,
			     (tims.last <= tims.end) ? (tims.last - tims.start) * pxns : wavewidth - 1, liney);
	}
	pr_setgray(prc, 0.0);
	for (;;) {
		if (!h)
			break;
		tim = (h->time);
		if ((tim > tims.end) || (tim > tims.last))
			break;

		x0 = (tim - tims.start) * pxns;
		if (x0 < -1)
			x0 = -1;
		else if (x0 > wavewidth)
			break;

		h2 = h->next;
		if (!h2)
			break;
		h2tim = tim = (h2->time);
		if (tim > tims.last)
			tim = tims.last;
		else if (tim > tims.end + 1)
			tim = tims.end + 1;
		x1 = (tim - tims.start) * pxns;

		if (x1 < -1)
			x1 = -1;
		else if (x1 > wavewidth)
			x1 = wavewidth;

		/* draw trans */
		type = (!(h->flags&(HIST_REAL|HIST_STRING))) ? vtype(t,h->v.vector) : 0;
		if (x0 > -1)
			if (use_roundcaps) {
				if (type == 2) {
					if (lasttype != -1) {
						pr_draw_line(prc,
							     x0 - 1, y0,
							     x0, yu);
						pr_draw_line(prc,
							     x0, yu,
							     x0 - 1, y1);
					}
				} else if (lasttype == 2) {
					pr_draw_line(prc,
						     x0 + 1, y0,
						     x0, yu);
					pr_draw_line(prc,
						     x0, yu,
						     x0 + 1, y1);
				} else {
					if (lasttype != type) {
						pr_draw_line(prc,
							     x0 - 1, y0,
							     x0, yu);
						pr_draw_line(prc,
							     x0, yu,
							     x0 - 1, y1);
						pr_draw_line(prc,
							     x0 + 1, y0,
							     x0, yu);
						pr_draw_line(prc,
							     x0, yu,
							     x0 + 1, y1);
					} else {
						pr_draw_line(prc,
							     x0 - 2, y0,
							     x0 + 2, y1);
						pr_draw_line(prc,
							     x0 + 2, y0,
							     x0 - 2, y1);
					}
				}
			} else {
				pr_draw_line(prc,
					     x0, y0,
					     x0, y1);
			}


		if (x0 != x1) {
			if (type == 2) {
				if (use_roundcaps) {
					pr_draw_line(prc,
						     x0 + 1, yu,
						     x1 - 1, yu);
				} else {
					pr_draw_line(prc,
						     x0, yu,
						     x1, yu);
				}
			} else {

				if (use_roundcaps) {
					pr_draw_line(prc,
						     x0 + 2, y0,
						     x1 - 2, y0);
					pr_draw_line(prc,
						     x0 + 2, y1,
						     x1 - 2, y1);
				} else {
					pr_draw_line(prc,
						     x0, y0,
						     x1, y0);
					pr_draw_line(prc,
						     x0, y1,
						     x1, y1);
				}


				if (x0 < 0)
					x0 = 0;	/* fixup left margin */

				width = ((prc->gpd == &ps_print_device) || (x1 < wavewidth)) ? x1 - x0 : wavewidth - x0;	/* truncate render
																 * window for non-ps */

				if (width > vector_padding) {
					if (h->flags & HIST_REAL) {
						if (!(h->flags & HIST_STRING)) {
							ascii = convert_ascii_real((double *) h->v.vector);
						} else {
							ascii = convert_ascii_string((char *) h->v.vector);
						}
					} else {
						ascii = convert_ascii_vec(t, h->v.vector);
					}


					if (((pixlen = gdk_string_measure(wavefont, ascii)) + vector_padding <= width) || ((x1 >= wavewidth) && (prc->gpd == &ps_print_device))) {
						pr_draw_string(prc, x0 + 2, ytext, ascii, pixlen, ysiz);
					} else {
						char           *mod;

						mod = bsearch_trunc(ascii, width - vector_padding);
						if (mod) {
							*mod = '+';
							*(mod + 1) = 0;
							pr_draw_string(prc, x0 + 2, ytext, ascii, maxlen_trunc, ysiz);
						}
					}
				}
			}
		} else {
			newtime = (((gdouble) (x1 + WAVE_OPT_SKIP)) * nspx) + tims.start + shift_timebase;	/* skip to next pixel */
			h3 = bsearch_node(t->n.nd, newtime);
			if (h3->time > h->time) {
				h = h3;
				lasttype = type;
				continue;
			}
		}

		if (ascii) {
			free_2(ascii);
			ascii = NULL;
		}
		h = h->next;
		lasttype = type;
	}

	tims.start += shift_timebase;
	tims.end += shift_timebase;
}


/*
 * draw vector traces 
 */
static void 
pr_draw_vptr_trace(pr_context * prc, Trptr t, vptr v, int which)
{
	TimeType        x0, x1, newtime, width;
	int             y0, y1, yu, liney, ytext;
	TimeType        tim, h2tim;
	vptr            h, h2, h3;
	char           *ascii = NULL;
	int             pixlen, ysiz;
	int             type;
	int             lasttype = -1;

	tims.start -= shift_timebase;
	tims.end -= shift_timebase;

	h = v;
	liney = ((which + 2) * fontheight) - 2;
	y1 = ((which + 1) * fontheight) + 2;
	y0 = liney - 2;
	yu = (y0 + y1) / 2;
	ytext = yu - (wavefont->ascent / 2) + wavefont->ascent;

	ysiz = wavefont->ascent - 1;
	if (ysiz < 1)
		ysiz = 1;

	if ((display_grid) && (enable_horiz_grid)) {
		pr_setgray(prc, 0.75);
		pr_draw_line(prc,
			     (tims.start < tims.first) ? (tims.first - tims.start) * pxns : 0, liney,
			     (tims.last <= tims.end) ? (tims.last - tims.start) * pxns : wavewidth - 1, liney);
	}
	pr_setgray(prc, 0.0);
	for (;;) {
		if (!h)
			break;
		tim = (h->time);
		if ((tim > tims.end) || (tim > tims.last))
			break;

		x0 = (tim - tims.start) * pxns;
		if (x0 < -1)
			x0 = -1;
		else if (x0 > wavewidth)
			break;

		h2 = h->next;
		if (!h2)
			break;
		h2tim = tim = (h2->time);
		if (tim > tims.last)
			tim = tims.last;
		else if (tim > tims.end + 1)
			tim = tims.end + 1;
		x1 = (tim - tims.start) * pxns;
		if (x1 < -1)
			x1 = -1;
		else if (x1 > wavewidth)
			x1 = wavewidth;

		/* draw trans */
		type = vtype2(t, h);
		if (x0 > -1)
			if (use_roundcaps) {
				if (type == 2) {
					if (lasttype != -1) {
						pr_draw_line(prc,
							     x0 - 1, y0,
							     x0, yu);
						pr_draw_line(prc,
							     x0, yu,
							     x0 - 1, y1);
					}
				} else if (lasttype == 2) {
					pr_draw_line(prc,
						     x0 + 1, y0,
						     x0, yu);
					pr_draw_line(prc,
						     x0, yu,
						     x0 + 1, y1);
				} else {
					if (lasttype != type) {
						pr_draw_line(prc,
							     x0 - 1, y0,
							     x0, yu);
						pr_draw_line(prc,
							     x0, yu,
							     x0 - 1, y1);
						pr_draw_line(prc,
							     x0 + 1, y0,
							     x0, yu);
						pr_draw_line(prc,
							     x0, yu,
							     x0 + 1, y1);
					} else {
						pr_draw_line(prc,
							     x0 - 2, y0,
							     x0 + 2, y1);
						pr_draw_line(prc,
							     x0 + 2, y0,
							     x0 - 2, y1);
					}
				}
			} else {
				pr_draw_line(prc,
					     x0, y0,
					     x0, y1);
			}


		if (x0 != x1) {
			if (type == 2) {
				if (use_roundcaps) {
					pr_draw_line(prc,
						     x0 + 1, yu,
						     x1 - 1, yu);
				} else {
					pr_draw_line(prc,
						     x0, yu,
						     x1, yu);
				}
			} else {

				if (use_roundcaps) {
					pr_draw_line(prc,
						     x0 + 2, y0,
						     x1 - 2, y0);
					pr_draw_line(prc,
						     x0 + 2, y1,
						     x1 - 2, y1);
				} else {
					pr_draw_line(prc,
						     x0, y0,
						     x1, y0);
					pr_draw_line(prc,
						     x0, y1,
						     x1, y1);
				}

				if (x0 < 0)
					x0 = 0;	/* fixup left margin */

				width = ((prc->gpd == &ps_print_device) || (x1 < wavewidth)) ? x1 - x0 : wavewidth - x0;	/* truncate render
																 * window for non-ps */

				if (width > vector_padding) {
					ascii = convert_ascii(t, h);

					if (((pixlen = gdk_string_measure(wavefont, ascii)) + vector_padding <= width) || ((x1 >= wavewidth) && (prc->gpd == &ps_print_device))) {
						pr_draw_string(prc, x0 + 2, ytext, ascii, pixlen, ysiz);
					} else {
						char           *mod;

						mod = bsearch_trunc(ascii, width - vector_padding);
						if (mod) {
							*mod = '+';
							*(mod + 1) = 0;

							pr_draw_string(prc,
								       x0 + 2, ytext, ascii, maxlen_trunc, ysiz);
						}
					}
				}
			}
		} else {
			newtime = (((gdouble) (x1 + WAVE_OPT_SKIP)) * nspx) + tims.start + shift_timebase;	/* skip to next pixel */
			h3 = bsearch_vector(t->n.vec, newtime);
			if (h3->time > h->time) {
				h = h3;
				lasttype = type;
				continue;
			}
		}

		if (ascii) {
			free_2(ascii);
			ascii = NULL;
		}
		h = h->next;
		lasttype = type;
	}

	tims.start += shift_timebase;
	tims.end += shift_timebase;
}

static void
pr_rendertraces(pr_context * prc)
{
	if (!topmost_trace) {
		topmost_trace = traces.first;
	}
	if (topmost_trace) {
		Trptr           t;
		hptr            h;
		vptr            v;
		int             i, num_traces_displayable;

		num_traces_displayable = wavearea->allocation.height / (fontheight);
		num_traces_displayable--;	/* for the time trace that is
						 * always there */

		t = topmost_trace;
		for (i = 0; ((i < num_traces_displayable) && (t)); i++) {
			if (!(t->flags & (TR_EXCLUDE | TR_BLANK))) {
				shift_timebase = t->shift;
				if (!t->vector) {
					h = bsearch_node(t->n.nd, tims.start);
					DEBUG(printf
					      ("Bit Trace: %s, %s\n", t->name, t->n.nd->nname));
					DEBUG(printf
					      ("Start time: " TTFormat ", Histent time: " TTFormat
					       "\n", tims.start, (h->time + shift_timebase)));

					if (!t->n.nd->ext) {
						pr_draw_hptr_trace(prc, t, h, i, 1);
					} else {
						pr_draw_hptr_trace_vector(prc, t, h, i);
					}
				} else {
					v = bsearch_vector(t->n.vec, tims.start);
					DEBUG(printf
					      ("Vector Trace: %s, %s\n", t->name, t->n.vec->name));
					DEBUG(printf
					      ("Start time: " TTFormat ", Vectorent time: "
					       TTFormat "\n", tims.start,
					       (v->time + shift_timebase)));
					pr_draw_vptr_trace(prc, t, v, i);
				}
			} else {
				pr_draw_hptr_trace(prc, NULL, NULL, i, 0);
			}
			t = t->next;
		}
	}
	pr_draw_named_markers(prc);
	pr_draw_marker(prc);
}

/**********************************************/

static int
pr_RenderSig(pr_context * prc, Trptr t, int i)
{
	int             texty, liney;
	int             retval;
	char            sbuf[65];
	int             bufclen;

	UpdateSigValue(t);	/* in case it's stale on nonprop */

	if ((t->name) && (t->shift)) {
		sbuf[0] = '(';
		reformat_time(sbuf + 1, t->shift, time_dimension);
		strcpy(sbuf + (bufclen = strlen(sbuf + 1) + 1), ")");
		bufclen++;
	} else {
		bufclen = 0;
	}

	liney = ((i + 2) * fontheight) - 2;

	texty = liney - (signalfont->descent);

	retval = liney - fontheight + 1;

	if (t->flags & TR_HIGHLIGHT)
		pr_setgray(prc, 0.75);
	else
		pr_setgray(prc, 0.95);

	pr_draw_box(prc, 2, retval + 1,
		    pr_signal_fill_width - 2, retval + fontheight - 3);
	pr_setgray(prc, 0.75);

	pr_draw_line(prc, 0, liney, pr_signal_fill_width - 1, liney);

	/* if (!(t->flags&TR_BLANK)) */
	{
		int             maxwidth = 0;

		if (t->name)
			maxwidth = strlen(t->name) + bufclen;
		if ((t->asciivalue) && (!(t->flags & TR_EXCLUDE)))
			maxwidth += strlen(t->asciivalue);
		if (maxwidth) {
			gdouble         realwidth;
			char           *buf;
			buf = wave_alloca(maxwidth + 1);
			buf[0] = 0;
			if (t->name) {
				strcpy(buf, t->name);
				if (bufclen)
					strcat(buf, sbuf);
			}
			if ((t->asciivalue) && (!(t->flags & TR_EXCLUDE)))
				strcat(buf, t->asciivalue);

			realwidth = maxwidth * ps_chwidth;

			if (maxwidth == 0)
				return (retval);
			pr_setgray(prc, 0.0);
			pr_draw_string(prc, 3, texty - 1, buf, realwidth,
				  signalfont->ascent - signalfont->descent);
		}
	}

	return (retval);
}

static void
pr_RenderSigs(pr_context * prc, int trtarget)
{
	Trptr           t;
	int             i, trwhich, width;
	int             num_traces_displayable;
	GtkAdjustment  *hadj;
	gint            xsrc;

	hadj = GTK_ADJUSTMENT(signal_hslider);
	xsrc = (gint) hadj->value;

	num_traces_displayable = signalarea->allocation.height / (fontheight);
	num_traces_displayable--;	/* for the time trace that is always
					 * there */

	pr_setgray(prc, 0.75);
	pr_draw_line(prc, 0, fontheight - 1, pr_signal_fill_width - 1,
		     fontheight - 1);
	pr_setgray(prc, 0.0);
	pr_draw_string(prc, 3, fontheight, "Time",
		       (width =
		     gdk_string_measure(wavefont, "Time")) * 2, fontheight);

	ps_chwidth =
		((gdouble) (pr_signal_fill_width - 6)) / ((gdouble) (ps_nummaxchars));

	t = traces.first;
	trwhich = 0;
	while (t) {
		if ((trwhich < trtarget) && (t->next)) {
			trwhich++;
			t = t->next;
		} else {
			break;
		}
	}

	topmost_trace = t;
	if (t) {
		for (i = 0; (i < num_traces_displayable) && (t); i++) {
			pr_RenderSig(prc, t, i);
			t = t->next;
		}
	}
}

/**********************************************/

void
print_image(pr_context * prc)
{
	GtkAdjustment  *sadj;
	int             trtarget;

	if ((traces.total + 1) * fontheight > wavearea->allocation.height)
		liney_max = wavearea->allocation.height;
	else
		liney_max = (traces.total + 1) * fontheight;

	pr_signal_fill_width = ps_MaxSignalLength();
	pr_wave_init(prc);
	pr_header(prc);
	pr_rendertimes(prc);
	pr_rendertraces(prc);

	pr_signal_init(prc);

	sadj = GTK_ADJUSTMENT(wave_vslider);
	trtarget = (int) (sadj->value);
	pr_RenderSigs(prc, trtarget);

	pr_trailer(prc);
}

void
print_ps_image(FILE * wave, gdouble px, gdouble py)
{
	pr_context      prc;

	prc.gpd = &ps_print_device;
	prc.PageX = px;		/* Legal page width */
	prc.PageY = py;		/* Legal page height */
	prc.LM = 1;		/* Left Margin (inch) */
	prc.RM = 1;		/* Right Margin (inch) */
	prc.BM = 1;		/* Bottom Margin (inch) */
	prc.TM = 1;		/* Top Margin (inch) */
	prc.handle = wave;
	prc.fullpage = ps_fullpage;
	print_image(&prc);
}

void
print_mif_image(FILE * wave, gdouble px, gdouble py)
{
	pr_context      prc;

	prc.gpd = &mif_print_device;
	prc.PageX = px;		/* Legal page width */
	prc.PageY = py;		/* Legal page height */
	prc.LM = 1;		/* Left Margin (inch) */
	prc.RM = 1;		/* Right Margin (inch) */
	prc.BM = 1;		/* Bottom Margin (inch) */
	prc.TM = 1;		/* Top Margin (inch) */
	prc.tr_x = 0;
	prc.tr_y = 0;
	prc.handle = wave;
	prc.fullpage = ps_fullpage;
	print_image(&prc);
}
