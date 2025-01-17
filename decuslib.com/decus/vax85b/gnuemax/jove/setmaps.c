Y
#define TXT_TO_C 
 
#include "func_defs.c" 
 
#undef EOF 
#undef FILE 
#include <stdio.h> 
 
match(choices, what) 
register struct funct	choices[]; 
register char	*what; 
{ 
	register int	len; 
	int	i, 
		found = 0, 
		save, 
		exactmatch = -1; 
 
	len = strlen(what); 
	for (i = 0; choices[i].Name; i++) { 
		if (*what != *choices[i].Name) 
			continue; 
		if (strncmp(what, choices[i].Name, len) == 0) { 
			if (strcmp(what, choices[i].Name) == 0) 
				exactmatch = i; 
			save = i; 
			found++;	/* Found one. */ 
		} 
	} 
 
	if (found == 1) 
		return save; 
	return exactmatch; 
} 
 
char * 
PPchar(c) 
int	c; 
{ 
	static char	str[10]; 
	char	*cp = str; 
 
	if (c == '\033') 
		strcpy(cp, "ESC"); 
	else if (c < ' ') 
		ignore(sprintf(cp, "C-%c", c + '@')); 
	else if (c == '\177') 
		strcpy(cp, "^?"); 
	else 
		ignore(sprintf(cp, "%c", c)); 
	return cp; 
} 
 
main() 
{ 
	FILE	*ifile, 
		*of; 
	char	line[100], 
		comname[70]; 
	int	comnum, 
		unbndnum, 
		ch; 
 
	ifile = stdin; 
	of = stdout; 
	if (ifile == NULL || of == NULL) { 
		printf("Cannot read input or write output\n"); 
		exit(1); 
	} 
	unbndnum = match(commands, "unbound"); 
	while (fgets(line, sizeof line, ifile) != NULL) { 
		if (sscanf(line, "	\"%[^\"]\"", comname) != 1) { 
			fprintf(of, line); 
			ch = 0; 
			continue; 
		} 
		comnum = match(commands, comname); 
		if (comnum < 0) { 
			fprintf(stderr, "Cannot find command %s\n", comname); 
			exit(1); 
		} 
		if (comnum == unbndnum) 
			fprintf(of, "	(data_obj *) 0,                 /* %s */\n", PPchar(ch++)); 
		else 
			fprintf(of, "	(data_obj *) &commands[%d],	/* %s */\n", comnum, PPchar(ch++)); 
	} 
	fclose(of); 
	fclose(ifile); 
	exit(0); 
} 
 
