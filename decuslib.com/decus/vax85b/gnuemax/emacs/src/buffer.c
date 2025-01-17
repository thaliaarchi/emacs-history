/* Buffer manipulation primitives for GNU Emacs. 
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
#include <sys/param.h> 
#undef NULL 
#include "config.h" 
#include "lisp.h" 
#include "window.h" 
#include "commands.h" 
#include "buffer.h" 
#include "syntax.h" 
 
Lisp_Object Fset_buffer (); 
 
int ask_about_buffer_names;	/* If nonzero, Emacs will ask 
				   instead of synthesizing a unique name in 
				   the case where visit-file encounters a 
				   conflict in generated buffer names. */ 
 
/* Alist of all buffer names vs the buffers. */ 
/* This used to be a variable, but is no longer, 
 to prevent lossage due to user rplac'ing this alist or its elements.  */ 
Lisp_Object Vbuffer_alist; 
 
/* Function to call to install major mode. 
  nil means use the major mode of the selected buffer.  */ 
 
Lisp_Object Vdefault_major_mode; 
 
Lisp_Object Qfundamental_mode; 
 
Lisp_Object QSFundamental;	/* A string "Fundamental" */ 
 
Lisp_Object Vdefault_mode_line_format; 
 
int default_case_fold_search; 
 
int default_tab_width; 
int default_ctl_arrow; 
int default_truncate_lines; 
 
int default_fill_column; 
int default_left_margin; 
 
nsberror (spec) 
     Lisp_Object spec; 
{ 
  if (XTYPE (spec) == Lisp_String) 
    error ("No buffer named %s", XSTRING (spec)->data); 
  error ("Invalid buffer argument"); 
} 
 
DEFUN ("buffer-list", Fbuffer_list, Sbuffer_list, 0, 0, 0, 
  "Return a list of all buffers.") 
  () 
{ 
  return Fmapcar (Qcdr, Vbuffer_alist); 
} 
 
DEFUN ("get-buffer", Fget_buffer, Sget_buffer, 1, 1, 0, 
  "Return the buffer named NAME (a string).\n\ 
It is found by looking up NAME in  buffer-alist.\n\ 
If there is no buffer named NAME, nil is returned.\n\ 
NAME may also be a buffer; it is returned.") 
  (name) 
     Lisp_Object name; 
{ 
  if (XTYPE (name) == Lisp_Buffer) 
    return name; 
  CHECK_STRING (name, 0); 
 
  return Fcdr (Fassoc (name, Vbuffer_alist)); 
} 
 
DEFUN ("get-file-buffer", Fget_file_buffer, Sget_file_buffer, 1, 1, 0, 
  "Return the buffer visiting file FILENAME (a string).\n\ 
If there is no such buffer, nil is returned.") 
  (filename) 
     Lisp_Object filename; 
{ 
  Lisp_Object tail, buf, tem; 
  CHECK_STRING (filename, 0); 
  filename = Fexpand_file_name (filename, Qnil); 
 
  for (tail = Vbuffer_alist; LISTP (tail); tail = XCONS (tail)->cdr) 
    { 
      buf = Fcdr (XCONS (tail)->car); 
      if (XTYPE (buf) != Lisp_Buffer) continue; 
      if (XTYPE (XBUFFER (buf)->filename) != Lisp_String) continue; 
      tem = Fstring_equal (XBUFFER (buf)->filename, filename); 
      if (!NULL (tem)) 
	return buf; 
    } 
  return Qnil; 
} 
 
/* Incremented for each buffer created, to assign the buffer number. */ 
int buffer_count; 
 
DEFUN ("get-buffer-create", Fget_buffer_create, Sget_buffer_create, 1, 1, 0, 
  "Like get-buffer but creates a buffer named NAME and returns it if none already exists.") 
  (name) 
     Lisp_Object name; 
{ 
  Lisp_Object buf, function; 
  int count = specpdl_ptr - specpdl; 
  register struct buffer *b; 
  struct buffer *bx; 
  unsigned char *data; 
 
  buf = Fget_buffer (name); 
  if (!NULL (buf)) return buf; 
 
  b = (struct buffer *) malloc (sizeof (struct buffer)); 
  if (!b) memory_full (); 
 
  data = (unsigned char *) malloc (b->text.gap = 20); 
  if (!data) memory_full (); 
  b->text.p1 = data - 1; 
  b->text.p2 = data - 1 + b->text.gap; 
  b->text.size1 = b->text.size2 = 0; 
  b->text.modified = 1; 
  b->text.dotloc = 1; 
  b->text.head_clip = 1; 
  b->text.tail_clip = 0; 
 
  b->name = name; 
  b->filename = Qnil; 
  b->modtime = 0; 
  b->save_modified = 1; 
  b->backed_up = *(int*) &Qnil; 
  b->auto_save_modified = 0; 
  b->auto_save_file_name = Qnil; 
  b->markers = Qnil; 
  b->mark = Qnil; 
  b->number = make_number (++buffer_count); 
  b->save_length = make_number (0); 
  b->last_window_start = 1; 
  if (bf_cur) 
    b->directory = bf_cur->directory; 
  else 
    b->directory = Qnil; 
  b->read_only = Qnil; 
  if (XSTRING (name)->data[0] != ' ') 
    make_undo_records (b); 
 
  reset_buffer (b); 
 
  XSETTYPE (buf, Lisp_Buffer); 
  bx = b;			/* Use of bx avoids compiler bug on Sun */ 
  XSETBUFFER (buf, bx); 
  Vbuffer_alist = nconc2 (Vbuffer_alist, Fcons (Fcons (name, buf), Qnil)); 
 
  function = Vdefault_major_mode; 
  if (NULL (function)) 
    function = bf_cur->major_mode; 
 
  if (NULL (function) || EQ (function, Qfundamental_mode)) 
    return buf; 
 
  /* To select a nonfundamental mode, 
     select the buffer temporarily and then call the mode function. */ 
 
  record_unwind_protect (save_excursion_restore, save_excursion_save ()); 
			  
  Fset_buffer (buf); 
  Fapply (function, Qnil); 
 
  unbind_to (count); 
  return buf; 
} 
 
reset_buffer (b) 
     register struct buffer *b; 
{ 
  b->keymap = Qnil; 
  b->abbrev_table = Vfundamental_mode_abbrev_table; 
  b->tab_width = make_number (default_tab_width); 
  b->fill_column = make_number (default_fill_column); 
  b->left_margin = make_number (default_left_margin); 
  b->case_fold_search = default_case_fold_search ? Qt : Qnil; 
 
  b->syntax_table_v = XVECTOR (Vstandard_syntax_table); 
  b->mode_line_format = Vdefault_mode_line_format; 
  b->auto_fill_hook = Qnil; 
  b->local_var_alist = Qnil; 
  b->ctl_arrow = default_ctl_arrow ? Qt : Qnil; 
  b->truncate_lines = default_truncate_lines ? Qt : Qnil; 
  b->selective_display = Qnil; 
 
  b->major_mode = Qfundamental_mode; 
  b->mode_name = QSFundamental; 
  b->minor_modes = Qnil; 
} 
 
DEFUN ("create-file-buffer", Fcreate_file_buffer, Screate_file_buffer, 
  1, 1, 0, 
  "Creates a suitably named buffer for visiting FILENAME, and returns it.") 
  (filename) 
     Lisp_Object filename; 
{ 
  Lisp_Object base, name, tem; 
  int count; 
  char number[10]; 
 
  CHECK_STRING (filename, 0); 
 
  base = Ffile_name_nondirectory (filename); 
 
  tem = Fget_buffer (base); 
  if (NULL (tem)) 
    return Fget_buffer_create (base); 
 
  if (INTERACTIVE && ask_about_buffer_names) 
    { 
      tem = read_minibuf_string (MinibufLocalNSMap, Qnil, format1 ( 
"Buffer name %s is in use, type a new name or <CR> to clobber: ", base)); 
      return Fget_buffer_create (XSTRING (tem)->size ? tem : base); 
    } 
 
  count = 1; 
  while (1) 
    { 
      sprintf (number, "<%d>", ++count); 
      name = concat2 (base, build_string (number)); 
      tem = Fget_buffer (name); 
      if (NULL (tem)) 
	return Fget_buffer_create (name); 
    } 
} 
 
DEFUN ("buffer-name", Fbuffer_name, Sbuffer_name, 0, 1, 0, 
  "Return the name of BUFFER, as a string.\n\ 
No arg means return name of current buffer.") 
  (buffer) 
     Lisp_Object buffer; 
{ 
  if (NULL (buffer)) 
    return bf_cur->name; 
  if (XTYPE (buffer) != Lisp_Buffer) 
    wrong_type_argument (Qbufferp, buffer, 0); 
  return XBUFFER (buffer)->name; 
} 
 
DEFUN ("buffer-number", Fbuffer_number, Sbuffer_number, 0, 1, 0, 
  "Return the number of BUFFER.\n\ 
No arg means return number of current buffer.") 
  (buffer) 
     Lisp_Object buffer; 
{ 
  if (NULL (buffer)) 
    return bf_cur->number; 
  if (XTYPE (buffer) != Lisp_Buffer) 
    wrong_type_argument (Qbufferp, buffer, 0); 
  return XBUFFER (buffer)->number; 
} 
 
DEFUN ("buffer-file-name", Fbuffer_file_name, Sbuffer_file_name, 0, 1, 0, 
  "Return name of file BUFFER is visiting, or NIL if none.\n\ 
No argument means use current buffer as BUFFER.") 
  (buffer) 
     Lisp_Object buffer; 
{ 
  if (NULL (buffer)) 
    return bf_cur->filename; 
  if (XTYPE (buffer) != Lisp_Buffer) 
    wrong_type_argument (Qbufferp, buffer, 0); 
  return XBUFFER (buffer)->filename; 
} 
 
DEFUN ("buffer-local-variables", Fbuffer_local_variables, 
  Sbuffer_local_variables, 
  0, 1, 0, 
  "Return alist of buffer-local variables of BUFFER.\n\ 
Each element looks like (SYMBOL . VALUE).\n\ 
No argument means use current buffer as BUFFER.") 
  (buffer) 
     Lisp_Object buffer; 
{ 
  if (NULL (buffer)) 
    return bf_cur->local_var_alist; 
  if (XTYPE (buffer) != Lisp_Buffer) 
    wrong_type_argument (Qbufferp, buffer, 0); 
  return XBUFFER (buffer)->local_var_alist; 
} 
 
DEFUN ("buffer-modified-p", Fbuffer_modified_p, Sbuffer_modified_p, 
  0, 1, 0, 
  "Return t if BUFFER is modified since file last read in or saved.\n\ 
No argument means use current buffer as BUFFER.") 
  (buffer) 
     Lisp_Object buffer; 
{ 
  struct buffer *buf; 
  if (NULL (buffer)) 
    buf = bf_cur; 
  else if (XTYPE (buffer) == Lisp_Buffer) 
    buf = XBUFFER (buffer); 
  else 
    wrong_type_argument (Qbufferp, buffer, 0); 
 
  bf_cur->text.modified = bf_modified; 
  return buf->save_modified < buf->text.modified ? Qt : Qnil; 
} 
 
DEFUN ("set-buffer-modified-p", Fset_buffer_modified_p, Sset_buffer_modified_p, 
  1, 1, 0, 
  "Mark current buffer as modified or unmodified according to FLAG.") 
  (flag) 
     Lisp_Object flag; 
{ 
  bf_cur->save_modified = NULL (flag) ? bf_modified : 0; 
  RedoModes++; 
  return flag; 
} 
 
/* Return number of modified buffers that exist now. */ 
 
int 
ModExist () 
{ 
  Lisp_Object tail, buf; 
  struct buffer *b; 
  int modcount = 0; 
 
  bf_cur->text.modified = bf_modified; 
 
  for (tail = Vbuffer_alist; !NULL (tail); tail = Fcdr (tail)) 
    { 
      buf = Fcdr (Fcar (tail)); 
      b = XBUFFER (buf); 
      if (!NULL (b->filename) && b->save_modified < b->text.modified) 
	modcount++; 
    } 
 
  return modcount; 
} 
 
DEFUN ("rename-buffer", Frename_buffer, Srename_buffer, 1, 1, 
  "sRename buffer (to new name): ", 
  "Change current buffer's name to NEWNAME (a string).") 
  (name) 
     Lisp_Object name; 
{ 
  Lisp_Object tem, buf; 
 
  CHECK_STRING (name, 0); 
  tem = Fget_buffer (name); 
  if (!NULL (tem)) 
    error("Buffer \"%s\" already exists", XSTRING (name)->data); 
 
  bf_cur->name = name; 
  XSETTYPE (buf, Lisp_Buffer), XSETBUFFER (buf, bf_cur); 
  return Fsetcar (Frassq (buf, Vbuffer_alist), name); 
} 
 
DEFUN ("other-buffer", Fother_buffer, Sother_buffer, 0, 1, 0, 
  "Return an interesting buffer other than BUFFER.\n\ 
If BUFFER is omitted or nil, some interesting buffer is returned.") 
  (buffer) 
     Lisp_Object buffer; 
{ 
  Lisp_Object tail, buf; 
 
  for (tail = Vbuffer_alist; !NULL (tail); tail = Fcdr (tail)) 
    { 
      buf = Fcdr (Fcar (tail)); 
      if (EQ (buf, buffer)) 
	continue; 
      if (XSTRING (XBUFFER (buf)->name)->data[0] != ' ') 
	return buf; 
    } 
  return Fget_buffer_create (build_string ("*scratch*")); 
} 
 
DEFUN ("buffer-flush-undo", Fbuffer_flush_undo, Sbuffer_flush_undo, 1, 1, 0, 
  "Make BUFFER stop keeping undo information.") 
  (buf) 
     Lisp_Object buf; 
{ 
  CHECK_BUFFER (buf, 0); 
  if (XBUFFER (buf)->undodata) 
    free_undo_records (XBUFFER (buf)); 
  XBUFFER (buf)->undodata = 0; 
  return Qnil; 
} 
 
Lisp_Object 
Fdelete_buffer_internal (buf) 
     Lisp_Object buf; 
{ 
  register struct buffer *b = XBUFFER (buf); 
 
  if (NULL (b->name)) 
    return Qnil; 
 
#ifdef subprocesses 
  kill_buffer_processes (buf); 
#endif subprocesses 
 
  free (b->text.p1 + 1); 
  if (b->undodata) 
    free_undo_records (b); 
 
  b->name = Qnil; 
  Vbuffer_alist = Fdelq (Frassq (buf, Vbuffer_alist), Vbuffer_alist); 
 
  Freplace_buffer_in_windows (buf); 
 
  return Qnil; 
} 
 
DEFUN ("kill-buffer", Fkill_buffer, Skill_buffer, 1, 1, "bKill buffer: ", 
  "One arg, a string or a buffer.  Get rid of the specified buffer.") 
  (bufname) 
     Lisp_Object bufname; 
{ 
  Lisp_Object buf, answer; 
  register char *reply; 
 
  if (NULL (bufname)) 
    buf = Fcurrent_buffer (); 
  else 
    buf = Fget_buffer (bufname); 
  if (NULL (buf)) 
    nsberror (bufname); 
 
  bf_cur->text.modified = bf_modified; 
 
  if (INTERACTIVE && !NULL (XBUFFER (buf)->filename) 
      && XBUFFER (buf)->text.modified > XBUFFER (buf)->save_modified) 
    { 
      answer = Fyes_or_no_p (format1 ("Buffer %s modified; kill anyway? ", 
				      XSTRING (bufname)->data)); 
      if (NULL (answer)) 
	return Qnil; 
    } 
  Fdelete_buffer_internal (buf); 
  return Qnil; 
} 
 
/* Put the element for buffer `buf' at the front of buffer-alist. 
 This is done when a buffer is selected "visibly". 
 It keeps buffer-alist in the order of recency of selection 
 so that other_buffer will return something nice.  */ 
 
record_buffer (buf) 
     Lisp_Object buf; 
{ 
  Lisp_Object aelt, link; 
  aelt = Frassq (buf, Vbuffer_alist); 
  link = Fmemq (aelt, Vbuffer_alist); 
  XCONS(link)->cdr = Fdelq (aelt, Vbuffer_alist); 
  Vbuffer_alist = link; 
} 
 
DEFUN ("switch-to-buffer", Fswitch_to_buffer, Sswitch_to_buffer, 1, 1, "BSwitch to buffer: ", 
  "One arg, a string or buffer.  Select the specified buffer in the current window.") 
  (bufname) 
     Lisp_Object bufname; 
{ 
  Lisp_Object buf; 
  if (NULL (bufname)) 
    buf = Fother_buffer (Fcurrent_buffer ()); 
  else 
    buf = Fget_buffer_create (bufname); 
  Fset_buffer (buf); 
  record_buffer (buf); 
 
  Fshow_buffer (EQ (selected_window, minibuf_window) 
		  ? Fnext_window (minibuf_window) : selected_window, 
		buf); 
 
  return Qnil; 
} 
 
DEFUN ("pop-to-buffer", Fpop_to_buffer, Spop_to_buffer, 1, 1, "BPop to buffer: ", 
  "One arg, a string or window.  Select the specified buffer, popping up a window.") 
  (bufname) 
     Lisp_Object bufname; 
{ 
  Lisp_Object buf; 
  if (NULL (bufname)) 
    buf = Fother_buffer (Fcurrent_buffer ()); 
  else 
    buf = Fget_buffer_create (bufname); 
  Fset_buffer (buf); 
  record_buffer (buf); 
  Fselect_window (Fdisplay_buffer (buf)); 
  return Qnil; 
} 
 
DEFUN ("current-buffer", Fcurrent_buffer, Scurrent_buffer, 0, 0, 0, 
  "Return the current buffer as a Lisp buffer object.") 
  () 
{ 
  Lisp_Object buf; 
  XSETTYPE (buf, Lisp_Buffer), XSETBUFFER (buf, bf_cur); 
  return buf; 
} 
 
DEFUN ("set-buffer", Fset_buffer, Sset_buffer, 1, 1, 0, 
  "Set the current buffer to the buffer or buffer name supplied as argument.\n\ 
This function will not accomplish buffer switching interactively.\n\ 
This is because returning to command level always selects\n\ 
the buffer tied to the current window,\n\ 
and this function has no effect on what buffer that is.\n\ 
Use switch-to-buffer or pop-to-buffer for interactive buffer selection.") 
  (bufname) 
     Lisp_Object bufname; 
{ 
  Lisp_Object buffer; 
  buffer = Fget_buffer (bufname); 
  if (NULL (buffer)) 
    nsberror (bufname); 
  SetBfp (XBUFFER (buffer)); 
  return buffer; 
} 
 
extern int last_known_column_dot; 
 
/* set the current buffer to p */ 
SetBfp (p) 
     register struct buffer *p; 
{ 
  register struct buffer *c = bf_cur; 
  register struct window *w = XWINDOW (selected_window); 
  register struct buffer *swb; 
  Lisp_Object tail, valcontents; 
  enum Lisp_Type tem; 
 
  if (c == p) 
    return; 
 
  if (w) 
    swb = NULL (selected_window) ? 0 : XBUFFER (w->buffer); 
 
  if (p && NULL (p->name)) 
    error ("Selecting deleted buffer"); 
  windows_or_buffers_changed = 1; 
 
  if (c) 
    { 
      if (c == swb) 
	Fset_marker (w->dotm, make_number (dot), w->buffer); 
 
      if (dot < FirstCharacter || dot > NumCharacters + 1) 
	abort (); 
 
      c->text = bf_text; 
    } 
  bf_cur = p; 
  bf_text = p->text; 
  if (p == swb) 
    { 
      SetDot (marker_position (w->dotm)); 
      if (dot < FirstCharacter) 
	dot = FirstCharacter; 
      if (dot > NumCharacters + 1) 
	dot = NumCharacters + 1; 
    } 
  last_known_column_dot = -1;   /* invalidate indentation cache */ 
 
  /* Look down buffer's list of local Lisp variables 
     to find and update any that forward into C variables. */ 
 
  for (tail = p->local_var_alist; !NULL (tail); tail = XCONS (tail)->cdr) 
    { 
      valcontents = XSYMBOL (XCONS (XCONS (tail)->car)->car)->value; 
      if ((XTYPE (valcontents) == Lisp_Buffer_Local_Value 
	   || XTYPE (valcontents) == Lisp_Some_Buffer_Local_Value) 
	  && (tem = XTYPE (XCONS (valcontents)->car), 
	      (tem == Lisp_Boolfwd || tem == Lisp_Intfwd 
	       || tem == Lisp_Objfwd))) 
	/* Just reference the variable 
	     to cause it to become set for this buffer.  */ 
	Fsymbol_value (XCONS (XCONS (tail)->car)->car); 
    } 
 
  /* Do the same with any others that were local to the previous buffer */ 
 
  if (c) 
    for (tail = c->local_var_alist; !NULL (tail); tail = XCONS (tail)->cdr) 
      { 
	valcontents = XSYMBOL (XCONS (XCONS (tail)->car)->car)->value; 
	if ((XTYPE (valcontents) == Lisp_Buffer_Local_Value 
	     || XTYPE (valcontents) == Lisp_Some_Buffer_Local_Value) 
	    && (tem = XTYPE (XCONS (valcontents)->car), 
		(tem == Lisp_Boolfwd || tem == Lisp_Intfwd 
		 || tem == Lisp_Objfwd))) 
	  /* Just reference the variable 
               to cause it to become set for this buffer.  */ 
	  Fsymbol_value (XCONS (XCONS (tail)->car)->car); 
      } 
} 
 
/* set the current buffer to p "just for redisplay" */ 
SetBfx (p) 
     register struct buffer *p; 
{ 
  if (bf_cur == p) 
    return; 
 
  bf_cur->text = bf_text; 
  bf_cur = p; 
  bf_text = p->text; 
} 
 
DEFUN ("erase-buffer", Ferase_buffer, Serase_buffer, 0, 0, 0, 
  "Delete the entire contents of the current buffer.") 
  () 
{ 
  int odot = dot; 
 
  Fwiden (); 
  del_range (1, NumCharacters + 1); 
  if (dot != 1)			/* Should be impossible, but appears to happen */ 
    abort (); 
  bf_cur->last_window_start = 1; 
  return Qnil; 
} 
 
validate_region (b, e) 
     Lisp_Object *b, *e; 
{ 
  int i; 
 
  CHECK_NUMBER_COERCE_MARKER (*b, 0); 
  CHECK_NUMBER_COERCE_MARKER (*e, 1); 
 
  if (XINT (*b) > XINT (*e)) 
    i = XINT (*b), XINT (*b) = XINT (*e), XINT (*e) = i; 
 
  if (!(FirstCharacter <= XINT (*b) && XINT (*b) <= XINT (*e) 
        && XINT (*e) <= 1 + NumCharacters)) 
    Fsignal (Qargs_out_of_range, Fcons (*b, Fcons (*e, Qnil))); 
} 
 
list_buffers_1 (files) 
     Lisp_Object files; 
{ 
  Lisp_Object tail, buf, col1, col2, col3, minspace, tem, mode; 
  register struct buffer *old = bf_cur, *b; 
  int desired_dot = 0; 
 
  bf_cur->text.modified = bf_modified; 
 
  XFASTINT (col1) = 18; 
  XFASTINT (col2) = 24; 
  XFASTINT (col3) = 39; 
  XFASTINT (minspace) = 1; 
 
  SetBfp (XBUFFER (Vstandard_output)); 
 
  mode = intern ("Buffer-menu-mode"); 
  if (!EQ (mode, bf_cur->major_mode) 
      && (tem = Ffboundp (mode), !NULL (tem))) 
    Fapply (mode, Qnil); 
  Fbuffer_flush_undo (Vstandard_output); 
 
  write_string ("\ 
 M Buffer         Size  Mode           File\n\ 
 - ------         ----  ----           ----\n", -1); 
 
  for (tail = Vbuffer_alist; !NULL (tail); tail = Fcdr (tail)) 
    { 
      buf = Fcdr (Fcar (tail)); 
      b = XBUFFER (buf); 
      /* Don't mention the minibuffers. */ 
      if (XSTRING (b->name)->data[0] == ' ') 
	continue; 
      /* Optionally don't mention buffers that lack files. */ 
      if (!NULL (files) && NULL (b->filename)) 
	continue; 
      /* Identify the current buffer. */ 
      if (b == old) 
	desired_dot = dot; 
      write_string (b == old ? "." : " ", -1); 
      /* Identify modified buffers */ 
      write_string (b->text.modified > b->save_modified ? "* " : "  ", -1); 
      Fprinc (b->name, Qnil); 
      Findent_to (col1, minspace); 
      XFASTINT (tem) = b->text.size1 + b->text.size2; 
      Fprin1 (tem, Qnil); 
      Findent_to (col2, minspace); 
      Fprinc (b->mode_name, Qnil); 
      Findent_to (col3, minspace); 
      if (!NULL (b->filename)) 
	Fprinc (b->filename, Qnil); 
      write_string ("\n", -1); 
    } 
 
  SetBfp (old); 
  if (desired_dot) 
    XBUFFER (Vstandard_output)->text.dotloc = desired_dot; 
} 
 
DEFUN ("list-buffers", Flist_buffers, Slist_buffers, 0, 1, "", 
  "Display a list of names of existing buffers.\n\ 
Inserts it in buffer *Buffer List* and displays that.\n\ 
Note that buffers with names starting with spaces are omitted.\n\ 
Non-null optional arg FILES-ONLY means mention only file buffers.") 
  (files) 
     Lisp_Object files; 
{ 
  internal_with_output_to_temp_buffer ("*Buffer List*", 
				       list_buffers_1, files); 
  return Qnil; 
} 
 
DEFUN ("kill-all-local-variables", Fkill_all_local_variables, Skill_all_local_variables, 
  0, 0, 0, 
  "Eliminate all the buffer-local variable values of the current buffer.\n\ 
This buffer will then see the default values of all variables.") 
  () 
{ 
  Lisp_Object alist, sym, tem; 
 
  for (alist = bf_cur->local_var_alist; !NULL (alist); alist = XCONS (alist)->cdr) 
    { 
      sym = XCONS (XCONS (alist)->car)->car; 
 
      /* Put the symbol into a consistent state, 
	 set up for access in the current buffer with the default value */ 
 
      tem = XCONS (XCONS (XSYMBOL (sym)->value)->cdr)->cdr; 
      XCONS (tem)->car = tem; 
      XCONS (XCONS (XSYMBOL (sym)->value)->cdr)->car = Fcurrent_buffer (); 
      store_symval_forwarding (sym, XCONS (XSYMBOL (sym)->value)->car, XCONS (tem)->cdr); 
    } 
 
  reset_buffer (bf_cur); 
  return Qnil; 
} 
 
init_buffer_once () 
{ 
  Lisp_Object tem; 
 
  /* Must do these before making the first buffer! */ 
 
  Vdefault_mode_line_format 
    = build_string ("--%1*%1*-Emacs: %17b   %M   %[(%m)%]----%3p-%-"); 
  default_case_fold_search = 1; 
 
  QSFundamental = build_string ("Fundamental"); 
 
  default_tab_width = 8; 
  default_truncate_lines = 0; 
  default_ctl_arrow = 1; 
 
  default_fill_column = 70; 
  default_left_margin = 0; 
 
  Vbuffer_alist = Qnil; 
  bf_cur = 0; 
  Qfundamental_mode = intern ("fundamental-mode"); 
  Vdefault_major_mode = Qfundamental_mode; 
  Fset_buffer (Fget_buffer_create (build_string ("*scratch*"))); 
} 
 
init_buffer () 
{ 
  char buf[MAXPATHLEN]; 
 
  Fset_buffer (Fget_buffer_create (build_string ("*scratch*"))); 
  getwd(buf); 
  strcat (buf, "/"); 
  bf_cur->directory = build_string (buf); 
} 
 
/* initialize the buffer routines */ 
syms_of_buffer () 
{ 
  staticpro (&Qfundamental_mode); 
  staticpro (&QSFundamental); 
  staticpro (&Vbuffer_alist); 
 
  DefLispVar ("default-mode-line-format", &Vdefault_mode_line_format, 
    "Default value of mode-line-format for new buffers."); 
 
  DefBufferLispVar ("mode-line-format", &bf_cur->mode_line_format, 
    "Template string for displaying mode line for current buffer.\n\ 
Each buffer has its own value of this variable.\n\ 
The string is printed verbatim in the mode line\n\ 
except for %-constructs:\n\ 
  %b -- print buffer name.   %f -- print visited file name.\n\ 
  %* -- print *, % or space.   %m -- print value of mode-name.\n\ 
  %s -- print process status.   %M -- print value of global-mode-string.\n\ 
  %p -- print percent of buffer above top of window, or top, bot or all.\n\ 
  %[ -- print one [ for each recursive editing level.  %] similar.\n\ 
  %% -- print %.   %- -- print infinitely many dashes.\n\ 
Decimal digits after the % specify field width to pad or truncate to."); 
 
  DefBoolVar ("default-case-fold-search", &default_case_fold_search, 
    "*Default value of case-fold-search for new buffers."); 
  DefBufferLispVar ("case-fold-search", &bf_cur->case_fold_search, 
    "*Non-nil if searches should ignore case.\n\ 
Separate value in each buffer."); 
 
  DefBufferLispVar ("mode-name", &bf_cur->mode_name, 
    "Pretty name of current buffer's major mode (a string)."); 
 
  DefBufferLispVar ("minor-modes", &bf_cur->minor_modes, 
    "List of minor modes enabled in current buffer.\n\ 
Each element is (FUNCTION-SYMBOL . PRETTY-STRING)."); 
 
  DefIntVar ("default-fill-column", &default_fill_column, 
    "*Default value of fill-column for new buffers."); 
  DefBufferLispVar ("fill-column", &bf_cur->fill_column, 
    "*Column beyone which automatic line-wrapping should happen.\n\ 
Separate value in each buffer."); 
 
  DefIntVar ("default-left-margin", &default_left_margin, 
    "*Default value of left-margin for buffers that don't override it."); 
  DefBufferLispVar ("left-margin", &bf_cur->left_margin, 
    "*I don't know what this does."); 
 
  DefIntVar ("default-tab-width", &default_tab_width, 
    "*Default value of tab-width for new buffers."); 
  DefBufferLispVar ("tab-width", &bf_cur->tab_width, 
    "*Distance between tab stops (for display of tab characters), in columns.\n\ 
Separate value in each buffer."); 
 
  DefIntVar ("default-ctl-arrow", &default_ctl_arrow, 
    "*Default value of ctl-arrow for new buffers."); 
  DefBufferLispVar ("ctl-arrow", &bf_cur->ctl_arrow, 
    "*Non-nil means display control chars with uparrow.\n\ 
Nil means use backslash and octal digits.\n\ 
Separate value in each buffer."); 
 
  DefIntVar ("default-truncate-lines", &default_truncate_lines, 
    "*Default value of truncate-lines for new buffers."); 
  DefBufferLispVar ("truncate-lines", &bf_cur->truncate_lines, 
    "*Non-nil means do not display continuation lines;\n\ 
give each line of text one screen line.\n\ 
Separate value in each buffer."); 
 
  DefBufferLispVar ("default-directory", &bf_cur->directory, 
    "*Name of default directory of current buffer.  Should end with slash."); 
 
  DefBufferLispVar ("auto-fill-hook", &bf_cur->auto_fill_hook, 
    "Function called (if non-nil) after self-inserting a space at column beyond fill-column"); 
 
  DefBufferLispVar ("buffer-file-name", &bf_cur->filename, 
    "Name of file visited in current buffer, or nil if not visiting a file."); 
 
  DefBufferLispVar ("buffer-auto-save-file-name", 
		    &bf_cur->auto_save_file_name, 
    "Name of file for auto-saving current buffer,\n\ 
or nil if buffer should not be auto-saved."); 
 
  DefBufferLispVar ("buffer-read-only", &bf_cur->read_only, 
    "*Non-nil if this buffer is read-only\n\ 
\(save only on explicit request, with query)."); 
 
  DefBufferLispVar ("buffer-backed-up", &bf_cur->backed_up, 
    "Non-nil if this buffer's file has been backed up.\n\ 
Backing up is done before the first time the file is saved."); 
 
  DefBufferLispVar ("buffer-saved-size", &bf_cur->save_length, 
    "Length of current buffer when last read in, saved or auto-saved.\n\ 
0 initially."); 
  DefBufferLispVar ("buffer-number", &bf_cur->number, 
    "Number of current buffer, in order of creation."); 
 
  DefBufferLispVar ("selective-display", &bf_cur->selective_display, 
    "t enables selective display:\n\ 
 after a ^M, all the rest of the line is invisible.\n\ 
 ^M's in the file are written into files as newlines.\n\ 
Integer n as value means display only lines\n\ 
 that start with less than n columns of space."); 
 
  DefBoolVar ("ask-about-buffer-names", &ask_about_buffer_names, 
    "*Non-nil means ask user for buffer name when there is a conflict.\n\ 
The default is to generate a unique name with no interaction."); 
 
  DefLispVar ("default-major-mode", &Vdefault_major_mode, 
    "*Major mode for new buffers.  Defaults to fundamental-mode.\n\ 
nil here means use current buffer's major mode."); 
 
  DefBufferLispVar ("major-mode", &bf_cur->major_mode, 
    "Symbol for buffer's major mode."); 
 
  defsubr (&Sbuffer_list); 
  defsubr (&Sget_buffer); 
  defsubr (&Sget_file_buffer); 
  defsubr (&Sget_buffer_create); 
  defsubr (&Screate_file_buffer); 
  defsubr (&Sbuffer_name); 
  defsubr (&Sbuffer_number); 
  defsubr (&Sbuffer_file_name); 
  defsubr (&Sbuffer_local_variables); 
  defsubr (&Sbuffer_modified_p); 
  defsubr (&Sset_buffer_modified_p); 
  defsubr (&Srename_buffer); 
  defsubr (&Sother_buffer); 
  defsubr (&Sbuffer_flush_undo); 
  defsubr (&Skill_buffer); 
  defsubr (&Serase_buffer); 
  defsubr (&Sswitch_to_buffer); 
  defsubr (&Spop_to_buffer); 
  defsubr (&Scurrent_buffer); 
  defsubr (&Sset_buffer); 
  defsubr (&Slist_buffers); 
  defsubr (&Skill_all_local_variables); 
} 
 
keys_of_buffer () 
{ 
  defkey (CtlXmap, 'b', "switch-to-buffer"); 
  defkey (CtlXmap, 'k', "kill-buffer"); 
  defkey (CtlXmap, Ctl ('B'), "list-buffers"); 
} 
