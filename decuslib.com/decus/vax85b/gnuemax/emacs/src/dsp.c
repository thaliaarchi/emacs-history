/* Display initialization */ 
 
/*		Copyright (c) 1981,1980 James Gosling 
   Enhancements copyright (c) 1984 Fen Labalme and Richard Stallman 
   Distributed by Fen Labalme, with permission from James Gosling. 
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
#include "config.h" 
#include <stdio.h> 
#include "disp_extern.h" 
#include <sys/ioctl.h> 
#include <sgtty.h> 
#include <fcntl.h> 
 
struct sgttyb old;		/* The initial tty mode bits */ 
static struct tchars OldTchars; 
static struct ltchars OldLtchars; 
static int OldLmode; 
static int old_fcntl_flags; 
static int old_fcntl_owner; 
 
/* This may also be defined in stdio, 
 but if so, this does no harm, 
 and using the same name avoids wasting the other one's space.  */ 
char _sobuf[BUFSIZ]; 
 
InitDsp () { 
    struct sgttyb   sg; 
    gtty (0, &old); 
    sg = old; 
#ifdef INTERRUPT_INPUT 
    old_fcntl_flags = fcntl (0, F_GETFL, 0); 
    fcntl (0, F_SETFL, old_fcntl_flags | FASYNC); 
    old_fcntl_owner = fcntl (0, F_GETOWN, 0); 
    fcntl (0, F_SETOWN, getpid ()); 
#endif INTERRUPT_INPUT 
 
#ifdef CBREAK_INPUT 
    ioctl (0, TIOCGETC, &OldTchars); 
    ioctl (0, TIOCGLTC, &OldLtchars); 
    ioctl (0, TIOCLGET, &OldLmode); 
    sg.sg_flags = (sg.sg_flags & ~(ECHO | CRMOD | XTABS | ANYP)) | CBREAK; 
    {   struct tchars tchars; 
	struct ltchars ltchars; 
	int lmode; 
	tchars.t_intrc = 07; 
	tchars.t_quitc = -1; 
	tchars.t_startc = -1; 
	tchars.t_stopc = -1; 
	tchars.t_eofc = -1; 
	tchars.t_brkc = -1; 
	ltchars.t_suspc = -1; 
	ltchars.t_dsuspc = -1; 
	ltchars.t_rprntc = -1; 
	ltchars.t_flushc = -1; 
	ltchars.t_werasc = -1; 
	ltchars.t_lnextc = -1; 
	lmode = OldLmode | LLITOUT; 
	ioctl (0, TIOCSETC, &tchars); 
	ioctl (0, TIOCSLTC, &ltchars); 
	ioctl (0, TIOCLSET, &lmode); 
    } 
#else 
    sg.sg_flags = (sg.sg_flags & ~(ECHO | CRMOD | XTABS)) | RAW; 
#endif CBREAK_INPUT 
 
    ioctl (0, TIOCSETN, &sg); 
    ScreenGarbaged = 1; 
    setbuf (stdout, _sobuf); 
    term_init (); 
    if (tt.t_window) (*tt.t_window) (0); 
} 
 
RstDsp () { 
    if (tt.t_window) (*tt.t_window) (0); 
    (*tt.t_topos) (ScreenLength, 1); 
    (*tt.t_wipeline) (0); 
    (*tt.t_cleanup) (); 
    fflush (stdout); 
#ifdef INTERRUPT_INPUT 
    fcntl (0, F_SETFL, old_fcntl_flags); 
    fcntl (0, F_SETOWN, old_fcntl_owner); 
#endif 
#ifdef CBREAK_INPUT 
    ioctl (0, TIOCSETC, &OldTchars); 
    ioctl (0, TIOCSLTC, &OldLtchars); 
    ioctl (0, TIOCLSET, &OldLmode); 
#endif 
    ioctl (0, TIOCSETN, &old); 
} 
