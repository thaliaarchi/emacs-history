head     1.1;A
access   ; 
symbols  ; 
locks    bbanerje:1.1; strict; 
comment  @# @; 
 
 
1.1 
date     85.03.31.18.10.27;  author bbanerje;  state Exp; 
branches ; 
next     ; 
 
 
desc 
@@ 
 
 
 
1.1 
log 
@Initial revision 
@ 
text 
@# Makefile for GNU Emacs. 
# Copyright (C) 1985 Richard M. Stallman. 
 
# This file is part of GNU Emacs. 
 
# GNU Emacs is distributed in the hope that it will be useful, 
# but without any warranty.  No author or distributor 
# accepts responsibility to anyone for the consequences of using it 
# or for whether it serves any particular purpose or works at all, 
# unless he says so in writing. 
 
# Everyone is granted permission to copy, modify and redistribute 
# GNU Emacs, but only under the conditions described in the 
# document "GNU Emacs copying permission notice".   An exact copy 
# of the document is supposed to have been given to you along with 
# GNU Emacs so that you can know how you may redistribute it all. 
# It should be in a file named COPYING.  Among other things, the 
# copyright notice and this notice must be preserved on all copies. 
 
 
LIBES= 
CFLAGS= -O -Demacs 
#CFLAGS= -g -Demacs -DDisplayDebug 
# DO NOT use -R.  There is a special hack described in lastfile.c 
# which is used instead.  Some initialized data areas are modified 
# at initial startup, then labeled as part of the text area when 
# Emacs is dumped for the first time, and never changed again. 
 
# You may replace -g with -O if you don't believe in debugging. 
 
# -Demacs is needed to make some files produce the 
# correct version for use in Emacs. 
# -DDisplayDebug can be removed safely; 
# that would make one or two files a little smaller. 
 
.SUFFIXES: .v 
 
.v.o: 
	co -q $< 
	${CC} ${CFLAGS} -c $*.c 
	${RM} -f $*.c 
 
# lastfile must follow all files 
# whose initialized data areas should be dumped as pure by dump-emacs. 
obj=    display.o fixedpoint.o dispnew.o xdisp.o window.o \ 
	cm.o TrmC100.o TrmAmb.o TrmTERM.o TrmVT100.o \ 
	emacs.o dsp.o keyboard.o macros.o keymap.o \ 
	buffer.o insdel.o marker.o \ 
	minibuf.o fileio.o dired.o filemode.o userid.o \ 
	cmds.o casefiddle.o indent.o search.o regex.o undo.o \ 
	alloc.o data.o doc.o editfns.o callint.o \ 
	eval.o fns.o print.o read.o \ 
	abbrev.o syntax.o unexec.o mocklisp.o bytecode.o \ 
	process.o callproc.o \ 
	malloc.o \ 
	lastfile.o 
 
lisp=	../lisp/simple.elc ../lisp/files.elc ../lisp/window.elc \ 
	../lisp/indent.elc ../lisp/loaddefs.el \ 
	../lisp/startup.elc ../lisp/lisp.elc \ 
	../lisp/page.elc ../lisp/register.elc \ 
	../lisp/paragraphs.elc ../lisp/lisp-mode.elc \ 
	../lisp/text-mode.elc ../lisp/fill.elc \ 
	../lisp/c-mode.elc ../lisp/isearch.elc \ 
	../lisp/replace.elc ../lisp/abbrev.elc \ 
	../lisp/buff-menu.elc 
 
all: xemacs 
 
xemacs: temacs ${lisp} 
	../etc/make-docfile auxdoc.c ${obj} ${lisp} ../lisp/version.el > ../etc/DOCSTR 
	temacs -l loadup inc dump 
 
temacs: crt0.o ${obj} 
	ld -X -o temacs crt0.o ${obj} -ltermlib ${LIBES} -lc -lg 
 
TrmAmb.o: TrmAmb.c Trm.h cm.h config.h 
TrmC100.o: TrmC100.c Trm.h config.h 
TrmTERM.o: TrmTERM.c Trm.h config.h cm.h 
TrmVT100.o: TrmVT100.c Trm.h cm.h config.h 
 
cm.o: cm.c cm.h 
malloc.o: malloc.c 
 
# These are the files of Lisp proper 
 
alloc.o: alloc.c config.h buffer.h 
data.o: data.c config.h buffer.h 
doc.o: doc.c config.h 
eval.o: eval.c 
fns.o: fns.c config.h buffer.h 
print.o: print.c config.h buffer.h window.h process.h 
read.o: read.c config.h buffer.h paths.h 
 
editfns.o: editfns.c config.h buffer.h window.h 
callint.o: callint.c config.h buffer.h 
 
# These are the editor itself 
 
abbrev.o: abbrev.c config.h buffer.h syntax.h 
buffer.o: buffer.c config.h buffer.h window.h syntax.h 
callproc.o: callproc.c config.h commands.h paths.h 
casefiddle.o: casefiddle.c config.h buffer.h syntax.h 
cmds.o: cmds.c config.h buffer.h 
crt0.o: crt0.c 
dired.o: dired.c 
display.o: display.c config.h display.h fixedpoint.h Trm.h 
dispnew.o: dispnew.c config.h display.h fixedpoint.h window.h Trm.h 
dsp.o: dsp.c config.h buffer.h disp_extern.h buffer.h Trm.h 
emacs.o: emacs.c config.h buffer.h 
fileio.o: fileio.c config.h buffer.h 
fixedpoint.o: fixedpoint.c display.h fixedpoint.h 
indent.o: indent.c config.h buffer.h indent.h 
insdel.o: insdel.c config.h window.h buffer.h 
keyboard.o: keyboard.c config.h window.h buffer.h 
keymap.o: keymap.c config.h buffer.h 
macros.o: macros.c config.h buffer.h 
marker.o: marker.c config.h buffer.h 
minibuf.o: minibuf.c config.h window.h buffer.h disp_extern.h 
process.o: process.c config.h buffer.h process.h 
regex.o: regex.c regex.h syntax.h 
search.o: search.c config.h buffer.h syntax.h 
syntax.o: syntax.c config.h syntax.h buffer.h 
undo.o: undo.c config.h undo.h buffer.h 
unexec.0: unexec.c 
window.o: window.c config.h buffer.h window.h 
xdisp.o: xdisp.c config.h window.h disp_extern.h commands.h buffer.h 
 
lastfile.o: lastfile.c 
@ 
