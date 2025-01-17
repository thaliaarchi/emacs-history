Y
/* Jonathan Payne at Lincoln-Sudbury Regional High School 5-25-83 
 
   jove_buf.c 
    
   Contains commands that deal with creating, selecting, killing and 
   listing buffers.  (And find-file) */ 
 
#include "jove.h" 
 
#include <sys/stat.h> 
 
#define MAXBUFFERS 100 
 
char	*Mainbuf = "Main"; 
 
int	flen, 
	blen; 
 
Buffer	*world,		/* First in the list */ 
	*curbuf, 
	*lastbuf = 0;	/* Last buffer we were in so we have a default 
			   buffer during a select buffer. */ 
 
TogMinor(bit) 
{ 
	curbuf->b_minor ^= bit; 
	UpdModLine++; 
} 
 
static  
AllInts(str) 
register char	*str; 
{ 
	register char	c; 
 
	while ((c = *str++) >= '0' && c <= '9') 
		; 
	return c == 0; 
} 
 
Buffer * 
buf_exists(name) 
char	*name; 
{ 
	Buffer	*bp; 
	int	n; 
 
	if (name == 0) 
		return 0; 
 
	for (bp = world; bp; bp = bp->b_next) 
		if (bp->b_name != 0 && strcmp(bp->b_name, name) == 0) 
			return bp; 
 
	/* Doesn't match any names.  Try for a buffer number */ 
 
	if (AllInts(name) && (n = atoi(name)) > 0) { 
		for (bp = world; n > 0; bp = bp->b_next) { 
			if (bp == 0) 
				break; 
			--n; 
		} 
		return bp; 
	} 
	return 0; 
} 
 
Buffer * 
file_exists(fname) 
char	*fname; 
{ 
	struct stat	stbuf; 
	Buffer	*bp; 
 
	if (fname) { 
		if (stat(fname, &stbuf) == -1) 
			stbuf.st_ino = 0; 
		for (bp = world; bp; bp = bp->b_next) { 
			if (stbuf.st_ino != 0 && bp->b_ino == stbuf.st_ino) 
				return bp; 
			if (bp->b_fname == 0) 
				continue; 
			if (strcmp(PathRelative(bp->b_fname), fname) == 0) 
				return bp; 
		} 
	} 
	return 0; 
} 
 
Buffer * 
findbuf() 
{ 
	register Buffer	*bp, 
			*lastbp; 
 
	lastbp = 0; 
	for (bp = world; bp; lastbp = bp, bp = bp->b_next) 
		; 
 
	bp = (Buffer *) emalloc(sizeof (Buffer)); 
	if (lastbp) 
		lastbp->b_next = bp; 
	else 
		world = bp; 
	bp->b_first = 0; 
	bp->b_next = 0; 
 
	return bp; 
} 
 
Buffer * 
mak_buf(fname, bname) 
char	*fname, 
	*bname; 
{ 
	register Buffer	*freebuf; 
	register int	i; 
 
	freebuf = buf_exists(bname); 
	if (!freebuf) { 
		freebuf = findbuf(); 
		freebuf->b_fname = freebuf->b_name = 0; 
		setbname(freebuf, bname); 
		setfname(freebuf, fname); 
		set_ino(freebuf); 
		freebuf->b_marks = 0; 
		freebuf->b_themark = 0;		/* Index into markring */ 
		/* No marks yet */ 
		for (i = 0; i < NMARKS; i++) 
			freebuf->b_markring[i] = 0; 
		freebuf->b_modified = 0; 
		freebuf->b_type = FILE;  /* File until proven SCRATCH */ 
		freebuf->b_minor = DefMinor;	/* Default minor mode */ 
		freebuf->b_major = FUNDMODE; 
		freebuf->b_first = 0; 
		initlist(freebuf); 
	} 
	return freebuf; 
} 
 
char * 
ralloc(obj, size) 
char	*obj; 
{ 
	char	*new; 
	if (obj) 
		new = realloc(obj, (unsigned) size); 
	if (new == 0 || !obj) 
		new = emalloc(size); 
	if (new == 0) 
		error("No memory in ralloc"); 
	return new; 
} 
 
ReNamBuf() 
{ 
	char	*new = 0, 
		*prompt = FuncName(), 
		*second = "%s already exists; new name? "; 
 
	for (;;) { 
		new = ask((char *) 0, prompt, new); 
		if (!buf_exists(new)) 
			break; 
		prompt = second; 
	} 
	setbname(curbuf, new); 
} 
 
setbname(bp, name) 
Buffer	*bp; 
char	*name; 
{ 
	UpdModLine++;	/* Kludge ... but speeds things up considerably */ 
	if (name) { 
		bp->b_name = ralloc(bp->b_name, strlen(name) + 1); 
		strcpy(bp->b_name, name); 
	} else 
		bp->b_name = 0; 
} 
 
setfname(bp, name) 
Buffer	*bp; 
char	*name; 
{ 
	char	wholename[MAXFILLEN]; 
 
	UpdModLine++;	/* Kludge ... but speeds things up considerably */ 
	if (name) { 
		PathParse(name, wholename); 
		bp->b_fname = ralloc(bp->b_fname, strlen(wholename) + 1); 
		strcpy(bp->b_fname, wholename); 
	} else 
		bp->b_fname = 0; 
} 
 
set_ino(bp) 
Buffer	*bp; 
{ 
	struct stat	stbuf; 
 
	if (bp->b_fname == 0 || stat(bp->b_fname, &stbuf) == -1) { 
		bp->b_ino = 0; 
		bp->b_ctime = 0; 
	} else { 
		bp->b_ino = stbuf.st_ino; 
		bp->b_ctime = stbuf.st_ctime; 
	} 
} 
 
/* Find the file `fname' into buf and put in in window `wp' */ 
 
Buffer * 
do_find(wp, fname) 
Window	*wp; 
char	*fname; 
{ 
	Buffer	*oldb = curbuf, 
		*bp; 
 
	oldb = curbuf; 
	bp = file_exists(fname); 
	if (bp == 0) { 
		bp = mak_buf(fname, (char *) 0); 
		bufname(bp); 
		SetBuf(bp); 
		read_file(bp->b_fname); 
		SetBuf(oldb); 
	} 
	if (wp) 
		tiewind(wp, bp); 
	return bp; 
} 
 
FindFile() 
{ 
	char	*name, 
		fnamebuf[MAXFILLEN], 
		**filenames; 
 
	name = ask_file(curbuf->b_fname, FuncName(), fnamebuf, FIL_ONLY); 
	lastbuf = curbuf; 
#ifdef GLOBBER 
	if (needsGlobbing(name)) { 
		filenames = (char **) glob(name, 1); 
		if (filenames) { 
			register char **fnames = filenames; 
			while (*fnames != 0) { 
				SetBuf(do_find(curwind, *fnames)); 
				fnames++; 
			} 
			blkfree(filenames); 
		} 
	} else 
		SetBuf(do_find(curwind, name)); 
#else 
	SetBuf(do_find(curwind, name)); 
#endif 
} 
 
int	DefMinor = 0; 
 
SetBuf(newbuf) 
Buffer	*newbuf; 
{ 
	if (newbuf == curbuf) 
		return; 
	lastbuf = curbuf; 
	lsave(); 
	curbuf = newbuf; 
	resetmode(curbuf); 
	getDOT(); 
 
#ifdef PROCS 
	if (lastbuf->b_type != curbuf->b_type) { 
		if (curbuf->b_type == IPROCESS) 
			PushPBs();		/* Push process bindings */ 
		else if (lastbuf->b_type == IPROCESS) 
			PopPBs(); 
	} 
	assign_p();		/* Set cur_proc */ 
#endif 
} 
 
mkbuflist(bnamp) 
char	**bnamp; 
{ 
	register Buffer	*bp; 
	register int i = 1; 
 
	for (bp = world; bp; bp = bp->b_next) { 
		if (bp->b_name != 0) 
			*bnamp++ = bp->b_name; 
		i++; 
		if (i >= MAXBUFFERS) break; 
	} 
	*bnamp = 0; 
} 
 
char * 
getbexist(prompt) 
char	*prompt; 
{ 
	char	*bnames[100]; 
	int	offset; 
 
	mkbuflist(bnames); 
	offset = complete(bnames, prompt, NOTHING); 
	return offset >= 0 ? bnames[offset] : 0; 
} 
 
char * 
ask_buf() 
 
{ 
	char	*bnames[MAXBUFFERS], 
		*bname; 
	int	offset; 
 
	mkbuflist(bnames); 
	offset = complete(bnames, FuncName(), LF_FORCE|RET_STATE); 
	if (offset == EOF) 
		complain((char *) 0); 
	if (offset == ORIGINAL) 
		bname = Minibuf + strlen(FuncName()); 
	else if (offset == NULLSTRING) { 
		if (lastbuf) 
			bname = lastbuf->b_name; 
		else 
			complain((char *) 0); 
	} else if (offset < 0) 
		complain((char *) 0); 
	else 
		bname = bnames[offset]; 
 
	return bname; 
} 
 
BufSelect() 
{ 
	char	*bname; 
 
	bname = ask_buf(); 
	lastbuf = curbuf; 
	SetBuf(do_select(curwind, bname)); 
} 
 
Buffer * 
do_select(wp, name) 
Window	*wp; 
char	*name; 
{ 
	Buffer	*new; 
 
	new = mak_buf((char *) 0, name); 
	if (wp) 
		tiewind(wp, new); 
	return new; 
} 
 
defb_wind(bp) 
Buffer *bp; 
{ 
	Window	*wp = fwind; 
 
	do { 
		if (wp->w_bufp == bp) { 
			if (bp == curbuf) 
				ignore(do_select(wp, Mainbuf)); 
			else { 
				Window	*save = wp->w_next; 
 
				del_wind(wp); 
				wp = save->w_prev; 
			} 
		}				 
		wp = wp->w_next; 
	} while (wp != fwind); 
} 
 
Buffer * 
getNMbuf() 
{ 
	Buffer	*delbuf; 
	char	*bname; 
 
	bname = ask_buf(); 
	if (strcmp(bname, Mainbuf) == 0) 
		return 0; 
	delbuf = buf_exists(bname);		/* Definitely exists */ 
	if (delbuf->b_modified) 
		confirm("%s modified, are you sure? ", bname); 
	return delbuf; 
} 
 
BufErase() 
{ 
	Buffer	*delbuf; 
 
	if (delbuf = getNMbuf()) { 
		initlist(delbuf); 
		SetUnmodified(delbuf); 
	} 
} 
 
#define okay_free(ptr)	if (ptr) free(ptr) 
 
do_bufkill(delbuf) 
	register Buffer *delbuf; 
{ 
	register Buffer	*b, 
			*lastb = 0; 
	extern Buffer	*perr_buf; 
 
#ifdef PROCS 
	pbuftiedp(delbuf);	/* Make sure buffer is not tied to a process */ 
#endif 
	defb_wind(delbuf); 
	if (curbuf == delbuf) 
		SetBuf(curwind->w_bufp); 
	lfreelist(delbuf->b_first); 
 
	for (b = world; b; lastb = b, b = b->b_next) 
		if (b == delbuf) 
			break; 
	if (lastb) 
		lastb->b_next = delbuf->b_next; 
	else 
		world = delbuf->b_next; 
 
	okay_free((char *) delbuf->b_name); 
	okay_free((char *) delbuf->b_fname); 
	free((char *) delbuf); 
 
	if (delbuf == lastbuf) 
		lastbuf = curbuf; 
	if (perr_buf == delbuf) { 
		ErrFree(); 
		perr_buf = 0; 
	} 
} 
 
BufKill() 
{ 
	Buffer	*delbuf; 
 
	if ((delbuf = getNMbuf()) == 0) 
		return; 
	do_bufkill(delbuf); 
} 
 
int UseBufferEdit = 1;		/* buf edit by default */ 
 
BufList() 
{ 
	int	bcount = 0;		/* To give each buffer a number */ 
	int	tcount = 0; 
	int	lcount = 0; 
	Buffer	*bps[MAXBUFFERS], 
		*bp, 
		*thisbuf = curbuf; 
	Buffer	*bufeditor; 
	int	btyp[MAXBUFFERS]; 
	int	what; 
	char	*format = " %-1s %-2s  %-11s %-*s  %-s"; 
	char	c; 
	int	buf_width = 11; 
	register Buffer	*b; 
 
	if (UseBufferEdit) 
		TellWBuffers("Buffer list", 0x3); 
	else if (UseBuffers) 
		TellWBuffers("Buffer list", 0x1); 
	else 
		TellWScreen(0); 
	bufeditor = curbuf; 
 
#define ALIVE		0 
#define DEATHROW	1 
	for (b = world; b; b = b->b_next) { 
		buf_width = max(buf_width, b->b_name ? strlen(b->b_name) : 0); 
		if (thisbuf == (bps[bcount] = b)) 
			tcount = bcount; 
		if (lastbuf == b) 
			lcount = bcount; 
		btyp[bcount] = ALIVE; 
		bcount++; 
	} 
	bps[bcount] = world; 
	bcount = -1; 
 
#define HDRLINES 4 
	ignore(DoTell("(* means buffer needs saving.)")); 
	ignore(DoTell(NullStr)); 
	ignore(DoTell(format, NullStr, "NO", "Buffer Type", buf_width, "Buffer Name", "File Name")); 
	ignore(DoTell(format, NullStr, "--", "------ ----", buf_width, "------ ----", "---- ----")); 
	for (bp = world; bp; bp = bp->b_next) { 
		bcount++; 
		what = DoTell(format,	IsModified(bp) ? "*" : NullStr, 
					itoa(bcount), 
					bp->b_type == SCRATCH ? 
							"Scratch" : 
					bp->b_type == PROCESS ? 
							"Process" : 
					bp->b_type == IPROCESS ? 
							"I-process" : 
					bp->b_type == FILE ?  
							"File" : "Unknown", 
					buf_width, 
					/* For the * (variable length field) */ 
					bp->b_name, 
					filename(bp)); 
 
		if (what == ABORT || what == STOP) 
			break; 
	} 
	if (UseBuffers || UseBufferEdit) { 
		ToFirst();		/* Go to the beginning of the file */ 
		NotModified(); 
	} 
	StopTelling(); 
	if (!UseBufferEdit) return; 
#define MarkDeath DelNChar(); Insert('D'); btyp[tcount++] = DEATHROW; 
#define MarkLive  DelNChar(); Insert(' '); btyp[tcount++] = ALIVE; 
#define MarkClean ForChar(); DelNChar(); Insert(' '); 
	c = '.'; 
	do { 
		bp = bps[tcount]; 
		switch (c) { 
		case 'd': case 'D': case CTL(D): case CTL(?): 
			if (tcount == 0) { 
				message("Cannot kill buffer \"Main\""); 
			} else { 
				MarkDeath; 
			} 
			break; 
		case 'n': case 'N': case CTL(N): case '\n': case '\r': 
			tcount++; break; 
		case 'p': case 'P': case CTL(P): case CTL(H): 
			tcount--; break; 
		case 'f': case 'F': case CTL(F): 
			tcount += SIZE(curwind) - 1; break; 
		case 'b': case 'B': case CTL(B): 
			tcount -= SIZE(curwind) - 1; break; 
		case 'u': case 'U': 
			MarkLive; break; 
		case '<': case CTL(A): 
			tcount = 0; break; 
		case '>': case CTL(E): 
			tcount = bcount; break; 
		case 'l': case 'L': case CTL(L): 
			RedrawDisplay(); break; 
		case 's': case 'S': case CTL(S): 
			if (bp->b_fname) { 
				SetBuf(bp); 
				SaveFile(); 
				SetBuf(bufeditor); 
				MarkClean; 
			} else 
				message("Sorry, buffer has no filename..."); 
			break; 
		case CTL(G): case 'q': case 'Q': case CTL(Q): 
			bp = thisbuf; 
			goto abort_BufEdit; 
		case '.': 
			break; 
		default: 
			message("Valid commands: <sp>, d, n, p, f, b, u, <, >, s, q"); 
		} 
		if (tcount > bcount) 
			tcount = bcount; 
		else if (tcount < 0) 
			tcount = 0; 
		SetLine(next_line(curbuf->b_first, tcount+HDRLINES)); 
		redisplay(); 
	} while ((c = getchar()) != ' '); 
	for (what = 0; what <= bcount; what++) { 
		if (btyp[what] == DEATHROW) { 
			do_bufkill(bps[what]); 
			if (what == tcount) tcount = 0; 
			if (what == lcount) lcount = 0; 
		} 
	} 
	bp = bps[tcount]; 
abort_BufEdit: 
	SetBuf(do_select(curwind, bp->b_name)); 
	lastbuf = bps[lcount]; 
} 
 
bufname(bp) 
Buffer	*bp; 
{ 
	char	tmp[100], 
		tmp1[100], 
		*cp; 
	int	try = 1; 
 
	if ((cp = bp->b_fname) == 0) 
		complain("No file name"); 
		 
	if (*cp == '.') 
		++cp; 
	strcpy(tmp, cp); 
	cp = rindex(tmp, '/'); 
	if (cp) 
		strncpy(tmp, &cp[1], sizeof tmp); 
 
	strcpy(tmp1, tmp); 
	while (buf_exists(tmp)) { 
		ignore(sprintf(tmp, "%s.%d", tmp1, try)); 
		try++; 
	} 
	setbname(bp, tmp); 
} 
 
initlist(bp) 
Buffer	*bp; 
{ 
	lfreelist(bp->b_first); 
	ignore(listput(bp, (Line *) 0));		/* First line in Buffer */ 
	 
	bp->b_dot->l_dline = NullLine; 
	bp->b_char = 0; 
	AllMarkSet(bp, bp->b_dot, 0); 
	if (bp == curbuf) 
		getDOT(); 
	initwinds(bp); 
} 
 
 
