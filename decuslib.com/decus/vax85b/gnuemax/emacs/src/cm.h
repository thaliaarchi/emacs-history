/* Cursor motion calculation definitions for GNU Emacs 
   Copyright (C) 1985 Richard M. Stallman. 
 
This file is part of GNU Emacs. 
 
GNU Emacs is distributed in the hope that it will be useful, 
but without any warranty.  No author or distributor 
accepts responsibility to anyone for the consequences of using it 
or for whether it serves any particular purpose or works at all, 
unless he says so in writing. 
 
Everyone is granted permission to copy, modify and redistribute 
GNU Emacs, but only under the conditions described in the 
document "GNU Emacs copying permission notice".   An exact copy 
of the document is supposed to have been given to you along with 
GNU Emacs so that you can know how you may redistribute it all. 
It should be in a file named COPYING.  Among other things, the 
copyright notice and this notice must be preserved on all copies.  */ 
 
 
 
/* This structure holds everything needed to do cursor motion except the pad 
   character (PC) and the output speed of the terminal (ospeed), which 
   termcap wants in global variables. */ 
 
struct cm { 
	/* Cursor position */ 
	int	cm_curY,	/* current row */ 
		cm_curX;	/* current column */ 
				/* -1 in either one means position unknown */ 
	/* Capabilities from termcap(5) (including extensions) */ 
	char	*cm_up,		/* up (up) */ 
		*cm_down,	/* down (do) */ 
		*cm_left,	/* left (bs) */ 
		*cm_right,	/* right (nd) */ 
		*cm_home,	/* home (ho) */ 
		*cm_cr,		/* carriage return (cr) */ 
		*cm_ll,		/* last line (ll) */ 
		*cm_abs,	/* absolute (cm) */ 
		*cm_habs,	/* horizontal absolute (ch) */ 
		*cm_vabs,	/* vertical absolute (cv) */ 
		*cm_ds,		/* "don't send" string (ds) */ 
		*cm_tab;	/* tab (ta) */ 
	int	cm_tabwidth,	/* tab width (tw) */ 
		cm_cols,	/* Number of cols on screen (co) */ 
		cm_rows,	/* Number of rows on screen (li) */ 
		cm_autowrap:1,	/* autowrap flag (am) */ 
		cm_magicwrap:1,	/* vt100s: cursor stays in last col but 
				   will wrap if next char is printing (xn) */ 
		cm_usetabs:1,	/* if set, use tabs */ 
		cm_autolf:1;	/* \r performs a \r\n (rn) */ 
	/* Costs */ 
	int	cc_up,		/* cost for up */ 
		cc_down,	/* etc */ 
		cc_left, 
		cc_right, 
		cc_home, 
		cc_cr, 
		cc_ll, 
		cc_abs,		/* abs costs are actually min costs */ 
		cc_habs, 
		cc_vabs, 
		cc_tab; 
	/* Functions */ 
	int	(*cx_costinit) (),/* initialize cost factors */ 
		(*cx_goto) (),	/* (*cx_goto) (row, col) moves cursor to 
				   specified coordinates (0 origin!!) */ 
		(*cx_put) ();	/* points to putc function (for tputs) */ 
} Wcm; 
 
char	PC;			/* Pad character */ 
short	ospeed;			/* Output speed (from sg_ospeed) */ 
 
/* Shorthand */ 
#ifndef NoCMShortHand 
#define	curY		Wcm.cm_curY 
#define	curX		Wcm.cm_curX 
#define	Up		Wcm.cm_up 
#define	Down		Wcm.cm_down 
#define	Left		Wcm.cm_left 
#define	Right		Wcm.cm_right 
#define	Home		Wcm.cm_home 
#define	CR		Wcm.cm_cr 
#define	LastLine	Wcm.cm_ll 
#define	Tab		Wcm.cm_tab 
#define	TabWidth	Wcm.cm_tabwidth 
#define	DontSend	Wcm.cm_ds 
#define	AbsPosition	Wcm.cm_abs 
#define	ColPosition	Wcm.cm_habs 
#define	RowPosition	Wcm.cm_vabs 
#define	AutoWrap	Wcm.cm_autowrap 
#define	MagicWrap	Wcm.cm_magicwrap 
#define	UseTabs		Wcm.cm_usetabs 
#define	AutoLF		Wcm.cm_autolf 
#define	ScreenRows	Wcm.cm_rows 
#define	ScreenCols	Wcm.cm_cols 
 
#define	cmcostinit()	(*Wcm.cx_costinit) () 
#define	cmgoto(row,col)	(*Wcm.cx_goto) (row, col) 
#define	cmat(row,col)	(curY = (row), curX = (col)) 
#define	cmplus(n)	{if ((curX += (n)) >= ScreenCols && !MagicWrap)\ 
				if (AutoWrap) curX = 0, curY++; else curX--;} 
#define	cmputc		Wcm.cx_put 
#endif 
