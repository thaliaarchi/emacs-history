/* Indentation functions.N
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
#include "indent.h" 
#include "window.h" 
 
#define CR '\015' 
 
int indent_tabs_mode; 
 
#define min(a, b) ((a) < (b) ? (a) : (b)) 
#define max(a, b) ((a) > (b) ? (a) : (b)) 
 
/* These three values memoize the current column to avoid recalculation */ 
/* Some things in buflow.c set last_known_column_dot to -1 
  to mark the memoized value as invalid */ 
 
/* Last value returned by current_column */ 
int last_known_column; 
/* Value of dot when current_column was called */ 
int last_known_column_dot; 
/* Value of bf_modified when current_column was called */ 
int last_known_column_modified; 
 
DEFSIMPLE ("current-column", Fcurrent_column, Scurrent_column, 
  "Return the horizontal position of dot.  The left margin is column 0.\n\ 
Ignores finite width of screen,", 
  Lisp_Int, XSETINT, current_column ()) 
 
current_column () 
{ 
  register int col; 
  register unsigned char *ptr, *stop, c; 
  register int tab_seen; 
  register int post_tab; 
  register int tab_width = XINT (bf_cur->tab_width); 
  int ctl_arrow = !NULL (bf_cur->ctl_arrow); 
 
  if (dot == last_known_column_dot 
      && bf_modified == last_known_column_modified) 
    return last_known_column; 
 
  ptr = &CharAt (dot - 1) + 1; 
  stop = dot <= bf_s1 + 1 ? bf_p1 + 1 : bf_p2 + bf_s1 + 1; 
  if (tab_width <= 0) tab_width = 1; 
 
  col = 0, tab_seen = 0, post_tab = 0; 
 
  while (1) 
    { 
      if (ptr == stop) 
	{ 
	  if (ptr == bf_p1 + 1) 
	    break; 
	  stop = bf_p1 + 1; 
	  ptr = stop + bf_s1; 
	  if (!bf_s1) break; 
	} 
 
      c = *--ptr; 
      if (c >= 040 && c < 0177) 
	{ 
	  col++; 
	} 
      else if (c == '\n') 
	break; 
      else if (c == '\t') 
	{ 
	  if (tab_seen) 
	    col = ((col + tab_width) / tab_width) * tab_width; 
 
	  post_tab += col; 
	  col = 0; 
	  tab_seen = 1; 
	} 
      else 
	col += (ctl_arrow && c < 0200) ? 2 : 4; 
    } 
 
  if (tab_seen) 
    { 
      col = ((col + tab_width) / tab_width) * tab_width; 
      col += post_tab; 
    } 
 
  last_known_column = col; 
  last_known_column_dot = dot; 
  last_known_column_modified = bf_modified; 
 
  return col; 
} 
 
ToCol (col) 
     int col; 
{ 
  register int fromcol = current_column (); 
  register int n; 
  register int tab_width = XINT (bf_cur->tab_width); 
 
  if (fromcol > col) 
    return; 
 
  if (tab_width <= 0) tab_width = 1; 
 
  if (indent_tabs_mode) 
    { 
      n = col / tab_width - fromcol / tab_width; 
      if (n) 
	{ 
	  while (n-- > 0) 
	    InsCStr ("\t", 1); 
 
	  fromcol = (col / tab_width) * tab_width; 
	} 
    } 
 
  while (fromcol < col) 
    { 
      InsCStr ("        ", min (8, col - fromcol)); 
      fromcol += min (8, col - fromcol); 
    } 
 
  last_known_column = col; 
  last_known_column_dot = dot; 
  last_known_column_modified = bf_modified; 
} 
 
DEFUN ("indent-to", Findent_to, Sindent_to, 1, 2, "nIndent to column: ", 
  "Indent from dot with tabs and spaces until COLUMN is reached.\n\ 
Always do at least MIN spaces even if that goes past COLUMN;\n\ 
by default, MIN is zero.") 
  (col, minimum) 
     Lisp_Object col, minimum; 
{ 
  int mincol; 
 
  CHECK_NUMBER (col, 0); 
  if (NULL (minimum)) 
    XFASTINT (minimum) = 0; 
  CHECK_NUMBER (minimum, 1); 
 
  mincol = current_column () + XINT (minimum); 
  if (mincol < XINT (col)) mincol = XINT (col); 
 
  ToCol (mincol); 
 
  XINT (col) = mincol; 
  return col; 
} 
 
DEFUN ("current-indentation", Fcurrent_indentation, Scurrent_indentation, 
  0, 0, 0, 
  "Return the indentation of the current line.\n\ 
This is the horizontal position of the character\n\ 
following any initial whitespace.") 
  () 
{ 
  Lisp_Object val; 
 
  XFASTINT (val) = position_indentation (ScanBf ('\n', dot, -1)); 
  return val; 
} 
 
position_indentation (pos) 
     register int pos; 
{ 
  register int col = 0; 
  register int c; 
  register int end = NumCharacters + 1; 
  register int tab_width = XINT (bf_cur->tab_width); 
  if (tab_width <= 0) tab_width = 1; 
 
  while (pos < end && 
	 (c = CharAt (pos), 
	  c == '\t' ? (col += tab_width - col % tab_width) 
	    : c == ' ' ? ++col : 0)) 
    pos++; 
 
  return col; 
} 
 
DEFUN ("move-to-column", Fmove_to_column, Smove_to_column, 1, 1, 0, 
  "Move dot to column COLUMN in the current line.\n\ 
Does not change the text, only dot.\n\ 
Ignores finite width of screen.") 
  (column) 
     Lisp_Object column; 
{ 
  register int pos = dot; 
  register int col = current_column (); 
  register int goal; 
  register int end = NumCharacters; 
  register int tab_width = XINT (bf_cur->tab_width); 
  register int ctl_arrow = !NULL (bf_cur->ctl_arrow); 
 
  Lisp_Object val; 
 
  if (tab_width <= 0) tab_width = 1; 
  CHECK_NUMBER (column, 0); 
  goal = XINT (column); 
  if (col > goal) 
    { 
      pos = ScanBf ('\n', pos, -1); 
      col = 0; 
    } 
 
  while (col < goal && pos <= end) 
    { 
      char c = CharAt (pos); 
      if (c == '\n') 
	break; 
      pos++; 
      col++; 
      if (c == '\t') 
	{ 
	  col += tab_width - 1; 
	  col = col / tab_width * tab_width; 
	} 
      else if (ctl_arrow && (c < 040 || c == 0177)) 
        col++; 
      else if (c < 040 || c >= 0177) 
        col += 3; 
    } 
 
  SetDot (pos); 
 
  last_known_column = col; 
  last_known_column_dot = dot; 
  last_known_column_modified = bf_modified; 
 
  XFASTINT (val) = col; 
  return val; 
} 
 
struct position 
compute_motion (from, fromvpos, fromhpos, to, tovpos, tohpos, width, hscroll) 
     int from, fromvpos, fromhpos, to, tovpos, tohpos; 
     register int width; 
     int hscroll; 
{ 
  register int cpos = fromhpos + (fromvpos << SHORTBITS); 
  register int target = tohpos + (tovpos << SHORTBITS); 
  register int pos; 
  register int c; 
  register int tab_width = XFASTINT (bf_cur->tab_width); 
  register int ctl_arrow = !NULL (bf_cur->ctl_arrow); 
  int selective 
    = XTYPE (bf_cur->selective_display) == Lisp_Int 
      ? XINT (bf_cur->selective_display) 
	: !NULL (bf_cur->selective_display) ? -1 : 0; 
  int prevpos; 
  int invis; 
  struct position val; 
 
  if (tab_width <= 0) tab_width = 1; 
  for (pos = from; pos < to && cpos < target; pos++) 
    { 
      prevpos = cpos; 
      c = CharAt (pos); 
      if (c >= 040 && c < 0177) 
	cpos++; 
      else if (c == '\t') 
	{ 
	  cpos += tab_width 
	    - (short) (cpos + hscroll - (hscroll > 0)) % tab_width; 
	} 
      else if (c == '\n') 
	{ 
	  if (selective > 0 && position_indentation (pos + 1) >= selective) 
	    { 
	      /* Skip any number of invisible lines all at once */ 
	      do 
		{ 
		  while (++pos < to && CharAt(pos) != '\n'); 
		} 
	      while (selective > 0 && position_indentation (pos + 1) >= selective); 
	      pos--; 
	      /* Allow for the " ..." that is displayed for them. */ 
	      cpos += 4; 
	      if ((short) cpos >= width) 
		cpos -= (short) cpos - width; 
	    } 
	  else 
	    cpos += (1 << SHORTBITS) - (short) cpos; 
	  cpos -= hscroll; 
	  if (hscroll > 0) cpos++; /* Count the ! on column 0 */ 
	} 
      else if (c == CR && selective < 0) 
	{ 
	  /* In selective display mode, 
	     everything from a ^M to the end of the line is invisible */ 
	invisible: 
	  while (pos < to && CharAt(pos) != '\n') pos++; 
	  pos--; 
	} 
      else 
	cpos += (ctl_arrow && c < 0200) ? 2 : 4; 
 
      if ((short) cpos >= width 
	  && ((short) cpos > width 
	      || (pos < NumCharacters 
		  && CharAt (pos + 1) != '\n'))) 
	{ 
	  if (cpos >= target) 
	    break; 
	  if (!hscroll && NULL (bf_cur->truncate_lines)) 
	    { 
	      cpos += (1 << SHORTBITS) - width; 
	    } 
	  else 
	    { 
	      while (pos < to && CharAt(pos) != '\n') pos++; 
	      pos--; 
	    } 
	} 
    } 
 
  val.bufpos = pos; 
  val.hpos = (short) cpos; 
  val.vpos = (cpos >> SHORTBITS) + (val.hpos < 0); 
  val.prevhpos = (short) prevpos; 
 
  /* Nonzero if have just continued a line */ 
  val.contin 
    = pos != from 
      && val.vpos != (prevpos >> SHORTBITS) + ((short) prevpos < 0) 
      && c != '\n'; 
 
  return val; 
} 
 
struct position 
vmotion (from, vtarget, width, hscroll) 
     register int from, vtarget, width; 
     int hscroll; 
{ 
  struct position pos; 
  /* vpos is cumulative vertical position, changed as from is changed */ 
  register int vpos = 0; 
  register int prevline; 
  register int first; 
  int lmargin = hscroll > 0 ? 1 - hscroll : 0; 
  int selective 
    = XTYPE (bf_cur->selective_display) == Lisp_Int 
      ? XINT (bf_cur->selective_display) 
	: !NULL (bf_cur->selective_display) ? -1 : 0; 
 
 retry: 
  if (vtarget > vpos) 
    { 
      /* Moving downward is simple, but must calculate from beg of line  
	 to determine hpos of starting point */ 
      if (from > FirstCharacter && CharAt (from - 1) != '\n') 
	{ 
	  prevline = ScanBf ('\n', from, -1); 
	  while (selective > 0 
		 && prevline > FirstCharacter 
		 && position_indentation (prevline) >= selective) 
	    prevline = ScanBf ('\n', prevline - 1, -1); 
	  pos = compute_motion (prevline, 0, lmargin, 
				from, 10000, 10000, 
				width, hscroll); 
	} 
      else 
	pos.hpos = lmargin; 
 
      return compute_motion (from, vpos, pos.hpos, 
			     1 + NumCharacters, vtarget, - (1 << (SHORTBITS - 1)), 
			     width, hscroll); 
    } 
 
  /* To move upward, go a line at a time until 
     we have gone at least far enough */ 
 
  first = 1; 
 
  while ((vpos > vtarget || first) && from > FirstCharacter) 
    { 
      prevline = from; 
      while (1) 
	{ 
	  prevline = ScanBf ('\n', prevline - 1, -1); 
	  if (prevline == FirstCharacter 
	      || selective <= 0 
	      || position_indentation (prevline) < selective) 
	    break; 
	} 
      pos = compute_motion (prevline, 0, lmargin, from, 10000, 10000, 
			    width, hscroll); 
      vpos -= pos.vpos; 
      first = 0; 
      from = prevline; 
    } 
 
  /* If we made exactly the desired vertical distance, 
     or if we hit beginning of buffer, 
     return point found */ 
  if (vpos >= vtarget) 
    { 
      pos.bufpos = from; 
      pos.vpos = vpos; 
      pos.hpos = lmargin; 
      pos.contin = 0; 
      pos.prevhpos = 0; 
      return pos; 
    } 
   
  /* Otherwise find the correct spot by moving down */ 
  goto retry; 
} 
 
DEFUN ("vertical-motion", Fvertical_motion, Svertical_motion, 1, 1, 0, 
  "Move to start of screen line LINES lines down.\n\ 
If LINES is negative, this is moving up.\n\ 
Sets dot to position found; this may be start of line\n\ 
 or just the start of a continuation line.\n\ 
Returns number of lines moved; may be closer to zero than LINES\n\ 
 if end of buffer was reached.") 
  (lines) 
     Lisp_Object lines; 
{ 
  struct position pos; 
  register struct window *w = XWINDOW (selected_window); 
 
  CHECK_NUMBER (lines, 0); 
 
  pos = vmotion (dot, XINT (lines), 
		 XFASTINT (w->width) - 1 
		 - (XFASTINT (w->width) + XFASTINT (w->left) 
		    != XFASTINT (XWINDOW (minibuf_window)->width)), 
		 /* Not XFASTINT since perhaps could be negative */ 
		 XINT (w->hscroll)); 
 
  SetDot (pos.bufpos); 
  return make_number (pos.vpos); 
} 
 
syms_of_indent () 
{ 
  DefBoolVar ("indent-tabs-mode", &indent_tabs_mode, 
    "*Indentation can insert tabs if this is non-nil."); 
  indent_tabs_mode = 1; 
 
  defsubr (&Scurrent_indentation); 
  defsubr (&Sindent_to); 
  defsubr (&Scurrent_column); 
  defsubr (&Smove_to_column); 
  defsubr (&Svertical_motion); 
} 
