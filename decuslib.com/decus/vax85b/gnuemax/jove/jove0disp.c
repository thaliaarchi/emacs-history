Y
/* Jonathan Payne at Lincoln-Sudbury Regional High School 5-25-83 
 
   jove_disp.c 
 
   This code figures out the best way to update the screen. 
   It contains the procedure "redisplay()" that should be called 
   whenever the screen needs updating.  This optimizes interline 
   movement and intraline movement, taking advantage of insert/delete 
   line/character features of the terminal (if they exist).  */ 
 
#include "jove.h" 
#include "termcap.h" 
 
#include <signal.h> 
 
#define	TOPGONE	01 
#define	CURGONE	02	/* Topline (curline) of window has been deleted 
			   since the last time a redisplay was called. */ 
 
extern char	mesgbuf[]; 
 
/* Kludge windows gets called by the routines that delete lines from the 
   buffer.  If the w->w_line or w->w_top are deleted and this procedure 
   is not called, the redisplay routine will barf. */ 
 
KludgeWindows(line1, line2) 
Line	*line1; 
register Line	*line2; 
{ 
	register Window	*w = fwind; 
	register Line	*lp; 
 
	do { 
		for (lp = line1->l_next; lp != line2->l_next; lp = lp->l_next) { 
			if (lp == w->w_top) 
				w->w_flags |= TOPGONE; 
			if (lp == w->w_line) 
				w->w_flags |= CURGONE; 
		} 
		w = w->w_next; 
	} while (w != fwind); 
} 
 
int	UpdWCalls,	/* Number of times we called UpdWindow for this window */ 
	IDstart,	/* First different line */ 
	NumDirty;	/* Number of dirty lines in this screen update */ 
 
extern int	RingBell; 
 
 
/* The redisplay algorithm: 
 
   Jove remembers where each buffer lines is on the screen in the array 
   `oimage' (old image).  UpdWindow() makes a new image, in `nimage', 
   by started from w->w_top and working to the bottom line of the window. 
   A line by line comparison starts from nimage[0] and oimage[0], and if 
   there are no differences, no insert/delete lines is done.  When there 
   is a difference, there are two possibilities: 
 
	Some lines were deleted in the buffer.  This is detected by looking 
     further down in the old image, for the line where the difference occurred 
     in the new image.  So where we used to have 1-2-3-4 (oimage), we now have 
     1-4, in which case two lines were deleted. 
 
        Some lines were inserted in the buffer.  This is detected by looking 
     further down in the new image, for the line where the difference occurred 
     in the old image.  So where we used to have 1-2-3, we now have 1-2-4-5-3, 
     in which case two lines were inserted (lines 4 and 5). 
 
   UpdWindow has a few optimizations in it, e.g. it checks for 
   mismatches AS it builds `nimage', and sets a variable to the line number 
   at which the difference occurred.  It also keeps a count of the number 
   of lines that need updating i.e. the line was changed by an editing 
   operation, or the start print column is different (the line just scrolled 
   left or right).  Imagine that a single character was inserted on the top 
   line of the screen.  The number of lines that are dirty = 1, so the loop 
   that checks to see that all the lines are up-to-date can terminate after 
   updating the first line WITHOUT checking the rest of the lines. */ 
 
redisplay() 
{ 
	register Window	*w = fwind; 
	int	lineno, 
		i; 
	register struct scrimage	*np; 
 
	if (curwind->w_line != curline) 
		curwind->w_offset = 0; 
	curwind->w_line = curwind->w_bufp->b_dot; 
	curwind->w_char = curwind->w_bufp->b_char; 
 
	if (InputPending = charp()) 
		return; 
 
	if (RingBell) { 
		if (VisBell && VB) 
			putstr(VB); 
		else 
			outchar('\007'); 
		RingBell = 0; 
	} 
 
	if (UpdMesg) 
		UpdateMesg(); 
 
	NumDirty = 0; 
	IDstart = -1; 
 
	for (lineno = 0, w = fwind; lineno < LI - 1; w = w->w_next) { 
		UpdWCalls = 0; 
		UpdWindow(w, lineno); 
		lineno += w->w_height; 
	} 
 
	if (IDstart != -1) {		/* Shucks this is gonna be slower */ 
		DoIDline(IDstart); 
		NumDirty = LI - 1; 
	} 
 
	np = nimage; 
 
	for (i = 0; i < LI - 1 && NumDirty > 0; i++, np++) { 
		if (np->Sflags & DIRTY) { 
			UpdLine(np->Window, i); 
			NumDirty--; 
		} 
		if (InputPending) 
			return; 
	} 
	if (InputPending) 
		return; 
	UpdModLine = 0; 
 
	if (Asking) { 
		Placur(LI - 1, calc_pos(mesgbuf, Asking)); 
			/* Nice kludge */ 
		flusho(); 
	} else 
		GotoDot(); 
} 
 
int	UpdModLine = 0, 
	UpdMesg = 0; 
 
int	CanScroll = 0; 
 
DoIDline(start) 
{ 
	register struct scrimage	*np = &nimage[start], 
					*op = &oimage[start]; 
	register int	i; 
	int	j; 
 
	/* Some changes have been made.  Try for insert or delete lines. 
	   If either case has happened, Addlines and/or DelLines will do 
	   necessary scrolling, also CONVERTING oimage to account for the 
	   physical changes.  The comparison continues from where the 
	   insertion/deletion takes place; this doesn't happen very often, 
	   usually it happens with more than one window with the same 
	   buffer. */ 
 
	if (!CanScroll) 
		return;		/* We should never have been called! */ 
 
	for (i = 0; i < LI - 1; i++, np++, op++) 
		if (np->Line != op->Line) 
			break; 
	for (np = nimage, op = oimage; i < LI - 1; i++) { 
		for (j = i + 1; j < LI - 1; j++) { 
			if (np[j].Line != 0 && np[j].Line == op[j].Line) 
				break; 
			if (np[j].Line == op[i].Line) { 
				if (np[j].Line == 0) 
					continue; 
				if (AddLines(i, j - i)) { 
					DoIDline(j); 
					return; 
				} 
				break; 
			} 
			if (np[i].Line == op[j].Line) { 
				if (np[i].Line == 0) 
					continue; 
				if (DelLines(i, j - i)) { 
					DoIDline(i); 
					return; 
				} 
				break; 
			} 
		} 
	} 
} 
 
/* Make nimage reflect what the screen should look like when we are done 
   with the redisplay.  This deals with horizontal scrolling.  Also makes 
   sure the current line of the Window is in the window. */ 
 
UpdWindow(w, start) 
register Window	*w; 
{ 
	Line	*lp; 
	int	i, 
		DotIsHere = 0, 
		upper,		/* Top of window */ 
		lower;		/* Bottom of window */ 
	register struct scrimage	*np, 
					*op; 
	int	savestart = IDstart, 
		notequal; 
 
	if (w->w_flags & CURGONE) { 
		w->w_line = w->w_bufp->b_dot; 
		w->w_char = w->w_bufp->b_char; 
	} 
	if (w->w_flags & TOPGONE) 
		CalcTop(w);	/* Reset topline of screen */ 
	w->w_flags = 0; 
 
	upper = start; 
	lower = upper + w->w_height - 1;	/* Don't include modeline */ 
	np = &nimage[upper]; 
	op = &oimage[upper]; 
	for (i = upper, lp = w->w_top; lp != 0 && i < lower; i++, np++, op++, lp = lp->l_next) { 
		np->Window = w; 
		np->Line = lp; 
 
		if (lp == w->w_line) { 
			w->w_dotcol = find_pos(lp, w->w_char); 
			w->w_dotline = i; 
 
			if (w->w_numlines) 
				w->w_dotcol += 8; 
			if (w->w_dotcol < np->StartCol || 
				(w->w_dotcol + 2) >= (np->StartCol + CO)) { 
				if (w->w_dotcol + 2 < CO) 
					w->w_offset = 0; 
				else 
					w->w_offset = w->w_dotcol - (CO / 2); 
			} 
			np->StartCol = w->w_offset; 
			DotIsHere++; 
		} else 
			np->StartCol = 0; 
 
		notequal = (np->Line != op->Line); 
		if ( 
		    (IDstart != -1) || 
		    (np->StartCol != op->StartCol) || 
		    (isdirty(lp)) || 
		    (notequal) ) { 
		    	np->Sflags = DIRTY; 
			NumDirty++; 
		    	if (IDstart == -1 && notequal) 
		    		IDstart = i; 
		} 
	} 
	if (!DotIsHere) {	/* Current line not in window */ 
		if (UpdWCalls != 0) { 
			printf("\rCalled UpdWindow too many times"); 
			finish(SIGHUP); 
		} 
		IDstart = savestart; 
		UpdWCalls++; 
		CalcScroll(w); 
		UpdWindow(w, start);	/* This time for sure */ 
		return; 
	} 
 
	/* Is structure assignment faster than copy each field seperately */ 
	if (i < lower) { 
		static struct scrimage	dirty_plate = { 0, DIRTY, 0, 0 }; 
		static struct scrimage	clean_plate = { 0, 0, 0, 0 }; 
 
		for (; i < lower; i++, np++, op++) { 
			if (np->Sflags = ((op->Line) ? DIRTY : 0)) { 
				if (IDstart == -1) 
					IDstart = i; 
				NumDirty++; 
				*np = dirty_plate; 
			} else 
				*np = clean_plate; 
		} 
	} 
		 
	np->Window = w; 
	np->Sflags = 0; 
	if (((np->Line = (Line *) w->w_bufp) != op->Line) || UpdModLine) { 
		if (IDstart < 0) 
			IDstart = i; 
		np->Sflags = MODELINE | DIRTY; 
		NumDirty++; 
	} 
} 
 
/* Write whatever is in mesgbuf (maybe we are Asking,  or just printed 
   a message.  Turns off the UpdateMesg line flag */ 
 
UpdateMesg() 
{ 
	i_set(LI - 1, 0); 
	if (swrite(mesgbuf)) { 
		cl_eol(); 
		UpdMesg = 0; 
	} 
	flusho(); 
} 
 
/* Goto the current position in the current window.  Presumably redisplay() 
   has already been called, and curwind->{w_dotline,w_dotcol} have been set 
   correctly. */ 
 
GotoDot() 
{ 
	if (InputPending) 
		return; 
	Placur(curwind->w_dotline, curwind->w_dotcol - 
				oimage[curwind->w_dotline].StartCol); 
	flusho(); 
} 
 
/* Put the current line of `w' in the middle of the window */ 
 
CalcTop(w) 
register Window	*w; 
{ 
	SetTop(w, prev_line(w->w_line, HALF(w))); 
} 
 
int	ScrollStep = 0;		/* Full scrolling */ 
 
/* Calculate the new topline of the screen; different when in single-scroll 
   mode */ 
 
CalcScroll(w) 
register Window	*w; 
{ 
	extern int	line_diff; 
	register int	up; 
 
	if (ScrollStep == 0)	/* Means just center it */ 
		CalcTop(w); 
	else { 
		up = inorder(w->w_line, 0, w->w_top, 0); 
		if (up)		/* Dot is above the screen */ 
			SetTop(w, prev_line(w->w_line, min(ScrollStep - 1, HALF(w)))); 
		else 
			SetTop(w, prev_line(w->w_line, 
					(SIZE(w) - 1) - 
					min(ScrollStep - 1, HALF(w)))); 
	} 
} 
 
UntilEqual(start) 
register int	start; 
{ 
	register struct scrimage	*np = &nimage[start], 
					*op = &oimage[start]; 
 
	while ((start < LI - 1) && (np->Line != op->Line)) { 
		np++; 
		op++; 
		start++; 
	} 
 
	return start; 
} 
 
/* Calls the routine to do the physical changes, and changes oimage to 
   reflect those changes. */ 
 
AddLines(at, num) 
register int	at, 
		num; 
{ 
	register  int	i; 
	int	bottom = UntilEqual(at + num); 
 
	if (num == 0 || num >= ((bottom - 1) - at)) 
		return 0;	/* We did nothing */ 
	v_ins_line(num, at, bottom - 1); 
 
	/* Now change oimage to account for the physical change */ 
 
	for (i = bottom - 1; i - num >= at; i--) 
		oimage[i] = oimage[i - num]; 
	for (i = 0; i < num; i++) 
		oimage[at + i].Line = 0; 
	return 1;	/* We did something */ 
} 
 
DelLines(at, num) 
register int	at, 
		num; 
{ 
	register int	i; 
	int	bottom = UntilEqual(at + num); 
 
	if (num == 0 || num >= ((bottom - 1) - at)) 
		return 0; 
	v_del_line(num, at, bottom - 1); 
 
	for (i = at; num + i < bottom; i++) 
		oimage[i] = oimage[num + i]; 
	for (i = bottom - num; i < bottom; i++) 
		oimage[i].Line = 0; 
	return 1; 
} 
 
/* Update line linenum in window w.  Only set oimage to nimage if 
 * the swrite or cl_eol works, that is nothing is interupted by  
 * characters typed 
 */ 
 
UpdLine(w, linenum) 
register Window	*w; 
register int	linenum; 
{ 
#ifdef ID_CHAR 
	int	hasIC = ((IC || IM) && DC); 
#endif 
	register struct scrimage	*np = &nimage[linenum]; 
 
	if (np->Sflags & MODELINE) 
		ModeLine(w); 
	else if (np->Line) { 
		np->Line->l_dline &= ~DIRTY; 
		np->Sflags &= ~DIRTY; 
		i_set(linenum, 0); 
#ifdef ID_CHAR 
		if (!hasIC && w->w_numlines) 
#else 
		if (w->w_numlines) 
#endif 
			ignore(swrite(sprint("%6d  ", (linenum - FLine(w) + 
					w->w_topnum)))); 
 
#ifdef ID_CHAR 
		if (hasIC) { 
			char	outbuf[132], 
				buff[LBSIZE], 
				*bptr; 
			int	fromcol = w->w_numlines ? 8 : 0; 
 
			if (w->w_numlines) { 
				ignore(sprintf(buff, "%6d  ", (linenum - FLine(w) + 
						w->w_topnum))); 
				ignore(getright(np->Line, buff + fromcol)); 
				bptr = buff; 
			} else 
				bptr = getcptr(np->Line, buff); 
			DeTab(np->StartCol, bptr, 
				outbuf, (sizeof outbuf) - 1); 
			if (!IDchar(outbuf, linenum, 0)) 
				oimage[linenum] = *np; 
			else if (i_set(linenum, 0), swrite(outbuf)) 
				do_cl_eol(linenum); 
			else 
				oimage[linenum].Line = (Line *) -1; 
		} else 
#endif ID_CHAR 
		    if (BufSwrite(linenum)) 
			do_cl_eol(linenum); 
		else 
			oimage[linenum].Line = (Line *) -1; 
	} else if (oimage[linenum].Line) {	/* Not the same ... make sure */ 
		i_set(linenum, 0); 
		do_cl_eol(linenum); 
	} 
} 
 
do_cl_eol(linenum) 
register int	linenum; 
{ 
	cl_eol(); 
	oimage[linenum] = nimage[linenum]; 
} 
 
/* From here to the end of the file is code that tries to utilize the 
   insert/delete character feature on some terminals.  It is very confusing 
   and not so well written code, AND there is a lot of it.  You may want 
   to use the space for something else. */ 
 
#ifdef ID_CHAR 
 
extern struct screenline	*Screen; 
int	InMode = 0; 
 
DeTab(StartCol, buf, outbuf, limit) 
register char	*buf; 
char	*outbuf; 
{ 
	register char	*op = outbuf, 
			c; 
	register int	pos = 0; 
 
#define OkayOut(ch)	if ((pos++ >= StartCol) && (op < &outbuf[limit]))\ 
				*op++ = ch;\ 
			else 
 
	while (c = *buf++) { 
		if (c == '\t') { 
			int	nchars = (tabstop - (pos % tabstop)); 
 
			while (nchars--) 
				OkayOut(' '); 
 
		} else if (c < ' ' || c == 0177) { 
			OkayOut('^'); 
			OkayOut(c == 0177 ? '?' : c + '@'); 
		} else 
			OkayOut(c); 
		if (pos - StartCol >= CO) { 
			op = &outbuf[CO - 1]; 
			*op++ = '!'; 
			break; 
		}			 
	} 
	*op = 0; 
} 
 
CopyTo(to, from, limit) 
register char	*to, 
		*from, 
		*limit; 
{ 
	while (from <= limit) 
		*to++ = *from++; 
} 
 
/* ID character routines full of special cases and other fun stuff like that. 
   It actually works thougth ... */ 
 
IDchar(new, lineno, col) 
register char	*new; 
{ 
	int	i, 
		j, 
		oldlen, 
		NumSaved; 
	register struct screenline	*sline = &Screen[lineno]; 
 
	oldlen = sline->s_length - sline->s_line; 
 
	for (i = col; i < oldlen && new[i] != 0; i++) 
		if (sline->s_line[i] != new[i]) 
			break; 
	if (new[i] == 0 || i == oldlen) 
		return !(new[i] == 0 && i == oldlen); 
 
	for (j = i + 1; j < oldlen && new[j]; j++) { 
		if (new[j] == sline->s_line[i]) { 
			NumSaved = IDcomp(new + j, sline->s_line + i, 
					strlen(new)) + NumSimilar(new + i, 
						sline->s_line + i, j - i); 
			if (OkayInsert(NumSaved, j - i)) { 
				InsChar(lineno, i, j - i, new); 
				ignore(IDchar(new, lineno, j)); 
				return 1;	/* Difference */ 
			} 
		} 
	} 
 
	for (j = i + 1; j < oldlen && new[i]; j++) { 
		if (new[i] == sline->s_line[j]) { 
			NumSaved = IDcomp(new + i, sline->s_line + j, 
					oldlen - j); 
			if (OkayDelete(NumSaved, j - i, new[oldlen] == 0)) { 
				DelChar(lineno, i, j - i); 
				ignore(IDchar(new, lineno, j)); 
				return 1; 
			} 
		} 
	} 
	return 1; 
} 
 
NumSimilar(s, t, n) 
register char	*s, 
		*t; 
{ 
	register int	num = 0; 
 
	while (n--) 
		if (*s++ == *t++) 
			num++; 
	return num; 
} 
 
IDcomp(s, t, len) 
register char	*s, 
		*t; 
{ 
	register int	i; 
	int	num = 0, 
		nonspace = 0; 
	char	c; 
 
	for (i = 0; i < len; i++) { 
		if ((c = *s++) != *t++) 
			break; 
		if (c != ' ') 
			nonspace++; 
		if (nonspace) 
			num++; 
	} 
 
	return num; 
} 
 
OkayDelete(Saved, num, samelength) 
{ 
	static int	DelIn = 0, 
			CElen = 0; 
 
	if (DelIn == 0) { 
		DelIn = strlen(DC); 
		CElen = strlen(CE); 
	} 
 
	/* If the old and the new are the same length, then we don't 
	 * have to clear to end of line.  We take that into consideration. 
	 */ 
	return ((Saved + (!samelength ? CElen : 0)) > (DelIn * num)); 
} 
 
OkayInsert(Saved, num) 
{ 
	int	n; 
 
	if (IM && EI) {	/* Good terminal.  Fewer characters in this case */ 
		static int	InsIn = 0; 
 
		if (InsIn == 0) 
			InsIn = strlen(IM); 
 
		if (InMode)	/* We are already in insert mode */ 
			n = num; 
		else 
			n = num + InsIn; 
	} else { 
		static int	IClen = 0; 
 
		if (IClen == 0) 
			IClen = strlen(IC); 
		n = num + num * IClen; 
	} 
	return Saved > n; 
} 
 
extern int	CapCol; 
extern char	*cursend; 
extern struct screenline	*Curline; 
 
DelChar(lineno, col, num) 
{ 
	register int	i; 
 
	Placur(lineno, col); 
	for (i = 0; i < num; i++) 
		putpad(DC, 1); 
	CopyTo(Screen[lineno].s_line + col, Screen[lineno].s_line + col + num, 
			Screen[lineno].s_length); 
	clrline(Screen[lineno].s_length - num, Screen[lineno].s_length); 
	Screen[lineno].s_length -= num; 
} 
 
InsChar(lineno, col, num, new) 
char	*new; 
{ 
	register char	*sp1, 
			*sp2,	/* To push over the array */ 
			*sp3;	/* Last character to push over */ 
 
	int	WithSpaces = !IM; 
		/* If no insert mode then the IC inserts spaces 
		 * for us, and the screen image has to reflect 
		 * that. 
		 */ 
	int	i; 
 
	i_set(lineno, 0); 
	sp2 = Curline->s_length + num; 
	if (sp2 >= cursend) { 
		i_set(lineno, CO - (sp2 - cursend) - 1); 
		cl_eol(); 
		sp2 = cursend - 1; 
	} 
	Curline->s_length = sp2; 
	sp1 = sp2 - num; 
	sp3 = Curline->s_line + col; 
 
	while (sp1 >= sp3) 
		*sp2-- = *sp1--; 
	sp1 = Curline->s_line + col; 
	new += col; 
	for (i = 0; i < num; i++) 
		*sp1++ = (WithSpaces) ? ' ' : new[i]; 
	/* The internal screen is correct, and now we have to do 
	 * the physical stuff 
	 */ 
 
	Placur(lineno, col); 
	if (!WithSpaces) { 
		if (!InMode) { 
			putpad(IM, 1); 
			InMode++; 
		} 
		for (i = 0; i < num; i++) { 
			outchar(new[i]); 
			CapCol++; 
		} 
	} else { 
		for (i = 0; i < num; i++) 
			putpad(IC, 1); 
	} 
} 
 
#endif ID_CHAR 
 
