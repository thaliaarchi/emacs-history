/* Lisp functions for making directory listings. 
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
#include <sys/types.h> 
#include <sys/time.h> 
#include <sys/stat.h> 
#include <sys/dir.h> 
#include <stdio.h> 
#undef NULL 
#include "config.h" 
#include "lisp.h" 
#include "buffer.h" 
#include "commands.h" 
 
#define min(a, b) ((a) < (b) ? (a) : (b)) 
 
extern Lisp_Object concat2 (); 
 
Lisp_Object Vcompletion_ignored_extensions; 
 
DEFUN ("directory-files", Fdirectory_files, Sdirectory_files, 1, 2, 0, 
  "Return a list of names of files in DIRECTORY.\n\ 
If FULL is non-NIL, absolute pathnames of the files are returned.") 
  (dirname, full) 
     Lisp_Object dirname, full; 
{ 
  DIR *d; 
  struct direct *dp; 
  char filename[MAXNAMLEN+2]; 
  Lisp_Object list, name; 
 
  dirname = Fexpand_file_name (dirname, Qnil); 
  if (!(d = opendir (XSTRING (dirname)->data))) 
    report_file_error ("Opening directory", Fcons (dirname, Qnil)); 
 
  list = Qnil; 
  filename[0] = '/'; 
  /* Loop reading blocks */ 
  while (1) 
    { 
      dp = readdir (d); 
      if (!dp) break; 
      if (dp->d_ino) 
	{ 
	  strncpy (filename+1, dp->d_name, dp->d_namlen); 
	  filename[dp->d_namlen+1] = 0; 
	  if (!NULL (full)) 
            name = concat2 (dirname, build_string (filename)); 
	  else 
	    name = build_string (filename + 1); 
	  list = Fcons (name, list); 
	} 
    } 
  closedir (d); 
  return Fsort (Fnreverse (list), Qstring_lessp); 
} 
 
DEFUN ("file-name-completion", Ffile_name_completion, Sfile_name_completion, 
  2, 2, 0, 
  "Complete file name FILE in directory DIR.\n\ 
Returns the longest string common to all filenames in DIR\n\ 
that start with FILE.\n\ 
If there is only one and FILE matches it exactly, returns t.\n\ 
Returns nil if DIR contains no name starting with FILE.") 
  (file, dirname) 
     Lisp_Object file, dirname; 
{ 
  DIR *d; 
  struct direct *dp; 
  int bestmatchsize, skip; 
  register int compare, matchsize; 
  unsigned char *p1, *p2; 
  int matchcount = 0; 
  Lisp_Object bestmatch, tem, elt; 
 
  dirname = Fexpand_file_name (dirname, Qnil); 
 
  if (!(d = opendir (XSTRING (dirname)->data))) 
    report_file_error ("Opening directory", Fcons (dirname, Qnil)); 
 
  bestmatch = Qnil; 
  /* Loop reading blocks */ 
  while (dp = readdir (d)) 
    { 
      if (!dp->d_ino || 
	  dp->d_namlen < XSTRING (file)->size || 
	  bcmp (dp->d_name, XSTRING (file)->data, XSTRING (file)->size)) 
	continue; 
 
      /* Compare each extension-to-be-ignored against end of this file name */ 
      /* and exit this for loop if a match is found */ 
      for (tem = Vcompletion_ignored_extensions; 
	   LISTP (tem); tem = XCONS (tem)->cdr) 
	{ 
	  elt = XCONS (tem)->car; 
	  if (XTYPE (elt) != Lisp_String) continue; 
	  skip = dp->d_namlen - XSTRING (elt)->size; 
	  if (skip < XSTRING (file)->size) continue; 
	   
	  if (bcmp (dp->d_name + skip, 
		    XSTRING (elt)->data, 
		    XSTRING (elt)->size)) 
	    continue; 
	  break; 
	} 
 
      /* Unless a match was found, process this name as a completion */ 
      if (NULL (tem)) 
	{ 
	  /* Update computation of how much all possible completions match */ 
 
	  matchcount++; 
	  if (NULL (bestmatch)) 
	    { 
	      bestmatch = make_string (dp->d_name, dp->d_namlen); 
	      bestmatchsize = dp->d_namlen; 
	    } 
	  else 
	    { 
	      compare = min (bestmatchsize, dp->d_namlen); 
	      p1 = XSTRING (bestmatch)->data; 
	      p2 = (unsigned char *) dp->d_name; 
	      for (matchsize = 0; matchsize < compare; matchsize++) 
		if (p1[matchsize] != p2[matchsize]) break; 
	      bestmatchsize = min (matchsize, bestmatchsize); 
	    } 
	} 
    } 
  closedir (d); 
 
  if (NULL (bestmatch)) 
    return Qnil;			     /* No completions found */ 
  if (matchcount == 1 && bestmatchsize == XSTRING (file)->size) 
    return Qt; 
  return Fsubstring (bestmatch, make_number (0), make_number (bestmatchsize)); 
} 
 
DEFUN ("file-name-all-completions", Ffile_name_all_completions, 
  Sfile_name_all_completions, 2, 2, 0, 
  "Return a list of all completions of file name FILE in directory DIR.") 
  (file, dirname) 
     Lisp_Object file, dirname; 
{ 
  DIR *d; 
  struct direct *dp; 
  Lisp_Object name, list; 
 
  dirname = Fexpand_file_name (dirname, Qnil); 
  if (!(d = opendir (XSTRING (dirname)->data))) 
    report_file_error ("Opening directory", Fcons (dirname, Qnil)); 
 
  list = Qnil; 
  /* Loop reading blocks */ 
  while (dp = readdir (d)) 
    { 
      if (dp->d_ino && 
	  dp->d_namlen >= XSTRING (file)->size && 
	  !bcmp (dp->d_name, XSTRING (file)->data, XSTRING (file)->size)) 
	{ 
	  /* This is a possible completion */ 
	  name = make_string (dp->d_name, dp->d_namlen); 
	  list = Fcons (name, list); 
	} 
    } 
  closedir (d); 
  return Fsort (Fnreverse (list), Qstring_lessp); 
} 
 
Lisp_Object 
make_time (time) 
     int time; 
{ 
  return Fcons (make_number (time >> 16), 
		Fcons (make_number (time & 0177777), Qnil)); 
} 
 
DEFUN ("file-attributes", Ffile_attributes, Sfile_attributes, 1, 1, 0, 
  "Return a list of attributes of file FILENAME.\n\ 
Elements are:\n\ 
 0. t for directory, string (name linked to) for symbolic link, or nil.\n\ 
 1. Number of links to file.\n\ 
 2. File uid.\n\ 
 3. File gid.\n\ 
 4. Last access time, as a list of two integers.\n\ 
  First integer has high-order 16 bits of time, second has low 16 bits.\n\ 
 5. Last modification time, likewise.\n\ 
 6. Creation time, likewise.\n\ 
 7. Size in bytes.\n\ 
 8. File modes, as a string of nine letters or dashes as in ls -l.") 
  (filename) 
     Lisp_Object filename; 
{ 
  Lisp_Object values[9]; 
  struct stat s; 
  char modes[10]; 
 
  filename = Fexpand_file_name (filename, Qnil); 
  if (lstat(XSTRING (filename)->data, &s)<0) 
    return Qnil; 
 
  values[0] = ((s.st_mode & S_IFMT)==S_IFDIR) ? Qt : Qnil; 
  if ((s.st_mode & S_IFMT) == S_IFLNK) 
    values[0] = Ffile_symlink_p (filename); 
  XFASTINT (values[1]) = s.st_nlink; 
  XFASTINT (values[2]) = s.st_uid; 
  XFASTINT (values[3]) = s.st_gid; 
  values[4] = make_time(s.st_atime); 
  values[5] = make_time(s.st_mtime); 
  values[6] = make_time(s.st_ctime); 
  XFASTINT (values[7]) = s.st_size; 
  filemodestring (&s, modes); 
  values[8] = make_string (modes, 10); 
  return Flist (9, values); 
} 
 
syms_of_dired () 
{ 
  defsubr (&Sdirectory_files); 
  defsubr (&Sfile_name_completion); 
  defsubr (&Sfile_name_all_completions); 
  defsubr (&Sfile_attributes); 
 
  DefLispVar ("completion-ignored-extensions", &Vcompletion_ignored_extensions, 
    "*Completion ignores filenames ending in any string in this list."); 
  Vcompletion_ignored_extensions = Qnil; 
} 
