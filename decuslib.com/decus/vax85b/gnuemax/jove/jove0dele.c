/*R
   Jonathan Payne at Lincoln-Sudbury Regional High School 5-25-83 
 
   jove_delete.c 
   
   Routines to perform deletion.  The word delete and line delete  
   use the same routine as "delete-to-killbuffer".  Character delete 
   uses some of the same stuff, but doesn't save the text in the 
   kill-buffer.  */ 
 
#include "jove.h" 
 
/* Assumes that either line1 or line2 is actual the current line, so it can 
   put its result into linebuf. */ 
 
patchup(line1, char1, line2, char2) 
register Line	*line1, 
		*line2; 
{ 
	char	line2buf[LBSIZE]; 
	register char	*twoptr; 
 
	if (line1 != line2) 
		KludgeWindows(line1, line2); 
	SetModified(curbuf); 
	if (line2 == curline) {		/* So we don't need to getline it */ 
		if (line1 == curline) 
			twoptr = linebuf; 
		else { 
			strcpy(line2buf, linebuf); 
			twoptr = line2buf; 
		} 
		SetLine(line1);		/* Line1 now in linebuf */ 
	} else 
		twoptr = getright(line2, line2buf); 
 
	curchar = char1; 
	linecopy(linebuf, curchar, twoptr + char2); 
 
	DFixMarks(line1, char1, line2, char2); 
	makedirty(curline); 
} 
 
/* Deletes the region by unlinking the lines in the middle, 
   and patching things up.  The unlinked lines are still in 
   order.  */ 
 
Line * 
reg_delete(line1, char1, line2, char2) 
Line	*line1, *line2; 
{ 
	Line	*retline; 
 
	if ((line1 == line2 && char1 == char2) || line2 == 0) 
		complain((char *) 0); 
	fixorder(&line1, &char1, &line2, &char2); 
 
	retline = nbufline();	/* New buffer line */ 
 
	ignore(getright(line1, genbuf)); 
	if (line1 == line2) 
		genbuf[char2] = '\0'; 
 
	retline->l_prev = 0; 
	retline->l_dline = putline(&genbuf[char1]); 
	patchup(line1, char1, line2, char2); 
 
	if (line1 == line2) 
		retline->l_next = 0; 
	else { 
		retline->l_next = line1->l_next; 
		ignore(getright(line2, genbuf)); 
		genbuf[char2] = '\0'; 
		line2->l_dline = putline(genbuf);	/* Shorten this line */ 
	} 
 
	if (line1 != line2) { 
		line1->l_next = line2->l_next; 
		if (line1->l_next) 
			line1->l_next->l_prev = line1; 
		else 
			curbuf->b_last = line1; 
		line2->l_next = 0; 
	} 
 
	return retline; 
} 
 
lremove(line1, line2) 
register Line	*line1, 
		*line2; 
{ 
	Line	*next = line1->l_next; 
 
	if (line1 == line2) 
		return; 
	line1->l_next = line2->l_next; 
	if (line1->l_next) 
		line1->l_next->l_prev = line1; 
	else 
		curbuf->b_last = line1; 
	lfreereg(next, line2);	/* Put region at end of free line list */ 
} 
 
/* Delete character forward */ 
 
DelNChar() 
{ 
	del_char(1); 
} 
 
/* Delete character backward hacking indentation */ 
 
DelPChar() 
{ 
	int cindent; 
	switch (curbuf->b_major) { 
		case CMODE: 
		case CLUMODE: 
		case EUCLIDMODE: 
		case PASCALMODE: 
		case ASMMODE: 
			if (curchar && curchar == get_indent(linebuf, &cindent)) { 
				DelWtSpace(); 
				ind_up_level(linebuf, cindent, curline, curchar); 
				return; 
			} 
		case TEXT: 
		case FUNDMODE: 
		case TEXMODE: 
		case SCRIBEMODE: 
		default: ; 
	} 
	del_char(0); 
} 
 
/* Delete some characters.  If deleting `forward' then call for_char 
 * to the final position otherwise call back_char.  Then delete the 
 * region between the two with patchup(). 
 */ 
 
del_char(forward) 
{ 
	Bufpos	newdot, 
		pt; 
 
	if (forward) { 
		DOTsave(&newdot); 
		ForChar(); 
		DOTsave(&pt); 
	} else { 
		DOTsave(&pt); 
		BackChar(); 
		DOTsave(&newdot);	/* New dot will be back here */ 
	} 
 
	/* Patchup puts the result in linebuf, which is fine with us */ 
	patchup(newdot.p_line, newdot.p_char, pt.p_line, pt.p_char); 
	lremove(newdot.p_line, pt.p_line); 
} 
 
/* This kills a region and puts it on the kill-ring.  If the last command 
   was one of the kill commands, the region is appended (prepended if 
   backwards) to the last entry. */ 
 
int	killptr = 0; 
Line	*killbuf[NUMKILLS]; 
 
reg_kill(line1, char1, line2, char2, backwards) 
Line	*line1, 
	*line2; 
{ 
	Line	*nl, 
		*ol; 
	char buf[LBSIZE]; 
 
	fixorder(&line1, &char1, &line2, &char2); 
	DotTo(line1, char1); 
 
	nl = reg_delete(line1, char1, line2, char2); 
 
	if (last_cmd != KILLCMD) { 
		killptr = ((killptr + 1) % NUMKILLS); 
		lfreelist(killbuf[killptr]); 
		killbuf[killptr] = nl; 
	} else { 
		/* Last cmd was a KILLCMD so merge the kills */ 
		if (backwards) { 
			Line *fl, *lastln; 
 
			fl = killbuf[killptr]; 
			lastln = lastline(nl); 
			ignore(getright(lastln, buf)); 
			ignore(getright(fl, genbuf)); 
			linecopy(buf, strlen(buf), genbuf); 
			lastln->l_dline = putline(buf); 
			killbuf[killptr] = nl; 
			lastln->l_next = fl->l_next; 
		} else { 
			ol = lastline(killbuf[killptr]); 
			ignore(getright(ol, buf)); 
			ignore(getright(nl, genbuf)); 
			linecopy(buf, strlen(buf), genbuf); 
			ol->l_dline = putline(buf); 
			makedirty(ol); 
			ol->l_next = nl->l_next; 
		} 
	} 
	this_cmd = KILLCMD; 
} 
 
DelReg() 
{ 
	register Mark	*mp = CurMark(); 
 
	reg_kill(mp->m_line, mp->m_char, curline, curchar, 0); 
} 
 
/* Save a region.  A pretend kill. */ 
 
CopyRegion() 
{ 
	Line	*nl; 
	Mark	*mp; 
	int	status, 
		mod = curbuf->b_modified; 
 
	mp = CurMark(); 
 
	if (mp->m_line == curline && mp->m_char == curchar) 
		complain((char *) 0); 
 
	killptr = ((killptr + 1) % NUMKILLS); 
	if (killbuf[killptr]) 
		lfreelist(killbuf[killptr]); 
	nl = killbuf[killptr] = nbufline(); 
	nl->l_dline = NullLine; 
	nl->l_next = nl->l_prev = 0; 
 
	status = inorder(mp->m_line, mp->m_char, curline, curchar); 
	if (status == -1) 
		return; 
 
	if (status) 
		ignore(DoYank(mp->m_line, mp->m_char, curline, curchar, 
				nl, 0, (Buffer *) 0)); 
	else 
		ignore(DoYank(curline, curchar, mp->m_line, mp->m_char, 
				nl, 0, (Buffer *) 0)); 
	curbuf->b_modified = mod; 
} 
 
DelWtSpace() 
{ 
	register char	c; 
 
	while (!eolp() && ((c = linebuf[curchar]) == ' ' || c == '\t')) 
		del_char(1); 
	while (curchar > 0 && ((c = linebuf[curchar - 1]) == ' ' || c == '\t')) 
		del_char(0); 
} 
 
DelBlnkLines() 
{ 
	exp = 1; 
	if (!blnkp(linebuf) && !eolp()) 
		return; 
	while (blnkp(linebuf) && curline->l_prev) 
		SetLine(curline->l_prev); 
 
	Eol(); 
	DelWtSpace(); 
	NextLine(); 
	while (blnkp(linebuf) && !eobp()) { 
		DelWtSpace(); 
		DelNChar(); 
	} 
	DelWtSpace(); 
} 
 
DelNWord() 
{ 
	dword(1); 
} 
 
DelPWord() 
{ 
	dword(0); 
} 
 
dword(forward) 
{ 
	Bufpos	savedot; 
 
	DOTsave(&savedot); 
	forward ? ForWord() : BackWord(); 
	reg_kill(savedot.p_line, savedot.p_char, curline, curchar, !forward); 
} 
 
 
DeleteComment() 
{ 
	register char *cp, *ep; 
	register int i; 
	extern char *indexs(), *ToComment(); 
 
	if (! *CommentBegin) { 
		s_mess("No comment..."); 
		return; 
	} 
	cp = ToComment(linebuf); 
	if (! cp) return; 
	curchar = (int)(cp - linebuf); 
	if (*CommentEnd) { 
		ep = indexs(CommentEnd, cp); 
		if (ep) { 
			ep += strlen(CommentEnd); 
			reg_kill(curline, curchar, curline, (int)(ep - linebuf), 0); 
		} else 
			return; 
	} else { 
		KillEOL(); 
	} 
	DelWtSpace(); 
} 
