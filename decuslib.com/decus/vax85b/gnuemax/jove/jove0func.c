/*R
   Jonathan Payne at Lincoln-Sudbury Regional High School 4-19-83 
 
   jove_funcs.c 
 
   This is like the second half of jove_extend.c.  It was divided 
   because the file was taking too long to compile ...  */ 
 
#include "jove.h" 
#include "termcap.h" 
 
#include <sys/stat.h> 
#ifdef PROCS 
#include <signal.h> 
#endif 
 
#ifdef FCOMPLETE 
extern char *fcomplete(); 
#endif 
 
struct macro	*macros = 0;		/* Macros */ 
 
data_obj	*LastFunc; 
 
AddMac(newm) 
struct macro	*newm; 
{ 
	struct macro	*mp, 
			*prev = 0; 
 
	for (mp = macros; mp; prev = mp, mp = mp->m_nextm) 
		if (mp == newm) 
			return; 
 
	if (prev) 
		prev->m_nextm = newm; 
	else 
		macros = newm; 
	newm->m_nextm = 0; 
	newm->Type = MACRO; 
} 
 
static char	Decimal[] = "%d", 
		Octal[] = "%o", 
		String[] = "\"%s\""; 
 
struct variable	variables[] = { 
/*	VARIABLE, "allow-^S-and-^Q", &OKXonXoff, Decimal, */ 
	VARIABLE, "auto-save-interval", &ASInterval, Decimal, 
	VARIABLE, "case-ignore-search", &CaseIgnore, Decimal, 
	VARIABLE, "comment-begin", (int *)CommentBegin, String, 
	VARIABLE, "comment-column", &CommentColumn, Decimal, 
	VARIABLE, "comment-end", (int *)CommentEnd, String, 
	VARIABLE, "file-creation-mode", &CreatMode, Octal, 
	VARIABLE, "files-should-end-with-newline", &EndWNewline, Decimal, 
	VARIABLE, "internal-tabstop", &tabstop, Decimal, 
	VARIABLE, "make-all-at-once", &MakeAll, Decimal, 
	VARIABLE, "mark-threshold", &MarkThresh, Decimal, 
	VARIABLE, "marks-should-float", &MarksShouldFloat, Decimal, 
#ifdef PROCS 
	VARIABLE, "max-lines-per-process", &PMaxLines, Decimal, 
#endif 
	VARIABLE, "meta-key", &MetaKey, Decimal, 
	VARIABLE, "paren-flash-delay", &PDelay, Decimal, 
	VARIABLE, "physical-tabstop", &phystab, Decimal, 
	VARIABLE, "right-margin", &RMargin, Decimal, 
	VARIABLE, "scroll-step", &ScrollStep, Decimal, 
	VARIABLE, "use-buffer-editor", &UseBufferEdit, Decimal, 
	VARIABLE, "use-temporary-buffers", &UseBuffers, Decimal, 
	VARIABLE, "visible-bell", &VisBell, Decimal, 
	VARIABLE, "write-files-on-make", &WtOnMk, Decimal, 
	VARIABLE, 0, 0, 0 
}; 
 
 
AskInt(prompt, fmt) 
char	*prompt, 
	*fmt; 
{ 
	char	*val = ask((char *) 0, prompt); 
	int	value; 
 
	if (strcmp(val, "on") == 0) 
		value = 1; 
	else if (strcmp(val, "off") == 0) 
		value = 0; 
	else 
		if (sscanf(val, fmt, &value) != 1) 
			value = 0; 
	return value; 
}	 
 
PrVar() 
{ 
	struct variable	*var; 
	char	fmt[20]; 
 
	if ((var = (struct variable *) findcom(variables, FuncName(), NOTHING)) == 0) 
		return; 
	ignore(sprintf(fmt, "%%s => %s", var->Base)); 
	if (var->Base == String) 
		s_mess(fmt, var->Name, var->Value); 
	else 
		s_mess(fmt, var->Name, *(var->Value)); 
} 
 
SetVar() 
{ 
	int	value; 
	char	*strval; 
	struct variable	*vp; 
 
	if ((vp = (struct variable *) findcom(variables, FuncName(), NOTHING)) == 0) 
		return; 
	if (vp->Base == String) { 
		strval = ask((char *)vp->Value, "%s%s", FuncName(), vp->Name); 
		if (strval != (char *)vp->Value) 
			strcpy((char *)vp->Value, strval); 
	} else { 
		value = AskInt(sprint("%s%s ", FuncName(), vp->Name), vp->Base); 
		*(vp->Value) = value; 
	} 
} 
 
XonXoffMode() 
{ 
	OKXonXoff = (!OKXonXoff); 
	ReInitTTY(); 
	UpdModLine++; 
} 
 
struct funct * 
FindFunc(func) 
register int 	(*func)(); 
{ 
	register struct funct	*fp; 
 
  	for (fp = commands; fp->Name; fp++) 
		if (fp->f_func == func) 
			return fp; 
	return 0; 
} 
 
BindFunc(map, letter, func) 
data_obj	*map[]; 
int	(*func)(); 
{ 
	static struct funct	*fp = 0; 
 
	if (fp == 0 || fp->f_func != func) 
		fp = FindFunc(func); 
	map[letter] = (data_obj *) fp; 
} 
 
char * 
FuncName() 
{ 
	static char	func[60]; 
 
	return sprintf(func, ": %s ", LastFunc->Name); 
} 
 
int	Interactive;	/* True when we invoke with the command handler? */ 
 
ExecFunc(fp, interactive) 
data_obj	*fp; 
{ 
	int	bindinteract = Interactive; 
 
	Interactive = interactive; 
	LastFunc = fp; 
	switch (fp->Type) { 
	case MACRO: 
		DoMacro(fp); 
		break; 
 
	case FUNCTION: 
	    { 
	    	struct funct	*func = (struct funct *) fp; 
		if (func->f_func) 
			(*func->f_func)(); 
	    } 
	} 
	Interactive = bindinteract; 
} 
 
/* Can only define the keyboard macro.  IT can be renamed to another 
   macro.  If the keyboard macro is renamed, we make a copy of it. 
   The keyboard macro is ALWAYS the keyboard macro. 
  
   We can't define or run the same macro recursively.  Each macro has  
   a bit saying whether or not it is currently being executed/defined. */ 
 
struct macro	KeyMacro;	/* Macro used for defining */ 
 
#define NMACROS	40		/* This is bad bad BAD! */ 
 
struct macro	*macstack[NMACROS]; 
int	stackp = 0; 
 
char	*rem = "remember"; 
 
FixMacros() 
{ 
	register int	i; 
	struct macro	*mp; 
 
	for (i = 0; macstack[i]; i++) { 
		mp = macstack[i]; 
		macstack[i] = 0; 
		mp->Flags = mp->Offset = 0; 
	} 
	stackp = -1; 
	KeyMacro.Flags = KeyMacro.Offset = 0; 
} 
 
MacErr(name1, name2) 
char	*name1, 
	*name2; 
{ 
	KeyMacro.Flags = 0; 
	MacNolen(&KeyMacro); 
	complain("Can't %s recursively; no longer %sing", name1, name2); 
} 
 
Remember() 
{ 
	if (KeyMacro.Flags & DEFINE) 
		MacErr(rem, rem); 
	KeyMacro.Flags |= DEFINE; 
	MacNolen(&KeyMacro); 
	message("Remembering..."); 
} 
 
/* Is `c' a prefix character */ 
 
PrefChar(c) 
{ 
	return (int) IsPrefix(mainmap[c]); 
} 
 
Forget() 
{ 
	char	*cp; 
	struct macro	*m = &KeyMacro; 
 
	if (m->Flags & DEFINE) { 
		message("Keyboard macro defined"); 
		m->Flags &= ~DEFINE; 
		cp = &m->Body[m->MacLength - 2]; 
		if (PrefChar(*cp)) 
			m->MacLength -= 2; 
		else if (commands[*++cp].f_func == Forget) 
			m->MacLength--; 
	} 
} 
 
ExecMacro() 
{ 
	DoMacro(&KeyMacro); 
} 
 
DoMacro(mac) 
struct macro	*mac; 
{ 
	if (mac->Flags & DEFINE) 
		MacErr("define", rem); 
	if (mac->Flags & EXECUTE) 
		MacErr("execute", "execut"); 
	if (++stackp >= NMACROS) 
		complain("Too many macros at once!"); 
	macstack[stackp] = mac; 
	mac->Offset = 0; 
	mac->Ntimes = exp; 
	mac->Flags |= EXECUTE; 
} 
 
struct macro * 
macexists(name) 
char	*name; 
{ 
	register struct macro	*mp; 
 
	for (mp = macros; mp; mp = mp->m_nextm) 
		if (strcmp(mp->Name, name) == 0) 
			return mp; 
	return 0; 
} 
 
NameMac() 
{ 
	char	*name; 
	struct macro	*m; 
 
	if (KeyMacro.MacLength == 0) 
		complain("Define it first!"); 
	if (KeyMacro.Flags & (DEFINE | EXECUTE)) 
		complain("Can't name while defining/executing"); 
	if ((m = macexists(name = ask((char *) 0, FuncName()))) == 0) 
		m = (struct macro *) emalloc(sizeof *m); 
	else { 
		if (strcmp(name, KeyMacro.Name) == 0) 
			complain("Can't name it that!"); 
		free(m->Name); 
		free(m->Body); 
	} 
	name = copystr(name); 
	m->Type = KeyMacro.Type; 
	m->MacLength = KeyMacro.MacLength; 
	m->MacBuflen = KeyMacro.MacBuflen; 
	m->Body = emalloc(m->MacBuflen); 
	copynchar(m->Body, KeyMacro.Body, m->MacLength); 
	m->Ntimes = m->Flags = m->Offset = 0;	/* At the beginning */ 
	m->Name = name; 
	AddMac(m); 
}	 
 
MacInit() 
{ 
	AddMac(&KeyMacro); 
	MacNolen(&KeyMacro); 
	KeyMacro.Name = "keyboard-macro"; 
	KeyMacro.MacBuflen = 16; 
	KeyMacro.Body = emalloc(KeyMacro.MacBuflen); 
	KeyMacro.Ntimes = KeyMacro.Flags = 0; 
	FixMacros(); 
} 
 
MacNolen(m) 
struct macro	*m; 
{ 
	m->MacLength = m->Offset = 0; 
} 
 
MacPutc(c) 
int	c; 
{ 
	if (KeyMacro.MacLength >= KeyMacro.MacBuflen) { 
		KeyMacro.Body = realloc(KeyMacro.Body, (unsigned) KeyMacro.MacLength + 16); 
		if (KeyMacro.Body == 0) 
			MacErr("malloc", rem); 
		KeyMacro.MacBuflen += 16; 
	} 
	KeyMacro.Body[KeyMacro.Offset++] = c; 
	KeyMacro.MacLength++; 
} 
 
MacGetc() 
{ 
	if (macstack[stackp]->Offset == macstack[stackp]->MacLength) { 
		macstack[stackp]->Offset = 0; 
		if (--macstack[stackp]->Ntimes == 0) { 
			macstack[stackp]->Flags &= ~EXECUTE; 
			stackp--; 
		} 
		return (*Getchar)(); 
	} 
	return macstack[stackp]->Body[macstack[stackp]->Offset++]; 
} 
 
extern int	InJoverc; 
 
extern data_obj *FindCom();	/* In func_defs.c */ 
 
data_obj * 
FindVariable(prompt, flags) 
char	*prompt; 
{ 
	char	*strings[512]; 
	register char	**strs = strings; 
	register int	com; 
	register struct variable	*f = variables; 
 
	for (; f->Name; f++) 
		*strs++ = f->Name; 
	*strs = 0; 
 
	if ((com = complete(strings, prompt, flags)) < 0) 
		return InJoverc ? (data_obj *) -1 : 0; 
	f = &variables[com]; 
	return (data_obj *) f; 
} 
 
data_obj * 
FindMac(prompt, flags) 
char	*prompt; 
{ 
	char	*strings[512]; 
	register char	**strs = strings; 
	register int	com; 
	register struct macro	*m = macros; 
 
	for (; m; m = m->m_nextm) 
		*strs++ = m->Name; 
	*strs = 0; 
 
	if ((com = complete(strings, prompt, flags)) < 0) 
		return InJoverc ? (data_obj *) -1 : 0; 
	m = macros; 
	while (--com >= 0) 
		m = m->m_nextm; 
	return (data_obj *) m; 
} 
 
data_obj * 
findcom(funcs, prompt, flags) 
data_obj	*funcs; 
char	*prompt; 
{ 
	if (funcs == (data_obj *) commands) 
		return FindCom(prompt, flags); 
	if (funcs == (data_obj *) macros) 
		return FindMac(prompt, flags); 
	if (funcs == (data_obj *) variables) 
		return FindVariable(prompt, flags); 
	return 0; 
} 
 
RunMacro() 
{ 
	struct macro	*m; 
 
	if (m = (struct macro *) findcom(macros, FuncName(), NOTHING)) 
		DoMacro(m); 
} 
 
WriteMacs() 
{ 
	struct macro	*m; 
	int	macfd, 
		namelen; 
	char	*file, 
		buf[256], 
		fbuf[MAXFILLEN]; 
 
	ignore(sprintf(buf, "%s/.jove_macros", getenv("HOME"))); 
	file = ask_file(buf, FuncName(), fbuf, FIL_ONLY); 
	if ((macfd = creat(file, 0644)) == -1) 
		complain(IOerr("create", file)); 
	/* Don't write the keyboard macro which is always the first */ 
	for (m = macros->m_nextm; m; m = m->m_nextm) { 
		if (m->MacLength == 0) 
			continue; 
		ignore(write(macfd, (char *) &m->MacLength, sizeof m->MacLength)); 
		namelen = strlen(m->Name) + 1;	/* Including the null */ 
		ignore(write(macfd, (char *) &namelen, sizeof namelen)); 
		ignore(write(macfd, m->Name, namelen)); 
		ignore(write(macfd, m->Body, m->MacLength)); 
	} 
	ignore(close(macfd)); 
} 
 
ReadMacs() 
{ 
	char	*file; 
	struct macro	*m; 
	int	macfd, 
		namelen, 
		bodylen; 
 
	file = ask((char *) 0, FuncName()); 
	if ((macfd = open(file, 0)) == -1) 
		complain(IOerr("open", file)); 
 
	while (read(macfd, (char *) &bodylen, sizeof m->MacLength) == sizeof m->MacLength) { 
		m = (struct macro *) emalloc (sizeof *m); 
		m->MacLength = bodylen; 
		m->MacBuflen = (m->MacLength + 16) & ~017; 
		ignore(read(macfd, (char *) &namelen, sizeof namelen)); 
		m->Name = emalloc(namelen); 
		ignore(read(macfd, m->Name, namelen)); 
		m->Body = emalloc(m->MacBuflen); 
		ignore(read(macfd, m->Body, m->MacLength)); 
		AddMac(m); 
	} 
	ignore(close(macfd)); 
} 
 
is_directory(filename) 
	register char *filename; 
{ 
	struct stat stbuf; 
 
	if (stat(filename, &stbuf)) 
		return(0); 
	return((int)((stbuf.st_mode & S_IFMT) == S_IFDIR)); 
} 
 
#ifdef DIRLIST 
static 
strip_trailing_slashes(s) 
	char *s; 
{ 
	register char *cp; 
	if (s == 0) return; 
	cp = s + strlen(s) - 1; 
	while (cp > s && *cp == '/') 
		cp--; 
	*++cp = '\0'; 
} 
 
DirList() 
{ 
	char *directory; 
	char filespec[MAXFILLEN]; 
 
	directory = ask_file(getenv("HOME"), FuncName(), filespec, 0); 
	do_dirTell(directory, "Directory listing", is_directory(directory)); 
} 
 
do_dirTell(dirp, bufp, star) 
	char *dirp, *bufp; 
{ 
	char **possible; 
	char *vv[2]; 
	int i, what, freep; 
	int verbose = exp_p; 
	strip_trailing_slashes(dirp); 
	freep = needsGlobbing(dirp); 
	if (star && !freep) { 
		if (is_directory(dirp)) 
			strcat(dirp, "/*"); 
		else 
			strcat(dirp, "*"); 
		freep = 1; 
	} 
	if (freep) { 
		possible = (char **) glob(dirp, 0); 
	} else { 
		vv[0] = dirp; 
		vv[1] = 0; 
		possible = vv; 
	} 
	if (UseBuffers) 
		TellWBuffers(bufp, 1); 
	else 
		TellWScreen(0); 
	if (possible) { 
		register char **pp = possible; 
		do { 
			what = tellFile(*pp, verbose); 
			if (what == ABORT) { 
				if (freep) blkfree(possible); 
				complain("Aborted."); 
			} 
			if (what == STOP) 
				break; 
		} while (*pp++); 
		if (freep) blkfree(possible); 
	} 
	if (UseBuffers) 
		ToFirst(); 
	StopTelling(); 
} 
 
static 
tellFile(fname, verb) 
	char *fname; 
	int verb; 
{ 
	static char *doLine = 0; 
	static char *thisFile; 
	static int  doCount = 0; 
	int status; 
	if (!doLine) { 
		if (MAXWIDTH <= CO) abort(); 
		doLine = emalloc(MAXWIDTH); 
		thisFile = doLine; 
		*thisFile = 0; 
	} 
	if (fname) { 
		int i = strlen(fname); 
		char *cp, *filnam, ftype; 
		struct stat stb; 
		int perLine = CO / 25; 
		int n = 24 + ((CO % 25) / perLine); 
		doCount++; 
		filnam = (char *) dbackup(fname, (char *)fname+i, '/'); 
		if (filnam != fname) filnam++; 
		if (lstat(fname, &stb)) { 
			ftype = '?'; 
		} else { 
			char buf[BUFSIZ]; 
			int cc; 
			switch (stb.st_mode & S_IFMT) { 
			case S_IFDIR: 
				ftype = '/'; break; 
			case S_IFLNK: 
				ftype = '@'; 
				cc = readlink(fname, buf, BUFSIZ); 
				if (cc >= 0) { 
					buf[cc] = 0; 
				} 
				if (stat(fname, &stb) < 0) 
					break; 
				if ((stb.st_mode & S_IFMT) == S_IFDIR) { 
					ftype = '/'; 
				} 
				break; 
			case S_IFSOCK: 
				ftype = '='; break; 
			default: 
				ftype = ((stb.st_mode & ~S_IFMT) & 0111) ? '*' : ' '; 
			} 
		} 
		i = strlen(filnam); 
		if (i > n) { 
			i = n - 1; 
			filnam[i] = 0;   /* truncate if too long */ 
		} 
		cp = sprint("%s%c%-*s", filnam, ftype, n-i, NullStr); 
		strncpy(thisFile, cp, n); 
		thisFile += n; 
		*thisFile = 0; 
		if (doCount == perLine) { 
			thisFile = doLine; 
			doCount = 0; 
			return DoTell(doLine); 
		} 
		return OKAY; 
	} 
	if (doCount) 
		status = DoTell(doLine); 
	else 
		status = STOP; 
	if (doLine) free(doLine); 
	doLine = 0; 
	doCount = 0; 
	return status; 
} 
#endif 
