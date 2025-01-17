Y
#include "jove.h" 
 
int	line_pos; 
 
ForChar() 
{ 
	register int	num = exp; 
 
	exp = 1; 
	while (num--) { 
		if (eolp()) {			/* Go to the next Line */ 
			if (curline->l_next == 0) 
				break; 
			SetLine(curline->l_next); 
		} else 
			curchar++; 
	} 
} 
 
BackChar() 
{ 
	register int	num = exp; 
 
	exp = 1; 
	while (num--) { 
		if (bolp()) { 
			if (curline->l_prev == 0) 
				break; 
			SetLine(curline->l_prev); 
			Eol(); 
		} else 
			--curchar; 
	} 
} 
 
NextLine() 
{ 
	if (lastp(curline)) 
		OutOfBounds(); 
	down_line(1); 
} 
 
PrevLine() 
{ 
	if (firstp(curline)) 
		OutOfBounds(); 
	down_line(0); 
} 
 
OutOfBounds() 
{ 
	complain(NullStr); 
} 
 
down_line(down) 
{ 
	Line	*(*func)() = down ? next_line : prev_line; 
	Line	*line; 
 
	line = (*func)(curline, exp); 
 
	this_cmd = line_CMD; 
 
	if (last_cmd != line_CMD) 
		line_pos = calc_pos(linebuf, curchar); 
 
	SetLine(line);		/* Curline is in linebuf now */ 
	curchar = how_far(curline, line_pos); 
} 
 
/* Returns what cur_char should be for that pos. */ 
 
how_far(line, ypos) 
Line	*line; 
{ 
	register char	*pp; 
	char	*base; 
	register int	cur_char; 
	char	c; 
	register int	y; 
 
	base = pp = getcptr(line, genbuf); 
 
	cur_char = 0; 
	y = 0; 
 
	while (c = *pp++) { 
		if (y >= ypos) 
			return cur_char; 
		if (c == 0177) 
			y++; 
		else if (c < ' ') { 
			if (c == 011) 
				y += ((tabstop - y % tabstop) - 1); 
			else 
				y++; 
		} 
		y++; 
		cur_char++; 
	} 
 
	return pp - base - 1; 
} 
 
Bol() 
{ 
	curchar = 0; 
} 
 
Eol() 
{ 
	curchar += strlen(&linebuf[curchar]); 
} 
 
Eof() 
{ 
	PushPntp(curbuf->b_last); 
	ToLast(); 
} 
 
Bof() 
{ 
	PushPntp(curbuf->b_first); 
	ToFirst(); 
} 
 
char	REsent[] = "[?.!]\"  \\|[.?!]  \\|[.?!][\"]*$"; 
 
Bos() 
{ 
	int	num = exp; 
	Bufpos	*bp, 
		save; 
 
	exp = 1; 
	while (num--) { 
onceagain: 
		bp = dosearch(REsent, -1, 1); 
		DOTsave(&save); 
		if (bp == 0) { 
			ToFirst(); 
			break; 
		} 
		SetDot(bp); 
		to_word(1); 
		if (curline == save.p_line && curchar >= save.p_char) { 
			SetDot(bp); 
			goto onceagain; 
		} 
	} 
} 
 
Eos() 
{ 
	int	num = exp; 
	Bufpos	*bp; 
 
	exp = 1; 
	while (num-- && (bp = dosearch(REsent, 1, 1))) 
		SetDot(bp); 
	if (bp == 0) 
		ToLast(); 
	else 
		to_word(1); 
} 
 
ForWord() 
{ 
	register char	c; 
	register int	num = exp; 
 
	exp = 1; 
	while (--num >= 0) { 
		to_word(1); 
		while ((c = linebuf[curchar]) != 0 && isword(c)) 
			curchar++; 
		if (eobp()) 
			break; 
	} 
	this_cmd = 0;	/* Semi kludge to stop some unfavorable behavior */ 
} 
 
BackWord() 
{ 
	register int	num = exp; 
	register char	c; 
 
	exp = 1; 
	while (--num >= 0) { 
		to_word(-1); 
		while (!bolp() && (c = linebuf[curchar - 1], isword(c))) 
			--curchar; 
		if (bobp()) 
			break; 
	} 
	this_cmd = 0; 
} 
 
/* End of Window */ 
 
Eow() 
{ 
	SetLine(next_line(curwind->w_top, SIZE(curwind) - 1 - 
			min(SIZE(curwind) - 1, exp - 1))); 
	if (!exp_p) 
		Eol(); 
} 
 
/* Beginning of Window */ 
 
Bow() 
{ 
	SetLine(next_line(curwind->w_top, min(SIZE(curwind) - 1, exp - 1))); 
} 
 
 
