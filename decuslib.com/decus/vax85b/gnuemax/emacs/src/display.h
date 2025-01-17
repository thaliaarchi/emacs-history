/* Definitions for screen updating from screen lines */ 
 
/*		Copyright (c) 1981,1980 James Gosling 
   Enhancements copyright (c) 1984 Fen Labalme, Chris Torek 
      and Richard Stallman. 
   Distributed by Fen Labalme, with permission from James Gosling, 
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
 
#include "disp_extern.h" 
 
int	IDdebug,		/* line insertion/deletion debug switch */ 
	RDdebug;		/* line redraw debug switch */ 
 
int VisibleBell;		/* If true and the terminal will support it 
				   then the screen will flash instead of 
				   feeping when an error occurs */ 
int InverseVideo;	/* If true and the terminal will support it 
				   then we will use inverse video */ 
 
int WindowSize;		/* the number of lines on which line ID 
				   operations should be done */ 
int baud_rate;			/* Terminal speed, so we can calculate 
				   the number of characters required to 
				   make the cursor sit still for n secs. */ 
 
struct display_line *FreeLines;	/* free space list */ 
 
 /* the current (physical) screen */ 
struct display_line *PhysScreen[MScreenLength + 1]; 
 
 /* temporary Copy of PhysScreen made in UpdateScreen */ 
struct display_line *OPhysScreen[MScreenLength + 1]; 
 
 /* the desired (virtual) screen */ 
struct display_line *DesiredScreen[MScreenLength + 1]; 
 
 
/* the following macros are used to access terminal specific routines. 
   Really, no one outside of display.c should be using them, except for 
   the initialize/cleanup routines */ 
#define topos (*tt.t_topos) 
#define reset (*tt.t_reset) 
#define INSmode (*tt.t_INSmode) 
#define insertlines (*tt.t_inslines) 
#define deletelines (*tt.t_dellines) 
#define blanks (*tt.t_blanks) 
#define wipeline (*tt.t_wipeline) 
#define wipescreen (*tt.t_wipescreen) 
#define deletechars (*tt.t_delchars) 
#define dumpstring (*tt.t_writechars) 
 
#include "fixedpoint.h" 
 
struct Msquare { 
    fixedpoint cost;		/* the value of Mij */ 
    short      fromi,		/* the coordinates of the square that */ 
	       fromj;		/* gives the minimum cost */ 
}; 
