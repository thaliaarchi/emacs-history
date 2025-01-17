/* Interface definitions for display code. 
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
#include "Trm.h" 
 
#define ScreenLength (tt.t_length) 
#define ScreenWidth (tt.t_width) 
 
/* Nonzero means do not assume anything about current 
 contents of actual terminal screen */ 
 
int ScreenGarbaged; 
 
/* Desired terminal cursor position (to show position of dot), 
 origin one */ 
 
int cursX, cursY; 
 
/* Nonzero means last display completed and cursor is really at cursX, cursY. 
 Zero means it was preempted. */ 
 
int display_completed; 
 
/* Display line structure. 
This structure records the contents of a line 
either as already on the display 
or as we desire to have it on the display. 
 
PhysScreen is a vector of pointers to lines 
 describing the actual contents of the screen. 
DesiredScreen is a vector of pointers to lines 
 describing what we want to put on the screen. 
 These were made from the buffers being displayed 
 by the file window.c 
 
The code in this file compares those two vectors of display lines 
 and performs the updating. 
 
As display lines are used only to go in those vectors, 
 the most display lines that ever ought to exist is 
 twice the maximum screen size.  There is an error check 
 to detect creating more than this many. 
*/ 
 
/* The following atrocity is to save space on most machines, but 
   run efficiently on the Vax, where accessing a longword is faster 
   than a word or a byte... */ 
#ifdef vax 
typedef int small;		/* a (relatively) small number */ 
#else vax 
typedef short small; 
#endif vax 
 
struct display_line {		/* a line as it appears in a list of 
				   lines (as in the physical and virtual 
				   display lists) */ 
    int     hash;		/* hash value for this line, 0 if not 
				   known */ 
    struct display_line *next;		/* pointer to the next line in a list of 
				   lines */ 
    small   DrawCost;		/* the cost of redrawing this line */ 
    small   length;		/* the number of valid characters in the 
				   line */ 
    char    highlighted;	/* true iff this line is to be 
				   highlighted */ 
    char    physical;		/* nonzero means line appears in PhysScreen */ 
    char    body[MScreenWidth + 4];	/* the actual text of the line */ 
}; 
 
 
/* Allocate a line structure for screen line `vpos' (origin 0) 
 to start output in it at `hpos' (origin 0, may be negative) */ 
 
struct display_line *get_display_line (), *new_display_line (); 
