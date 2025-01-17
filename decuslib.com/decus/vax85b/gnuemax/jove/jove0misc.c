Y
/* 
   Jonathan Payne at Lincoln-Sudbury Regional High School 5-25-83 
   
   jove_meta.c 
 
   Various commands that are (by default) invoked by ESC-key. 
   Not a very good name since the ESC key can be rebound...  */ 
 
#include "jove.h" 
 
#include <ctype.h> 
#include <signal.h> 
 
Digit() 
{ 
	GetExp(LastKeyStruck); 
} 
 
Undo() 
{ 
	/* Save current version (undo undoes itself) */ 
	strcpy(genbuf, linebuf); 
	getDOT(); 
	/* Put the old version on the disk so */ 
	SavLine(curline, genbuf); 
} 
 
#ifdef lint 
Ignorl(a) 
long	a; 
{ 
	a = a; 
} 
#	define ignorl(a)	Ignorl(a) 
#else 
#	define ignorl(a)	a 
#endif 
 
prCTIME() 
{ 
	time_t	now; 
	char	*cp; 
	extern char	*ctime(); 
 
	ignorl(time(&now)); 
	cp = ctime(&now); 
	*(cp + strlen(cp) - 1) = 0;		/* Get rid of \n */ 
	s_mess("%s%s", FuncName(), cp); 
} 
 
extern int	alarmed; 
 
FourTime() 
{ 
	int	oldc = LastKeyStruck, 
		newc; 
	int	nexp; 
 
	alarmed = 0; 
	exp_p = 1; 
	this_cmd = ARG_CMD; 
	do { 
		if ((nexp = exp * 4) != 0) 
			exp = nexp; 
		if (!alarmed) 
			newc = waitfor(2); 
		else 
			newc = (*Getchar)(); 
		add_stroke(newc); 
		if (alarmed) 
			message(key_strokes); 
	} while (newc == oldc); 
	if (isdigit(newc)) { 
		exp = 0; 
		GetExp(newc); 
	} 
	else 
		dispatch(newc); 
} 
 
int	exp_p, 
	exp; 
 
GetExp(c) 
{ 
	int	sign = 1, 
		i; 
 
	if (exp_p) 
		i = exp; 
	else 
		i = 0; 
	if (c == '-') { 
		sign = -1; 
		goto getnextc; 
	} 
	for (;;) { 
		if (alarmed) 
			message(key_strokes); 
		if (isdigit(c)) 
			i = i * 10 + (c - '0'); 
		else { 
			exp_p = 1; 
			if (i == 0) 
				i = 1; 
			exp = i * sign; 
			this_cmd = ARG_CMD; 
			dispatch(c); 
			return; 
		} 
getnextc: 
		if (!alarmed) 
			c = waitfor(2); 
		else { 
			add_mess(NullStr); 
			c = (*Getchar)(); 
		} 
		add_stroke(c); 
	} 
} 
 
ChrToOct() 
{ 
	int	c = getch(); 
 
	ins_str(sprint("\\%03o", c)); 
} 
 
StrLength() 
{ 
	static char	inquotes[] = "Where are the quotes?"; 
	char	*first = StrIndex(-1, linebuf, curchar, '"'); 
	char	*last = StrIndex(1, linebuf, curchar, '"'); 
	char	c; 
	int	numchars = 0; 
 
	if (first == 0 || last == 0) 
		complain(inquotes); 
	first++; 
	while (first < last) { 
		c = *first++; 
		if (c == '\\') { 
			int	num; 
 
			if (*first < '0' || *first > '9') 
				++first; 
			else { 
				num = 3; 
				while (num-- && (c = *first++) >= '0' && 
						c <= '9' && first < last) 
					; 
			} 
		} 
		numchars++; 
	} 
	s_mess("%d characters", numchars); 
} 
 
/* Transpos cur_char with cur_char - 1 */ 
 
TransChar() 
{ 
	char	c; 
 
	if (curchar == 0) 
		complain((char *) 0);	/* BEEP */ 
	c = linebuf[curchar - 1]; 
	exp = 1; 
	DelPChar(); 
	if (eolp()) { 
		BackChar(); 
		Insert(c); 
		ForChar(); 
	} else { 
		ForChar(); 
		Insert(c); 
		BackChar(); 
	} 
} 
 
UpScroll() 
{ 
	SetTop(curwind, next_line(curwind->w_top, exp)); 
	if (in_window(curwind, curline) == -1) 
		SetLine(next_line(curwind->w_top, HALF(curwind))); 
} 
 
DownScroll() 
{ 
	SetTop(curwind, prev_line(curwind->w_top, exp)); 
	if (in_window(curwind, curline) == -1) 
		SetLine(next_line(curwind->w_top, HALF(curwind))); 
} 
 
Leave() 
{ 
	longjmp(mainjmp, QUIT); 
} 
 
KillEos() 
{ 
	Line	*saveline = curline; 
	int	savechar = curchar, 
		backwd = 0; 
 
	if (exp < 0) { 
		backwd = 1; 
		exp = -exp; 
		Bos(); 
	} else 
		Eos(); 
 
	reg_kill(saveline, savechar, curline, curchar, backwd); 
} 
 
KillEOL() 
{ 
	Line	*line2; 
	int	char2; 
 
	if (exp_p) { 
		line2 = next_line(curline, exp); 
		if ((LineDist(curline, line2) < exp) || (line2 == curline)) 
			char2 = length(line2); 
		else 
			char2 = 0; 
	} else if (eolp()) { 
		line2 = next_line(curline, 1); 
		if (line2 == curline) 
			char2 = length(curline); 
		else 
			char2 = 0; 
	} else { 
		line2 = curline; 
		char2 = length(curline); 
	} 
	reg_kill(curline, curchar, line2, char2, 0); 
} 
 
EscPrefix() 
{ 
	HandlePref(pref1map); 
} 
 
CtlxPrefix() 
{ 
	HandlePref(pref2map); 
} 
 
MiscPrefix() 
{ 
	HandlePref(miscmap); 
} 
 
HandlePref(map) 
data_obj	**map; 
{ 
	data_obj	*fp; 
	int	c, 
		invokingchar = LastKeyStruck; 
 
	c = waitfor(2); 
	if (c == CTL(G)) { 
		message("Aborted"); 
		return; 
	} 
 
	add_stroke(c); 
	if (alarmed) 
		message(key_strokes); 
 
	fp = map[c]; 
	if (fp == 0) 
		Unknown(invokingchar, c); 
	else 
		ExecFunc(fp, 0); 
} 
 
Yank() 
{ 
	Line	*line, 
		*lp; 
	Bufpos	*dot; 
 
	if (killbuf[killptr] == 0) 
		complain("Nothing to yank!"); 
	lsave(); 
	this_cmd = YANKCMD; 
	line = killbuf[killptr]; 
	lp = lastline(line); 
	dot = DoYank(line, 0, lp, length(lp), curline, curchar, curbuf); 
	SetMark(); 
	SetDot(dot); 
} 
 
WtModBuf() 
{ 
	DoWtModBuf(NullStr, NullStr); 
} 
 
ASbuffers() 
{ 
	register Buffer	*b; 
	Buffer	*saveb = curbuf; 
 
	for (b = world; b; b = b->b_next) { 
		if (!IsModified(b) || (b->b_modified < ASInterval)) 
			continue; 
		b->b_modified = 1; 
		f_mess("(Auto save)"); 
		SetBuf(b); 
		if (MinorMode(Save) == 0) 
			continue; 
		SaveBuf(AS_PREFIX, AS_SUFFIX, 0); 
	} 
	SetBuf(saveb); 
} 
 
ASdel_saves() 
{ 
	register Buffer	*b; 
	Buffer	*saveb = curbuf; 
	char	name[256]; 
 
	for (b = world; b; b = b->b_next) { 
		SetBuf(b); 
		if (MinorMode(Save) == 0) 
			continue; 
		cons_fname(name, AS_PREFIX, AS_SUFFIX); 
		if (strcmp(name, curbuf->b_fname) == 0) 
			continue; 
			/* Just in case we are unlinking the only remaining 
			   copy of the saved file (out of carelessness) */ 
 
		ignore(unlink(name)); 
	} 
	SetBuf(saveb); 
} 
 
DoWtModBuf(prefix, suffix) 
char	*prefix, 
	*suffix; 
{ 
	int	askp = exp_p; 
	Buffer	*oldb = curbuf,	 
		*b;		 
 
	for (b = world; b; b = b->b_next) { 
		if (!IsModified(b) || b->b_type != FILE) 
			continue; 
		SetBuf(b);	/* Make this current Buffer */ 
		if (curbuf->b_fname == 0) 
			setfname(b, ask((char *) 0, "Buffer \"%s\" needs a file name: ", b->b_name)); 
 
		SaveBuf(prefix, suffix, askp); 
		SetUnmodified(curbuf); 
	} 
	SetBuf(oldb); 
} 
 
/* Construct a file name from the current buffer's file name and the  
   prefix and suffix strings */ 
 
cons_fname(name, prefix, suffix) 
char	*name, 
	*prefix, 
	*suffix; 
{ 
	char	*dirp; 
	char	*basename = curbuf->b_fname ? curbuf->b_fname : curbuf->b_name; 
 
	dirp = rindex(basename, '//'); 
	if (dirp == 0) 
		ignore(sprintf(name, "%s%s%s", prefix, basename, suffix)); 
	else { 
		strncpy(name, basename, 1 + (dirp - basename)); 
		name[1 + (dirp - basename)] = 0; 
		ignore(sprintf(&name[strlen(name)], "%s%s%s", prefix, dirp + 1, suffix)); 
	} 
} 
 
SaveBuf(prefix, suffix, askp) 
char	*prefix, 
	*suffix; 
{ 
	char	*yorn, 
		name[256]; 
 
	cons_fname(name, prefix, suffix); 
	if (askp) { 
		yorn = ask((char *) 0, "Write %s? ", name); 
		if (*yorn != 'Y' && *yorn != 'y') 
			return; 
	} 
	file_write(name, 0); 
} 
 
/* 
   Jonathan Payne at Lincoln-Sudbury Regional High School 5-25-83 
 
   jove_case.c 
 
   contains case case-region and case-word functions.  */ 
 
DOTsave(buf) 
Bufpos *buf; 
{ 
	buf->p_line = curline; 
	buf->p_char = curchar; 
} 
 
CapWord() 
{ 
	int num = exp; 
 
	exp = 1;	/* So all the commands are done once */ 
 
	while (num--) { 
		to_word(1);	/* Go to the beginning of the next word. */ 
		if (eobp()) 
			break; 
		upper(&linebuf[curchar]);	/* Cap this word. */ 
		SetModified(curbuf);	/* Or else lsave won't do anything */ 
		makedirty(curline); 
		curchar++; 
		while (!eolp() && isword(linebuf[curchar])) { 
			lower(&linebuf[curchar]); 
			curchar++; 
		} 
	} 
} 
 
case_word(up) 
{ 
	Bufpos bp; 
 
	DOTsave(&bp); 
	ForWord();	/* Go to end of the region */ 
	case_reg(bp.p_line, bp.p_char, curline, curchar, up); 
} 
 
upper(c) 
register char	*c; 
{ 
	if (*c >= 'a' && *c <= 'z') 
		*c -= 040; 
} 
 
lower(c) 
register char	*c; 
{ 
	if (*c >= 'A' && *c <= 'Z') 
		*c += 040; 
} 
 
case_reg(line1, char1, line2, char2, up) 
Line	*line1, 
	*line2; 
{ 
	char lbuf[LBSIZE]; 
 
	SetModified(curbuf); 
	fixorder(&line1, &char1, &line2, &char2); 
	lsave(); 
	ignore(getline(line1->l_dline, lbuf)); 
	for (;;) { 
		if (line1 == line2 && char1 == char2) 
			break; 
		if (lbuf[char1] == '\0') { 
			char1 = 0; 
			line1->l_dline = putline(lbuf); 
			makedirty(line1); 
			if (lastp(line1)) 
				break; 
			line1 = line1->l_next; 
			ignore(getline(line1->l_dline, lbuf)); 
			continue; 
		} 
		if (up) 
			upper(&lbuf[char1]); 
		else 
			lower(&lbuf[char1]); 
		char1++; 
	} 
	line1->l_dline = putline(lbuf); 
	makedirty(line1); 
	getDOT(); 
} 
 
CasRegLower() 
{ 
	CaseReg(0); 
} 
 
CasRegUpper() 
{ 
	CaseReg(1); 
} 
 
CaseReg(up) 
{ 
	register Mark	*mp = CurMark(); 
 
	case_reg(curline, curchar, mp->m_line, mp->m_char, up); 
} 
 
UppWord() 
{ 
	case_word(1); 
} 
 
LowWord() 
{ 
	case_word(0); 
} 
 
get_indent(buf, hpos) 
	char *buf; 
	int *hpos; 
{ 
	register char	*cp, c; 
	int cpos; 
 
	for (cp = buf; c = *cp; cp++) { 
		if (c != ' ' && c != '\t') 
			break; 
	} 
	cpos = (int)(cp - buf); 
	*hpos = calc_pos(buf, cpos); 
	return(cpos); 
} 
 
ToIndent() 
{ 
	int hpos; 
	curchar = get_indent(linebuf, &hpos); 
} 
 
ind_prev(buf, cline, cchar) 
	char *buf; 
	Line *cline; 
	int cchar; 
{ 
	register Line *lp = cline->l_prev; 
 
	DotTo(cline, cchar); 
	genbuf[0] = '\0'; 
	while (lp && blnkp(genbuf)) { 
		getline(lp->l_dline, genbuf); 
		lp = lp->l_prev; 
	} 
	whitesp(genbuf, buf); 
} 
 
ind_up_level(buf, cindent, cline, cchar) 
	char *buf; 
	int cindent; 
	Line *cline; 
	int cchar; 
{ 
	register Line *lp = cline->l_prev; 
	int nindent = cindent; 
 
	DotTo(cline, cchar); 
	genbuf[0] = '\0'; 
	while (lp && (blnkp(genbuf) || nindent >= cindent)) { 
		getline(lp->l_dline, genbuf); 
		(void) get_indent(genbuf, &nindent); 
		lp = lp->l_prev; 
	} 
	whitesp(genbuf, buf); 
} 
 
ForwardPara() 
{ 
	find_blankline(1); 
} 
 
BackwardPara() 
{ 
	find_blankline(0); 
} 
 
find_blankline(direction) 
	int direction; 
{ 
	extern NextLine(), PrevLine(); 
	int (*mover)() = direction ? NextLine : PrevLine; 
 
	Bol(); 
	while (blnkp(linebuf)) { 
		(*mover)(); 
	} 
	while (!blnkp(linebuf)) { 
		(*mover)(); 
	} 
} 
 
GoLine() 
{ 
	if (exp_p == 0) 
		return; 
	SetLine(next_line(curbuf->b_first, exp - 1)); 
} 
 
int	RMargin = RMARGIN; 
 
Justify() 
{ 
	Bufpos	*bp; 
	Line	*start, 
		*end; 
	char	*ParaStr = "^[!#-/:->~]\\|^$"; 
 
	bp = dosearch(ParaStr, -1, 1);	/* Beginning of paragraph */ 
	if (bp)		/* Not the tab case */ 
		start = bp->p_line->l_next; 
	else 
		start = curbuf->b_first; 
	bp = dosearch(ParaStr, 1, 1);		/* End of paragraph */ 
	if (bp) 
		end = bp->p_line->l_prev; 
	else 
		end = curbuf->b_last; 
	DoJustify(start, 0, end, length(end), 0); 
} 
 
FillColumn() 
{ 
	RMargin = calc_pos(linebuf, curchar); 
	s_mess("Fill column = %d", RMargin); 
} 
 
DoJustify(l1, c1, l2, c2, scrunch) 
Line	*l1, 
	*l2; 
{ 
	int	okay_char; 
	char	*cp; 
	Mark	*savedot = MakeMark(curline, curchar, FLOATER), 
		*endmark; 
 
	exp = 1; 
	fixorder(&l1, &c1, &l2, &c2);	/* l1/c1 will be before l2/c2 */ 
	SetLine(l1); 
	endmark = MakeMark(l2, c2, FLOATER); 
 
	for (;;) { 
		while (curchar < RMargin) { 
			if (curline == endmark->m_line && curchar >= endmark->m_char) 
				goto outahere; 
			okay_char = curchar; 
			if (eolp()) { 
				DelNChar();	/* Delete line separator */ 
				Insert(' '); 
			} else { 
				cp = StrIndex(1, linebuf, curchar, ' '); 
				if (cp == 0) 
					Eol(); 
				else 
					curchar = 1 + (cp - linebuf); 
			} 
			if (linebuf[curchar] == ' ' || (!bolp() && linebuf[curchar - 1] == ' ')) { 
				DelWtSpace(); 
				if (!bolp() && index("?.!", linebuf[curchar - 1])) 
					Insert(' '); 
				/* Two spaces after end of sentence */ 
				Insert(' '); 
			} 
		} 
 
		if (okay_char != 0) 
			curchar = okay_char; 
		else if (eolp()) { 
			if (curline == endmark->m_line && curchar >= endmark->m_char) 
				goto outahere; 
			Bol(); 
			NextLine(); 
			continue; 
		} 
		DelWtSpace(); 
		LineInsert(); 
		if (scrunch && TwoBlank()) { 
			Eol(); 
			DelNChar(); 
			Bol(); 
		} 
	} 
outahere: 
	ToMark(savedot);	/* Back to where we were */ 
	DelMark(endmark);	/* Free up marks */ 
	DelMark(savedot); 
	this_cmd = 0;		/* So everything is under control */ 
} 
 
NotModified() 
{ 
	SetUnmodified(curbuf); 
} 
 
