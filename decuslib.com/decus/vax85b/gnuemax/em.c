/*R
 * Unix 6 
 * Editor 
 * 
 */ 
 
/* Mods: George Coulouris (QMC) 
 *	March, 76 
 * 
 *	prompts (suppress with `-p' flag) 
 *	", %, &, to display a screen full of context 
 *	`x' - as `s' but interactive 
 *	`n' flag when appended to `s' or `x' commands 
 *		prints number of replacements 
 *	% == current file name in ! commands 
 *	!! == repeat the last ! command you executed 
 *	-e flag == "elfic" mode :- 
 *		no "w", "r\n" commands, auto w before q 
 *	`o' command for text input with local editing via control keys 
 *	`b' to set a threshold for automatic line breaks in `o' mode. 
 *	`h' displays a screen full of help with editor commands 
 *		(the help is in /usr/lib/emhelp) 
 * 
 * Ian Johnstone (AGSM) 
 *	September, 77 
 * 
 *	When receive terminate (14) signal, output current file 
 *	to "saved.file" and tell user about this - then exit. 
 * 
 * Greg Rose 
 *	77/09/12 
 */ 
#define	AUTOW 
/*	tin command - writes files every n minutes 
 *	A pair of bells are output to warn the user. 
 *	You need the clktim sys call for this one - else remove the define 
 *	& replaced by old pattern 
 * 
 * Craig McGregor (CSU) 
 *	77/12/23 
 * 
 *	% in shell command may be escaped with \ 
 *	and evaluate it when executed 
 *	correct o command rubouts; ignore nulls; use bell for errors 
 *	fix to allow x command in a global command 
 *	mods to keep labels on altered lines 
 *	The `o' command modified to use the LCASE bit 
 *	before converting "\b" to upper-case B. 
 *	-l flag - use l instead of p for default 
 *	o mode commands ^p and ^t added 
 * 
 * Kevin Hill 
 *	April - May, 78 
 * 
 *	j and jo commands added 
 *	fixes to remember various strings on error 
 *	print out line number of error if coming from file 
 *	verbose errors - assisted by IanJ - use '-s' to suppress 
 *	when '-e' (elfic) used, suppress empty file and match file messages 
 *	allow user to specify screen size (min. 4) 
 *	print out number of nulls thrown away, and add \n to last line 
 *	if necessary 
 *	address before , defaults to 1; address after defaults to $ 
 *	global pattern default is every line 
 *	may change directory 
 *	z command to print out interesting things 
 *	allowed for ranges: [a-z] etc. 
 *	o mode fixes - various 
 *	improve substitute algorithm 
 *	shell escape - ! is replaced by last escape 
 *	various elfic fixes 
 *	fix sloshing so that it is now correct! 
 *	wx option - make executable 
 *	reject junk files: objects, packed, etc. 
 * 
 * David Milway 
 *	May, 78 
 * 
 *	wa option - append if file present 
 * 
 * Russell Robinson & Kevin Hill 
 *	Feb, 80 
 * 
 *	query if try to quit without writing 
 * 
 * Richard Bullock 
 *	Oct, 80 
 * 
 *	e with no args re-edits same file 
 *	also fixed up 'e <space>' command so it does not 
 *		delete the buffer 
 * Chris Maltby 
 *	Dec, 80 
 * 
 *	improved signal handling 
 */ 
 
#include <signal.h> 
 
#define	FNSIZE	64 
#define	LBSIZE	512 
#define	INITLINES	200	/* initial number of lines */ 
#define	ESIZE	128 
#define	GBSIZE	256 
#define	NBRA	9 
#define	EOF	-1 
#define	UNIXBUFL 100		/* must be < LBSIZE */ 
 
#define	CBRA	1 
#define	CCHR	2 
#define	CDOT	4 
#define	CCL	6 
#define	NCCL	8 
#define	CDOL	10 
#define	CEOT	11 
#define	CKET	12 
 
#define	STAR	01 
 
#define	READ	0 
#define	WRITE	1 
 
/* screen characteristics */ 
#define	LINES	24 
#define	LENGTH	80 
#define	PROMPT	';' 
#define	CONFIRM	'.' 
 
#define	OPEN	'/' 
#define	BELL	'\007' 
#define	ESCAPE	'\033' 
#define	SPACE	' ' 
#define	BACKSL	'\\' 
#define	RUBOUT	'\177' 
#define	CTRLA	'\01' 
#define	CTRLB	'\02' 
#define	CTRLC	'\03' 
#define	CTRLD	'\04' 
#define	CTRLE	'\05' 
#define	CTRLF	'\06' 
#define	CTRLG	'\07'		/* == bell */ 
#define	CTRLH	'\010'		/* == backspace */ 
#define	CTRLI	'\011'		/* == TAB */ 
#define	CTRLL	'\014'		/* == form-feed */ 
#define	CTRLP	'\020' 
#define	CTRLR	'\022' 
#define	CTRLT	'\024' 
#define	CTRLV	'\026' 
#define	CTRLW	'\027' 
#define	CTRLX	'\030' 
#define	CTRLZ	'\032' 
 
/* 
 *	popular error messages 
 */ 
 
char e_write[]	= "Write error - file truncated"; 
char e_syn[]	= "Syntax"; 
char e_match[]	= "No match"; 
char e_elfic[]	= "Elfic"; 
char e_addr[]	= "Address";	/* illegal address */ 
char e_reg[]	= "Reg name";	/* invalid register name */ 
char e_file[]	= "File"; 
char e_linsz[]	= "Line size"; 
char e_buf[]	= "Buffer empty"; 
char e_unix[]	= "Command too long";	/* shell escape */ 
char e_undef[]	= "&, %, or subexpression undefined"; 
char e_int[]	= "Interrupt"; 
 
short	peekc; 
char	lastc; 
char	savedfile[FNSIZE]; 
char	file[FNSIZE]; 
char	linebuf[LBSIZE]; 
char	rhsbuf[LBSIZE/2]; 
char	expbuf[ESIZE+4]; 
short	circfl; 
short	*zero; 
short	*dot; 
short	*dol; 
short	*endcore; 
short	*fendcore; 
short	*addr1; 
short	*addr2; 
char	genbuf[LBSIZE]; 
long	count; 
char	*nextip; 
char	*linebp; 
short	ninbuf; 
short	nline; 
short	io; 
short	pflag; 
int	(*onhup)(); 
int	(*onintr)(); 
int	(*onquit)(); 
short	vflag;	/* 0 ==> e/ed; 1 ==> em; -1 ==> silence */ 
short	listf; 
short	col; 
char	*globp; 
short	tfile; 
short	tline; 
char	*tfname; 
char	*loc1; 
char	*loc2; 
char	*locs; 
char	ibuff[512]; 
short	iblock; 
char	obuff[512]; 
short	oblock; 
short	ichanged; 
short	nleft; 
short	names[26]; 
char	*braslist[NBRA]; 
char	*braelist[NBRA]; 
 
short	elfic;			/* true if "elfic" (-e) flag */ 
short	firstfrig;		/* 0 -> first time in em; -1 -> initial file read */ 
char	unixbuffer[UNIXBUFL]; 
short	xflag;			/* used in 'xchange' command */ 
 
short	oflag; 
char	*lnp, *gnp, *brp; 
#include <termio.h> 
struct termio termio; 
#include	<setjmp.h> 
jmp_buf	setexit; 
 
char	lflag; 
 
#ifdef	AUTOW 
unsigned	writetime; 
short	justwrit;	/* for the auto-write */ 
short	delaywrite; 
short	writewaiting;	/* kludge for 'o' mode, since it flogs a buffer */ 
#endif	AUTOW 
 
short	joinonly;	/* for join-only option */ 
short	istty;		/* set when input commands are from a tty */ 
short	linenr;		/* line number of each command as taken from a file */ 
short	currlnnr;	/* for the current command line number */ 
short	sflag;		/* for '-s' option */ 
short	lines;			/* number of lines on screen for ", %, and & */ 
short	oldline, *thestart; 
char	*ssp, *slp;	/* pointers to genbuf and linebuf */ 
short	nbra;		/* number of subexpressions */ 
short	chkflg;		/* flag for reading funny files */ 
short	changed;	/* non-zero when any changes made since last 'w' */ 
 
extern char *getline(); 
extern char *getblock(); 
extern short *address(); 
 
main(argc, argv) 
char **argv; 
{ 
	register char *p1, *p2; 
	register short eflag; 
	extern short interrupt(); 
	extern  terminate(); 
 
#ifdef	AUTOW 
	extern  catchclock(); 
 
	signal(SIGALRM, catchclock); 
#endif	AUTOW 
	onquit = signal(SIGQUIT, SIG_IGN); 
	signal(SIGTERM, terminate); 
	onhup = signal(SIGHUP, SIG_IGN); 
	if (onhup == SIG_IGN) 
		signal(SIGHUP, terminate); 
	if ((*argv)[1] == 'm') vflag = 1; 
	elfic = 0; 
	argv++; 
	if (argc > 1 && **argv=='-') 
	{ 
		p1 = &(*argv)[1]; 
		while (*p1 != '\0') 
			switch (*p1++) 
			{ 
 
		    case 'e':	elfic = 1; 
				break; 
 
		    case 'l':	lflag++; 
				break; 
 
		    case 's':	sflag++; 
				break; 
			} 
		if ((*argv)[1] == '\0') vflag = -1; 
		argv++; 
		argc--; 
	} 
	if (argc>1) 
	{ 
		p1 = *argv; 
		p2 = savedfile; 
		while (*p2++ = *p1++); 
		globp = "r"; 
	} 
	fendcore = (short *)sbrk(0); 
	if (vflag > 0) putnls("Editor"); 
	istty = isatty(0); 
	eflag = 0; 
	setjmp(setexit);	/* catch init errors */ 
	if (eflag) exit(1); 
	eflag = 1; 
	lines = LINES; 
	linenr = 1; 
	tfile = -1; 
	init(); 
	if (signal(SIGINT, SIG_IGN) != SIG_IGN) signal(SIGINT, interrupt); 
	setjmp(setexit); 
	commands(vflag); 
	unlink(tfname); 
	exit(0); 
} 
 
terminate(sig) 
{ 
	signal(SIGTERM, SIG_IGN); 
	signal(SIGHUP, SIG_IGN); 
	if (dol != zero) 
	{ 
		addr2 = 0; 
		writeout("saved.file"); 
		if (sig == SIGTERM) 
			putnls("Terminated - tmp file written to \"saved.file\"");  
	} 
	unlink(tfname); 
	exit(1); 
} 
 
#ifdef	AUTOW 
catchclock() 
{ 
	register short *ad1, *ad2; 
	long	csav; 
	char	bell =	BELL; 
 
	if (delaywrite) { writewaiting++; return; } 
	signal(SIGALRM, catchclock); 
	justwrit++; 
	if (dol != zero)		/* empty file */ 
	{ 
		onintr = signal(SIGINT, SIG_IGN); 
		ad1 = addr1; 
		ad2 = addr2; 
		addr2 = 0; 
		csav = count; 
		write(2, &bell, 1); 
		writeout(-3); 
		write(2, &bell, 1); 
		addr1 = ad1; 
		addr2 = ad2; 
		count = csav; 
		signal(SIGINT, onintr); 
	} 
	alarm(writetime); 
} 
#endif	AUTOW 
 
commands(prompt) 
{ 
	short getfile(), gettty(); 
	register short *a1; 
	register char c; 
	register char *p; 
	short r, n; 
	short fd; 
 
	for (;;) 
	{ 
 
		if (prompt != -2) currlnnr = linenr;	/* i.e., not global */ 
 
		if (pflag) 
		{ 
			pflag = 0; 
			addr1 = addr2 = dot; 
			goto print; 
		} 
		if (prompt>0 && globp==0) putch(PROMPT); 
		addr1 = addr2 = 0; 
		xflag = 0; 
#ifdef	AUTOW 
		if (writewaiting) 
		{ 
			delaywrite = writewaiting = 0; 
			catchclock(); 
			justwrit = 0; 
		} 
#endif	AUTOW 
		a1 = address(); 
		if ((c = getchar()) == ';' && a1) 
		{ dot = addr1 = addr2 = a1; a1 = address(); c = getchar(); } 
		if (c == ',') 
		{ 
			if (a1 == 0) 
				if (dol == zero) error(e_buf, 0); 
				else addr1 = zero + 1; 
			else addr1 = a1; 
			if ((a1 = address()) == 0) addr2 = dol; 
			else addr2 = a1; 
			c = getchar(); 
		} 
		else addr2 = a1; 
		if (addr1==0) addr1 = addr2; 
		if (c>='A' && c<='Z') c |= 040; 
		switch(c) 
		{ 
	 
	    case 'a':	setdot(); 
			newline(); 
			changed++; 
			if (append(gettty, addr2) == 0) changed--; 
			continue; 
	 
	    case 'c':	setdot(); 
			newline(); 
			changed++; 
			fd = *addr1;		/* borrow 'fd' */ 
			delete(); 
			if (append(gettty, addr1-1) > 0) retag(*addr1, fd); 
			continue; 
	 
	    case 'd':	setdot(); 
			newline(); 
			changed++; 
			delete(); 
			continue; 
	 
	    case 'e':	if (elfic) error(e_elfic, 0); 
			setnoaddr(); 
			peekc = getchar(); 
			if (peekc != ' ' && peekc != '\n') error(e_syn, 0); 
			query(1); 
			firstfrig = -1; 
			filename(1); 
			init(); 
			addr2 = zero; 
			goto caseread; 
	 
	    case 'f':	setnoaddr(); 
			if ((c = getchar()) != '\n') 
			{ 
				peekc = c; 
				if (elfic && *savedfile) error(e_elfic, 0); 
				filename(1); 
			} 
			putnls(savedfile); 
			continue; 
	 
	    case 'g':	global(1); 
			continue; 
	 
	    case 'h':	newline(); 
			if ((fd = open("/usr/lib/emhelp", 0)) < 0) 
			{ 
				putnls("/usr/lib/emhelp not found"); 
				continue; 
			} 
			while ((n = read(fd, linebuf, 512)) > 0) write(1, linebuf, n); 
			close(fd); 
			continue; 
	 
	    case 'i':	setdot(); 
			nonzero(); 
			newline(); 
			changed++; 
			if (append(gettty, addr2-1) == 0) changed--; 
			continue; 
	 
	    case 'j':	if (addr2 == 0) addr2 = (addr1 = dot) + 1; 
			if (addr1 > addr2) error(e_addr, 0); 
			if (addr1 == addr2) addr2++; 
			nonzero(); 
			joinonly = 0; 
			if ((peekc = getchar()) == 'o' || peekc == 'O') 
			{ 
				joinonly++; 
				peekc = 0; 
			} 
			newline(); 
			join(); 
			changed++; 
			continue; 
	 
	    case 'k':	if ((c = getchar()) >= 'A' && c <= 'Z') 
				c |= 040; 
			else if (c < 'a' || c > 'z') error(e_reg, 0); 
			newline(); 
			setdot(); 
			nonzero(); 
			names[c-'a'] = *addr2 | 01; 
			continue; 
	 
	    case 'm':	move(0); 
			continue; 
	 
	    case '\n':	if (addr2==0) addr2 = dot+1; 
			addr1 = addr2; 
			if (lflag) listf++; 
			goto print; 
 
	    case 'l':	listf++; 
	    case 'p':	newline(); 
	    print:	setdot(); 
			nonzero(); 
			a1 = addr1; 
			do putnls(getline(*a1++)); while (a1 <= addr2); 
			dot = addr2; 
			listf = 0; 
			continue; 
	 
	    case 'o':	setdot(); 
#ifdef	AUTOW 
			delaywrite++; 
#endif	AUTOW 
			op(globp); 
#ifdef	AUTOW 
			delaywrite = 0; 
#endif	AUTOW 
			continue; 
	 
	    case 'q':	setnoaddr(); 
			newline(); 
			if (elfic && dol != zero) writeout(0); 
			else query(0); 
			unlink(tfname); 
			exit(0); 
	 
	    case 'r': 
			filename(0); 
	    caseread: 
#ifdef	AUTOW 
			delaywrite++; 
#endif	AUTOW 
			if (firstfrig == -1)	/* from 'e' command */ 
				changed = 0; 
			if ((io = open(file, 0)) < 0) 
			{ 
				lastc = '\n'; 
				error(file, -1);	/* -1 for open error */ 
			} 
			setall(); 
			ninbuf = 0; 
			if (firstfrig == -1)	/* first read of this file */ 
				firstfrig = 1; 
			else changed++; 
			if (append(getfile, addr2) == 0 && changed) 
				changed--; 
			exfile(); 
			if (listf && vflag >= 0)		/* borrows 'listf' */ 
			{ 
				putchar('('); 
				if ((count = listf) > 0) putd(); 
				else puts("many"); 
				putnls(" null(s) stripped from file)"); 
			} 
			listf = 0; 
#ifdef	AUTOW 
			delaywrite = 0; 
#endif	AUTOW 
			continue; 
	 
	    case 'x':	xflag = 1; 
	    case 's':	setdot(); 
			nonzero(); 
			substitute(globp); 
			changed++; 
			xflag = 0; 
			continue; 
	 
#ifndef	AUTOW 
	    case 't': 
#else 
	    case 't':	if ((peekc = getchar()) == 'i' || peekc == 'I') 
			{ 
				fd = 0; peekc = 0; 
				if (*savedfile == 0) error(e_file, 0); 
				while (((c = getchar()) >= '0') && c <= '9') 
					fd = fd * 10 + c - '0'; 
				if (c != '\n') error(e_syn, 0); 
				writetime = fd * 60; 
				alarm(writetime); 
				continue; 
			} 
#endif	AUTOW 
			move(1); 
			continue; 
 
	    case 'v':	global(0); 
			continue; 
	 
	    case 'w':	if (elfic) error(e_elfic, 0); 
#ifdef	AUTOW 
			delaywrite++; 
#endif	AUTOW 
			if ((peekc = getchar()) == 'a' || peekc == 'A') 
			{ 
				peekc = 0; 
				writeout(-1); 
			} 
			else if (peekc == 'x' || peekc == 'X') 
			{ 
				peekc = 0; 
				writeout(-2); 
			} 
			else writeout(0); 
#ifdef	AUTOW 
			delaywrite = 0; 
#endif	AUTOW 
			continue; 
	 
	    case 'z':	setnoaddr(); 
			newline(); 
			printstrings(); 
			continue; 
 
	    case '"':	setdot(); 
			fetchnr(); 
			newline(); 
			if (addr1 < zero) error(e_addr, 0); 
			dot = addr1; 
			if (dot == dol) error(e_addr, 0); 
			addr1 = dot + 1; 
			addr2 = dot + lines - 1; 
			if (addr2 > dol) addr2 = dol; 
	    outlines:	if (pflag == 0 && lflag) listf++; 
			for (a1 = addr1; a1 <= addr2; a1++) 
				putnls(getline(*a1)); 
			dot = addr2; 
			pflag = listf = 0; 
			continue; 
	 
	    case '&':	setdot(); 
			fetchnr(); 
			newline(); 
			nonzero(); 
			dot = addr1; 
			addr1 = dot - (lines-2); 
			addr2 = dot; 
			if (addr1 <= zero) addr1 = zero + 1; 
			goto outlines; 
	 
	    case '%': 
			fetchnr(); 
			newline(); 
			setdot(); 
			nonzero(); 
			dot = addr1; 
			addr1 = dot - (lines - 2) / 2; 
			addr2 = addr1 + lines - 3; 
			if (addr1 <= zero) addr1 = zero + 1; 
			if (addr2 > dol) addr2 = dol; 
			if (pflag == 0 && lflag) listf++; 
			for (a1 = addr1; a1 <= addr2; a1++) 
			{ 
				putnls(getline(*a1)); 
				if (a1 == dot) screensplit(); 
			} 
			pflag = listf = 0; 
			continue; 
	 
	    case '>':	if (vflag > 0) vflag = 0; 
			newline(); 
			longjmp(setexit, 0); 
	 
	    case '<':	vflag = 1; 
			newline(); 
			longjmp(setexit, 0); 
	 
	    case '=':	setall(); 
			newline(); 
			count = (addr2-zero) & 077777; 
			putd(); 
			putchar('\n'); 
			continue; 
	 
	    case '!':	setnoaddr(); 
			callunix(); 
			continue; 
	 
	    case (char)EOF:	if (prompt == -2 || ! istty) return; 
			continue; 
	 
		} 
		error(e_syn, 0); 
	} 
} 
 
writeout(filen) 
char *filen; 
{ 
	register char *p1, *p2; 
	register char *flag = 0; 
	extern write(); 
 
	if ((int)filen == -1 || (int)filen == -2) { flag = filen; filen = 0; } 
	setall(); 
	nonzero(); 
	if (filen == 0) 
	{ 
		if (elfic) 
		{ 
			p1 = savedfile; 
			if (*p1 == 0) error(e_file, 0); 
			p2 = file; 
			while (*p2++ = *p1++); 
		} 
		else filename(0); 
		filen = file; 
	} 
	if ((int)flag == -1 && (io = open(filen, 1)) >= 0) lseek(io, (long)0, 2); 
#ifdef	AUTOW 
	else if ((int)filen == -3) 
	{ 
		if ((io = creat(savedfile, 0666)) < 0) 
		{ 
			putnls("Auto-write - cannot write file ?"); 
			return; 
		} 
	} 
#endif 
	else if ((io = creat(filen, (int)flag == -2 ? 0777 : 0666)) < 0) 
		error(filen, 1); 
	{ 
		int fred; 
 
		fred = changed; 
		changed = 1; 
		putfile(); 
		exfile(); 
		if (addr1 == zero+1 && addr2 == dol) 
			changed = 0;	/* only reset if writing the lot */ 
					/* and not interrupted */ 
		else if (chkflg == 1) 
			changed = fred; 
	} 
} 
 
short * 
address() 
{ 
	register short *a1, minus, c; 
	short n; 
	short relerr; 
 
	minus = 0; 
	a1 = 0; 
	for (;;) 
	{ 
		c = getchar(); 
		if ('0'<=c && c<='9') 
		{ 
			n = 0; 
			do 
			{ 
				n *= 10; 
				n += c - '0'; 
			} while ((c = getchar())>='0' && c<='9'); 
			peekc = c; 
			if (a1==0) a1 = zero; 
			if (minus<0) n = -n; 
			a1 += n; 
			minus = 0; 
			continue; 
		} 
		relerr = 0; 
		if (a1 || minus) relerr++; 
		switch(c) 
		{ 
 
	    case ' ': 
	    case '\t':	continue; 
	 
	    case '+':	minus++; 
			if (a1==0) a1 = dot; 
			continue; 
 
	    case '-': 
	    case '^':	minus--; 
			if (a1==0) a1 = dot; 
			continue; 
	 
	    case '?': 
	    case '/':	compile(c); 
			a1 = dot; 
			for (;;) 
			{ 
				if (c=='/') { a1++; if (a1 > dol) a1 = zero; } 
				else if (--a1 < zero) a1 = dol; 
				if (execute(0, a1)) break; 
				if (a1==dot) error(e_match, 0); 
			} 
			break; 
	 
	    case '$':	a1 = dol; 
			break; 
	 
	    case '.':	a1 = dot; 
			break; 
 
	    case '\'':	if ((c = getchar()) >= 'A' && c <= 'Z') 
				c |= 040; 
			else if (c < 'a' || c > 'z') error(e_reg, 0); 
			c -= 'a'; 
			for (a1 = zero+1; a1 <= dol; a1++) 
				if (names[c] == (*a1|01)) break; 
			break; 
	 
	    default:	peekc = c; 
			if (a1==0) return(0); 
			a1 += minus; 
			if (a1<zero || a1>dol) error(e_addr, 0); 
			return(a1); 
		} 
		if (relerr) error(e_syn, 0); 
	} 
} 
 
setdot() 
{ 
	if (addr2 == 0) addr1 = addr2 = dot; 
	if (addr1 > addr2) error(e_addr, 0); 
} 
 
setall() 
{ 
	if (addr2==0) 
	{ 
		addr1 = zero+1; 
		addr2 = dol; 
		if (dol==zero) addr1 = zero; 
	} 
	setdot(); 
} 
 
setnoaddr() 
{ 
	if (addr2) error(e_syn, 0); 
} 
 
nonzero() 
{ 
	if (addr1<=zero || addr2>dol) error(e_addr, 0); 
} 
 
newline() 
{ 
	register short c; 
 
	if ((c = getchar()) == '\n') return; 
	if (c=='p' || c=='l' || c=='P' || c=='L') 
	{ 
		pflag++; 
		if (c=='l' || c=='L') listf++; 
		if (getchar() == '\n') return; 
	} 
	error(e_syn, 0); 
} 
 
fetchnr() 
{ 
	register short n; 
	register char c; 
 
	if ((c = getchar()) >= '0' && c <= '9') 
	{ 
		n = 0; 
		do 
		{ 
			n *= 10; 
			n += c - '0'; 
		} while ((c = getchar()) >= '0' && c <= '9'); 
		if (n < 4) error(e_syn, 0);	/* min. is 4 lines */ 
		lines = n; 
	} 
	peekc = c; 
} 
 
retag (newlabel, oldlabel) 
{ 
	register short	ol, nl; 
	register short	*ip; 
 
	ol = oldlabel | 01; 
	nl = newlabel | 01; 
	for (ip = &names[0]; ip < &names[26]; ip++) 
		if (*ip == ol) *ip = nl; 
} 
 
filename(flag) 
{ 
	register char *p1, *p2; 
	register short c; 
 
	chkflg = 1; 
	count = 0; 
	c = getchar(); 
	if (c=='\n' || c==EOF) 
	{ 
		if (elfic && firstfrig) error(e_elfic, 0); 
		if (firstfrig == 0) 
			firstfrig = -1; 
		p1 = savedfile; 
		if (*p1==0) error(e_file, 0); 
		p2 = file; 
		while (*p2++ = *p1++); 
		chkflg = -1; 
		return; 
	} 
	if (c!=' ') error(e_syn, 0); 
	while ((c = getchar()) == ' '); 
	if (c=='\n') error(e_syn, 0); 
	p1 = file; 
	do *p1++ = c; while ((c = getchar()) != '\n'); 
	*p1 = 0; 
	if (flag || savedfile[0]==0) 
	{ 
		p1 = savedfile; 
		p2 = file; 
		while (*p1++ = *p2++); 
	} 
} 
 
exfile() 
{ 
	close(io); 
	io = -1; 
#ifndef	AUTOW 
	if (vflag >= 0) 
	{ 
#else 
	if (vflag>=0 && !justwrit) 
	{ 
#endif	AUTOW 
		putd(); 
		putchar('\n'); 
	} 
} 
 
short 
interrupt() 
{ 
	signal(SIGINT, interrupt); 
	putchar('\n'); 
	lastc = '\n'; 
	error(e_int, 0); 
} 
 
query(ecom) 
register short ecom; 
{ 
	register char *cp; 
	short chr; 
	static char sure[] = "Are you sure (y-n)? "; 
 
	if (! changed || elfic || ! istty || sflag) return; 
	if (ecom)		/* querying an 'e' command */ 
	{ 
		cp = rhsbuf; 
		*cp++ = 0;	/* 'empty' it */ 
		while ((*cp = getchar()) != '\n') 
		{ 
			if (*cp == (char)EOF) error(e_syn, 0); 
			if (++cp >= &rhsbuf[sizeof rhsbuf-2]) error(e_linsz, 0); 
		} 
		*++cp = 0; 
		globp = 0; 
	} 
	/* 
	 * must write message out directly as 'puts' 
	 * buffers output until newline char or 64 chars 
	 */ 
	write(1, sure, sizeof sure - 1); 
	if (((chr = getchar()) | 040) != 'y' || (chr = getchar()) != '\n') 
	{ 
		if (chr != '\n') 
			while (getchar() != '\n'); 
		longjmp(setexit, 0); 
	} 
	if (ecom == 0) return;	/* querying 'q' command */ 
	globp = &rhsbuf[1]; 
} 
 
error(s, syserr)	/* syserr == -1 for open err; non-zero for sys err */ 
register char *s; 
{ 
	register short c; 
	extern	char *sys_errlist[]; 
	extern	short   sys_nerr; 
	extern	short   errno; 
 
	listf = count = 0; 
	pflag = (syserr != -1 && s != e_match);	/* flog 'pflag' as cleared later anyway */ 
	if (! istty) 
	{ 
		if (vflag >= 0 || pflag) 
		{ count = currlnnr; putd(); putchar(' '); } 
		if (pflag) lseek(0,(long)0,2); 
	} 
	if (sflag) { if (s == e_match) putnls("??"); else putnls("?"); } 
	else if (vflag >= 0 || pflag) 
	{ 
		if (s == e_addr && dol==zero) s = e_buf; 
		puts(s); 
		if (syserr && errno < sys_nerr) 
		{ 
			puts(": "); 
			puts(sys_errlist[errno]); 
		} 
		putnls(" ?"); 
	} 
 
	pflag = 0; 
	if (globp) lastc = '\n'; 
	globp = 0; 
	peekc = lastc; 
	while ((c = getchar()) != '\n' && c != EOF); 
	if (io > 0) { close(io); io = -1; } 
	longjmp(setexit, 0); 
} 
 
getchar() 
{ 
#ifdef	AUTOW 
	register short count; 
 
#endif 
	if (lastc=peekc) { peekc = 0; return(lastc); } 
	if (globp) 
	{ 
		if ((lastc = *globp++) != 0) return(lastc); 
		globp = 0; 
		return(EOF); 
	} 
#ifndef	AUTOW 
	if (read(0, &lastc, 1) <= 0) return(lastc = EOF); 
#else 
	while ((count = read(0, &lastc, 1)) <= 0) 
		if (count < 0 && justwrit) justwrit = 0; 
		else if (! istty) return(lastc = EOF); 
#endif	AUTOW 
	lastc &= 0177; 
	if (! istty && lastc=='\n') linenr++; 
	return(lastc); 
} 
 
short 
gettty() 
{ 
	register short c; 
	char *gf; 
	register char *p; 
 
	p = linebuf; 
	gf = globp; 
	while ((c = getchar()) != '\n') 
	{ 
		if (c==EOF) { if (gf) peekc = c; return(c); } 
		if ((c &= 0177) == 0) continue; 
		*p++ = c; 
		if (p >= &linebuf[LBSIZE-2]) error(e_linsz, 0); 
	} 
	*p++ = 0; 
	if (linebuf[0]=='.' && linebuf[1]==0) return(EOF); 
	return(0); 
} 
 
checkfile(fstwd) 
register short fstwd; 
{ 
	static short list[] = 
	{ 
		0404,			/* pascal obj */ 
		0407, 0410, 0411,	/* objects */ 
		017437,			/* packed - 70 */ 
		017037,			/* packed - VAX */ 
		0177545,		/* archives */ 
		0,			/* end-of-list */ 
	}; 
	register short  *lp = list; 
 
	while (*lp) 
		if (*lp++ == fstwd) 
		{ 
			putnls("Illegal file type"); 
			return 1; 
		} 
	return 0; 
} 
 
short 
getfile() 
{ 
	register short c; 
	register char *lp, *fp; 
 
	lp = linebuf; 
	fp = nextip; 
	do 
	{ 
		if (--ninbuf < 0) 
		{ 
			if ((ninbuf = read(io, genbuf, LBSIZE)-1) < 0) 
				if (lp == linebuf) return(EOF); 
				else 
				{ 
					if (vflag >= 0) putnls("No `\\n' on last line - `\\n' added."); 
					if (lp >= &linebuf[LBSIZE]) { lastc = '\n'; error(e_linsz, 0); } 
					*lp = 0; 
					return(0); 
				} 
			fp = genbuf; 
			if (chkflg) 
			{ 
				if (checkfile(*(short *)genbuf)) 
				{ 
					if (chkflg < 0) savedfile[0] = 0; 
					return EOF; 
				} 
				chkflg = 0; 
			} 
		} 
		if (lp >= &linebuf[LBSIZE]) { lastc = '\n'; error(e_linsz, 0); } 
		if ((*lp++ = c = *fp++ & 0177) == 0) 
		{ 
			lp--; 
			if (listf >= 0) listf++; 
			continue; 
		} 
		count++; 
	} while (c != '\n'); 
	*--lp = 0; 
	nextip = fp; 
	return(0); 
} 
 
putfile() 
{ 
	short *a1; 
	register char *fp, *lp; 
	register short nib; 
 
	nib = 512; 
	fp = genbuf; 
	a1 = addr1; 
	do 
	{ 
		lp = getline(*a1++); 
		for (;;) 
		{ 
			if (--nib < 0) 
			{ 
				nib = fp - genbuf; 
				if (write(io, genbuf, nib) != nib) 
					error(e_write, 1); 
				nib = 511; 
				fp = genbuf; 
			} 
			count++; 
			if ((*fp++ = *lp++) == 0) { fp[-1] = '\n'; break; } 
		} 
	} while (a1 <= addr2); 
	nib = fp - genbuf; 
	if (write(io, genbuf, nib) != nib) error(e_write, 1); 
} 
 
append(f, a) 
short *a; 
short (*f)(); 
{ 
	register short *a1, *a2, *rdot; 
	short tl; 
 
	nline = 0; 
	dot = a; 
	while ((*f)() == 0) 
	{ 
		if (dol >= endcore) 
		{ 
			if ((short *)sbrk(1024) == (short *)-1) error("Too many lines", 0); 
			endcore += 1024/(sizeof *endcore); 
		} 
		tl = putline(linebuf); 
		nline++; 
		a1 = ++dol; 
		a2 = a1+1; 
		rdot = ++dot; 
		while (a1 > rdot) *--a2 = *--a1; 
		*rdot = tl; 
	} 
	return(nline); 
} 
 
callunix() 
{ 
	register  pid; 
	register rpid; 
	register char *lp, *fp; 
 
	pid = 0; 
	lp = linebuf; 
	for (;;) 
	{ 
		if ((*lp = getchar()) == '\n') break; 
		if (*lp == '!') 
		{ 
			fp = unixbuffer; 
			while (*lp++ = *fp++) 
				if (lp >= &linebuf[UNIXBUFL]) error(e_unix, 0); 
			lp--; 
			pid++; 
			continue; 
		} 
		if (*lp == BACKSL) 
		{ 
			fp = (char *)getchar(); 
			if ((char)fp == '\n' || (char)fp == '!') 
				*lp = (char)fp; 
			else 
				*++lp = (char)fp; 
		} 
		if (++lp >= &linebuf[UNIXBUFL]) error(e_unix, 0); 
	} 
	*lp = '\0'; 
	fp = unixbuffer; 
	lp = linebuf; 
	while (*fp++ = *lp++); 
	fp = unixbuffer; 
	lp = linebuf; 
	while (*lp = *fp++) 
		if (*lp == BACKSL) 
		{ 
			if ((*++lp = *fp++) == '%') *--lp = '%'; 
			lp++; 
		} 
		else if (*lp++ == '%') 
		{ 
			pid++; lp--; 
			for (rpid = 0; *lp++ = savedfile[rpid]; rpid++) 
				if (lp >= &linebuf[LBSIZE]) error(e_unix, 0); 
			lp--; 
		} 
	if (pid && vflag >= 0) { putchar('!'); putnls(linebuf); } 
#ifdef	AUTOW 
	alarm(0); 
#endif 
	if ((pid = fork()) == 0) 
	{ 
		close(tfile); 
		signal(SIGHUP, onhup); 
		signal(SIGQUIT, onquit); 
		execl("/bin/sh", "sh", "-c", linebuf, 0); 
		exit(1); 
	} 
	if (pid == -1) error("Fork", 1);	/* fork failed */ 
	onintr = signal(SIGINT, SIG_IGN); 
#ifndef	AUTOW 
	while ((rpid = wait((int *)0)) != pid && rpid != -1); 
#else 
	while ((rpid = wait((int *)0)) != pid); 
	alarm(writetime); 
#endif	AUTOW 
	signal(SIGINT, onintr); 
	if (vflag >= 0) 
		putnls("!"); 
} 
 
join() 
{ 
	register char *bp, *lp; 
	register short nl; 
	short tl, toggle, *lptr; 
 
	lp = linebuf; toggle = 1; 
	for (lptr = addr1; lptr <= addr2; lptr++) 
	{ 
		tl = *lptr; 
		bp = getblock(tl,READ); nl = nleft; tl &= ~0177; 
		while (*lp = *bp++) 
		{ 
			if (toggle || joinonly) lp++; 
			else if (*lp != ' ' && *lp != CTRLI) { toggle++; lp++; } 
			if (--nl == 0) { bp = getblock(tl+=0200,READ); nl = nleft; } 
			if (lp >= &linebuf[LBSIZE-2]) error(e_linsz, 0); 
		} 
		if (lp >= &linebuf[LBSIZE-1]) error(e_linsz, 0); 
		while (--lp >= linebuf && (*lp==' ' || *lp==CTRLI) && ! joinonly); 
		lp++; 
		if (! joinonly) *lp++ = ' '; 
		toggle = 0; 
	} 
	if (! joinonly) lp[-1] = 0; 
	retag(*addr1 = putline(linebuf), *addr1); 
	addr1++; 
	if (addr2 == dol) delete(); else { delete(); dot--; } 
} 
 
delete() 
{ 
	register short *a1, *a2, *a3; 
 
	nonzero(); 
	a1 = addr1; 
	a2 = addr2+1; 
	a3 = dol; 
	dol -= a2 - a1; 
	do *a1++ = *a2++; while (a2 <= a3); 
	a1 = addr1; 
	if (a1 > dol) a1 = dol; 
	dot = a1; 
} 
 
char * 
getline(tl) 
{ 
	register char *bp, *lp; 
	register short nl; 
 
	lp = linebuf; 
	bp = getblock(tl, READ); 
	nl = nleft; 
	tl &= ~0177; 
	while (*lp++ = *bp++) 
		if (--nl == 0) 
		{ 
			bp = getblock(tl+=0200, READ); 
			nl = nleft; 
		} 
	return(linebuf); 
} 
 
putline(buffer) 
char	buffer[]; 
{ 
	register char *bp, *buff; 
	register short nl; 
	short tl; 
 
	buff = buffer; 
	tl = tline; 
	bp = getblock(tl, WRITE); 
	nl = nleft; 
	tl &= ~0177; 
	while (*bp = *buff++) 
	{ 
		if (*bp++ == '\n') 
		{ 
			*--bp = 0; 
			linebp = buff; 
			break; 
		} 
		if (--nl == 0) 
		{ 
			bp = getblock(tl+=0200, WRITE); 
			nl = nleft; 
		} 
	} 
	nl = tline; 
	tline += (((buff-buffer)+07)>>2) & 077776; 
	return(nl); 
} 
 
/*	format of core words :		bbbbbbbbboooooog 
 *	where : 
 *		b is block nr. in temp. file; 
 *		o is (offset in block) / 8; 
 *		g is set on global matches 
 */ 
 
char * 
getblock(atl, iof) 
{ 
	extern read(), write(); 
	register short bno, off; 
 
	bno = (atl>>7)&0777; 
	off = (atl<<2)&0770; 
	if (bno >= 511) error("Temp size - file incomplete", 0); 
	nleft = 512 - off; 
	if (bno==iblock) { ichanged |= iof; return(ibuff+off); } 
	if (bno==oblock) return(obuff+off); 
	if (iof==READ) 
	{ 
		if (ichanged) blkio(iblock, ibuff, write); 
		ichanged = 0; 
		iblock = bno; 
		blkio(bno, ibuff, read); 
		return(ibuff+off); 
	} 
	if (oblock>=0) blkio(oblock, obuff, write); 
	oblock = bno; 
	return(obuff+off); 
} 
 
blkio(b, buf, iofcn) 
char *buf; 
int (*iofcn)(); 
{ 
	lseek(tfile, ((long)b)<<9, 0); 
	if ((*iofcn)(tfile, buf, 512) != 512) 
		error("Temp i/o - file may be corrupted", 1); 
} 
 
init() 
{ 
	register char *p; 
	register short pid; 
 
	close(tfile); 
	tline = 0; 
	iblock = -1; 
	oblock = -1; 
	tfname = "/tmp/exxxxx"; 
	ichanged = 0; 
	pid = getpid(); 
	for (p = &tfname[11]; p > &tfname[6];) 
	{ 
		*--p = (pid&07) + '0'; 
		pid >>= 3; 
	} 
	if ((tfile = creat(tfname, 0660)) < 0) error("Create temp", 1); 
	close(tfile); 
	if ((tfile = open(tfname, 2)) < 0) error("Open temp", 1); 
	brk(fendcore+INITLINES); 
	endcore = fendcore - 2 + INITLINES; 
	dot = zero = dol = fendcore; 
} 
 
global(k) 
{ 
	register char *gp; 
	register short c; 
	register short *a1; 
	char globuf[GBSIZE]; 
	short dflt = 0; 
 
	if (globp) error(e_syn, 0); 
	setall(); 
	nonzero(); 
	if ((c=getchar())=='\n') error(e_syn, 0); 
	if (c >= 'a' && c <= 'z' || c >= 'A' && c <= 'Z') 
	{ 
		peekc = c; 
		dflt++; 
	} 
	else compile(c); 
	gp = globuf; 
	while ((c = getchar()) != '\n') 
	{ 
		if (c==EOF) error(e_syn, 0); 
		if (c==BACKSL) 
		{ 
			c = getchar(); 
			if (c!='\n') *gp++ = BACKSL; 
		} 
		*gp++ = c; 
		if (gp >= &globuf[GBSIZE-2]) error(e_linsz, 0); 
	} 
	*gp++ = '\n'; 
	*gp++ = 0; 
	c = 0; 
	for (a1=zero; a1<=dol; a1++) 
	{ 
		*a1 &= ~01; 
		if (a1>=addr1 && a1<=addr2 && (dflt || execute(0, a1))==k) 
			{ *a1 |= 01; c++; } 
	} 
	if (c == 0) error(e_match, 0); 
	for (a1=zero; a1<=dol; a1++) 
	{ 
		if (*a1 & 01) 
		{ 
			*a1 &= ~01; 
			dot = a1; 
			globp = globuf; 
			commands(-2); 
			a1 = zero; 
		} 
	} 
} 
 
substitute(inglob) 
char *inglob; 
{ 
	register char *sp, *lp; 
	register short *a1; 
	short gsubf; 
	short nn; 
	short nflag; 
	short getsub(); 
 
	gsubf = compsub(); 
	nflag = gsubf > 1; 
	nn = 0; 
	gsubf &= 01; 
	gsubf |= xflag; 
	for (a1 = addr1; a1 <= addr2; a1++) 
	{ 
		if (execute(0, a1)==0) continue; 
		inglob = (char *)((int)inglob | 01); 
		ssp = genbuf; slp = linebuf; 
		if (confirmed()) { dosub(); nn++; } 
		else donothing(); 
		if (gsubf) 
		{ 
			while (*loc2) 
			{ 
				if (execute(1, (short *)0) == 0) break; 
				if (confirmed()) { dosub(); nn++; } 
				else donothing(); 
			} 
		} 
		sp = ssp; lp = slp; 
		while (*sp++ = *lp++) if (sp >= &genbuf[LBSIZE]) error(e_linsz, 0); 
		retag(*a1 = putline(genbuf), *a1); 
		append(getsub, a1); 
		a1 += nline; 
		addr2 += nline; 
	} 
	if (inglob==0) error(e_match, 0); 
	if (nflag) 
	{ 
		putchar(' '); 
		count = nn; 
		putd(); 
		putchar('\n'); 
	} 
} 
 
extern char *place(); 
 
donothing() 
{ 
	char t1, t2; 
 
	t1 = rhsbuf[0]; 
	t2 = rhsbuf[1]; 
	rhsbuf[0] = '&'; 
	rhsbuf[1] = 0; 
	dosub(); 
	rhsbuf[0] = t1; 
	rhsbuf[1] = t2; 
} 
 
confirmed() 
{ 
	register char *ptr; 
	short	ch; 
	char *oldglobp; 
 
	if (xflag) 
	{ 
		col = 0; 
		for (ptr = genbuf; ptr < ssp; putchar(*ptr++)); 
		for (ptr = slp; *ptr; putchar(*ptr++)); 
		putchar('\n'); 
		underline(); 
		oldglobp = globp; 
		globp = 0; 
		ch = getchar(); 
		if (ch != '\n') 
		{ 
			while (getchar() != '\n'); 
			if (ch != CONFIRM) putnls("? '.' to confirm"); 
		} 
		globp = oldglobp; 
		return(ch == CONFIRM); 
	} 
	return(1); 
} 
 
underline() 
{ 
	short	i1, i2; 
	register short	i, j; 
 
	i1 = calccol(loc1); 
	i2 = calccol(loc2); 
	if (!listf && i1>i2) { i = i1; i1 = i2; i2 = i; } 
	j = i1 & ~7; 
	for (i = 0; i < j; i += 8) write(1, "\t", 1); 
	for (; i < i1; i++) write(1, " ", 1); 
	while (i != i2) 
	{ 
		if (++i >= (LENGTH - 1) && listf) { write(1, "\r", 1); i = 1; } 
		write(1, "^", 1); 
	} 
} 
 
screensplit() 
{ 
	register short a; 
 
	a = LENGTH - 2;		/* as our VDUs do funny things in the last column position! */ 
	col = 0; 
	while (a--) putchar('^'); 
	putchar('\n'); 
} 
 
/* 
 * Find actual column number 
 */ 
calccol(ll) 
char	*ll; 
{ 
	register short	i; 
	register char	*p; 
 
	i = 0; 
	for (p = genbuf; p < ssp; p++) 
	{ 
		if (*p=='\t' && !listf) i = (i+8) & ~7; 
		else if (*p=='\b' && !listf) i--; else i++; 
	} 
	for (p = slp; *p!='\0' && p<ll; p++) 
	{ 
		if (*p=='\t' && !listf) i = (i+8) & ~7; 
		else if (*p=='\b' && !listf) i--; else i++; 
	} 
	if (listf) i %= (LENGTH - 2); 
	return(i); 
} 
 
compsub() 
{ 
	register short c; 
	register char *rp; 
	short seof; 
	register char *p; 
	short gsubf; 
 
	gsubf = 0; 
	if ((seof = getchar()) == '\n') error(e_syn, 0); 
	compile(seof); 
	rp = rhsbuf; 
	p = linebuf; 
	for (;;) 
	{ 
		c = getchar(); 
		if (c == '%') 
		{ 
			if (*rp == 0) error(e_undef, 0); 
			while (*p++ = *rp++) if (p >= &linebuf[LBSIZE/2]) error(e_linsz, 0); 
			rp = rhsbuf; 
			p--; 
			continue; 
		} 
		if (c==BACKSL) 
		{ 
			if ((c = getchar()) >= '1' && c < NBRA + '1' && c >= nbra + '1') error(e_undef, 0); 
			c |= 0200; 
		} 
		if (c=='\n' && (globp == 0 || *globp == 0)) error(e_syn, 0); 
		if (c==seof) break; 
		*p++ = c; 
		if (p >= &linebuf[LBSIZE/2]) error(e_linsz, 0); 
	} 
	*p++ = 0; 
	for (p = linebuf; *rp++ = *p++; ); 
	if (((peekc = getchar()) | 040) == 'g') { peekc = 0; gsubf |= 1; } 
	if (((peekc = getchar()) | 040) == 'n') { peekc = 0; gsubf |= 2; } 
	newline(); 
	return(gsubf); 
} 
 
short 
getsub() 
{ 
	register char *p1, *p2; 
 
	p1 = linebuf; 
	if ((p2 = linebp) == 0) return(EOF); 
	while (*p1++ = *p2++); 
	linebp = 0; 
	return(0); 
} 
 
dosub() 
{ 
	register char *lp, *sp, *rp; 
	short c; 
 
	lp = slp; 
	sp = ssp; 
	rp = rhsbuf; 
	while (lp < loc1) *sp++ = *lp++; 
	while (c = *rp++) 
	{ 
		if (c=='&') { sp = place(sp, loc1, loc2); continue; } 
		if (c<0 && (c &= 0177) >='1' && c < NBRA+'1') 
		{ 
			sp = place(sp, braslist[c-'1'], braelist[c-'1']); 
			continue; 
		} 
		*sp++ = c; 
		if (sp >= &genbuf[LBSIZE]) error(e_linsz, 0); 
	} 
	slp = loc2; 
	ssp = sp; 
} 
 
char * 
place(asp, al1, al2) 
char *asp, *al1, *al2; 
{ 
	register char *sp, *l1, *l2; 
 
	sp = asp; 
	l1 = al1; 
	l2 = al2; 
	if (sp > &genbuf[LBSIZE - (l2 - l1)]) error(e_linsz, 0); 
	while (l1 < l2) *sp++ = *l1++; 
	return(sp); 
} 
 
move(cflag) 
{ 
	register short *adt, *ad1, *ad2; 
	short getcopy(); 
	short *address(); 
 
	setdot(); 
	nonzero(); 
	if ((adt = address())==0) error(e_addr, 0); 
	newline(); 
	ad1 = addr1; 
	ad2 = addr2; 
	changed++; 
	if (cflag) { ad1 = dol; append(getcopy, ad1++); ad2 = dol; } 
	ad2++; 
	if (adt<ad1) 
	{ 
		dot = adt + (ad2-ad1); 
		if ((++adt)==ad1) return; 
		reverse(adt, ad1); 
		reverse(ad1, ad2); 
		reverse(adt, ad2); 
	} 
	else if (adt >= ad2) 
	{ 
		dot = adt++; 
		reverse(ad1, ad2); 
		reverse(ad2, adt); 
		reverse(ad1, adt); 
	} 
	else error(e_addr, 0); 
} 
 
reverse(aa1, aa2) 
short *aa1, *aa2; 
{ 
	register short *a1, *a2, t; 
 
	a1 = aa1; 
	a2 = aa2; 
	for (;;) 
	{ 
		t = *--a2; 
		if (a2 <= a1) return; 
		*a2 = *a1; 
		*a1++ = t; 
	} 
} 
 
short 
getcopy() 
{ 
	if (addr1 > addr2) return(EOF); 
	getline(*addr1++); 
	return(0); 
} 
 
compile(aeof) 
{ 
	register short eof, c; 
	register char *ep; 
	char *lastep; 
	char bracket[NBRA], *bracketp; 
	short cclcnt; 
	char	tempbuf[ESIZE]; 
	short	savcirc; 
	char	*ep1; 
	char	fstch; 
 
	ep = expbuf; 
	eof = aeof; 
	bracketp = bracket; 
	if ((c = getchar()) == eof) { if (*ep==0) error(e_syn, 0); return; } 
	nbra = 0; 
	lastep = 0; 
	ep1 = tempbuf; 
	if (*ep) while (*ep != CEOT) *ep1++ = *ep++; 
	*ep1++ = CEOT; 
	ep = expbuf; 
	savcirc = circfl; 
	circfl = 0; 
	if (c=='^') { c = getchar(); circfl++; } 
	if (c=='\n' && *ep) error(e_syn, 0); 
	peekc = c; 
	for (;;) 
	{ 
		if (ep >= &expbuf[ESIZE]) goto cerror; 
		c = getchar(); 
		if (c==eof) { *ep++ = CEOT; return; } 
		if (c!='*') lastep = ep; 
		switch (c) 
		{ 
 
	    case BACKSL:if ((c = getchar())=='(') 
			{ 
				if (nbra >= NBRA) goto cerror; 
				*ep++ = CBRA; 
				*bracketp++ = nbra; 
				*ep++ = nbra++; 
				continue; 
			} 
			if (c == ')') 
			{ 
				*ep++ = CKET; 
				if (bracketp <= bracket) goto cerror; 
				*ep++ = *--bracketp; 
				continue; 
			} 
			*ep++ = CCHR; 
			if (c=='\n') goto cerror; 
			*ep++ = c; 
			continue; 
 
	    case '.':	*ep++ = CDOT; 
			continue; 
 
	    case '\n':	goto cerror; 
 
	    case '*':	if (lastep == 0) 
			{ 
				lastep = ep; 
				goto defchar; 
			} 
			if (*lastep==CBRA || *lastep==CKET) 
				goto cerror; 
			*lastep |= STAR; 
			continue; 
 
	    case '$':	if ((peekc=getchar()) != eof) goto defchar; 
			*ep++ = CDOL; 
			continue; 
 
	    case '[':	*ep++ = CCL; 
			*ep++ = 0; 
			cclcnt = 1; 
			if ((c=getchar()) == '^') 
			{ 
				c = getchar(); 
				ep[-2] = NCCL; 
			} 
			do 
			{ 
				if (c=='\n') goto cerror; 
				if (c == '-' && cclcnt > 1) 
				{ 
					if ((c = getchar()) == '\n' || c <= ep[-1] || 
						ep >= &expbuf[ESIZE-2]) goto cerror; 
					*ep++ = 0; *ep++ = c; cclcnt += 2; 
					continue; 
				} 
				else if (c == BACKSL && (c = getchar()) == '\n') goto cerror; 
				*ep++ = c; 
				cclcnt++; 
				if (ep >= &expbuf[ESIZE]) goto cerror; 
			} while ((c = getchar()) != ']'); 
			lastep[1] = cclcnt; 
			continue; 
 
	    case '%':	if (*(ep1 = rhsbuf) == 0) { *expbuf = 0; error(e_undef, 0); } 
			while (*ep1) 
			{ 
				*ep++ = CCHR; 
				*ep++ = *ep1++ & 0177; 
				if (ep >= &expbuf[ESIZE]) goto cerror; 
			} 
			continue; 
 
	    case '&':	if (*tempbuf == CEOT) { *expbuf = 0; error(e_undef, 0); } 
			if (savcirc) 
				if (ep == expbuf) circfl++; 
				else { lastep = ep; *ep++ = CCHR; *ep++ = '^'; } 
			ep1 = tempbuf; 
			while ((*ep = *ep1++) != CEOT) 
			{ 
				if (ep >= &expbuf[ESIZE]) goto cerror; 
				switch (*ep) 
				{ 
 
			    case CBRA:	ep1++; 
					if (nbra >= NBRA) goto cerror; 
					*bracketp++ = nbra; 
					*++ep = nbra++; 
					break; 
 
			    case CKET:	ep1++; 
					if (bracketp <= bracket) goto cerror; 
					*++ep = *--bracketp; 
					break; 
 
			    case CCL: 
			    case NCCL: 
			    case CCL|STAR: 
			    case NCCL|STAR: 
					*++ep = *ep1++; 
					break; 
 
			    case CDOT: 
			    case CDOT|STAR: 
			    case CCHR: 
			    case CCHR|STAR: 
					lastep = ep++; 
					continue; 
 
			    default:	ep++; 
					continue; 
				} 
 
				ep++; 
				lastep = ep-2; 
			} 
			if (ep[-1] == CDOL) 
				if ((peekc = getchar()) != eof) 
				{ 
					ep[-1] = CCHR; 
					*ep++ = '$'; 
				} 
			continue; 
 
	    defchar: 
	    default:	*ep++ = CCHR; 
			*ep++ = c; 
		} 
	} 
cerror:	expbuf[0] = 0; 
	error(e_syn, 0); 
} 
 
printstrings() 
{ 
	register char *ep, flag; 
	register short count; 
	static char empty[] = "empty"; 
 
	listf++; 
	write(1, "& = ", col = 4); 
	if (*expbuf) 
	{ 
		putchar('/'); 
		if (circfl) putchar('^'); 
		for (ep = expbuf; *ep != CEOT; ) 
			switch(*ep++) 
			{ 
	 
		    case CCHR:	putchar(*ep++); continue; 
	 
		    case CCHR|STAR: 
				putchar(*ep++); putchar('*'); continue; 
	 
		    case CBRA:	ep++; putchar(BACKSL); putchar('('); continue; 
	 
		    case CKET:	ep++; putchar(BACKSL); putchar(')'); continue; 
	 
		    case CDOT:	putchar('.'); continue; 
	 
		    case CDOT|STAR: 
				putchar('.'); putchar('*'); continue; 
	 
		    case CDOL:	putchar('$'); continue; 
	 
		    case NCCL|STAR: 
		    case CCL|STAR: 
		    case NCCL: 
		    case CCL:	flag = *(ep-1); count = *ep++; putchar('['); 
				if ((flag & ~STAR) == NCCL) putchar('^'); 
				while (--count) 
					if (*ep=='\0') { ep++; putchar('-'); } 
					else putchar(*ep++); 
				putchar(']'); 
				if (flag & STAR) putchar('*'); 
			} 
		putnls("/"); 
	} 
	else putnls(empty); 
	write(1, "% = ", col = 4); 
	if (*rhsbuf) 
	{ 
		putchar('/'); 
		for (ep = rhsbuf; *ep; ) 
		{ 
			if ((flag = *ep++) < 0) 
			{ 
				putchar('\\'); 
				flag &= 0177; 
			} 
			putchar(flag); 
		} 
		putnls("/"); 
	} 
	else putnls(empty); 
	write(1, "! = ", col = 4); 
	if (*unixbuffer) 
	{ 
		putchar('/'); 
		for (ep = unixbuffer; *ep; putchar(*ep++)); 
		putnls("/"); 
	} 
	else putnls(empty); 
	listf = 0; 
} 
 
execute(gf, addr) 
short *addr; 
{ 
	register char *p1, *p2, c; 
 
	if (gf) 
	{ 
		if (circfl) return(0); 
		locs = p1 = loc2; 
	} 
	else 
	{ 
		if (addr==zero) return(0); 
		p1 = (char *)getline(*addr); 
		locs = 0; 
	} 
	p2 = expbuf; 
	if (circfl) { loc1 = p1; return(advance(p1, p2)); } 
	if (*p2==CCHR)				/* fast check for first character */ 
	{ 
		c = p2[1]; 
		do 
		{ 
			if (*p1!=c) continue; 
			if (advance(p1, p2)) { loc1 = p1; return(1); } 
		} while (*p1++); 
		return(0); 
	} 
	do					/* regular algorithm */ 
		if (advance(p1, p2)) { loc1 = p1; return(1); } 
	while (*p1++); 
	return(0); 
} 
 
advance(alp, aep) 
char *alp, *aep; 
{ 
	register char *lp, *ep, *curlp; 
	char *nextep; 
 
	lp = alp; 
	ep = aep; 
	for (;;) 
		switch (*ep++) 
		{ 
 
	    case CCHR:	if (*ep++ == *lp++) continue; return(0); 
	 
	    case CDOT:	if (*lp++) continue; return(0); 
	 
	    case CDOL:	if (*lp==0) continue; return(0); 
	 
	    case CEOT:	loc2 = lp; return(1); 
	 
	    case CCL:	if (cclass(ep, *lp++, 1)) { ep += *ep; continue; } return(0); 
	 
	    case NCCL:	if (cclass(ep, *lp++, 0)) { ep += *ep; continue; } return(0); 
	 
	    case CBRA:	braslist[*ep++] = lp; continue; 
	 
	    case CKET:	braelist[*ep++] = lp; continue; 
	 
	    case CDOT|STAR: 
			curlp = lp; 
			while (*lp++); 
			goto star; 
	 
	    case CCHR|STAR: 
			curlp = lp; 
			while (*lp++ == *ep); 
			ep++; 
			goto star; 
	 
	    case CCL|STAR: 
	    case NCCL|STAR: 
			curlp = lp; 
			while (cclass(ep, *lp++, ep[-1]==(CCL|STAR))); 
			ep += *ep; 
	 
		star:	do 
			{ 
				lp--; 
				if (lp==locs) break; 
				if (advance(lp, ep)) return(1); 
			} while (lp > curlp); 
			return(0); 
	 
	    default:	error(e_syn, 0); 
		} 
} 
 
cclass(aset, ac, af) 
char *aset; 
{ 
	register char *set, c; 
	register short n; 
 
	set = aset; 
	if ((c = ac) == 0) return(0); 
	n = *set++; 
	while (--n) 
		if (*set == '\0') 
		{ 
			if (c > set[-1] && c <= set[1]) return(af); 
			set++; set++; n--; 
		} 
		else if (*set++ == c) return(af); 
	return(!af); 
} 
 
putd() 
{ 
	register short r; 
 
	r = count % 10; 
	if (count /= 10) putd(); 
	putchar(r + '0'); 
} 
 
puts(as) 
char *as; 
{ 
	register char *sp; 
 
	sp = as; 
	while (*sp) putchar(*sp++); 
} 
 
putnls(as) 
char *as; 
{ 
 
	col = 0; 
	puts(as); 
	putchar('\n'); 
} 
 
char	line[70]; 
char	*linp =	line; 
 
putchar(c) 
register short c; 
{ 
	register char *lp; 
 
	lp = linp; 
	if (listf) 
	{ 
		col++; 
		if (col >= LENGTH - 1) { col = 1; *lp++ = BACKSL; *lp++ = '\n'; } 
		if (c=='\t') { c = '>'; goto esc; } 
		if (c=='\b') 
		{ 
			c = '<'; 
		esc:	*lp++ = '-'; 
			*lp++ = '\b'; 
			*lp++ = c; 
			goto out; 
		} 
		if (c<' ' && c!= '\n') 
		{ 
			*lp++ = BACKSL; 
			*lp++ = ((c >> 6) & 03) + '0'; 
			*lp++ = ((c >> 3) & 07) + '0'; 
			*lp++ = (c & 07) + '0'; 
			col += 2; 
			goto out; 
		} 
	} 
	*lp++ = c; 
out:	if(c == '\n' || lp >= &line[64]) 
	{ 
		linp = line; 
		write(1, line, lp-line); 
		return; 
	} 
	linp = lp; 
} 
 
op(inglob) 
{ 
	register short	*a1; 
	register char	*lp, *sp; 
	char	seof, ch; 
	short	t, nl; 
	short	getopen(); 
	short	getnil(); 
	int	rubout(); 
 
	ch = peekc = getchar(); 
	switch (ch) 
	{ 
 
    case BACKSL:t = 1; 
		delete(); 
		changed++; 
		addr2 = addr1; 
		break; 
 
    case ';': 
    case '+':	t = 0; 
		break; 
 
    case '-':	t =1; 
		break; 
 
    default:	goto normal; 
	} 
 
	peekc = 0; 
	if (addr1 != addr2) error(e_addr, 0); 
	oflag = 0; 
	append(getnil, addr2-t); 
	changed++; 
	addr1 = addr2 -= (t-1); 
	setdot(); 
	nonzero(); 
 
normal:	if (addr1 == zero) error(e_addr, 0); 
	seof = getchar(); 
	if (seof == '\n') { loc2 = linebuf-1; seof = '\0'; } 
	else compile(seof); 
	onintr = signal(SIGINT, SIG_IGN); 
	setcbrk();	/* terminal into cbrk mode*/ 
 
	for (a1 = addr1; a1 <= addr2; a1++) 
	{ 
		if (seof != '\0') 
		{ 
			if (execute(0, a1) == 0) continue; 
		} 
		else getline(*a1); 
		putnls("\\\r"); 
		sp = genbuf; 
		inglob |= 01; 
		for (lp = linebuf; lp < loc2;) 
		{ 
			putch(*lp); 
			*sp++ = *lp++; 
		} 
		lnp = lp; 
		gnp = sp; 
 
		oldline = *a1; thestart = a1; nline = 0; 
		if (onintr != SIG_IGN) 
			signal(SIGINT, rubout); 
		oflag = gopen();	/* open the current line */ 
		retag(*a1 = putline(linebuf), *a1);	/* write revised line */ 
		nl = append(getopen, a1); 
		signal(SIGINT, SIG_IGN); 
		a1 += nl; 
		addr2 += nl; 
#ifdef	AUTOW 
		if (writewaiting) 
		{ 
			delaywrite = writewaiting = 0; 
			catchclock(); 
			delaywrite++; 
		} 
#endif	AUTOW 
	} 
	setcook();	/* terminal into cooked mode */ 
	signal(SIGINT, onintr); 
	putchar('\n'); 
	if (inglob == 0) error(e_match, 0);	/* no return */ 
} 
 
short 
getnil() 
{ 
	if (oflag == EOF) return(EOF); 
	linebuf[0] = '\0'; 
	oflag = EOF; 
	return(0); 
} 
 
setcbrk() 
{ 
	if (ioctl(0, TCGETA, &termio) < 0) 
	{ 
		signal(SIGINT, onintr); 
		error("Gtty", 1);	/* no return */ 
	} 
	termio.c_lflag &= ~ICANON; 
	termio.c_cc[VMIN] = 1; 
	termio.c_cc[VTIME] = -1; 
	ioctl(0, TCSETAW, &termio); 
} 
 
setcook() 
{ 
	termio.c_lflag |= ICANON; 
	termio.c_cc[VEOF] = CEOF; 
	termio.c_cc[VEOL] = 0; 
	ioctl(0, TCSETAW, &termio); 
} 
 
inword(c) 
char c; 
{ 
	if (c>='0' && c<='9') return(1); 
	c &= 0137;				/* convert to upper case */ 
	if (c>='A' && c<='Z') return(1); 
	return(0); 
} 
 
rescan() 
{ 
	register char *lp, *sp; 
 
	lp = linebuf; 
	sp = genbuf; 
	while ((*lp++ = *sp++) != '\0') 
		if (lp > linebuf+LBSIZE) { *(--lp) = '\0'; break; } 
} 
 
rubout() 
{ 
	signal(SIGINT, SIG_IGN); 
	*thestart++ = oldline; 
	if (nline)			/* some lines added */ 
	{ 
		addr1 = thestart; 
		addr2 = thestart + nline - 1; 
		if (addr2 == dol) delete(); else { delete(); dot--; } 
	} 
	putnls("\\\r"); 
	setcook(); 
	lastc = '\n'; 
	signal(SIGINT, onintr); 
	error(e_int, 0);		/* no return */ 
} 
 
/* 
 * Leaves revised line in linebuf, 
 * returns 0 if more to follow, 
 * EOF if last line. 
 */ 
 
gopen() 
{ 
	register char *lp, *sp, *rp; 
	char ch, *br, *pr; 
	short retcode; 
	char erase, kill; 
	int (*savint)(); 
 
	kill = termio.c_cc[VKILL]; 
	erase = termio.c_cc[VERASE]; 
	lp = lnp; 
	sp = gnp; 
 
	for (;;) 
	{ 
		ch = getchar(); 
		if (ch == kill) 
		{ 
			sp = genbuf;		/* delete displayed line backward */ 
			changed++; 
			goto verify; 
		} 
		if (ch == erase) goto erasechr; 
		if (ch >= ' ' && ch != BACKSL)	/* not a control chr */ 
		{ 
			*--lp = ch; 
			changed++; 
			goto forward; 
		} 
		switch(ch)			/* what's left */ 
		{ 
 
	    case CTRLD: 
	    case ESCAPE:	/* close the line (see case '\n' also) */ 
			putb(lp); 
			while (*sp++ = *lp++); 
			rescan(); 
			return(EOF); 
 
	    case CTRLA:				/* verify line */ 
	    verify:	putnls("\\\r"); 
			*sp = '\0'; 
			putb(genbuf); 
			continue; 
 
	    case CTRLB:				/* back a word */ 
			if (sp == genbuf) goto backquery; 
			while ((*--lp = *--sp) == SPACE) 
				if (sp < genbuf) goto out; 
			if (inword(*sp)) 
			{ 
				while (inword(*--lp = *--sp)) 
					if (sp < genbuf) goto out; 
				if (*sp == SPACE) 
					while ((*--lp = *--sp) == SPACE) 
						if (sp < genbuf) goto out; 
			} 
			else while (sp >= genbuf && !inword(*sp)) 
				*lp-- = *sp--; 
	    out:	sp++; 
			lp++; 
			goto verify; 
 
	    case CTRLC:				/* forward one char */ 
			if (*lp == 0) goto backquery; 
			putch(*lp); 
	    forward:	*sp++ = *lp++;				/* one character */ 
			continue; 
 
	    case CTRLE:	putb(lp); 
			goto verify; 
 
	    case CTRLF:	while (*lp++);			/* delete forward */ 
			lp--; 
			goto verify; 
 
	    case CTRLP:	while (*lp != '\0')		/* skip to eol */ 
			{ 
				putch(*sp++ = *lp++); 
			} 
			continue; 
 
	    case CTRLR:	while (*sp++ = *lp++);		/* re-set to start of line */ 
			rescan(); 
			lp = linebuf; 
			sp = genbuf; 
			goto verify; 
 
	    case CTRLT:					/* backup one character */ 
			if (sp == genbuf) goto backquery; 
			*--lp = *--sp; 
			goto verify; 
 
	    case CTRLV:	rp = sp;			/* verify spelling */ 
			pr = unixbuffer + UNIXBUFL - 2; 
			*pr = 0; 
			while (*(--rp) == SPACE); 
			while (inword(*rp) && rp >= genbuf) 
			{ 
				*--pr = *rp--; 
				if (*pr >= 'A' && *pr <= 'Z') 
					*pr += 'a' - 'A'; 
			} 
			if (*pr == 0) goto backquery; 
			putnls("!!"); 
			setcook(); 
			if (fork() == 0) 
			{ 
				close(tfile); 
				signal(SIGHUP, onhup); 
				signal(SIGQUIT, onquit); 
				execl("/bin/predict", "predict", pr, 0); 
				putnls("Sorry, can't spell today"); 
				exit(1); 
			} 
			savint = signal(SIGINT, SIG_IGN); 
			wait(&retcode); 
			signal(SIGINT, savint); 
			setcbrk(); 
			putnls("!!"); 
			goto verify; 
 
	    case CTRLW:					/* forward one word */ 
			if (*lp == '\0') goto backquery; 
			while (*lp == SPACE) putch(*sp++ = *lp++); 
			if (inword(*lp)) 
			{ 
				while (inword(*lp)) 
				{ 
					putch(*sp++ = *lp++); 
				} 
				if (*lp == SPACE) 
				{ 
					if (*lp == SPACE) 
						while (lp[1] == SPACE) 
							putch(*sp++ = *lp++); 
				} 
			} 
			else while (*lp && !inword(*lp)) 
				{ 
					putch(*sp++ = *lp++); 
				} 
			break; 
 
	    case CTRLZ:					/* delete a word */ 
			if (sp == genbuf) goto backquery; 
			while (*--sp == SPACE && sp >= genbuf); 
			if (sp < genbuf) goto zout; 
			if (inword(*sp)) 
			{ 
				while (inword(*--sp) && sp >= genbuf); 
				if (sp < genbuf) goto zout; 
				if (*sp == SPACE) 
				{ 
					while (*--sp == SPACE && sp >= genbuf); 
					if (sp < genbuf) goto zout; 
				} 
			} 
			else while (sp>=genbuf && !inword(*sp)) 
				sp--; 
	    zout:	sp++; 
			changed++; 
			goto verify; 
 
	    case CTRLX: 
	    erasechr:	if (sp == genbuf) goto backquery; 
			sp--; 
			changed++; 
			if (ch == CTRLX) goto verify; 
			if (erase == CTRLH)	/* backspace */ 
			{ 
				putch(' ');	/* erase the character */ 
				putch(CTRLH); 
			} 
			continue; 
 
	    case '\n': 
	    case '\r':				/* 
						 * split line; actually handled at 
						 * end of switch block 
						 */ 
			ch = '\n'; 
			*sp++ = ch; 
			br = sp; 
			changed++; 
			break; 
 
	    case '\0':	continue; 
 
	    case BACKSL:			/* special symbols */ 
			ch = getchar(); 
			if (ch!=BACKSL && ch!=erase && ch!=kill) 
				if (termio.c_iflag & IUCLC) 
					switch (ch) 
					{ 
	 
				    case '(':	ch = '{'; break; 
	 
				    case ')':	ch = '}'; break; 
	 
				    case '!':	ch = '|'; break; 
	 
				    case '^':	ch = '~'; break; 
	 
				    case '\'':	ch = '`'; break; 
	 
				    default:	if (ch>='a' && ch<='z') ch -= 040; 
						else { peekc = ch; ch = BACKSL; } 
						break; 
					} 
				else { peekc = ch; ch = BACKSL; } 
 
	    case CTRLI: 
	    case CTRLG:			/* allow for bell, tab, form-feed */ 
	    case CTRLL:	*--lp = ch; 
			changed++; 
			goto forward; 
 
	    default:	help(); 
			goto verify; 
		} 
 
		if (ch == '\n') 
		{					/* split line */ 
			lnp = sp; 
			while (*sp++ = *lp++);			/* move the rest over */ 
			brp = linebuf + (br - genbuf); 
			lnp = linebuf + (lnp - br); 
			rescan(); 
			*(brp - 1) = '\0'; 
			return(0); 
		} 
		else continue; 
 
backquery:	putch(BELL); 
	}				/* end of forloop block */ 
}					/* end of gopen */ 
 
 
				/* 
				 * calls gopen, deals with multiple lines etc. 
				 */ 
short 
getopen() 
{ 
	register char *lp, *sp; 
 
	if (oflag == EOF) return(EOF); 
 
				/* otherwise, multiple lines */ 
 
	lp = linebuf; 
	sp = brp; 
	while (*lp++ = *sp++);			/* move it down */ 
	sp = genbuf; 
	lp = linebuf; 
	while (lp < lnp) *sp++ = *lp++; 
	gnp = sp; 
					/* should check whether empty line returned */ 
	oflag = gopen(); 
	return(0); 
} 
 
putch(ch) 
char ch; 
{ 
	write(1, &ch, 1); 
} 
 
putb(ptr) 
char *ptr;	/*display string */ 
{ 
	register char *p; 
 
	p = ptr; 
	if (*p == '\0') return; 
	while (*(++p)); 
	write(1, ptr, p-ptr); 
} 
 
help() 
{ 
	putnls("\n"); 
	putnls("\terase and kill characters unchanged"); 
	putnls("\t^A\tdisplay Again\t\t^P\tskip to end"); 
	putnls("\t^B\tBackup word\t\t^R\tRe-scan from start"); 
	putnls("\t^C\tnext Character\t\t^T\tbackup one character"); 
	putnls("\tESCAPE\t\t\t\t^V\tverify spelling"); 
	putnls("\tor ^D\tclose line and exit\t^W\tnext Word"); 
	putnls("\t^E\tdisplay to End\t\t^X\tdelete character"); 
	putnls("\t^F\tdelete line Forward\t^Z\tdelete word"); 
	putnls("\t^O\thelp\t\t\tDEL\texit unchanged"); 
	putnls("\n\tOther characters (including RETURN) inserted as typed"); 
} 
 
 
 
