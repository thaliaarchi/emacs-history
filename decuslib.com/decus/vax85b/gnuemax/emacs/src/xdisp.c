/* Display generation from window structure and buffer text. 
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
#include "config.h" 
#include <ctype.h> 
#include "lisp.h" 
#include "window.h" 
#include "disp_extern.h" 
#include "buffer.h" 
#include "indent.h" 
#include "commands.h" 
#include "macros.h" 
 
int RecurseDepth;		/* Depth of recursion in recursive edits */ 
 
#define min(a, b) ((a) < (b) ? (a) : (b)) 
#define max(a, b) ((a) > (b) ? (a) : (b)) 
 
/* The buffer position of the first character appearing 
 entirely or partially on the current screen line. 
 Or zero, which disables the optimization for the current screen line. */ 
int this_line_bufpos; 
 
/* Number of characters past the end of this line, 
   including the terminating newline */ 
int this_line_endpos; 
 
/* The vertical position of this screen line. */ 
int this_line_vpos; 
 
/* Hpos value for start of display on this screen line. 
 Usually zero, but negative if first character really began on previous line */ 
int this_line_hpos; 
 
/* Buffer that this_line variables are describing. */ 
struct buffer *this_line_buffer; 
 
/* Value of minibuf_message when it was last acted on. 
  If this is nonzero, there is a message on the screen 
  in the minibuffer and it should be erased as soon 
  as it is no longer requested to appear. */ 
char *prev_minibuf_message; 
 
Lisp_Object Vglobal_mode_string; 
 
Lisp_Object Vglobal_minor_modes; 
 
/* The number of lines to try scrolling a 
  window by when dot leaves the window; if 
  it is <=0 then dot is centered in the window */ 
int scroll_step; 
 
/* Nonzero if try_window_id has made blank lines at window bottom 
 since the last redisplay that paused */ 
int blank_end_of_window; 
 
/* Number of windows showing the buffer of the selected window.  */ 
static int buffer_shared; 
 
/* display_text_line sets these to the screen position (origin 0) of dot, 
  whether the window is selected or not. 
 Set one to -1 first to determine whether dot was found afterwards.  */ 
 
int dot_vpos; 
int dot_hpos; 
 
int debug_end_pos; 
 
extern int default_ctl_arrow;	/* Nonzero means display ctl chars */ 
				/* with uparrow in mode lines, etc */ 
 
struct position display_text_line (); 
 
DEFUN ("redraw-display", Fredraw_display, Sredraw_display, 0, 0, "", 
  "Clear the screen and output again what is supposed to appear on it.") 
  () 
{ 
    ScreenGarbaged++; 
    return Qnil; 
} 
 
static char buf[200]; 
 
/* dump an informative message to the minibuf */ 
message (m, a1, a2, a3) 
{ 
  if (!INTERACTIVE) return; 
  sprintf (buf, m, a1, a2, a3); 
  minibuf_message = buf; 
  display_minibuf_message (); 
  UpdateScreen (1, 1); 
} 
 
/* Specify m, a string, as a message in the minibuf.  */ 
message1 (m) 
     char *m; 
{ 
  if (!INTERACTIVE) return; 
  minibuf_message = m; 
  display_minibuf_message (); 
  UpdateScreen (1, 1); 
} 
 
display_minibuf_message () 
{ 
  register int vpos; 
  register struct display_line *line; 
 
  if (minibuf_message || !MinibufDepth) 
    { 
      vpos = XFASTINT (XWINDOW (minibuf_window)->top); 
      line = get_display_line (vpos, 0); 
      display_string (XWINDOW (minibuf_window), line, 
		      minibuf_message ? minibuf_message : "", 
		      0, 0); 
 
      /* If desired cursor location is on this line, put it at end of text */ 
      if (cursY == vpos + 1) 
	cursX = line->length + 1; 
    } 
  else if (!EQ (minibuf_window, selected_window)) 
    windows_or_buffers_changed++; 
 
  if (EQ (minibuf_window, selected_window)) 
    this_line_bufpos = 0; 
 
  prev_minibuf_message = minibuf_message; 
} 
 
/* Do a screen update, taking possible shortcuts into account. 
 This is the main external entry point for redisplay */ 
 
DoDsp (SaveMiniBuf) 
{ 
  register struct window *w = XWINDOW (selected_window); 
  register int pause; 
  int inhibit_hairy_id = 0; 
  int must_finish = 0; 
  int all_windows = 0; 
  register int tlbufpos, tlendpos; 
  struct position pos; 
 
  if (ScreenGarbaged) 
    { 
      windows_or_buffers_changed++; 
      /* Mark all windows as INaccurate, 
	   to avoid problems if this display pauses due to input 
	   and we must finish next time without ScreenGarbaged set */ 
      mark_window_display_accurate (XWINDOW (minibuf_window)->prev, 0); 
    } 
 
  if (minibuf_message || 
      (prev_minibuf_message && !SaveMiniBuf)) 
    { 
      display_minibuf_message (); 
      must_finish = 1; 
    } 
 
  if (clip_changed || windows_or_buffers_changed) 
    RedoModes++; 
 
  /* Detect case that we need to write a star in the mode line.  */ 
  if (XFASTINT (w->last_modified) < bf_modified 
      && XFASTINT (w->last_modified) <= bf_cur->save_modified) 
    w->redo_mode_line = Qt; 
 
  all_windows = RedoModes || buffer_shared > 1; 
 
  tlbufpos = this_line_bufpos; 
  tlendpos = this_line_endpos; 
  if (!all_windows && tlbufpos > 0 && NULL (w->redo_mode_line) 
      /* Dot must be on the line that we have info recorded about */ 
      && dot >= tlbufpos 
      && dot <= bf_s1 + bf_s2 + 1 - tlendpos 
      /* All text outside that line, including its final newline, 
	 must be unchanged */ 
      && (XFASTINT (w->last_modified) >= bf_modified 
	  || (beg_unchanged >= tlbufpos - 1 
	      && bf_s1 >= tlbufpos - 1 
	      && end_unchanged >= tlendpos 
	      && bf_s2 >= tlendpos)) 
      /* Make sure recorded data applies to current buffer, etc */ 
      && NULL (w->force_start) 
      && bf_cur == XBUFFER (w->buffer) 
      && this_line_buffer == bf_cur) 
    { 
      if (tlbufpos > FirstCharacter && CharAt (tlbufpos - 1) != '\n' 
	  && (tlbufpos == NumCharacters + 1 
	      || CharAt (tlbufpos) == '\n')) 
	/* Former continuation line has disappeared by becoming empty */ 
	; 
      else if (XFASTINT (w->last_modified) < bf_modified 
	       || EQ (selected_window, minibuf_window)) 
	{ 
	  dot_vpos = -1; 
	  display_text_line (w, tlbufpos, this_line_vpos, this_line_hpos); 
	  /* If line contains dot, is not continued, 
		 and ends at same distance from eob as before, we win */ 
	  if (dot_vpos >= 0 && this_line_bufpos 
	      && this_line_endpos == tlendpos) 
	    { 
	      cursX = dot_hpos + 1; 
	      if (XFASTINT (w->width) != ScreenWidth) 
		preserve_other_columns (w); 
	      goto update; 
	    } 
	} 
      else if (dot == XFASTINT (w->last_dot)) 
	{ 
	  if (!must_finish) 
	    return; 
	} 
      else 
	{ 
	  pos = compute_motion (tlbufpos, 0, XINT (w->hscroll) ? 1 - XINT (w->hscroll) : 0, 
				dot, 2, - (1 << (SHORTBITS - 1)), 
				XFASTINT (w->width) - 1 
				- (XFASTINT (w->width) + XFASTINT (w->left) != ScreenWidth), 
				XINT (w->hscroll)); 
	  if (pos.vpos < 1) 
	    { 
	      cursX = max (XFASTINT (w->left), pos.hpos) + 1; 
	      goto update; 
	    } 
	} 
      /* Text changed drastically or dot moved off of line */ 
      cancel_line (this_line_vpos); 
    } 
 
  this_line_bufpos = 0; 
 
  if (all_windows) 
    redisplay_all_windows (); 
  else 
    { 
      inhibit_hairy_id = redisplay_window (selected_window, 1); 
      if (XFASTINT (w->width) != ScreenWidth) 
	preserve_other_columns (w); 
    } 
 
update:  
  pause = UpdateScreen (0, inhibit_hairy_id); 
 
  /* If screen does not match, prevent doing single-line-update next time */ 
  if (pause) 
    this_line_bufpos = 0; 
 
  /* Now text on screen agrees with windows, so 
     put info into the windows for partial redisplay to follow */ 
 
  if (!pause) 
    { 
      register struct buffer_text *t 
	= XBUFFER (w->buffer) == bf_cur 
	  ? &bf_text : &XBUFFER (w->buffer)->text; 
 
      blank_end_of_window = 0; 
      clip_changed = 0; 
      unchanged_modified = t->modified; 
      beg_unchanged = t->size1, end_unchanged = t->size2; 
 
      XFASTINT (w->last_dot) = t->dotloc; 
      XFASTINT (w->last_dot_x) = cursX; 
      XFASTINT (w->last_dot_y) = cursY; 
 
      if (all_windows) 
	mark_window_display_accurate (XWINDOW (minibuf_window)->prev, 1); 
      else 
	{ 
	  w->redo_mode_line = Qnil; 
	  XFASTINT (w->last_modified) = t->modified; 
	  if (XFASTINT (w->window_end_pos) < 0) 
	    XFASTINT (w->window_end_pos) = -1 - XFASTINT (w->window_end_pos); 
	} 
      RedoModes = 0; 
      windows_or_buffers_changed = 0; 
    } 
} 
 
mark_window_display_accurate (window, flag) 
     Lisp_Object window; 
     int flag; 
{ 
  register struct window *w; 
 
  for (;!NULL (window); window = w->next) 
    { 
      w = XWINDOW (window); 
 
      XFASTINT (w->last_modified) 
	= !flag ? 0 
	  : XBUFFER (w->buffer) == bf_cur 
	    ? bf_modified : XBUFFER (w->buffer)->text.modified; 
      if (XFASTINT (w->window_end_pos) < 0) 
	XFASTINT (w->window_end_pos) = -1 - XFASTINT (w->window_end_pos); 
      w->redo_mode_line = Qnil; 
 
      if (!NULL (w->vchild)) 
	mark_window_display_accurate (w->vchild, flag); 
      if (!NULL (w->hchild)) 
	mark_window_display_accurate (w->hchild, flag); 
    } 
} 
 
int do_id = 1; 
 
/* Do full redisplay of one or all windows. 
  This does not include updating the screen; 
  just generating lines to pass to UpdateScreen.  */ 
 
/* Entry point to redisplay all windows */ 
 
redisplay_all_windows () 
{ 
  buffer_shared = 0; 
 
  redisplay_windows (XWINDOW (minibuf_window)->prev); 
} 
 
redisplay_windows (window) 
     Lisp_Object window; 
{ 
  for (; !NULL (window); window = XWINDOW (window)->next) 
    redisplay_window (window, 0); 
} 
 
redisplay_window (window, just_this_one) 
     Lisp_Object window; 
     int just_this_one; 
{ 
  register struct window *w = XWINDOW (window); 
  int height; 
  register int ldot = dot; 
  struct buffer *old = bf_cur; 
  register int width = XFASTINT (w->width) - 1 
    - (XFASTINT (w->width) + XFASTINT (w->left) != ScreenWidth); 
  register int startp = marker_position (w->start); 
  register int hscroll = XINT (w->hscroll); 
  struct position pos; 
  int inhibit_hairy_id = 0; 
 
  /* If this is a combination window, do its children; that's all.  */ 
 
  if (!NULL (w->vchild)) 
    return redisplay_windows (w->vchild); 
  if (!NULL (w->hchild)) 
    return redisplay_windows (w->hchild); 
  if (NULL (w->buffer)) 
    abort (); 
 
  if (RedoModes) 
    w->redo_mode_line = Qt; 
 
  /* Otherwise set up data on this window; select its buffer and dot value */ 
 
  height = XFASTINT (w->height); 
  if (w != XWINDOW (minibuf_window)) 
    height--; 
  else if (minibuf_message) 
    return; 
 
  SetBfx (XBUFFER (w->buffer)); 
  if (!just_this_one 
      && bf_cur == XBUFFER (XWINDOW (selected_window)->buffer)) 
    buffer_shared++; 
 
  if (!EQ (window, selected_window)) 
    SetDot (marker_position (w->dotm)); 
 
  /* Handle case where place to start displaying has been specified */ 
 
  if (!NULL (w->force_start)) 
    { 
      w->redo_mode_line = Qt; 
      w->force_start = Qnil; 
      XFASTINT (w->last_modified) = 0; 
      if (!try_window (window, startp)) 
	{ 
	  /* If dot does not appear, move dot so it does appear */ 
	  pos = compute_motion (startp, 0, hscroll ? 1 - hscroll : 0, 
				NumCharacters + 1, height / 2, 
				- (1 << (SHORTBITS - 1)), 
				width, hscroll); 
	  SetDot (pos.bufpos); 
	  if (w != XWINDOW (selected_window)) 
	    Fset_marker (w->dotm, make_number (dot), Qnil); 
	  else 
	    ldot = dot; 
 
	  if (EQ (window, selected_window)) 
	    { 
	      cursX = max (0, pos.hpos) + 1 + XFASTINT (w->left); 
	      cursY = pos.vpos + 1 + XFASTINT (w->top); 
	    } 
	} 
      goto done; 
    } 
 
  /* Handle case where text has not changed, only dot, 
     and it has not moved off the screen */ 
 
  if (XFASTINT (w->last_modified) >= bf_modified 
      && dot >= startp && !clip_changed 
      && (just_this_one || XFASTINT (w->width) == ScreenWidth) 
      /* Cannot do this in minibuffer, 
	 since it does not take the prompt string into account. */ 
      && !EQ (window, minibuf_window)) 
    { 
      pos = compute_motion (startp, 0, hscroll ? 1 - hscroll : 0, 
			    dot, 10000, 10000, width, hscroll); 
 
      if (pos.vpos < height) 
	{ 
	  /* Ok, dot is still on screen */ 
	  if (w == XWINDOW (selected_window)) 
	    { 
	      /* These variables are supposed to be origin 1 */ 
	      cursX = max (0, pos.hpos) + 1 + XFASTINT (w->left); 
	      cursY = pos.vpos + 1 + XFASTINT (w->top); 
	    } 
/* This doesn't do the trick, because if a window to the right of 
 this one must be redisplayed, this does nothing because there 
 is nothing in DesiredScreen yet, and then the other window is 
 redisplayed, making likes that are empty in this window's columns. 
	  if (XFASTINT (w->width) != ScreenWidth) 
	    preserve_my_columns (w); 
*/ 
	  goto done; 
	} 
      /* Don't bother trying redisplay with same start; 
	we already know it will lose */ 
    } 
  else if (just_this_one && !EQ (window, minibuf_window) 
	   && dot >= startp 
	   && XFASTINT (w->last_modified) 
	   && XFASTINT (w->window_end_pos) >= 0 
	   && do_id && !clip_changed 
	   && !blank_end_of_window 
	   && XFASTINT (w->width) == ScreenWidth 
	   && try_window_id (selected_window)) 
    { 
/*       inhibit_hairy_id = 1;   */ 
      goto done; 
    } 
  else if (startp >= FirstCharacter && startp <= NumCharacters + 1 
	   /* Avoid starting display at end of buffer! */ 
	   && (startp <= NumCharacters || startp == FirstCharacter)) 
    { 
      /* Try to redisplay starting at same place as before */ 
      /* If point has not moved off screen, accept the results */ 
      if (try_window (window, startp)) 
	goto done; 
      else 
	cancel_my_columns (w); 
    } 
 
  XFASTINT (w->last_modified) = 0; 
  w->redo_mode_line = Qt; 
 
  /* Try to scroll by specified few lines */ 
 
  if (scroll_step) 
    { 
      if (dot > startp) 
	{ 
	  pos = vmotion (bf_s1 + bf_s2 + 1 - XFASTINT (w->window_end_pos), 
			 scroll_step, width, hscroll); 
	  if (pos.vpos >= height) 
	    goto scroll_fail; 
	} 
 
      pos = vmotion (startp, dot < startp ? - scroll_step : scroll_step, 
		     width, hscroll); 
 
      if (dot >= pos.bufpos) 
	{ 
	  if (try_window (window, pos.bufpos)) 
	    goto done; 
	  else 
	    cancel_my_columns (w); 
	} 
    scroll_fail: ; 
    } 
 
  /* Finally, just choose place to start which centers dot */ 
 
  pos = vmotion (dot, - height / 2, width, hscroll); 
  try_window (window, pos.bufpos); 
 
done: 
  /* If window not full width, must redo its mode line 
     if the window to its side is being redone */ 
  if ((!NULL (w->redo_mode_line) 
       || (!just_this_one && width < ScreenWidth - 1)) 
      && !EQ (window, minibuf_window)) 
    display_mode_line (w); 
 
  SetBfx (old); 
  SetDot (ldot); 
 
  return inhibit_hairy_id; 
} 
 
int dot_vpos; 
int dot_hpos; 
 
/* Do full redisplay on one window, 
  starting at position `pos', 
  and return nonzero if dot appears in the displayed text */ 
 
try_window (window, pos) 
     Lisp_Object window; 
     register int pos; 
{ 
  register struct window *w = XWINDOW (window); 
  register int height = XFASTINT (w->height) - !EQ (window, minibuf_window); 
  register int vpos = XFASTINT (w->top); 
  register int last_text_vpos = vpos; 
 
  struct position val; 
 
  Fset_marker (w->start, make_number (pos), Qnil); 
 
  dot_vpos = -1; 
  val.hpos = XINT (w->hscroll) ? 1 - XINT (w->hscroll) : 0; 
 
  while (--height >= 0) 
    { 
      val = display_text_line (w, pos, vpos, val.hpos); 
      vpos++; 
      if (pos != val.bufpos) 
	last_text_vpos 
	  /* Next line, unless prev line ended in end of buffer with no cr */ 
	  = vpos - (val.vpos && CharAt (val.bufpos - 1) != '\n'); 
      pos = val.bufpos; 
    } 
 
  /* If last line is continued in middle of character, 
     include the split character in the text considered on the screen */ 
  if (val.hpos < XINT (w->hscroll) ? 1 - XINT (w->hscroll) : 0) 
    pos++; 
 
  /* Make this -1 minus what it really should be, so that it is negative. 
     That serves as a signal that it is not really valid. 
     When screen updating is through, 
     change this to its correct positive value */ 
  XFASTINT (w->window_end_pos) = -1 - (bf_s1 + bf_s2 + 1 - pos); 
  XFASTINT (w->window_end_vpos) = last_text_vpos - XFASTINT (w->top); 
  return dot_vpos >= 0; 
} 
 
/* Try to redisplay when buffer is modified locally, 
 computing insert/delete line to preserve text outside 
 the bounds of the changes */ 
 
try_window_id (window) 
     Lisp_Object window; 
{ 
  register int pos; 
  register struct window *w = XWINDOW (window); 
  register int height = XFASTINT (w->height) - !EQ (window, minibuf_window); 
  int top = XFASTINT (w->top); 
  int start = marker_position (w->start); 
  int width = XFASTINT (w->width) - 1 
    - (XFASTINT (w->width) + XFASTINT (w->left) != ScreenWidth); 
  int hscroll = XINT (w->hscroll); 
  int lmargin = hscroll > 0 ? 1 - hscroll : 0; 
  register int vpos; 
  int tem; 
  int last_text_vpos = 0; 
  int stop_vpos; 
 
  struct position val, bp, ep, xp, pp; 
  int scroll_amount = 0; 
  int delta; 
 
  if (bf_s1 < beg_unchanged) 
    beg_unchanged = bf_s1; 
  if (bf_s2 < end_unchanged) 
    end_unchanged = bf_s2; 
 
  if (beg_unchanged + 1 < start) 
    return 0;			/* Give up if changes go above top of window */ 
 
  /* Find position before which nothing is changed.  */ 
  bp = compute_motion (start, 0, lmargin, 
		       beg_unchanged + 1, 10000, 10000, width, hscroll); 
  if (bp.vpos >= height) 
    return dot < bp.bufpos && !bp.contin; 
 
  vpos = bp.vpos; 
 
  /* Find beginning of that screen line.  Must display from there.  */ 
  bp = vmotion (bp.bufpos, 0, width, hscroll); 
 
  pos = bp.bufpos; 
  val.hpos = lmargin; 
 
  /* If about to start displaying at the beginning of a continuation line, 
     really start with previous screen line, in case it was not 
     continued as of the last redisplay */ 
  if (bp.contin && bp.bufpos - 1 == beg_unchanged) 
    { 
      bp = vmotion (bp.bufpos, -1, width, hscroll); 
      --vpos; 
      pos = bp.bufpos; 
    } 
 
  if (bp.contin && bp.hpos != lmargin) 
    { 
      val.hpos = bp.prevhpos - width + lmargin; 
      pos--; 
    } 
 
  bp.vpos = vpos; 
 
  /* Find first newline after which no more is changed */ 
  ep = compute_motion (pos, vpos, lmargin, 
		       ScanBf ('\n', 
			       bf_s1 + bf_s2 + 1 - max (end_unchanged, bf_tail_clip), 
			       1), 
		       height, - (1 << (SHORTBITS - 1)), 
		       width, hscroll); 
 
  /* If changes reach past the text available on the screen, 
     just display rest of screen.  */ 
  if (ep.bufpos > bf_s1 + bf_s2 + 1 - XFASTINT (w->window_end_pos)) 
    stop_vpos = height; 
  else 
    stop_vpos = ep.vpos; 
 
  /* If no newline before ep, the line ep is on includes some changes 
     that must be displayed.  Make sure we don't stop before it.  */ 
  /* Also, if changes reach all the way until ep.bufpos, 
     it is possible that something was deleted after the 
     newline before it, so the following line must be redrawn. */ 
  if (stop_vpos == ep.vpos && ep.bufpos != FirstCharacter 
      && (CharAt (ep.bufpos - 1) != '\n' 
	  || ep.bufpos == bf_s1 + bf_s2 + 1 - end_unchanged)) 
    stop_vpos = ep.vpos + 1; 
 
  /* If changes do not reach to bottom of window, 
     figure out how much to scroll the rest of the window */ 
  if (stop_vpos < height) 
    { 
      /* Now determine how far up or down the rest of the window has moved */ 
      xp = compute_motion (ep.bufpos, ep.vpos, ep.hpos, 
			   bf_s1 + bf_s2 + 1 - XFASTINT (w->window_end_pos), 
			   10000, 0, width, hscroll); 
 
      scroll_amount = xp.vpos - XFASTINT (w->window_end_vpos); 
      XFASTINT (w->window_end_vpos) += scroll_amount; 
 
      /* Before doing any scrolling, verify that dot will be on screen. */ 
      if (dot > ep.bufpos && !(dot <= xp.bufpos && xp.bufpos < height)) 
	{ 
	  if (dot <= xp.bufpos) 
	    { 
	      pp = compute_motion (ep.bufpos, ep.vpos, ep.hpos, 
				   dot, height, - (1 << (SHORTBITS - 1)), 
				   width, hscroll); 
	    } 
	  else 
	    { 
	      pp = compute_motion (xp.bufpos, xp.vpos, xp.hpos, 
				   dot, height, - (1 << (SHORTBITS - 1)), 
				   width, hscroll); 
	    } 
	  if (pp.bufpos < dot || pp.vpos == height) 
	    return 0; 
	} 
 
      if (stop_vpos - scroll_amount >= height 
	  || ep.bufpos == xp.bufpos) 
	{ 
	  if (scroll_amount < 0) 
	    stop_vpos -= scroll_amount; 
	  scroll_amount = 0; 
	} 
      else if (scroll_amount) 
	{ 
	  tem = scroll_screen_lines (ep.vpos + top - scroll_amount, 
				     top + height - max (0, scroll_amount), 
				     scroll_amount); 
	  if (!tem) stop_vpos = height; 
	} 
    } 
 
  /* In any case, do not display past bottom of window */ 
  if (stop_vpos >= height) 
    { 
      stop_vpos = height; 
      scroll_amount = 0; 
    } 
 
  dot_vpos = -1; 
 
  /* Handle case where pos is before w->start -- 
     can happen if part of line had been clipped and is not clipped now */ 
  if (vpos == 0 && pos < marker_position (w->start)) 
    Fset_marker (w->start, make_number (pos), Qnil); 
 
  /* Redisplay the lines where the text was changed */ 
  last_text_vpos = vpos; 
  while (vpos < stop_vpos) 
    { 
      val = display_text_line (w, pos, top + vpos++, val.hpos); 
      if (pos != val.bufpos) 
	last_text_vpos 
	  /* Next line, unless prev line ended in end of buffer with no cr */ 
	    = vpos - (val.vpos && CharAt (val.bufpos - 1) != '\n'); 
      pos = val.bufpos; 
    } 
 
  /* There are two cases: 
     1) we have displayed down to the bottom of the window 
     2) we have scrolled lines below stop_vpos by scroll_amount  */ 
 
  if (vpos == height) 
    { 
      /* If last line is continued in middle of character, 
	 include the split character in the text considered on the screen */ 
      if (val.hpos < lmargin) 
	val.bufpos++; 
      XFASTINT (w->window_end_vpos) = last_text_vpos; 
      XFASTINT (w->window_end_pos) = -1 - (bf_s1 + bf_s2 + 1 - val.bufpos); 
    } 
 
  /* If scrolling made blank lines at window bottom, 
     redisplay to fill those lines */ 
  if (scroll_amount < 0) 
    { 
      vpos = xp.vpos; 
      pos = xp.bufpos; 
      val.hpos = lmargin; 
      if (pos == NumCharacters + 1) 
	vpos = height + scroll_amount; 
      else if (xp.contin && xp.hpos != lmargin) 
	{ 
	  val.hpos = xp.prevhpos - width + lmargin; 
	  pos--; 
	} 
 
      blank_end_of_window = 1; 
 
      while (vpos < height) 
	{ 
	  val = display_text_line (w, pos, top + vpos++, val.hpos); 
	  pos = val.bufpos; 
	} 
    } 
 
  /* Attempt to adjust end-of-text positions to new bottom line */ 
  if (scroll_amount) 
    { 
      delta = height - xp.vpos; 
      if (delta < 0 
	  || (delta > 0 && xp.bufpos <= NumCharacters) 
	  || (delta == 0 && xp.hpos)) 
	{ 
	  val = vmotion (bf_s1 + bf_s2 + 1 - XFASTINT (w->window_end_pos), 
			 delta, width, hscroll); 
	  XFASTINT (w->window_end_pos) = bf_s1 + bf_s2 + 1 - val.bufpos; 
	  XFASTINT (w->window_end_vpos) += val.vpos; 
	} 
    } 
 
  /* If dot was not in a line that was displayed, find it */ 
  if (dot_vpos < 0) 
    { 
      val = compute_motion (start, 0, lmargin, dot, 10000, 10000, 
			    width, hscroll); 
      /* Admit failure if dot is off screen now */ 
      if (val.vpos >= height) 
	{ 
	  for (vpos = 0; vpos < height; vpos++) 
	    cancel_line (vpos + top); 
	  return 0; 
	} 
      dot_vpos = val.vpos; 
      dot_hpos = val.hpos; 
      cursX = max (0, dot_hpos) + 1 + XFASTINT (w->left); 
      cursY = dot_vpos + 1 + XFASTINT (w->top); 
    } 
 
  if (debug_end_pos) 
    { 
      val = compute_motion (start, 0, lmargin, NumCharacters + 1, 
			    height, - (1 << (SHORTBITS - 1)), 
			    width, hscroll); 
      if (val.vpos != XFASTINT (w->window_end_vpos)) 
	abort (); 
      if ((XFASTINT (w->window_end_pos) < 0 
	   ? -1 - XFASTINT (w->window_end_pos) 
	   : XFASTINT (w->window_end_pos)) 
	  != bf_s1 + bf_s2 + 1 - val.bufpos) 
	abort (); 
    } 
 
  return 1; 
} 
 
/* Display one line of window w, starting at position `start' in w's buffer. 
 Display starting at horizontal position `hpos', 
  which is normally zero or negative. 
  A negative value causes output up to hpos = 0 to be discarded. 
  This is done for negative hscroll, or when this is a continuation line 
  and the continuation occurred in the middle of a multi-column character. 
 
 Display on position `vpos' on the screen.  (origin 0). 
 
 Returns a `struct position' giving character to start next line with 
 and where to display it, including a zero or negative hpos. 
 The vpos field is not really a vpos; it is 1 unless the line is continued */ 
 
struct position 
display_text_line (w, start, vpos, hpos) 
     struct window *w; 
     int start; 
     int vpos; 
     int hpos; 
{ 
  register int pos = start; 
  register int c; 
  register char *p1; 
  int end; 
  register int pause; 
  register unsigned char *p; 
  char *endp; 
  register char *startp; 
  register char *p1prev; 
  register struct display_line *line; 
  int tab_width = XFASTINT (XBUFFER (w->buffer)->tab_width); 
  int ctl_arrow = !NULL (XBUFFER (w->buffer)->ctl_arrow); 
  int width = XFASTINT (w->width) - 1 
    - (XFASTINT (w->width) + XFASTINT (w->left) != ScreenWidth); 
  struct position val; 
  int lastpos; 
  int invis; 
  int hscroll = XINT (w->hscroll); 
  int truncate = hscroll 
    || !NULL (XBUFFER (w->buffer)->truncate_lines); 
  int selective 
    = XTYPE (bf_cur->selective_display) == Lisp_Int 
      ? XINT (bf_cur->selective_display) 
	: !NULL (bf_cur->selective_display) ? -1 : 0; 
 
  hpos += XFASTINT (w->left); 
  line = get_display_line (vpos, XFASTINT (w->left)); 
  if (tab_width <= 0) tab_width = 1; 
 
  if (w == XWINDOW (minibuf_window) && start == 1 && minibuf_prompt 
      && vpos == XFASTINT (XWINDOW (minibuf_window)->top)) 
    hpos = display_string (w, line, minibuf_prompt, hpos, 
			   !truncate ? '\\' : '$', 
			   0); 
 
  p1 = line->body + hpos; 
 
  end = NumCharacters + 1; 
 
  startp = line->body + XFASTINT (w->left); 
  endp = startp + width; 
 
  /* Loop generating characters. 
   Stop at end of buffer, before newline, 
   or if reach or pass continuation column.  */ 
 
  pause = pos; 
  while (p1 < endp) 
    { 
      p1prev = p1; 
      if (pos == pause) 
	{ 
	  if (pos == end) 
	    break; 
	  if (pos == dot && dot_vpos < 0) 
	    { 
	      dot_vpos = vpos; 
	      dot_hpos = p1 - startp; 
	    } 
 
	  pause = end; 
	  if (pos < dot && dot < pause) 
	    pause = dot; 
	  if (pos <= bf_s1 && bf_s1 + 1 < pause) 
	    pause = bf_s1 + 1; 
 
	  p = &CharAt (pos); 
	} 
      c = *p++; 
      if (c >= 040 && c < 0177) 
	{ 
	  if (p1 >= startp) 
	    *p1 = c; 
	  p1++; 
	} 
      else if (c == '\n') 
	{ 
	  invis = 0; 
	  while (pos < end 
		 && selective > 0 
		 && position_indentation (pos + 1) >= selective) 
	    { 
	      invis = 1; 
	      pos = ScanBf ('\n', pos + 1, 1); 
	      if (CharAt (pos - 1) == '\n') 
		pos--; 
	    } 
	  if (invis) 
	    { 
	      p1 += 4; 
	      if (p1 - startp > width) 
		p1 = endp; 
	      strncpy (p1prev, " ...", p1 - p1prev); 
	    } 
	  break; 
	} 
      else if (c == '\t') 
	{ 
	  do 
	    { 
	      if (p1 >= startp) 
		*p1 = ' '; 
	      p1++; 
	    } 
	  while ((p1 - startp + hscroll - (hscroll > 0)) % tab_width); 
	} 
      else if (c == Ctl('M') && !NULL (bf_cur->selective_display)) 
	{ 
	  pos = ScanBf ('\n', pos, 1); 
	  if (CharAt (pos - 1) == '\n') 
	    pos--; 
	  break; 
	} 
      else if (c < 0200 && ctl_arrow) 
	{ 
	  if (p1 >= startp) 
	    *p1 = '^'; 
	  p1++; 
	  if (p1 >= startp) 
	    *p1 = c ^ 0100; 
	  p1++; 
	} 
      else 
	{ 
	  if (p1 >= startp) 
	    *p1 = '\\'; 
	  p1++; 
	  if (p1 >= startp) 
	    *p1 = (c >> 6) + '0'; 
	  p1++; 
	  if (p1 >= startp) 
	    *p1 = (7 & (c >> 3)) + '0'; 
	  p1++; 
	  if (p1 >= startp) 
	    *p1 = (7 & c) + '0'; 
	  p1++; 
	} 
      pos++; 
    } 
 
  val.hpos = - XINT (w->hscroll); 
  if (val.hpos) 
    { 
      val.hpos++; 
      /* If line not empty, insert truncation-at-left marker */ 
      if (pos != start) 
	{ 
	  *startp = '$'; 
	  if (p1 <= startp) 
	    p1 = startp + 1; 
	  if (line->length <= XFASTINT (w->left)) 
	    line->length = XFASTINT (w->left) + 1; 
	} 
    } 
  val.vpos = 1; 
 
  /* Handle continuation in middle of a character */ 
  /* by backing up over it */ 
  if (p1 > endp) 
    { 
      /* Start the next line with that same character */ 
      pos--; 
      /* but at a negative hpos, to skip the columns output on this line.  */ 
      val.hpos += p1prev - endp; 
      /* Keep in this line everything up to the continuation column.  */ 
      p1 = endp; 
    } 
 
  /* Finish deciding which character to start the next line on, 
     and what hpos to start it at. 
     Also set `lastpos' to the last position which counts as "on this line" 
     for cursor-positioning.  */ 
 
  lastpos = pos; 
 
  if (pos < NumCharacters + 1) 
    { 
      if (CharAt (pos) == '\n') 
	/* If stopped due to a newline, start next line after it */ 
	pos++; 
      else 
	/* Stopped due to right margin of window */ 
	{ 
	  if (truncate) 
	    { 
	      *p1++ = '$'; 
	      /* Truncating => start next line after next newline, 
		 and dot is on this line if it is before the newline, 
		 and skip none of first char of next line */ 
	      pos = ScanBf ('\n', pos, 1); 
	      val.hpos = XINT (w->hscroll) ? 1 - XINT (w->hscroll) : 0; 
 
	      lastpos = pos - (CharAt (pos - 1) == '\n'); 
	    } 
	  else 
	    { 
	      *p1++ = '\\'; 
	      val.vpos = 0; 
	      lastpos = 0; 
	    } 
	} 
    } 
 
  if (start <= dot && dot <= lastpos && dot_vpos < 0) 
    { 
      dot_vpos = vpos; 
      dot_hpos = p1 - startp; 
    } 
 
  if (dot_vpos == vpos) 
    { 
      if (dot_hpos < 0) dot_hpos = 0; 
      if (dot_hpos > width) dot_hpos = width; 
      dot_hpos += XFASTINT (w->left); 
      if (w == XWINDOW (selected_window)) 
	{ 
	  cursY = dot_vpos + 1; 
	  cursX = dot_hpos + 1; 
 
	  /* Line is not continued and did not start in middle of character */ 
	  if (hpos == (XINT (w->hscroll) ? 1 - XINT (w->hscroll) : 0) 
	      && val.vpos) 
	    { 
	      this_line_bufpos = start; 
	      this_line_buffer = bf_cur; 
	      this_line_vpos = vpos; 
	      this_line_hpos = hpos; 
	      this_line_endpos = bf_s1 + bf_s2 + 1 - lastpos; 
	    } 
	  else 
	    this_line_bufpos = 0; 
	} 
    } 
 
  if (XFASTINT (w->width) + XFASTINT (w->left) != ScreenWidth) 
    { 
      endp++; 
      if (p1 < startp) p1 = startp; 
      while (p1 < endp) *p1++ = ' '; 
      *p1++ = '|'; 
    } 
  line->length = max (line->length, p1 - line->body); 
  line->body[line->length] = 0; 
 
  val.bufpos = pos; 
  return val; 
} 
 
/* Display the mode line for window w */ 
 
display_mode_line (w) 
     struct window *w; 
{ 
  int vpos = XFASTINT (w->height) + XFASTINT (w->top) - 1; 
  struct display_line *line = get_display_line (vpos, XFASTINT (w->left)); 
 
  register unsigned char *s = XSTRING (bf_cur->mode_line_format)->data; 
 
  display_string (w, line, s, XFASTINT (w->left), 0, 1); 
  if (XFASTINT (w->width) == ScreenWidth) 
    line->highlighted = 1; 
} 
 
char * 
fmodetrunc (str, width) 
     char *str; 
     long width; 
{ 
  static char buf[100]; 
  char *bp = buf; 
  long len; 
     
  len = strlen(str); 
  if (width && width < len) 
    { 
      strcpy(buf,str+len-width); 
      if (buf[0] != '/') 
	while (*bp) 
	  if (*bp++ == '/') 
	    { 
	      bp--; 
	      *--bp = '$'; 
	      return bp; 
	    } 
      buf[0] = '$'; 
      return buf; 
    } 
  return str; 
} 
 
char * 
decode_mode_spec (w, string, data_ptr, len_ptr, max_ptr) 
     struct window *w; 
     register char *string; 
     char **data_ptr; 
     int *len_ptr; 
     int *max_ptr; 
{ 
  register int width = 0; 
  register char c; 
  Lisp_Object lstr, proc, list; 
  register char *str; 
  int len; 
  int pos, total; 
  static char tbuf[MScreenWidth + 1]; 
 
  lstr = Qnil, str = 0; 
 
  while (isdigit (c = *string++)) 
    width = width * 10 + (c - '0'); 
 
  switch (c) 
    { 
    case 'b':  
      lstr = bf_cur->name; 
      if (width && XSTRING (lstr)->size > width) 
	width = min (2 * width, XSTRING (lstr)->size); 
      if (width && XSTRING (lstr)->size > width) 
	{ 
	  str = (char *) alloca (width + 1); 
	  bcopy (XSTRING (lstr)->data, str, width - 3); 
	  bcopy ("...", str + width - 3, 4); 
	  lstr = Qnil; 
	} 
      break; 
 
    case 'f':  
      if (NULL (bf_cur->filename)) 
	str = "[none]"; 
      else if (XTYPE (bf_cur->filename) == Lisp_String) 
	str = fmodetrunc (XSTRING (bf_cur -> filename)->data, width); 
      break; 
 
    case 'm':  
      lstr = bf_cur->mode_name; 
      total = min (XSTRING (lstr)->size, sizeof tbuf - 1); 
      bcopy (XSTRING (lstr)->data, tbuf, total); 
      len = 0; 
      list = bf_cur->minor_modes; 
      while (1) 
	{ 
	  if (!LISTP (list) && !len) 
	    { 
	      list = Vglobal_minor_modes; 
	      len = 1; 
	    } 
 
	  if (!(total < sizeof tbuf - 1 && LISTP (list))) 
	    break; 
 
	  lstr = XCONS (list)->car; 
	  if (!LISTP (lstr)) 
	    goto foo; 
	  lstr = XCONS (lstr)->cdr; 
	  if (XTYPE (lstr) != Lisp_String) 
	    goto foo; 
	  tbuf[total++] = ' '; 
	  pos = min (XSTRING (lstr)->size, sizeof tbuf - 1 - total); 
	  bcopy (XSTRING (lstr)->data, tbuf + total, pos); 
	  total += pos; 
	foo: 
	  list = Fcdr (list); 
	} 
      str = tbuf; 
      tbuf[total] = 0; 
      if (defining_kbd_macro) 
	strcat (tbuf, " Def"); 
      lstr = Qnil; 
      break; 
 
    case 'M':  
      lstr = Vglobal_mode_string; 
      break; 
 
    case '*':  
      str = !NULL (bf_cur->read_only) ? "%" 
	    : bf_modified > bf_cur->save_modified ? "*" :"-"; 
      break; 
 
    case 's': 
      /* status of process */ 
      proc = Fget_buffer_process (Fcurrent_buffer ()); 
      if (NULL (proc)) 
	str = "no process"; 
      else 
	lstr = Fsymbol_name (Fprocess_status (proc)); 
      break; 
 
    case 'p': 
      pos = marker_position (w->start); 
      total = NumCharacters + 1 - FirstCharacter; 
 
      if ((XFASTINT (w->window_end_pos) < 0 
	   ? -1 - XFASTINT (w->window_end_pos) 
	   : XFASTINT (w->window_end_pos)) 
	  <= bf_tail_clip) 
	{ 
	  if (pos <= FirstCharacter) 
	    str = "All"; 
	  else 
	    str = "Bottom"; 
	} 
      else if (pos <= FirstCharacter) 
	str = "Top"; 
      else 
	{ 
	  sprintf (tbuf, "%2d%%", 
		   ((pos - FirstCharacter) * 100 + total - 1) / total); 
	  str = tbuf; 
	} 
      break; 
 
    case '[':  
      str = "[[[[[[[[[[" + 10 - (RecurseDepth - MinibufDepth); 
      if (RecurseDepth - MinibufDepth > 10) 
	str = "[[[... "; 
      break; 
 
    case ']':  
      str = "]]]]]]]]]]" + 10 - (RecurseDepth - MinibufDepth); 
      if (RecurseDepth - MinibufDepth > 10) 
	str = " ...]]]"; 
      break; 
 
    case '-': 
      str = "--------------------------------------------------------------------------------------------------------------------------------------------"; 
    } 
 
  /* Report the chosen mode item to the caller */ 
 
  if (str) 
    *data_ptr = str, *len_ptr = strlen (str); 
  else if (XTYPE (lstr) == Lisp_String) 
    *data_ptr = (char *) XSTRING (lstr)->data, 
    *len_ptr = XSTRING (lstr)->size; 
  else 
    *data_ptr = 0, *len_ptr = 0; 
 
  /* Report specified truncation or padding */ 
 
  *max_ptr = width; 
 
  /* Tell caller how much of mode line format was used up */ 
 
  return string; 
} 
 
/* Display `string' on one line of window `w', starting at `hpos'. 
 Display on the display_line `line', which should have 
  been obtained by get_display_line (vpos, hpos) 
  or in some suitable manner. 
 
 `truncate' is character to display at end if truncated. 
 `modeline' nonzero means substitute for % constructs. 
 
 Returns ending hpos */ 
 
 
display_string (w, line, string, hpos, truncate, modeline) 
     struct window *w; 
     register struct display_line *line; 
     char *string; 
     int hpos; 
     int truncate; 
     int modeline; 
{ 
  register int c; 
  register char *p; 
  register char *p1; 
  int width = XFASTINT (w->width) - 1 
    - (XFASTINT (w->width) + XFASTINT (w->left) != ScreenWidth); 
  int hscroll = XINT (w->hscroll); 
  int tab_width = 8; 
  register char *start; 
  register char *end; 
  char *modeeltstring; 
  int modeeltleft = 0; 
  int modeeltmax = 0; 
 
  p1 = line->body + hpos; 
  start = line->body + XFASTINT (w->left); 
  end = start + width; 
 
  while (p1 < end) 
    { 
      if (modeeltmax) 
	{ 
	  modeeltmax--; 
	  if (modeeltleft-- > 0) 
	    c = *modeeltstring++; 
	  else 
	    c = ' '; 
	} 
      else 
	{ 
	  c = *string++; 
	  if (!c) break; 
	  if (c == '%' && modeline) 
	    { 
	      string = decode_mode_spec (w, string, &modeeltstring, 
					 &modeeltleft, &modeeltmax); 
 
	      if (!modeeltmax) 
		modeeltmax = modeeltleft; 
	      else if (modeeltleft > modeeltmax) 
		modeeltleft = modeeltmax; 
 
	      continue; 
	    } 
	} 
 
      if (c >= 040 && c < 0177) 
	{ 
	  if (p1 >= start) 
	    *p1 = c; 
	  p1++; 
	} 
      else if (c == '\t') 
	{ 
	  do 
	    { 
	      if (p1 >= start) 
		*p1 = ' '; 
	      p1++; 
	    } 
	  while ((p1 - start + hscroll - (hscroll > 0)) % tab_width); 
	} 
      else if (c < 0200 && default_ctl_arrow) 
	{ 
	  if (p1 >= start) 
	    *p1 = '^'; 
	  p1++; 
	  if (p1 >= start) 
	    *p1 = c ^ 0100; 
	  p1++; 
	} 
      else 
	{ 
	  if (p1 >= start) 
	    *p1 = '\\'; 
	  p1++; 
	  if (p1 >= start) 
	    *p1 = (c >> 6) + '0'; 
	  p1++; 
	  if (p1 >= start) 
	    *p1 = (7 & (c >> 3)) + '0'; 
	  p1++; 
	  if (p1 >= start) 
	    *p1 = (7 & c) + '0'; 
	  p1++; 
	} 
    } 
 
  if (c) 
    { 
      p1 = end; 
      if (truncate) *p1++ = truncate; 
    } 
 
  line->length = max (line->length, p1 - line->body); 
  line->body[line->length] = 0; 
  return p1 - line->body; 
} 
 
syms_of_xdisp () 
{ 
  DefLispVar ("global-mode-string", &Vglobal_mode_string, 
    "*String which mode line can display (if its format requests to)."); 
  Vglobal_mode_string = Qnil; 
 
  DefLispVar ("global-minor-modes", &Vglobal_minor_modes, 
    "Alist of minor modes that are not per buffer.\n\ 
Cdr of each element is a string to display in mode line."); 
 
  DefIntVar ("scroll-step", &scroll_step, 
    "*The number of lines to try scrolling a window by when dot moves out.\n\ 
If that fails to bring dot back on screen, dot is centered instead.\n\ 
If this is zero, dot is always centered after it moves off screen."); 
 
  DefIntVar ("debug-end-pos", &debug_end_pos, "Don't ask"); 
 
  defsubr (&Sredraw_display); 
} 
 
/* initialize the window system */ 
init_xdisp () 
{ 
  Lisp_Object root_window; 
  struct window *mini_w;	/* Not register for Sun compiler's sake.  */ 
 
  this_line_bufpos = 0; 
 
  mini_w = XWINDOW (minibuf_window); 
  root_window = mini_w->prev; 
 
  minibuf_message = 0; 
  prev_minibuf_message = 0; 
 
  XFASTINT (XWINDOW (root_window)->top) = 0; 
  set_window_height (root_window, ScreenLength - 1); 
  XFASTINT (mini_w->top) = ScreenLength - 1; 
  set_window_height (minibuf_window, 1); 
 
  XFASTINT (XWINDOW (root_window)->width) = ScreenWidth; 
  XFASTINT (mini_w->width) = ScreenWidth; 
} 
