/* /usr/src/local/jove/jove_util.c, Sun Nov 11 00:25:34 1984, ed fhsu@uw-june */ 
 
#include "jove.h" 
#include <signal.h> 
#include <ctype.h> 
 
Line * 
lastline(lp) 
register Line	*lp; 
{ 
	while (lp->l_next) 
		lp = lp->l_next; 
	return lp; 
} 
 
Unknown(pref, c) 
char	pref, 
	c; 
{ 
	s_mess("%c-%c unbound", pref, c); 
	rbell(); 
} 
 
Upper(c) 
register int	c; 
{ 
	return (islower(c) ? toupper(c) : c); 
} 
 
int	alarmed = 0; 
 
char	key_strokes[100]; 
 
add_stroke(c) 
{ 
	int	len = strlen(key_strokes); 
 
	if (len + 5 > (sizeof key_strokes)) 
		return; 
	PPchar(c, &key_strokes[strlen(key_strokes)]); 
	strcat(key_strokes, " "); 
} 
 
slowpoke() 
{ 
	alarmed++; 
	f_mess(key_strokes); 
} 
 
waitfor(sec) 
{ 
	int	c; 
 
#ifdef EUNICE 
	return (*Getchar)(); 
#else 
	alarmed = 0; 
	signal(SIGALRM, slowpoke); 
 
	ignore(alarm((unsigned) sec)); 
	c = (*Getchar)(); 
	if (!alarmed)			/* If we didn't get signaled ... */ 
		ignore(alarm(0));	/* we have to turn it off ouselves */ 
	ignorf(signal(SIGALRM, SIG_DFL)); 
	return c; 
#endif 
} 
 
/* dir > 0 means forward; else means backward. */ 
 
char * 
StrIndex(dir, buf, charpos, what) 
char	*buf, 
	what; 
{ 
	char	*cp = &buf[charpos], 
		c; 
 
	if (dir > 0) { 
		while (c = *cp++) 
			if (c == what) 
				return (cp - 1); 
	} else { 
		while (cp >= buf && (c = *cp--)) 
			if (c == what) 
				return (cp + 1); 
	} 
	return 0; 
} 
 
SetLine(line) 
register Line	*line; 
{ 
	DotTo(line, 0); 
} 
 
char * 
ask_file(def, prompt, buf, flags) 
char	*def, 
	*prompt, 
	*buf; 
{ 
	char	*ans; 
#ifdef FCOMPLETE 
	extern char *fcomplete(); 
 
	ans = fcomplete(def, prompt, flags); 
	strcpy(buf, ans); 
#else 
	ans = ask(def, prompt); 
	PathParse(ans, buf); 
	if ((flags&FIL_ONLY) && is_directory(buf)) 
		complain("Are you sure you want to read a directory file?"); 
#endif 
	return buf; 
} 
 
blnkp(buf) 
register char	*buf; 
{ 
	register char	c; 
 
	while ((c = *buf++) && (c == ' ' || c == '\t')) 
		; 
	return c == 0;	/* It's zero if we got to the end of the Line */ 
} 
 
Line * 
next_line(line, num) 
register Line	*line; 
register int	num; 
{ 
	register int	i; 
 
	if (line) 
		for (i = 0; i < num && line->l_next; i++, line = line->l_next) 
			; 
 
	return line; 
} 
 
Line * 
prev_line(line, num) 
register Line	*line; 
register int	num; 
{ 
	register int	i; 
 
	if (line) 
		for (i = 0; i < num && line->l_prev; i++, line = line->l_prev) 
			; 
 
	return line; 
} 
 
DotTo(line, col) 
Line	*line; 
{ 
	Bufpos	bp; 
 
	bp.p_line = line; 
	bp.p_char = col; 
	SetDot(&bp); 
} 
 
/* If bp->p_line is != current line, then save current line.  Then set dot 
   to bp->p_line, and if they weren't equal get that line into linebuf  */ 
 
SetDot(bp) 
register Bufpos	*bp; 
{ 
	register int	notequal; 
 
	if (bp == 0) 
		return; 
 
	notequal = bp->p_line != curline; 
	if (notequal) 
		lsave(); 
	if (bp->p_line) 
		curline = bp->p_line; 
	curchar = bp->p_char; 
	if (notequal) 
		getDOT(); 
} 
 
ToLast() 
{ 
	SetLine(curbuf->b_last); 
	Eol(); 
} 
 
int	MarkThresh = 22,	/* Average screen size ... */ 
	line_diff = 0; 
 
LineDist(nextp, endp) 
register Line	*nextp, 
		*endp; 
{ 
	ignore(inorder(nextp, 0, endp, 0)); 
	return line_diff; 
} 
 
inorder(nextp, char1, endp, char2) 
register Line	*nextp, 
		*endp; 
{ 
	int	count = 0; 
	register Line	*prevp = nextp; 
 
	line_diff = 0; 
 
	if (nextp == endp) 
		return char1 < char2; 
 
	while (nextp || prevp) { 
		if (nextp == endp || prevp == endp) 
			break; 
		if (nextp) 
			nextp = nextp->l_next; 
		if (prevp) 
			prevp = prevp->l_prev; 
		count++; 
	} 
	if (nextp == 0 && prevp == 0) 
		return -1; 
	line_diff = count; 
	return nextp == endp; 
} 
 
PushPntp(line) 
Line	*line; 
{ 
	if (curbuf->b_type != IPROCESS && LineDist(curline, line) >= MarkThresh) 
		SetMark(); 
} 
 
ToFirst() 
{ 
	SetLine(curbuf->b_first); 
} 
 
length(line) 
register Line	*line; 
{ 
	register char	*base, 
			*cp; 
 
	base = cp = getcptr(line, genbuf); 
 
	while (*cp++) 
		; 
	return cp - base - 1; 
} 
 
isword(c) 
register char	c; 
{ 
	return isalpha(c) || isdigit(c); 
} 
 
to_word(dir) 
{ 
	register char	c; 
 
	if (dir > 0) { 
		while ((c = linebuf[curchar]) != 0 && !isword(c)) 
			curchar++; 
		if (eolp()) { 
			if (curline->l_next == 0) 
				return; 
			SetLine(curline->l_next); 
			to_word(dir); 
			return; 
		} 
	} else { 
		while (!bolp() && (c = linebuf[curchar - 1], !isword(c))) 
			--curchar; 
		if (bolp()) { 
			if (curline->l_prev == 0) 
				return; 
			SetLine(curline->l_prev); 
			Eol(); 
			to_word(dir); 
		} 
	} 
} 
 
ModBufs() 
{ 
	register Buffer	*b; 
 
	for (b = world; b; b = b->b_next) 
		if (b->b_type == FILE && IsModified(b)) 
			return 1; 
	return 0; 
} 
 
char * 
filename(bp) 
Buffer	*bp; 
{ 
	return bp->b_fname ? PathRelative(bp->b_fname) : "[No file]"; 
} 
 
char * 
itoa(num) 
{ 
	static char	line[10]; 
 
	return sprintf(line, "%d", num); 
} 
 
#ifndef max 
max(a, b) 
int a, b; 
{ 
	return a > b ? a : b; 
} 
#endif 
 
#ifndef min 
min(a, b) 
{ 
	return a < b ? a : b; 
} 
#endif 
 
tiewind(wp, bp) 
Window	*wp; 
Buffer	*bp; 
{ 
	UpdModLine++;	/* Kludge ... but speeds things up considerably */ 
	wp->w_line = bp->b_dot; 
	wp->w_char = bp->b_char; 
	wp->w_bufp = bp; 
} 
 
/* Use this when you just want to look at a line without 
   changing anything.  It'll return linebuf if it is the 
   current line of the current buffer (no copying). */ 
 
char * 
getcptr(line, buf) 
Line	*line; 
char	*buf; 
{ 
	if (line == curline) 
		return linebuf; 
	else { 
		ignore(getline(line->l_dline, buf)); 
		return buf; 
	} 
} 
 
/* Use this when getcptr is not appropiate */ 
 
char * 
getright(line, buf) 
Line	*line; 
char	*buf; 
{ 
	if (line == curline) { 
		if (buf != linebuf) 
			strcpy(buf, linebuf); 
	} else 
		ignore(getline(line->l_dline, buf)); 
	return buf; 
} 
 
fixorder(line1, char1, line2, char2) 
register Line	**line1, 
		**line2; 
register int	*char1, 
		*char2; 
{ 
	Line	*tline; 
	int	tchar; 
 
	if (inorder(*line1, *char1, *line2, *char2)) 
		return; 
 
	tline = *line1; 
	tchar = *char1; 
	*line1 = *line2; 
	*char1 = *char2; 
	*line2 = tline; 
	*char2 = tchar; 
} 
inlist(first, what) 
register Line	*first, 
		*what; 
{ 
	while (first) { 
		if (first == what) 
			return 1; 
		first = first->l_next; 
	} 
	return 0; 
} 
 
/* Make `buf' modified and tell the redisplay code to update the modeline 
   if it will need to be changed. */ 
 
SetModified(buf) 
Buffer	*buf; 
{ 
	extern int	DOLsave; 
 
	if (!buf->b_modified) 
		UpdModLine++; 
	buf->b_modified++; 
	DOLsave++; 
} 
 
SetUnmodified(buf) 
Buffer	*buf; 
{ 
	if (buf->b_modified) 
		UpdModLine++; 
	buf->b_modified = 0; 
} 
 
numcomp(s1, s2) 
register char	*s1, 
		*s2; 
{ 
	register int	count = 0; 
 
	while (*s1 != 0 && *s1++ == *s2++) 
		count++; 
	return count; 
} 
 
char * 
copystr(str) 
char	*str; 
{ 
	char	*val = emalloc(strlen(str) + 1); 
 
	strcpy(val, str); 
	return val; 
} 
 
copynchar(t, f, len) 
#ifdef vax 
	char *t, *f; 
	int len; 
{ 
	asm("	movc3	12(ap),*8(ap),*4(ap)"); 
} 
#else 
register char	*t, 
		*f; 
register int	len; 
{ 
	while (--len >= 0) 
		*t++ = *f++; 
} 
#endif 
 
#ifdef vax 
 
strlen(s) 
	char *s; 
{ 
	asm("	locc	$0,$65535,*4(ap)"); 
	asm("	subl3	r0,$65535,r0"); 
} 
 
 
strncmp(s1, s2, len) 
	char *s1, *s2; 
	int len; 
{ 
	asm("	cmpc3	12(ap),*4(ap),*8(ap)"); 
} 
 
 
char * 
strncpy(s1, s2, len) 
	char *s1, *s2; 
	int len; 
{ 
	asm("	movc3	12(ap),*8(ap),*4(ap)"); 
	asm("	movl	4(ap),r0"); 
} 
 
#endif vax 
 
 
len_error(flag) 
{ 
	char	*mesg = "line too long"; 
 
	(flag == COMPLAIN) ? complain(mesg) : error(mesg); 
} 
 
/* Insert num number of c's at offset atchar in a linebuf of LBSIZE */ 
 
insert(c, buf, atchar, num, maxc) 
char	c, *buf; 
{ 
	register char	*pp, *pp1; 
	register int	len = atchar + strlen(&buf[atchar]); 
	int	numchars;	/* Number of characters to copy forward */ 
 
	if (len + num >= maxc) 
		len_error(COMPLAIN); 
	pp = &buf[len + 1];		/* + 1 so we can --pp (not pp--) */ 
	pp1 = &buf[len + num + 1]; 
	numchars = len - atchar; 
	while (numchars-- >= 0) 
		*--pp1 = *--pp; 
	pp = &buf[atchar]; 
	while (num--) 
		*pp++ = c; 
} 
 
TwoBlank() 
{ 
	return (curline->l_next && 
			(getline(curline->l_next->l_dline, 
			genbuf)[0] == '\0') && curline->l_next->l_next && 
			(getline(curline->l_next->l_next->l_dline, 
			genbuf)[0] == '\0')); 
} 
 
linecopy(onto, atchar, from) 
register char	*onto, *from; 
register int	atchar; 
{ 
	char	*base = onto; 
 
	onto += atchar; 
 
	while (*onto = *from++) 
		if (onto++ >= &base[LBSIZE - 2]) 
			len_error(ERROR); 
} 
 
char * 
IOerr(err, file) 
char	*err, *file; 
{ 
	return sprint("Couldn't %s \"%s\"", err, file); 
} 
 
getDOT() 
{ 
	ignore(getline(curline->l_dline, linebuf)); 
} 
 
SavLine(addr, buf) 
Line	*addr; 
char	*buf; 
{ 
	addr->l_dline = putline(buf); 
} 
 
pclose(p) 
int	p[]; 
{ 
	ignore(close(p[0])); 
	ignore(close(p[1])); 
} 
 
doread(fd, buf, n) 
char	*buf; 
{ 
	int	nread; 
 
	if ((nread = read(fd, buf, n)) != n) 
		complain("Cannot read %d(%d) bytes", n, nread); 
} 
 
dopipe(p) 
int	p[]; 
{ 
	if (pipe(p) == -1) 
		complain("Pipe failed"); 
} 
 
/* NOSTRICT */ 
 
char * 
emalloc(size) 
{ 
	char	*ptr; 
 
	if (ptr = malloc((unsigned) size)) 
		return ptr; 
	GCchunks(); 
	if (ptr = malloc((unsigned) size)) 
		return ptr; 
	error("out of memory"); 
	/* NOTREACHED */ 
} 
 
putstr(str) 
register char	*str; 
{ 
	register char	c; 
 
	while (c = *str++) 
		outchar(c); 
} 
 
#define MAXARGS 30 
 
char ** 
make_args(arg0, s, filnam) 
	char *arg0; 
	register char *s; 
	char *filnam; 
{ 
	static char *argp[MAXARGS+2]; 
	register int i = 0; 
 
	if (arg0) { 
		argp[0] = arg0; 
		i = 1; 
	} 
	while (s && i < MAXARGS) { 
		while (*s && (*s == ' ' || *s == '\t')) { 
			s++; 
		} 
		if (*s) { 
			argp[i] = s++; 
			i++; 
		} else { 
			break; 
		} 
		while (*s) { 
			if (*s == ' ' || *s == '\t') { 
				*s = '\0'; 
				s++; 
				break; 
			} 
			s++; 
		} 
	} 
	if (filnam) { 
		argp[i] = filnam; 
		i++; 
	} 
	argp[i] = (char *)0; 
	return(argp); 
} 
