/* terminal control module for terminals described by TERMCAP */ 
 
/*		Copyright (c) 1981,1980 James Gosling 
   Enhancements copyright (c) 1984 Fen Labalme and Chris Torek 
   Distributed by Fen Labalme, with permission from James Gosling. 
 
This file is part of GNU Emacs. 
 
GNU Emacs is distributed in the hope that it will be useful, 
but there is no warranty of any sort, and no contributor accepts 
responsibility for the consequences of using this program 
or for whether it serves any particular purpose. 
 
Everyone is granted permission to copy, modify and redistribute 
GNU Emacs, but only under the conditions described in the 
document "GNU Emacs copying permission notice".   An exact copy 
of the document is supposed to have been given to you along with 
GNU Emacs so that you can know how you may redistribute it all. 
It should be in a file named COPYING.  Among other things, the 
copyright notice and this notice must be preserved on any copy 
distributed.  */ 
 
 
/*  Notes: 
 - :rn: indicates that \r acts like \r\n 
 - :nn: indicates that \n doesn't work 
 - :tw#<n>: specifies that physical tabs work and have a width of <n> 
   (default is width 8 and used only if stty tabs was in effect) 
 - :ds=<str>: gives a "don't send" string; these characters will not be 
   written to the terminal for cursor motion (:cm=:).  If this is nonempty 
   the terminal MUST have an upline and a backspace.  If defined, it is 
   assumed to have a null in it.  If not defined, it defaults to 
   ^D\t\n\r^Q^S.  To allow any character use ":ds=:" (required for 
   Hazeltine 2000s). 
 */ 
 
#include <stdio.h> 
#include <ctype.h> 
#include "config.h" 
#include "Trm.h" 
#include "cm.h" 
 
char *tgetstr (), *malloc (), *getenv (); 
 
 
/* Capabilities from TERMCAP */ 
static char 
	*ALstr,			/* insert line */ 
	*CDstr,			/* clear to end of display */ 
	*CEstr,			/* clear to end of line */ 
	*CLstr,			/* clear screen */ 
	*DCstr,			/* delete character */ 
	*DLstr,			/* delete line */ 
	*DMstr,			/* begin delete mode */ 
	*EDstr,			/* end delete mode */ 
	*EIstr,			/* end insert mode */ 
	*ICstr,			/* insert character */ 
	*IMstr,			/* begin insert mode */ 
	*IPstr,			/* insert character pad */ 
	*SEstr,			/* end standout mode */ 
	*SOstr,			/* begin standout mode */ 
	*TEstr,			/* end cursor motion (editor) mode */ 
	*TIstr,			/* begin cursor motion mode */ 
	*VBstr,			/* visible bell */ 
	*VEstr,			/* end visual (screen editor) mode */ 
	*VSstr;			/* begin visual mode */ 
static int 
	SGnum,			/* sg# value, -1 iff no sg# */ 
	DBflag,			/* display retained below visible part */ 
	HZflag,			/* hazeltine, can't print tilde */ 
	MIflag,			/* can move in insert mode */ 
	MSflag,			/* can move in standout mode */ 
	ULflag;			/* _ underlines if printed on nonblank */ 
 
/* internal state */ 
static int 
	DMisIM,			/* delete mode == insert mode */ 
	curHL,			/* current highlight */ 
	desHL,			/* desired highlight */ 
	curIM,			/* current insert mode */ 
	desIM;			/* desired insert mode */ 
static char 
	HLflags[MScreenLength];	/* used iff SGnum >= 0 */ 
 
static 
INSmode (n) { 
    desIM = n; 
} 
 
static 
HLmode (n) { 
    desHL = n; 
} 
 
/* This routine deserves a bit of explanation.  If SGnum < 0, then 
   standout is well-behaved and all we have to do is dump the 
   appropriate string.  Otherwise, we assume: (1) CEstr gets rid 
   of standout; (2) standout goes from the "so" marker to the end 
   of the line or the last character on the line (whichever is 
   first); (3) SEstr is not needed unless there is no CEstr; (4) 
   normal cursor motion may be used to position onto the standout 
   marker. */ 
static 
setHL (override) { 
    register int des = override ? 0 : desHL; 
 
    if (SGnum < 0) { 
	if (des != curHL) { 
	    register char *s = des ? SOstr : SEstr; 
	    if (s) 
		tputs (s, 1, cmputc); 
	    curHL = des; 
	} 
	return; 
    } 
    if (HLflags[curY] == des || override) 
	return; 
    /* we must have been called from wipeline */ 
    if (des) { 
	ltopos (curY, 0); 
	tputs (SOstr, 1, cmputc); 
	cmplus (SGnum); 
	HLflags[curY]++; 
    } 
    else { 
	if (!CEstr && SEstr) { 
	    ltopos (curY, 0); 
	    tputs (SEstr, 1, cmputc); 
	    cmplus (SGnum); 
	} 
	HLflags[curY] = 0; 
    } 
} 
 
static 
setIM (override) { 
    register int des = override ? 0 : desIM; 
    if (des == curIM) 
	return; 
    tputs (des ? IMstr : EIstr, 1, cmputc); 
    curIM = des; 
} 
 
static 
inslines (n) { 
    register int i = n; 
    setHL (1); 
    while (--i >= 0) 
	tputs (ALstr, tt.t_length - curY, cmputc); 
    if (SGnum >= 0) {		/* adjust HLflags */ 
	for (i = tt.t_length - n; --i >= curY;) 
	    HLflags[i + n] = HLflags[i]; 
	for (i = curY + n; --i >= curY;) 
	    HLflags[i] = 0; 
    } 
} 
 
static 
dellines (n) { 
    register int i = n; 
    setHL (1); 
    while (--i >= 0) 
	tputs (DLstr, tt.t_length - curY, cmputc); 
    if (SGnum >= 0) {		/* adjust HLflags */ 
	for (i = curY + n; i < tt.t_length; i++) 
	    HLflags[i - n] = HLflags[i]; 
	for (i = tt.t_length - n; i < tt.t_length; i++) 
	    HLflags[i] = 0; 
    } 
    if (DBflag) { 
	if (CDstr) { 
	    topos (tt.t_length - n + 1, 1); 
	    tputs (CDstr, tt.t_length - curY, cmputc); 
	} 
	else { 
	    for (i = tt.t_length - n; ++i <= tt.t_length;) { 
		topos (i, 1); 
		wipeline (0, tt.t_width); 
	    } 
	} 
    } 
} 
 
static 
writechars (start, end) 
register char *start, *end; 
{ 
    setHL (0); 
    setIM (0); 
    while (start <= end) { 
	if (desIM && ICstr) 
	    tputs (ICstr, ScreenCols - curX, cmputc); 
	if (*start == '_' && !desIM && ULflag) { 
	    putchar (' '); 
	    tputs (Left, 1, cmputc);/* ASSUMES Left EXISTS */ 
	} 
	if (HZflag && *start == '~') 
	    putchar ('`'), ++start; 
	else 
	    putchar (*start++); 
	curX++; 
	if (desIM && IPstr) 
	    tputs (IPstr, ScreenCols - curX, cmputc); 
    } 
} 
 
static 
update_end () 
{ 
  setIM (1); 
  setHL (1); 
} 
 
static 
blanks (n) 
register int n; 
{ 
    setHL (0); 
    setIM (0); 
    while (--n >= 0) { 
	if (desIM && ICstr) 
	    tputs (ICstr, ScreenCols - curX, cmputc); 
	putchar (' '); 
	curX++; 
	if (desIM && IPstr) 
	    tputs (IPstr, ScreenCols - curX, cmputc); 
    } 
} 
 
static 
topos (row, col) { 
    if (HLflags[curY]) 
	col += SGnum; 
    ltopos (row - 1, col - 1); 
} 
 
static 
ltopos (row, col) { 
    if (curY == row && curX == col) 
	return; 
    if (!MSflag) 
	setHL (1); 
    if (!MIflag) 
	setIM (1); 
    cmgoto (row, col); 
} 
 
static 
flash () { 
    tputs (VBstr, 1, cmputc); 
} 
 
/* Compute cost of sending "str".  Store overhead in *ov and pad factor 
   in *pf */ 
static 
pfcostof (str, ov, pf) 
register char *str; 
int *ov; 
double *pf; 
{ 
    register int pad = 0; 
    extern int baud_rate; 
    while (isdigit (*str)) 
	pad = pad * 10 + *str++ - '0'; 
    pad *= 10; 
    if (*str == '.' && isdigit (str[1])) { 
	str++; 
	pad += *str++ - '0'; 
    } 
    if (*str == '*') { 
	*pf = pad * baud_rate / 100000.0 * tt.t_length / 2.0; 
	str++; 
    } 
    else 
	*pf = pad * baud_rate / 100000.0; 
    *ov = strlen (str); 
} 
 
/* Like pfcostof, compute cost of sending "str" -- however, str may be 
   undefined; if it is defined, use ScreenCols rather than tt.t_length 
   as the multiplier value for '*', and always add to *c */ 
static 
costof (str, c) 
register char *str; 
double *c; 
{ 
    register int pad; 
    extern int baud_rate; 
    if (str == 0) 
	return; 
    pad = 0; 
    while (isdigit (*str)) 
	pad = pad * 10 + *str++ - '0'; 
    pad *= 10; 
    if (*str == '.' && isdigit (str[1])) { 
	str++; 
	pad += *str++ - '0'; 
    } 
    if (*str == '*') { 
	*c += pad * baud_rate / 100000.0 * ScreenCols / 2.0; 
	str++; 
    } 
    else 
	*c += pad * baud_rate / 100000.0; 
    *c += strlen (str); 
} 
 
/* ARGSUSED */ 
static 
init (BaudRate, tabok) 
{ 
    if (TabWidth == 8) 
	UseTabs = tabok;	/* Use tabs if was stty tabs */ 
 
    if (ALstr) 
	pfcostof (ALstr, &tt.t_ILnov, &tt.t_ILnpf); 
    else 
	tt.t_ILov = MissingFeature; 
    if (DLstr) 
	pfcostof (DLstr, &tt.t_DLnov, &tt.t_DLnpf); 
    else 
	tt.t_DLov = MissingFeature; 
 
    /* The magic cost formulae below assume: (1) for 70% of inserts 
       we will already be in insert mode; (2) if delete mode == insert 
       mode, 50% of those will be in insert mode already */ 
    if (IMstr && EIstr) { 
	tt.t_ICmf = 0.0; 
	costof (IMstr, &tt.t_ICmf); 
	costof (EIstr, &tt.t_ICmf); 
	tt.t_ICov = tt.t_ICmf * 0.3; 
	tt.t_ICmf = 0.0; 
	costof (ICstr, &tt.t_ICmf); 
	costof (IPstr, &tt.t_ICmf); 
    } 
    else 
	tt.t_ICov = MissingFeature; 
    if (DCstr) { 
	tt.t_DCmf = 0.0; 
	costof (DMstr, &tt.t_DCmf); 
	costof (EDstr, &tt.t_DCmf); 
	tt.t_DCov = tt.t_DCmf * (DMisIM ? 0.5 : 1.0); 
	tt.t_DCmf = 0.0; 
	costof (DCstr, &tt.t_DCmf); 
    } 
    else 
	tt.t_DCov = MissingFeature; 
    cmcostinit ();		/* set up cursor motion costs */ 
} 
 
static 
reset () { 
    if (TIstr) 
	tputs (TIstr, 1, cmputc); 
    if (VSstr) 
	tputs (VSstr, 1, cmputc); 
    losecursor (); 
    wipescreen (); 
} 
 
static 
cleanup () { 
    setHL (1); 
    setIM (1); 
    if (VEstr) 
	tputs (VEstr, 1, cmputc); 
    if (TEstr) 
	tputs (TEstr, 1, cmputc); 
} 
 
/* h set iff changing highlight */ 
static 
wipeline (h, l) { 
    if (SGnum < 0) 
	setHL (1); 
    else if (SGnum > 0 && CEstr && h) 
	ltopos (curY, 0);	/* reposition to kill standout marker */ 
    if (CEstr) 
	tputs (CEstr, ScreenCols - curX, cmputc); 
    else {			/* have to do it the hard way */ 
	register int i = l - curX + 1; 
	if (i > 0) { 
	    setIM (1); 
	    cmplus (i); 
	    while (--i >= 0) 
		putchar (' '); 
	} 
    } 
    if (SGnum >= 0 && h) 
	setHL (0);		/* do the hard standout work */ 
} 
 
static 
wipescreen () { 
    tputs (CLstr, 1, cmputc); 
    if (SGnum >= 0) {		/* clear HLflags */ 
	register int i = tt.t_length; 
	while (--i >= 0) 
	    HLflags[i] = 0; 
    } 
    cmat (0, 0); 
} 
 
static 
delchars (n) register int n; { 
    if (DMstr) { 
	if (DMisIM) { 
	    if (curIM) 
		goto del; 
	    curIM++; 
	} 
	else 
	    setIM (1); 
	tputs (DMstr, 1, cmputc); 
    } 
del: 
    while (--n >= 0) 
	tputs (DCstr, ScreenCols - curX, cmputc); 
    /* bug! /etc/termcap pads EIstr but not EDstr; instead compare DM/IM */ 
    if (EDstr && !DMisIM) 
	tputs (EDstr, 1, cmputc); 
} 
 
/* Read all the interesting stuff out of termcap, and quit if the terminal 
   can't be used. */ 
TrmTERM (term) 
char *term; 
{ 
    static int   AMflag, 
		 BSflag, 
		 MTflag, 
		 NCflag, 
		 NNflag, 
		 RNflag, 
		 XNflag, 
		 XSflag, 
		 XTflag; 
    static int   UGnum; 
    static char *PCstr, 
		*USstr, 
		*UEstr; 
    char  *combuf, 
	  *fill; 
    char   tbuf[1020];		/* Better than 1024, for malloc below */ 
 
    static struct tcsinfo {	/* termcap string info */ 
	char   tcname[3]; 
	char **tcaddr; 
    } tcstrings[] = { 
	"al", &ALstr, 
	"bc", &Left, 
	"cd", &CDstr, 
	"ce", &CEstr, 
	"cl", &CLstr, 
	"ch", &ColPosition, 
	"cm", &AbsPosition, 
	"cr", &CR, 
	"cv", &RowPosition, 
	"dc", &DCstr, 
	"dl", &DLstr, 
	"dm", &DMstr, 
	"ds", &DontSend, 
	"ed", &EDstr, 
	"ei", &EIstr, 
	"ho", &Home, 
	"ic", &ICstr, 
	"im", &IMstr, 
	"ip", &IPstr, 
	"pc", &PCstr, 
	"ll", &LastLine, 
	"nd", &Right, 
	"nl", &Down, 
	"se", &SEstr, 
	"so", &SOstr, 
	"ta", &Tab, 
	"te", &TEstr, 
	"ti", &TIstr, 
	"ue", &UEstr, 
	"up", &Up, 
	"us", &USstr, 
	"vb", &VBstr, 
	"ve", &VEstr, 
	"vs", &VSstr, 
	{0}, 0 
    }; 
    struct tcninfo {		/* termcap numeric (and flag) info */ 
	char   tcname[3]; 
	int   *tcaddr; 
    }; 
    static struct tcninfo tcflags[] = { 
	"MT", &MTflag, 
	"am", &AMflag, 
	"bs", &BSflag, 
	"db", &DBflag, 
	"hz", &HZflag, 
	"in", &tt.t_needspaces, 
	"km", &MetaFlag, 
	"mi", &MIflag, 
	"ms", &MSflag, 
	"nc", &NCflag, 
	"nn", &NNflag, 
	"rn", &RNflag, 
	"ul", &ULflag, 
	"xn", &XNflag, 
	"xs", &XSflag, 
	"xt", &XTflag, 
	{0}, 0 
    }; 
    static struct tcninfo tcnums[] = { 
	"co", &ScreenCols, 
	"li", &ScreenRows, 
	"sg", &SGnum, 
	"tw", &TabWidth, 
	"ug", &UGnum, 
	{0}, 0 
    }; 
 
    Wcm_clear (); 
 
    if (tgetent (tbuf, term) <= 0) { 
	register char *p = getenv ("TERMCAP"); 
        fprintf (stderr, "\ 
I can't find terminal type \"%s\" in %s.\n\ 
See tset(1) for more info on terminal types.\n", 
	    term, p ? p : "/etc/termcap"); 
        exit (1); 
    } 
 
    if ((combuf = malloc (sizeof tbuf)) == 0) 
      abort (); 
    fill = combuf; 
 
 /* Gather the termcap info */ 
    { 
	register struct tcsinfo *p; 
	for (p = tcstrings; p -> tcaddr; p++) 
	    *p -> tcaddr = tgetstr (p -> tcname, &fill); 
    } 
    { 
	register struct tcninfo *p; 
	for (p = tcflags; p -> tcaddr; p++) 
	    *p -> tcaddr = tgetflag (p -> tcname); 
	for (p = tcnums; p -> tcaddr; p++) 
	    *p -> tcaddr = tgetnum (p -> tcname); 
    } 
 
    if (CLstr == 0) 
      { 
dumb: 
	fprintf (stderr, "\ 
Sorry, your terminal (\"%s\") is not powerful enough\n\ 
to run Emacs.  It is missing some important features.\n", term); 
	exit (1); 
      } 
    if (BSflag) 
	Left = "\b";		/* can't possibly be longer! */ 
    if (PCstr) 
	PC = *PCstr; 
    if (MTflag)			/* SWT's name for km */ 
	MetaFlag++; 
    if (TabWidth < 0) 
	TabWidth = 8; 
 
#define default(name,to) if (!name) name = to 
    default (Down, "\n"); 
    default (CR, "\r"); 
    default (Tab, "\t"); 
    default (DontSend, "\4\t\n\r\21\23");/* \4 => ^D, \21 => ^Q, \23 => ^S */ 
#undef default 
 
 /* Sigh ... I *had* to use bitfields, didn't I? */ 
    if (AMflag) 
	AutoWrap = 1; 
    if (RNflag) 
	AutoLF = 1; 
    if (XNflag) 
	MagicWrap = 1; 
 
    if (NCflag) 
	CR = 0; 
    if (NNflag) 
	Down = 0; 
    if (XTflag) 
	Tab = 0; 
    if (*DontSend == 0) 
	DontSend = 0;		/* binary is OK */ 
    if (Wcm_init ())		/* can't do cursor motion */ 
	goto dumb; 
 
    if (SOstr == 0) { 
	SGnum = UGnum; 
	SEstr = UEstr; 
	SOstr = USstr; 
    } 
 
    if (DMstr && IMstr) 
      DMisIM = !strcmp (DMstr, IMstr); 
    if (SGnum < 0 && XSflag) 
	SGnum = 0; 
 
    tt.t_INSmode = INSmode; 
    tt.t_HLmode = HLmode; 
    tt.t_inslines = inslines; 
    tt.t_dellines = dellines; 
    tt.t_blanks = blanks; 
    tt.t_writechars = writechars; 
    tt.t_init = init; 
    tt.t_cleanup = cleanup; 
    tt.t_reset = reset; 
    tt.t_wipeline = wipeline; 
    tt.t_wipescreen = wipescreen; 
    tt.t_topos = topos; 
    tt.t_UpdateEnd = update_end; 
    tt.t_delchars	  /* Zero => i/d char not supported */ 
      = (IMstr && (DMstr || DCstr)) ? delchars : 0; 
    tt.t_padc = PC; 
    if (VBstr) 
	tt.t_flash = flash; 
    tt.t_length = ScreenRows; 
    tt.t_width = ScreenCols; 
    if (AutoWrap && !MagicWrap) 
	tt.t_width--; 
    if (SGnum > 0)		/* a crying shame... */ 
	tt.t_width -= SGnum; 
} 
