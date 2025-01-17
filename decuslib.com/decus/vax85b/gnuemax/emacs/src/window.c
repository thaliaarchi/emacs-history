/* Window creation, deletion and examination for GNU Emacs. 
   Does not include redisplay. 
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
#include "lisp.h" 
#include "buffer.h" 
#include "window.h" 
#include "commands.h" 
#include "indent.h" 
 
Lisp_Object Qwindowp; 
 
Lisp_Object Fnext_window (), Fdelete_window (), Fselect_window (); 
Lisp_Object Fshow_buffer (), Fsplit_window (), Frecenter (); 
 
/* If a window gets smaller than either of these, it is removed. */ 
 
int window_min_height; 
int window_min_width; 
 
/* Nonzero implies pop_to_buffer should create windows. */ 
 
int pop_up_windows; 
 
/* display-buffer always splits the largest window  
 if that window is more than this high */ 
 
int split_height_threshold; 
 
/* Number of lines of continuity in scrolling by screenfuls.  */ 
 
int next_screen_context_lines; 
 
/* Incremented for each window created.  */ 
 
static int sequence_number; 
 
DEFUN ("windowp", Fwindowp, Swindowp, 1, 1, 0, 
  "Returns t if OBJ is a window.") 
  (obj) 
     Lisp_Object obj; 
{ 
  return XTYPE (obj) == Lisp_Window ? Qt : Qnil; 
} 
 
Lisp_Object 
make_window () 
{ 
  Lisp_Object val; 
  register struct window *p; 
 
  val = Fmake_vector ( 
    make_number ((sizeof (struct window) - sizeof (struct Lisp_Vector)) 
		 / sizeof (Lisp_Object)), 
    Qnil); 
  XSETTYPE (val, Lisp_Window); 
  p = XWINDOW (val); 
  XFASTINT (p->left) = XFASTINT (p->top) 
    = XFASTINT (p->height) = XFASTINT (p->width) 
      = XFASTINT (p->hscroll) = 0; 
  XFASTINT (p->last_dot_x) = XFASTINT (p->last_dot_y) = 0; 
  p->start = Fmake_marker (); 
  p->dotm = Fmake_marker (); 
  XFASTINT (p->sequence_number) = ++sequence_number; 
  XFASTINT (p->use_time) = 0; 
  return val; 
} 
 
DEFUN ("selected-window", Fselected_window, Sselected_window, 0, 0, 0, 
  "Return the window that the cursor appears in and commands apply to.") 
  () 
{ 
  return selected_window; 
} 
 
DEFUN ("pos-visible-in-window-p", Fpos_visible_in_window_p, 
  Spos_visible_in_window_p, 0, 2, 0, 
  "Return t if position POS is currently on the screen in WINDOW.\n\ 
Returns nil if that position is scrolled vertically out of view.\n\ 
POS defautls to dot; WINDOW, to the selected window.") 
  (pos, window) 
     Lisp_Object pos, window; 
{ 
  register struct window *w; 
  register int top; 
  register int height; 
  register int posint; 
  register struct buffer_text *text; 
  struct position posval; 
 
  if (NULL (pos)) 
    posint = dot; 
  else 
    { 
      CHECK_NUMBER_COERCE_MARKER (pos, 0); 
      posint = XINT (pos); 
    } 
 
  if (NULL (window)) 
    window = selected_window; 
  else 
    CHECK_WINDOW (window, 1); 
  w = XWINDOW (window); 
  top = marker_position (w->start); 
 
  if (posint < top) 
    return Qnil; 
 
  height = XFASTINT (w->height) - !EQ (window, minibuf_window); 
 
  bf_cur->text = bf_text; 
  text = &XBUFFER (w->buffer)->text; 
  if (XFASTINT (w->last_modified) >= text->modified) 
    { 
      /* If screen is up to date, 
	 use the info recorded about how much text fit on it. */ 
      if (posint < text->size1 + text->size2 + 1 - XFASTINT (w->window_end_pos) 
	  || (XFASTINT (w->window_end_vpos) < height)) 
	return Qt; 
      return Qnil; 
    } 
  else 
    { 
      if (posint > text->size1 + text->size2 + 1) 
	return Qnil; 
      /* If that info is not correct, calculate afresh */ 
      posval = compute_motion (top, 0, 0, 
			       posint, height, 0, 
			       XFASTINT (w->width) - 1 
			       - (XFASTINT (w->width) + XFASTINT (w->left) != XFASTINT (XWINDOW (minibuf_window)->width)), 
 
			       XINT (w->hscroll)); 
      return posval.vpos < height ? Qt : Qnil; 
    } 
} 
 
struct window * 
decode_window (window) 
     Lisp_Object window; 
{ 
  if (NULL (window)) 
    return XWINDOW (selected_window); 
 
  CHECK_WINDOW (window, 0); 
  return XWINDOW (window); 
} 
 
DEFUN ("window-buffer", Fwindow_buffer, Swindow_buffer, 0, 1, 0, 
  "Return the buffer that WINDOW is displaying.") 
  (window) 
     Lisp_Object window; 
{ 
  return decode_window (window)->buffer; 
} 
 
DEFUN ("window-height", Fwindow_height, Swindow_height, 0, 1, 0, 
  "Return the number of lines in WINDOW (including its mode line).") 
  (window) 
     Lisp_Object window; 
{ 
  return decode_window (window)->height; 
} 
 
DEFUN ("window-width", Fwindow_width, Swindow_width, 0, 1, 0, 
  "Return the number of columns in WINDOW.") 
  (window) 
     Lisp_Object window; 
{ 
  return decode_window (window)->width; 
} 
 
DEFUN ("window-hscroll", Fwindow_hscroll, Swindow_hscroll, 0, 1, 0, 
  "Return the number of columns by which WINDOW is scrolled from left margin.") 
  (window) 
     Lisp_Object window; 
{ 
  return decode_window (window)->hscroll; 
} 
 
DEFUN ("set-window-hscroll", Fset_window_hscroll, Sset_window_hscroll, 2, 2, 0, 
  "Set number of columns WINDOW is scrolled from left margin to NCOL.\n\ 
NCOL should be zero or positive.") 
  (window, ncol) 
     Lisp_Object window, ncol; 
{ 
  CHECK_NUMBER (ncol, 1); 
  if (XINT (ncol) < 0) XFASTINT (ncol) = 0; 
  if (XFASTINT (ncol) >= (1 << (SHORTBITS - 1))) 
    Fsignal (Qargs_out_of_range, Fcons (ncol, Qnil)); 
 
  clip_changed = 1;		/* Prevent redisplay shortcuts */ 
  return decode_window (window)->hscroll = ncol; 
} 
 
DEFUN ("window-edges", Fwindow_edges, Swindow_edges, 0, 1, 0, 
  "Return a list of the edge coordinates of WINDOW.\n\ 
\(LEFT TOP RIGHT BOTTOM), all relative to 0, 0 at top left corner of screen.") 
  (window) 
     Lisp_Object window; 
{ 
  register struct window *w = decode_window (window); 
 
  return Fcons (w->left, Fcons (w->top, 
           Fcons (make_number (XFASTINT (w->left) + XFASTINT (w->width)), 
		  Fcons (make_number (XFASTINT (w->top) 
				      + XFASTINT (w->height)), 
			 Qnil)))); 
} 
 
DEFUN ("window-dot", Fwindow_dot, Swindow_dot, 0, 1, 0, 
  "Return current value of dot in WINDOW.") 
  (window) 
     Lisp_Object window; 
{ 
  return Fmarker_position (decode_window (window)->dotm); 
} 
 
DEFUN ("window-start", Fwindow_start, Swindow_start, 0, 1, 0, 
  "Return position at which display currently starts in WINDOW.") 
  (window) 
     Lisp_Object window; 
{ 
  return Fmarker_position (decode_window (window)->start); 
} 
 
DEFUN ("set-window-dot", Fset_window_dot, Sset_window_dot, 2, 2, 0, 
  "Make dot value in WINDOW be at position POS in WINDOW's buffer.") 
  (window, pos) 
     Lisp_Object window, pos; 
{ 
  struct window *w = decode_window (window); 
  CHECK_NUMBER_COERCE_MARKER (pos, 1); 
  Fset_marker (w->dotm, pos, w->buffer); 
  return pos; 
} 
 
DEFUN ("set-window-start", Fset_window_start, Sset_window_start, 2, 2, 0, 
  "Make display in WINDOW start at position POS in WINDOW's buffer.") 
  (window, pos) 
     Lisp_Object window, pos; 
{ 
  struct window *w = decode_window (window); 
  CHECK_NUMBER_COERCE_MARKER (pos, 1); 
  Fset_marker (w->start, pos, w->buffer); 
  w->force_start = Qt; 
  w->redo_mode_line = Qt; 
  XFASTINT (w->last_modified) = 0; 
  return pos; 
} 
 
DEFUN ("delete-window", Fdelete_window, Sdelete_window, 0, 1, "", 
  "Remove WINDOW from the display.  Default is selected window.") 
  (window) 
     Lisp_Object window; 
{ 
  int osize; 
  Lisp_Object tem, parent; 
  register struct window *p; 
 
  if (NULL (window)) 
    window = selected_window; 
  else 
    CHECK_WINDOW (window, 0); 
 
  p = XWINDOW (window); 
  parent = p->parent; 
  if (NULL (parent)) 
    error ("Attempt to delete minibuffer or sole ordinary window"); 
 
  windows_or_buffers_changed++; 
 
  if (EQ (window, selected_window)) 
    Fselect_window (Fnext_window (window, Qnil)); 
 
  if (!NULL (p->buffer)) 
    unshow_buffer (p); 
 
  unchain_marker (XMARKER (p->dotm)); 
  unchain_marker (XMARKER (p->start)); 
 
  tem = p->next; 
  if (!NULL (tem)) 
    XWINDOW (tem)->prev = p->prev; 
 
  tem = p->prev; 
  if (!NULL (tem)) 
    XWINDOW (tem)->next = p->next; 
 
  if (EQ (window, XWINDOW (parent)->hchild)) 
    XWINDOW (parent)->hchild = p->next; 
  if (EQ (window, XWINDOW (parent)->vchild)) 
    XWINDOW (parent)->vchild = p->next; 
 
  /* Stretch the siblings to use all the available space */ 
  if (!NULL (XWINDOW (parent)->vchild)) 
    { 
      /* It's a vertical combination */ 
      osize = XFASTINT (XWINDOW (parent)->height); 
      XFASTINT (XWINDOW (parent)->height) 
	-= XFASTINT (p->height); 
      set_window_height (parent, osize); 
    } 
  if (!NULL (XWINDOW (parent)->hchild)) 
    { 
      /* It's a horizontal combination */ 
      osize = XFASTINT (XWINDOW (parent)->width); 
      XFASTINT (XWINDOW (parent)->width) 
	-= XFASTINT (p->width); 
      set_window_width (parent, osize); 
    } 
 
  /* If parent now has only one child, 
     put the child into the parent's place.  */ 
 
  tem = XWINDOW (parent)->hchild; 
  if (NULL (tem)) 
    tem = XWINDOW (parent)->vchild; 
  if (NULL (XWINDOW (tem)->next)) 
    replace_window (parent, tem); 
} 
 
/* Put replacement into the window structure in place of old. */ 
 
replace_window (old, replacement) 
     Lisp_Object old, replacement; 
{ 
  Lisp_Object tem; 
  register struct window *o = XWINDOW (old), *p = XWINDOW (replacement); 
 
  p->left = o->left; 
  p->top = o->top; 
  p->width = o->width; 
  p->height = o->height; 
 
  p->next = tem = o->next; 
  if (!NULL (tem)) 
    XWINDOW (tem)->prev = replacement; 
 
  p->prev = tem = o->prev; 
  if (!NULL (tem)) 
    XWINDOW (tem)->next = replacement; 
 
  p->parent = tem = o->parent; 
  if (!NULL (tem)) 
    { 
      if (EQ (XWINDOW (tem)->vchild, old)) 
	XWINDOW (tem)->vchild = replacement; 
      if (EQ (XWINDOW (tem)->hchild, old)) 
	XWINDOW (tem)->hchild = replacement; 
    } 
 
/*** Here, if replacement is a vertical combination 
and so is its new parent, we should make replacement's 
children be children of that parent instead.  ***/ 
} 
 
DEFUN ("next-window", Fnext_window, Snext_window, 0, 2, 0, 
  "Return next window after WINDOW in canonical ordering of windows.") 
  (window, yesmini) 
     Lisp_Object window, yesmini; 
{ 
  Lisp_Object tem; 
  if (NULL (window)) 
    window = selected_window; 
  else 
    CHECK_WINDOW (window, 0); 
  do 
    { 
      while (tem = XWINDOW (window)->next, NULL (tem)) 
	if (tem = XWINDOW (window)->parent, !NULL (tem)) 
	  window = tem; 
        else  /* window must be minibuf_window now */ 
	  { 
	    tem = XWINDOW (window)->prev; 
	    break; 
	  } 
      window = tem; 
      while (1) 
	{ 
	  if (!NULL (XWINDOW (window)->hchild)) 
	    window = XWINDOW (window)->hchild; 
	  else if (!NULL (XWINDOW (window)->vchild)) 
	    window = XWINDOW (window)->vchild; 
	  else break; 
	} 
    } 
  while (EQ (window, minibuf_window) && NULL (yesmini) && !MinibufDepth); 
  return window; 
} 
 
DEFUN ("previous-window", Fprevious_window, Sprevious_window, 0, 1, 0, 
  "Return previous window before WINDOW in canonical ordering of windows.") 
  (window) 
     Lisp_Object window; 
{ 
  Lisp_Object tem; 
  if (NULL (window)) 
    window = selected_window; 
  else 
    CHECK_WINDOW (window, 0); 
  do  /* at least once, and until not the minibuffer */ 
    { 
      while (tem = XWINDOW (window)->prev, NULL (tem)) 
	if (tem = XWINDOW (window)->parent, !NULL (tem)) 
	  window = tem; 
        else  /* window must be the root window now */ 
	  { 
	    tem = minibuf_window; 
	    break; 
	  } 
      window = tem; 
      while (1) 
	{ 
	  if (!NULL (XWINDOW (window)->hchild)) 
	    window = XWINDOW (window)->hchild; 
	  else if (!NULL (XWINDOW (window)->vchild)) 
	    window = XWINDOW (window)->vchild; 
	  else break; 
	  while (tem = XWINDOW (window)->next, !NULL (tem)) 
	    window = tem; 
	} 
    } 
  while (EQ (window, minibuf_window) && !MinibufDepth); 
  return window; 
} 
 
DEFUN ("other-window", Fother_window, Sother_window, 1, 1, "p", 
  "Select a different window by moving down ARG times.") 
  (n) 
     Lisp_Object n; 
{ 
  int i; 
  Lisp_Object w; 
 
  CHECK_NUMBER (n, 0); 
  w = selected_window; 
  i = XINT (n); 
 
  while (i > 0) 
    { 
      w = Fnext_window (w, Qnil); 
      i--; 
    } 
  while (i < 0) 
    { 
      w = Fprevious_window (w); 
      i++; 
    } 
  Fselect_window (w); 
  return Qnil; 
} 
 
Lisp_Object 
window_loop (type, obj) 
     int type; 
     Lisp_Object obj; 
{ 
  Lisp_Object w, w1, ret_w, tem; 
  register struct window *p, *q; 
 
  w = minibuf_window; 
  ret_w = Qnil; 
  while (1) 
    { 
      p = XWINDOW (w); 
      w1 = Fnext_window (w, Qt); 
      if (!EQ (w, minibuf_window)) 
	switch (type) 
	  { 
	  case 1: 
	    if (XBUFFER (p->buffer) == XBUFFER (obj)) 
	      return w; 
	    break; 
 
	  case 2: 
	    if (NULL (ret_w) || 
		XFASTINT (XWINDOW (ret_w)->use_time) > XFASTINT (p->use_time)) 
	      ret_w = w; 
	    break; 
 
	  case 3: 
	    if (p != XWINDOW (obj)) 
	      Fdelete_window (w); 
	    break; 
 
	  case 4: 
	    if (EQ (p->buffer, obj)) 
	      { 
		if (NULL (p->parent)) 
		  { 
		    tem = Fother_buffer (obj); 
		    if (NULL (tem)) 
		      tem = Fget_buffer_create (build_string ("*scratch*")); 
		    Fshow_buffer (w, tem); 
		    Fset_buffer (p->buffer); 
		  } 
		else 
		  Fdelete_window (w); 
	      } 
	    break; 
 
	  case 5: 
	    q = XWINDOW (ret_w); 
	    if (NULL (ret_w) || 
		(XFASTINT (p->height) * XFASTINT (p->width)) 
		< 
		(XFASTINT (q->height) * XFASTINT (q->width))) 
	      ret_w = w; 
	    break; 
 
	  case 6: 
	    if (EQ (p->buffer, obj)) 
	      { 
		tem = Fother_buffer (obj); 
		if (NULL (tem)) 
		  tem = Fget_buffer_create (build_string ("*scratch*")); 
		Fshow_buffer (w, tem); 
		Fset_buffer (p->buffer); 
	      } 
	    break; 
	  } 
      w = w1; 
      if (EQ (w, minibuf_window)) 
	return ret_w; 
    } 
}      
 
DEFUN ("get-lru-window", Fget_lru_window, Sget_lru_window, 0, 0, 0, 
  "Return the window least recently selected or used for display.") 
  () 
{ 
  return window_loop (2, Qnil); 
} 
 
DEFUN ("get-largest-window", Fget_largest_window, Sget_largest_window, 0, 0, 0, 
  "Return the largest window in area.") 
  () 
{ 
  return window_loop (5, Qnil); 
} 
 
DEFUN ("get-buffer-window", Fget_buffer_window, Sget_buffer_window, 1, 1, 0, 
  "Return a window currently displaying BUFFER, or nil if none.") 
  (buffer) 
     Lisp_Object buffer; 
{ 
  buffer = Fget_buffer (buffer); 
  CHECK_BUFFER (buffer, 0); 
 
  return window_loop (1, buffer); 
} 
 
DEFUN ("delete-other-windows", Fdelete_other_windows, Sdelete_other_windows, 
  0, 1, "", 
  "Make WINDOW (or the selected window) fill the screen.") 
  (w) 
     Lisp_Object w; 
{ 
  window_loop (3, !NULL (w) ? w : selected_window); 
  return Qnil; 
} 
 
DEFUN ("delete-windows-on", Fdelete_windows_on, Sdelete_windows_on, 
  1, 1, "bDelete windows on (buffer): ", 
  "Delete all windows showing BUFFER.") 
  (buffer) 
     Lisp_Object buffer; 
{ 
  if (!NULL (buffer)) 
    { 
      buffer = Fget_buffer (buffer); 
      CHECK_BUFFER (buffer, 0); 
      window_loop (4, buffer); 
    } 
  return Qnil; 
} 
 
DEFUN ("replace-buffer-in-windows", Freplace_buffer_in_windows, 
  Sreplace_buffer_in_windows, 
  1, 1, "bReplace buffer in windows: ", 
  "Replace BUFFER with some other buffer in all windows showing it.") 
  (buffer) 
     Lisp_Object buffer; 
{ 
  if (!NULL (buffer)) 
    { 
      buffer = Fget_buffer (buffer); 
      CHECK_BUFFER (buffer, 0); 
      window_loop (6, buffer); 
    } 
  return Qnil; 
} 
 
/* Set the height of WINDOW and all its inferiors.  */ 
 
set_window_height (window, height) 
     Lisp_Object window; 
     int height; 
{ 
  register struct window *w = XWINDOW (window); 
  register struct window *c; 
  int oheight = XFASTINT (w->height); 
  int top, pos, lastbot, opos, lastobot; 
  Lisp_Object tem, child; 
 
  if (window_min_height < 2) 
    window_min_height = 2; 
 
  if (height < (EQ(window, minibuf_window) ? 1 : window_min_height)) 
    { 
      Fdelete_window (window); 
      return; 
    } 
 
  XFASTINT (w->last_modified) = 0; 
  windows_or_buffers_changed++; 
  XFASTINT (w->height) = height; 
  if (!NULL (w->hchild)) 
    { 
      for (child = w->hchild; !NULL (child); child = XWINDOW (child)->next) 
	{ 
	  XWINDOW (child)->top = w->top; 
	  set_window_height (child, height); 
	} 
    } 
  else if (!NULL (w->vchild)) 
    { 
      lastbot = top = XFASTINT (w->top); 
      lastobot = 0; 
      for (child = w->vchild; !NULL (child);) 
	{ 
	  c = XWINDOW (child); 
 
	  opos = lastobot + XFASTINT (c->height); 
 
	  XFASTINT (c->top) = lastbot; 
 
	  pos = (((opos * height) << 1) + oheight) / (oheight << 1); 
 
	  /* This will delete child if it becomes too small! */ 
	  set_window_height (child, pos + top - lastbot); 
 
	  /* Now advance child to next window, 
	     and set lastbot if child was not just deleted.  */ 
	  tem = c->next; 
	  if (NULL (tem) || !EQ (XWINDOW (tem)->parent, window)) 
	    break; 
	  if (EQ (XWINDOW (tem)->prev, child)) 
	    lastbot = pos + top, lastobot = opos; 
	  child = tem; 
	} 
    } 
} 
 
/* Recursively set width of WINDOW and its inferiors. */ 
 
set_window_width (window, width) 
     Lisp_Object window; 
     int width; 
{ 
  register struct window *w = XWINDOW (window); 
  register struct window *c; 
  int owidth = XFASTINT (w->width); 
  int left, pos, lastright, opos, lastoright; 
  Lisp_Object tem, child; 
 
  if (width < window_min_width) 
    { 
      Fdelete_window (window); 
      return; 
    } 
 
  XFASTINT (w->last_modified) = 0; 
  XFASTINT (w->width) = width; 
  if (!NULL (w->vchild)) 
    { 
      for (child = w->vchild; !NULL (child); child = XWINDOW (child)->next) 
	{ 
	  XWINDOW (child)->left = w->left; 
	  set_window_width (child, width); 
	} 
    } 
  else if (!NULL (w->hchild)) 
    { 
      lastright = left = XFASTINT (w->left); 
      lastoright = 0; 
      for (child = w->hchild; !NULL (child);) 
	{ 
	  c = XWINDOW (child); 
 
	  opos = lastoright + XFASTINT (c->width); 
 
	  XFASTINT (c->left) = lastright; 
 
	  pos = (((opos * width) << 1) + owidth) / (owidth << 1); 
 
	  /* This will delete child if it becomes too small! */ 
	  set_window_width (child, pos + left - lastright); 
 
	  /* Now advance child to next window, 
	     and set lastright if child was not just deleted.  */ 
	  tem = c->next; 
	  if (NULL (tem) || !EQ (XWINDOW (tem)->parent, window)) 
	    break; 
	  if (EQ (XWINDOW (tem)->prev, child)) 
	    lastright = pos + left, lastoright = opos; 
	  child = tem; 
	} 
    } 
} 
 
static int window_select_count; 
 
DEFUN ("show-buffer", Fshow_buffer, Sshow_buffer, 2, 2, 0, 
  "Make WINDOW display BUFFER as its contents.\n\ 
BUFFER can be a buffer or buffer name.") 
  (window, buffer) 
     Lisp_Object window, buffer; 
{ 
  Lisp_Object tem; 
  struct window *w = decode_window (window); 
 
  buffer = Fget_buffer (buffer); 
  CHECK_BUFFER (buffer, 1); 
 
  if (NULL (XBUFFER (buffer)->name)) 
    error ("Attempt to display deleted buffer"); 
 
  tem = w->buffer; 
  if (!NULL (tem)) 
    unshow_buffer (w, tem); 
 
  w->buffer = buffer; 
  Fset_marker (w->dotm, 
	       make_number (XBUFFER (buffer) == bf_cur 
			    ? dot : XBUFFER (buffer)->text.dotloc), 
	       buffer); 
  Fset_marker (w->start, make_number (XBUFFER (buffer)->last_window_start), 
	       buffer); 
  XFASTINT (w->last_modified) = 0; 
  windows_or_buffers_changed++; 
  if (EQ (window, selected_window)) 
    Fset_buffer (buffer); 
 
  return Qnil; 
} 
 
/* Record info on buffer window w is displaying 
 when it is about to cease to display that buffer.  */ 
 
unshow_buffer (w) 
     struct window *w; 
{ 
  Lisp_Object buf; 
  buf = w->buffer; 
 
  if (XBUFFER (buf) != XMARKER (w->dotm)->buffer) 
    abort (); 
 
  if (w != XWINDOW (selected_window) 
      && EQ (buf, XWINDOW (selected_window)->buffer)) 
    return; 
 
  if (XBUFFER (buf) == bf_cur) 
    { 
      if (w != XWINDOW (selected_window)) 
	dot = marker_position (w->dotm); 
    } 
  else 
    XBUFFER (buf)->text.dotloc =  
      marker_position (w->dotm); 
  XBUFFER (buf)->last_window_start =  
    marker_position (w->start); 
} 
 
DEFUN ("select-window", Fselect_window, Sselect_window, 1, 1, 0, 
  "Select WINDOW.  Most editing will apply to WINDOW's buffer.\n\ 
The main editor command loop selects the buffer of the selected window\n\ 
before each command.") 
  (window) 
     Lisp_Object window; 
{ 
  register struct window *w = XWINDOW (window); 
  register struct window *ow = XWINDOW (selected_window); 
  register int newdot; 
 
  CHECK_WINDOW (window, 0); 
 
  if (NULL (w->buffer)) 
    error ("Trying to select window with no buffer"); 
 
  XFASTINT (w->use_time) = ++window_select_count; 
  if (EQ (window, selected_window)) 
    return window; 
 
  if (bf_cur == XBUFFER (ow->buffer)) 
    Fset_marker (ow->dotm, make_number (dot), ow->buffer); 
 
  selected_window = window; 
 
  Fset_buffer (w->buffer); 
  if (bf_cur == XBUFFER (ow->buffer)) 
    SetDot (marker_position (w->dotm)); 
 
  windows_or_buffers_changed++; 
 
  return window; 
} 
 
DEFUN ("display-buffer", Fdisplay_buffer, Sdisplay_buffer, 1, 1, 0, 
  "Make BUFFER appear in some window but don't select them.\n\ 
BUFFER can be a buffer or a buffer name.\n\ 
Returns the window displaying BUFFER.") 
  (buffer) 
     Lisp_Object buffer; 
{ 
  Lisp_Object window; 
  buffer = Fget_buffer (buffer); 
  CHECK_BUFFER (buffer, 0); 
 
  if (XBUFFER (XWINDOW (selected_window)->buffer) == XBUFFER (buffer)) 
    return selected_window; 
 
  window = Fget_buffer_window (buffer); 
  if (!NULL (window)) 
    return window; 
 
  if (pop_up_windows) 
    { 
      /* Don't try to create a window if would get an error */ 
      if (window_min_height < 2) 
	window_min_height = 2; 
      if (split_height_threshold < window_min_height << 1) 
	split_height_threshold = window_min_height << 1; 
 
      window = Fget_largest_window (); 
      if (window_height (window) >= split_height_threshold) 
	window = Fsplit_window (window, Qnil, Qnil); 
      else 
	{ 
	  window = Fget_lru_window (); 
	  if ((EQ (window, selected_window) 
	       || (EQ (selected_window, minibuf_window) 
		   && EQ (window, XWINDOW (minibuf_window)->prev))) 
	      && window_height (window) >= window_min_height << 1) 
	    window = Fsplit_window (window, Qnil, Qnil); 
	} 
    } 
  else 
    window = Fget_lru_window (); 
 
  Fshow_buffer (window, buffer); 
  return window; 
} 
 
temp_output_buffer_show (buf) 
     Lisp_Object buf; 
{ 
  struct buffer *old = bf_cur; 
  Lisp_Object window; 
 
  Fset_buffer (buf); 
  XBUFFER (buf)->save_modified = bf_modified; 
  SetDot (1); 
  bf_head_clip = 1; 
  bf_tail_clip = 0; 
  clip_changed = 1; 
  SetBfp (old); 
 
  window = Fdisplay_buffer (buf); 
  XFASTINT (XWINDOW (window)->hscroll) = 0; 
  Fset_marker (XWINDOW (window)->start, make_number (1), buf); 
  Fset_marker (XWINDOW (window)->dotm, make_number (1), buf); 
} 
 
make_dummy_parent (window) 
     Lisp_Object window; 
{ 
  Lisp_Object old, new; 
  register struct window *o, *p; 
  old = window; 
  XSETTYPE (old, Lisp_Vector); 
  new = Fcopy_sequence (old); 
  XSETTYPE (new, Lisp_Window); 
 
  o = XWINDOW (old); 
  p = XWINDOW (new); 
  XFASTINT (p->sequence_number) = ++sequence_number; 
 
  /* Put new into window structure in place of window */ 
  replace_window (window, new); 
 
  o->next = Qnil; 
  o->prev = Qnil; 
  o->vchild = Qnil; 
  o->hchild = Qnil; 
  o->parent = new; 
 
  p->start = Qnil; 
  p->dotm = Qnil; 
  p->buffer = Qnil; 
} 
 
DEFUN ("split-window", Fsplit_window, Ssplit_window, 0, 3, "", 
  "Split WINDOW, putting SIZE lines in the first of the pair.\n\ 
WINDOW defaults to selected one and SIZE to half its size.\n\ 
If optional third arg HOR-FLAG is non-nil, split side by side\n\ 
and put SIZE columns in the first of the pair.") 
  (window, chsize, horflag) 
     Lisp_Object window, chsize, horflag; 
{ 
  Lisp_Object new; 
  register struct window *o, *p, *w; 
  int size; 
 
  if (NULL (window)) 
    window = selected_window; 
  else 
    CHECK_WINDOW (window, 0); 
 
  o = XWINDOW (window); 
 
  if (NULL (chsize)) 
    { 
      if (!NULL (horflag)) 
	size = XFASTINT (o->width) >> 1; 
      else 
	size = XFASTINT (o->height) >> 1; 
    } 
  else 
    { 
      CHECK_NUMBER (chsize, 1); 
      size = XINT (chsize); 
    } 
 
  if (EQ (window, minibuf_window)) 
    error ("Attempt to split minibuffer window"); 
 
  if (NULL (horflag)) 
    { 
      if (window_min_height < 2) 
	window_min_height = 2; 
 
      if (size < window_min_height || 
	  size + window_min_height > XFASTINT (o->height)) 
	Fsignal (Qargs_out_of_range, Flist (3, &window)); 
      if (NULL (o->parent) || 
	  NULL (XWINDOW (o->parent)->vchild)) 
	{ 
	  make_dummy_parent (window); 
	  new = o->parent; 
	  XWINDOW (new)->vchild = window; 
	} 
    } 
  else 
    { 
      if (size < window_min_width || 
	  size + window_min_width > XFASTINT (o->width)) 
	Fsignal (Qargs_out_of_range, Flist (3, &window)); 
      if (NULL (o->parent) || 
	  NULL (XWINDOW (o->parent)->hchild)) 
	{ 
	  make_dummy_parent (window); 
	  new = o->parent; 
	  XWINDOW (new)->hchild = window; 
	} 
    } 
 
  /* Now we know that window's parent is a vertical combination 
     if we are dividing vertically, or a horizontal combination 
     if we are making side-by-side windows */ 
 
  windows_or_buffers_changed++; 
  new = make_window (); 
  p = XWINDOW (new); 
 
  p->next = o->next; 
  if (!NULL (p->next)) 
    XWINDOW (p->next)->prev = new; 
  p->prev = window; 
  o->next = new; 
  p->parent = o->parent; 
 
  Fshow_buffer (new, o->buffer); 
 
  /* Apportion the available screen space among the two new windows */ 
 
  if (!NULL (horflag)) 
    { 
      p->height = o->height; 
      p->top = o->top; 
      XFASTINT (p->width) = XFASTINT (o->width) - size; 
      XFASTINT (o->width) = size; 
      XFASTINT (p->left) = XFASTINT (o->left) + size; 
    } 
  else 
    { 
      p->left = o->left; 
      p->width = o->width; 
      XFASTINT (p->height) = XFASTINT (o->height) - size; 
      XFASTINT (o->height) = size; 
      XFASTINT (p->top) = XFASTINT (o->top) + size; 
    } 
 
  return new; 
} 
 
DEFUN ("enlarge-window", Fenlarge_window, Senlarge_window, 1, 2, "p", 
  "Make current window ARG lines bigger.\n\ 
From program, optional second arg non-nil means grow sideways ARG columns.") 
  (n, side) 
     Lisp_Object n, side; 
{ 
  CHECK_NUMBER (n, 0); 
  change_window_height (XINT (n), !NULL (side)); 
  return Qnil; 
} 
 
DEFUN ("shrink-window", Fshrink_window, Sshrink_window, 1, 2, "p", 
  "Make current window ARG lines smaller.\n\ 
From program, optional second arg non-nil means shrink sideways ARG columns.") 
  (n, side) 
     Lisp_Object n, side; 
{ 
  CHECK_NUMBER (n, 0); 
  change_window_height (-XINT (n), !NULL (side)); 
  return Qnil; 
} 
 
int 
window_height (window) 
     Lisp_Object window; 
{ 
  register struct window *p = XWINDOW (window); 
  return XFASTINT (p->height); 
} 
 
int 
window_width (window) 
     Lisp_Object window; 
{ 
  register struct window *p = XWINDOW (window); 
  return XFASTINT (p->width); 
} 
 
#define MINSIZE(window) \ 
  (widthflag ? window_min_width  \ 
   : (EQ (window, minibuf_window) ? 1 : window_min_height)) 
 
#define CURBEG(w) \ 
  *(widthflag ? (int *) &w->left : (int *) &w->top) 
 
#define CURSIZE(w) \ 
  *(widthflag ? (int *) &w->width : (int *) &w->height) 
 
/* Unlike set_window_height, this function 
 also changes the heights of the siblings so as to 
 keep everything consistent. */ 
 
change_window_height (delta, widthflag) 
     int delta, widthflag; 
{ 
  Lisp_Object window, parent, tem; 
  register struct window *p; 
  int *sizep; 
  int *startp; 
  int (*sizefun) () = widthflag ? window_width : window_height; 
  int (*setsizefun) () = widthflag ? set_window_width : set_window_height; 
  int opht, maxdelta; 
 
  window = selected_window; 
  while (1) 
    { 
      p = XWINDOW (window); 
      parent = p->parent; 
      if (NULL (parent)) 
	{ 
	  if (widthflag) 
	    error ("No other window to side of this one"); 
	  break; 
	} 
      if (widthflag ? !NULL (XWINDOW (parent)->hchild) 
	  : !NULL (XWINDOW (parent)->vchild)) 
	break; 
      window = parent; 
    } 
 
  sizep = &CURSIZE (p); 
  startp = &CURBEG (p); 
 
  if (*sizep + delta < MINSIZE (window)) 
    { 
      Fdelete_window (window); 
      return; 
    } 
 
  maxdelta = (!NULL (parent) ? sizefun (parent) 
	      : (*sizep 
		 + (tem = (!NULL (p->next) ? p->next : p->prev), 
		    sizefun (tem) - MINSIZE (tem)))); 
 
  if (delta > maxdelta) 
    /* This case traps trying to make the minibuffer 
       the full screen, or make the only window aside from the 
       minibuffer the full screen.  */ 
    delta = maxdelta; 
 
  if (!NULL (p->next) && 
      sizefun (p->next) - delta >= MINSIZE (p->next)) 
    { 
      setsizefun (p->next, sizefun (p->next) - delta); 
      setsizefun (window, *sizep + delta); 
      CURBEG (XWINDOW (p->next)) += delta; 
      /* This does not change size of p->next, 
	 but it propagates the new top edge to its children */ 
      setsizefun (p->next, sizefun (p->next)); 
    } 
  else if (!NULL (p->prev) && 
	   sizefun (p->prev) - delta >= MINSIZE (p->prev)) 
    { 
      setsizefun (p->prev, sizefun (p->prev) - delta); 
      CURBEG (p) -= delta; 
      setsizefun (window, *sizep + delta); 
    } 
  else 
    { 
      opht = sizefun (parent); 
 
      delta = (int) ((float) (delta * opht) 
		     / (float) (opht - *sizep - delta)); 
      /* Add delta lines or columns to this window, and to the parent, 
	 keeping things consistent while not affecting siblings.  */ 
      CURSIZE (XWINDOW (parent)) = opht + delta; 
      setsizefun (window, *sizep + delta); 
      /* Squeeze out delta lines or columns from our parent, 
	 shriking this window and siblings proportionately. 
	 This brings parent back to correct size. 
	 Delta was calculated so this makes this window the desired size, 
	 taking it all out of the siblings.  */ 
      setsizefun (parent, opht); 
    } 
 
  XFASTINT (p->last_modified) = 0; 
} 
 
static 
window_scroll (window, n) 
     Lisp_Object window; 
     int n; 
{ 
  register struct window *w = XWINDOW (window); 
  register int odot = dot; 
  register int ht, pos; 
  Lisp_Object tem; 
 
  ht = XFASTINT (w->height) - !EQ (selected_window, minibuf_window); 
 
  if (EQ (window, selected_window)) 
    XFASTINT (tem) = dot; 
  else 
    tem = Fmarker_position (w->dotm); 
 
  tem = Fpos_visible_in_window_p (tem, window); 
  if (NULL (tem)) 
    { 
      Frecenter (make_number (ht / 2)); 
    } 
 
  SetDot (marker_position (w->start)); 
  Fvertical_motion (make_number (n)); 
  pos = dot; 
  SetDot (odot); 
 
  if (pos < NumCharacters + 1) 
    { 
      Fset_marker (w->start, make_number (pos), w->buffer); 
      w->redo_mode_line = Qt; 
      XFASTINT (w->last_modified) = 0; 
      if (pos > odot) 
	SetDot (pos); 
      if (n < 0) 
	{ 
	  SetDot (pos); 
	  tem = Fvertical_motion (make_number (ht)); 
	  if (dot > odot || XFASTINT (tem) < ht) 
	    SetDot (odot); 
	  else 
	    Fvertical_motion (make_number (-1)); 
	} 
    } 
  else 
    Fsignal (Qend_of_buffer, Qnil); 
} 
 
DEFUN ("scroll-up", Fscroll_up, Sscroll_up, 0, 1, "P", 
  "Scroll text of current window upward ARG lines; or near full screen if no ARG.\n\ 
When calling from a program, supply a number as argument or nil.") 
  (n) 
     Lisp_Object n; 
{ 
  register int ht = window_height (selected_window) - 1; 
 
  if (NULL (n)) 
    window_scroll (selected_window, ht - next_screen_context_lines); 
  else if (EQ (n, Qminus)) 
    window_scroll (selected_window, next_screen_context_lines - ht); 
  else 
    { 
      if (XTYPE (n) == Lisp_Cons) 
	n = Fcar (n); 
      CHECK_NUMBER (n, 0); 
      window_scroll (selected_window, XINT (n), 0); 
    } 
  return Qnil; 
} 
 
DEFUN ("scroll-down", Fscroll_down, Sscroll_down, 0, 1, "P", 
  "Scroll text of current window downward ARG lines; or near full screen if no ARG.\n\ 
When calling from a program, supply a number as argument or nil.") 
  (n) 
     Lisp_Object n; 
{ 
  register int ht = window_height (selected_window) - 1; 
 
  if (NULL (n)) 
    window_scroll (selected_window, next_screen_context_lines - ht); 
  else if (EQ (n, Qminus)) 
    window_scroll (selected_window, ht - next_screen_context_lines); 
  else 
    { 
      if (XTYPE (n) == Lisp_Cons) 
	n = Fcar (n); 
      CHECK_NUMBER (n, 0); 
      window_scroll (selected_window, -XINT (n)); 
    } 
  return Qnil; 
} 
 
DEFUN ("scroll-left", Fscroll_left, Sscroll_left, 1, 1, "p", 
  "Scroll selected window display ARG columns left.") 
  (arg) 
     Lisp_Object arg; 
{ 
  CHECK_NUMBER (arg, 0); 
  return Fset_window_hscroll (selected_window, 
			      make_number (XINT (XWINDOW (selected_window)->hscroll) 
					   + XINT (arg))); 
} 
 
DEFUN ("scroll-right", Fscroll_right, Sscroll_right, 1, 1, "p", 
  "Scroll selected window display ARG columns right.") 
  (arg) 
     Lisp_Object arg; 
{ 
  CHECK_NUMBER (arg, 0); 
  return Fset_window_hscroll (selected_window, 
			      make_number (XINT (XWINDOW (selected_window)->hscroll) 
					   - XINT (arg))); 
} 
 
DEFUN ("scroll-other-window", Fscroll_other_window, Sscroll_other_window, 0, 1, "P", 
  "Scroll text of next window upward ARG lines; or near full screen if no ARG.\n\ 
The next window is the one below the current one; or the one at the top\n\ 
if the current one is at the bottom.\n\ 
When calling from a program, supply a number as argument or nil.") 
  (n) 
     Lisp_Object n; 
{ 
  Lisp_Object window; 
  struct buffer *old = bf_cur; 
  register int ht; 
  register int odot = dot; 
 
  window = Fnext_window (selected_window, Qnil); 
  ht = window_height (window) - 1; 
 
  if (EQ (window, selected_window)) 
    error ("There is no other window"); 
 
  Fset_buffer (XWINDOW (window)->buffer); 
 
  if (NULL (n)) 
    window_scroll (window, ht - next_screen_context_lines); 
  else if (EQ (n, Qminus)) 
    window_scroll (window, next_screen_context_lines - ht); 
  else 
    { 
      if (XTYPE (n) == Lisp_Cons) 
	n = Fcar (n); 
      CHECK_NUMBER (n, 0); 
      window_scroll (window, XINT (n)); 
    } 
 
  Fset_marker (XWINDOW (window)->dotm, make_number (dot), Qnil); 
  SetBfp (old); 
  SetDot (odot); 
  return Qnil; 
} 
 
DEFUN ("recenter", Frecenter, Srecenter, 0, 1, "P", 
  "Center dot in window and redisplay screen.  With ARG, put dot on line ARG.\n\ 
The desired position of dot is always relative to the current window.\n\ 
If there is no ARG (i.e., it is nil) then the entire screen is redisplayed.") 
  (n) 
     Lisp_Object n; 
{ 
  int ht = window_height (selected_window) 
	    - !EQ (selected_window, minibuf_window); 
  struct window *w = XWINDOW (selected_window); 
  register int odot = dot; 
 
  if (NULL (n)) 
    { 
      extern int ScreenGarbaged; 
      ScreenGarbaged++; 
      XFASTINT (n) = ht / 2; 
    } 
  else 
    { 
      n = Fprefix_numeric_value (n); 
      CHECK_NUMBER (n, 0); 
    } 
 
  if (XINT (n) < 0) XINT (n) += ht; 
 
  XINT (n) *= -1; 
 
  Fvertical_motion (n); 
  Fset_marker (w->start, make_number (dot), w->buffer); 
 
  SetDot (odot); 
  w->force_start = Qt; 
 
  return Qnil; 
} 
 
DEFUN ("move-to-screen-line", Fmove_to_screen_line, Smove_to_screen_line, 
  1, 1, "P", 
  "Position dot relative to window.\n\ 
With no argument, position at text at center of window.\n\ 
An argument specifies screen line; zero means top of window,\n\ 
negative means relative to bottom of window.") 
  (arg) 
     Lisp_Object arg; 
{ 
  int height = XFASTINT (XWINDOW (selected_window)->height); 
 
  if (!EQ (selected_window, minibuf_window)) height--; 
 
  if (NULL (arg)) 
    XFASTINT (arg) = height / 2; 
  else 
    { 
      arg = Fprefix_numeric_value (arg); 
      if (XINT (arg) < 0) 
	XINT (arg) += height; 
    } 
 
  SetDot (marker_position (XWINDOW (selected_window)->start)); 
  return Fvertical_motion (arg); 
} 
 
struct saved_window 
  { 
    Lisp_Object window; 
    Lisp_Object buffer, start, dotm, mark; 
    Lisp_Object left, top, width, height, hscroll; 
    Lisp_Object parent, prev; 
  }; 
 
struct save_window_data 
  { 
    long size; 
    struct Lisp_Vector *unused; 
    Lisp_Object nsaved; 
    Lisp_Object current_window; 
    Lisp_Object current_buffer; 
    struct saved_window info[1]; 
  }; 
 
save_window_restore (arg) 
     Lisp_Object arg; 
{ 
  register struct window *w; 
  register struct save_window_data *data = (struct save_window_data *) XINTPTR (arg); 
  register struct saved_window *p = data->info; 
  Lisp_Object tem; 
  register int k; 
 
  for (k = 0; k < XFASTINT (data->nsaved); k++, p++) 
    { 
      w = XWINDOW (p->window); 
      w->next = Qnil; 
 
      if (!NULL (p->parent)) 
	w->parent = data->info[XFASTINT (p->parent)].window; 
      else 
	w->parent = Qnil; 
 
      if (!NULL (p->prev)) 
	w->prev = data->info[XFASTINT (p->prev)].window, 
        XWINDOW (w->prev)->next = p->window; 
      else 
	{ 
	  w->prev = Qnil; 
	  if (!NULL (w->parent)) 
	    { 
	      if (EQ (p->width, XWINDOW (w->parent)->width)) 
		XWINDOW (w->parent)->vchild = p->window, 
		XWINDOW (w->parent)->hchild = Qnil; 
	      else 
		XWINDOW (w->parent)->hchild = p->window, 
		XWINDOW (w->parent)->vchild = Qnil; 
	    } 
	} 
      w->buffer = p->buffer; 
      w->left = p->left; 
      w->top = p->top; 
      w->width = p->width; 
      w->height = p->height; 
      w->hscroll = p->hscroll; 
      XFASTINT (w->last_modified) = 0; 
 
      /* Position first line in window */ 
      if (!NULL (w->buffer)) 
	{ 
	  Fset_marker (w->start, Fmarker_position (p->start), w->buffer); 
	  unchain_marker (p->start); 
	  Fset_marker (w->dotm, Fmarker_position (p->dotm), w->buffer); 
	  unchain_marker (p->dotm); 
	} 
 
      tem = p->mark; 
      if (NULL (tem)) 
	XBUFFER (w->buffer)->mark = Qnil; 
      else 
	{ 
	  if (NULL (XBUFFER (w->buffer)->mark)) 
	    XBUFFER (w->buffer)->mark = Fmake_marker (); 
 
	  Fset_marker (XBUFFER (w->buffer)->mark, 
		       Fmarker_position (p->mark), w->buffer); 
	  unchain_marker (p->mark); 
	} 
    } 
 
  Fselect_window (data->current_window); 
  Fset_buffer (data->current_buffer); 
 
  free (data); 
} 
 
count_windows (window) 
     struct window *window; 
{ 
  int count = 1; 
  if (!NULL (window->next)) 
    count += count_windows (XWINDOW (window->next)); 
  if (!NULL (window->vchild)) 
    count += count_windows (XWINDOW (window->vchild)); 
  if (!NULL (window->hchild)) 
    count += count_windows (XWINDOW (window->hchild)); 
  return count; 
} 
 
#define SAVE_DATA_SIZE(n_windows) \ 
  ((n_windows - 1) * sizeof (struct saved_window) \ 
   + sizeof (struct save_window_data)) 
 
Lisp_Object 
save_window_save () 
{ 
  Lisp_Object root; 
  int n_windows; 
  struct save_window_data *data; 
  Lisp_Object tem; 
  int size; 
 
  root = XWINDOW (minibuf_window)->prev; 
  n_windows = count_windows (XWINDOW (root)); 
 
  size = SAVE_DATA_SIZE (n_windows); 
  data = (struct save_window_data *) malloc (size); 
 
  data->size = (size - sizeof (long) - sizeof (struct Lisp_Vector *)) 
    / sizeof (Lisp_Object); 
  XFASTINT (data->nsaved) = n_windows; 
  data->current_window = selected_window; 
  XSETTYPE (data->current_buffer, Lisp_Buffer); 
  XSETBUFFER (data->current_buffer, bf_cur); 
 
  save_window_save_1 (root, data, 0); 
 
  XSETTYPE (tem, Lisp_Temp_Vector), XSETINTPTR (tem, data); 
  return tem; 
} 
 
save_window_save_1 (window, data, i) 
     Lisp_Object window; 
     struct save_window_data *data; 
     int i; 
{ 
  struct saved_window *p; 
  struct window *w; 
  Lisp_Object tem; 
 
  for (;!NULL (window); window = w->next) 
    { 
      p = &data->info[i]; 
      w = XWINDOW (window); 
 
      XFASTINT (w->temslot) = i++; 
      p->window = window; 
      p->buffer = w->buffer; 
      p->left = w->left; 
      p->top = w->top; 
      p->width = w->width; 
      p->height = w->height; 
      p->hscroll = w->hscroll; 
      if (!NULL (w->buffer)) 
	{ 
	  if (EQ (window, selected_window) 
	      && XBUFFER (w->buffer) == bf_cur) 
	    p->dotm = Fdot_marker (); 
	  else 
	    p->dotm = Fcopy_marker (w->dotm); 
 
	  p->start = Fcopy_marker (w->start); 
 
	  tem = XBUFFER (w->buffer)->mark; 
	  if (!NULL (tem)) 
	    p->mark = Fcopy_marker (tem); 
	  else 
	    p->mark = Qnil; 
	} 
      else 
	{ 
	  p->dotm = Qnil; 
	  p->start = Qnil; 
	  p->mark = Qnil; 
	} 
 
      if (NULL (w->parent)) 
	p->parent = Qnil; 
      else 
	p->parent = XWINDOW (w->parent)->temslot; 
 
      if (NULL (w->prev)) 
	p->prev = Qnil; 
      else 
	p->prev = XWINDOW (w->prev)->temslot; 
 
      if (!NULL (w->vchild)) 
	i = save_window_save_1 (w->vchild, data, i); 
      if (!NULL (w->hchild)) 
	i = save_window_save_1 (w->hchild, data, i); 
    } 
 
  return i; 
} 
 
DEFUN ("save-window-excursion", Fsave_window_excursion, Ssave_window_excursion, 
  0, UNEVALLED, 0, 
  "Execute body, preserving window sizes and contents.\n\ 
Restores which buffer appears in which window, where display starts,\n\ 
as well as dot and mark.") 
  (args) 
     Lisp_Object args; 
{ 
  Lisp_Object tem, val; 
  struct gcpro gcpro1; 
  int count = specpdl_ptr - specpdl; 
 
  tem = save_window_save (); 
 
  GCPRO1 (*XOBJFWD (tem)); 
  gcpro1.nvars 
    = SAVE_DATA_SIZE (XFASTINT (((struct save_window_data *) XINTPTR (tem)) 
				->nsaved)) 
      / 4; 
 
  record_unwind_protect (save_window_restore, tem); 
  val = Fprogn (args); 
  UNGCPRO; 
  unbind_to (count); 
  return val; 
} 
 
init_window_once () 
{ 
  extern Lisp_Object get_minibuffer (); 
  Lisp_Object root_window; 
 
  root_window = make_window (); 
  minibuf_window = make_window (); 
 
  XWINDOW (root_window)->next = minibuf_window; 
  XWINDOW (minibuf_window)->prev = root_window; 
 
  /* These values 9 and 10 are arbitrary, 
     just so that there is "something there." 
     Correct values are put in in init_xdisp */ 
 
  XFASTINT (XWINDOW (root_window)->width) = 10; 
  XFASTINT (XWINDOW (minibuf_window)->width) = 10; 
 
  XFASTINT (XWINDOW (root_window)->height) = 9; 
  XFASTINT (XWINDOW (minibuf_window)->top) = 9; 
  XFASTINT (XWINDOW (minibuf_window)->height) = 1; 
 
  Fshow_buffer (root_window, Fcurrent_buffer ()); 
  Fshow_buffer (minibuf_window, get_minibuffer (0)); 
 
  selected_window = root_window; 
} 
 
syms_of_window () 
{ 
  Qwindowp = intern ("windowp"); 
  staticpro (&Qwindowp); 
 
  /* Make sure all windows get marked */ 
  staticpro (&minibuf_window); 
 
  DefBoolVar ("pop-up-windows", &pop_up_windows, 
    "*Non-nil means display-buffer should make new windows."); 
  pop_up_windows = 1; 
 
  DefIntVar ("next-screen-context-lines", &next_screen_context_lines, 
    "*Number of lines of continuity when scrolling by screenfuls."); 
  next_screen_context_lines = 2; 
 
  DefIntVar ("split-height-threshold", &split_height_threshold, 
    "*display-buffer would prefer to split the largest window if this large.\n\ 
If there is only one window, it is split regardless of this value."); 
  split_height_threshold = 500; 
 
  DefIntVar ("window-min-height", &window_min_height, 
    "*Delete any window less than this tall (including its mode line)."); 
  window_min_height = 4; 
 
  DefIntVar ("window-min-width", &window_min_width, 
    "*Delete any window less than this wide."); 
  window_min_width = 10; 
 
  defsubr (&Sselected_window); 
  defsubr (&Swindowp); 
  defsubr (&Spos_visible_in_window_p); 
  defsubr (&Swindow_buffer); 
  defsubr (&Swindow_height); 
  defsubr (&Swindow_width); 
  defsubr (&Swindow_hscroll); 
  defsubr (&Sset_window_hscroll); 
  defsubr (&Swindow_edges); 
  defsubr (&Swindow_dot); 
  defsubr (&Swindow_start); 
  defsubr (&Sset_window_dot); 
  defsubr (&Sset_window_start); 
  defsubr (&Snext_window); 
  defsubr (&Sprevious_window); 
  defsubr (&Sother_window); 
  defsubr (&Sget_lru_window); 
  defsubr (&Sget_largest_window); 
  defsubr (&Sget_buffer_window); 
  defsubr (&Sdelete_other_windows); 
  defsubr (&Sdelete_windows_on); 
  defsubr (&Sreplace_buffer_in_windows); 
  defsubr (&Sdelete_window); 
  defsubr (&Sshow_buffer); 
  defsubr (&Sselect_window); 
  defsubr (&Sdisplay_buffer); 
  defsubr (&Ssplit_window); 
  defsubr (&Senlarge_window); 
  defsubr (&Sshrink_window); 
  defsubr (&Sscroll_up); 
  defsubr (&Sscroll_down); 
  defsubr (&Sscroll_left); 
  defsubr (&Sscroll_right); 
  defsubr (&Sscroll_other_window); 
  defsubr (&Srecenter); 
  defsubr (&Smove_to_screen_line); 
  defsubr (&Ssave_window_excursion); 
} 
 
keys_of_window () 
{ 
  defkey (CtlXmap, '1', "delete-other-windows"); 
  defkey (CtlXmap, '2', "split-window"); 
  defkey (CtlXmap, '0', "delete-window"); 
  defkey (CtlXmap, 'o', "other-window"); 
  defkey (CtlXmap, '^', "enlarge-window"); 
  defkey (CtlXmap, '<', "scroll-left"); 
  defkey (CtlXmap, '>', "scroll-right"); 
 
  defkey (GlobalMap, Ctl ('V'), "scroll-up"); 
  defkey (ESCmap, Ctl ('V'), "scroll-other-window"); 
  defkey (ESCmap, 'v', "scroll-down"); 
 
  defkey (GlobalMap, Ctl('L'), "recenter"); 
  defkey (ESCmap, 'r', "move-to-screen-line"); 
} 
