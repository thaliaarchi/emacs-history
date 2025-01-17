Y
#include "jove.h" 
#include <signal.h> 
#include <ctype.h> 
 
static int	NumArg = 1; 
 
#define ASKSIZE	132 
 
extern int	InJoverc; 
 
NoMacGetc() 
{ 
	int	c; 
 
	if (!InJoverc) 
		redisplay(); 
	if ((c = getchar()) == EOF) 
		finish(SIGHUP); 
	return c & 0177; 
} 
 
static char * 
w_forward(cp) 
register char	*cp; 
{ 
	while (*cp && !isword(*cp)) 
		cp++; 
	while (*cp && isword(*cp)) 
		cp++; 
	return cp; 
} 
 
static char * 
w_backward(base, cp) 
register char	*base, 
		*cp; 
{ 
	while (cp > base && !isword(*(cp - 1))) 
		cp--; 
	while (cp > base && isword(*(cp - 1))) 
		cp--; 
	return cp; 
} 
 
Ctlu(HowToRead) 
int	(*HowToRead)(); 
{ 
	register	c; 
 
	do { 
		NumArg *= 4; 
	} while ((c = (*HowToRead)()) == CTL(U)); 
	return c; 
} 
 
GetArg(HowToRead) 
int	(*HowToRead)(); 
{ 
	register int	c, 
			i = 0; 
 
	if (!isdigit(c = (*HowToRead)())) 
		return (c | 0200); 
	do 
		i = i * 10 + (c - '0'); 
	while (isdigit(c = (*HowToRead)())); 
	NumArg = i; 
	return c; 
} 
 
char * 
RunEdit(c, begin, cp, HowToRead) 
register	c; 
register char	*begin, 
		*cp; 
int	(*HowToRead)(); 
{ 
	switch (c) { 
	case CTL(@): 
		break; 
 
	case CTL(A): 
		cp = begin; 
		break; 
 
	case CTL(B): 
		if (cp > begin) 
			--cp; 
		break; 
 
	case CTL(D): 
		if (!*cp) 
			break; 
		cp++; 
		goto delchar; 
 
	case CTL(W): 
	case META(\177):	/* Delete previous word */ 
	    { 
	    	char	*tcp; 
 
		tcp = w_backward(begin, cp); 
		strcpy(tcp, cp); 
		cp = tcp; 
	    } 
	    break; 
 
	case META(D): 
	case META(d): 
		strcpy(cp, w_forward(cp)); 
		break;		/* Pretty neat huh */ 
 
	case CTL(E): 
		while (*cp) 
			cp++; 
		break; 
 
	case CTL(F): 
		if (*cp) 
			cp++; 
		break; 
 
	case CTL(K): 
		*cp = '\0'; 
		break; 
 
	case '\177': 
	case CTL(H): 
delchar: 
		if (cp == begin) 
			break; 
		strcpy(cp - 1, cp); 
		cp--; 
		break; 
 
	case CTL([):	/* ESCAPE */ 
		c = (*HowToRead)() | 0200; 
		return RunEdit(c, begin, cp, HowToRead); 
 
	case META(B): 
	case META(b): 
		cp = w_backward(begin, cp); 
		break; 
 
 	case META(F): 
	case META(f): 
		cp = w_forward(cp); 
		break; 
 
	case CTL(^): 
	case CTL(Q): 
		c = (*HowToRead)(); 
		/* Fall into... */ 
 
	case CTL(N): 
	case CTL(P): 
		ArgIns(begin, c == CTL(N)); 
		break; 
 
	case CTL(G): 
		return (char *) -1; 
 
	default: 
		if (c & 0200) 
			rbell(); 
		else { 
			insert(c, begin, cp - begin, 1, ASKSIZE); 
			cp++; 
		} 
	} 
	return cp; 
} 
 
extern int	Interactive; 
 
int	Asking = 0; 
char	Minibuf[ASKSIZE]; 
 
/* VARARGS2 */ 
 
char * 
ask(def, fmt, args) 
char	*def, 
	*fmt; 
{ 
	char	*cp, 
		*begin;	/* Beginning of real text */ 
	int	c; 
	extern int	(*Getchar)(); 
	int	(*HowToRead)() = Interactive ? NoMacGetc : Getchar; 
 
	format(Minibuf, fmt, &args); 
	message(Minibuf); 
	Asking = strlen(Minibuf);	/* Entirely for redisplay */ 
	begin = Minibuf + Asking; 
	cp = begin; 
	*cp = '\0'; 
 
	while ((c = (*HowToRead)()) != '\r' && c != '\n') { 
		NumArg = 1; 
		if (c == CTL(U)) 
			c = Ctlu(HowToRead); 
		if (c == CTL([)) 
			c = GetArg(HowToRead); 
		if (c == CTL([)) 
			c = (*HowToRead)() | 0200; 
		if (c == CTL(G)) 
			complain("Aborted"); 
		if (c == CTL(R)) { 
			register char	*tcp = def; 
			if (tcp > 0) { 
				while (*tcp) 
					cp = RunEdit(*tcp++, begin, cp, HowToRead); 
			} 
		} else 
			while (--NumArg >= 0) 
				cp = RunEdit(c, begin, cp, HowToRead); 
 
		/* Show the change */ 
		message(Minibuf); 
		Asking = cp - Minibuf; 
	} 
	Asking = 0; 
	if (cp == begin && *cp == '\0')	/* Nothing was typed */ 
		if (def == 0) 
			complain("No default"); 
		else 
			return def; 
	return begin; 
} 
 
ArgIns(at, next) 
char	*at; 
{ 
	int	inc = next ? 1 : -1; 
 
	if (*(argvp + inc) == 0) 
		inc = 0; 
	argvp += inc; 
	strcpy(at, *argvp); 
} 
 
 
