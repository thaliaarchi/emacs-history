Y
/* This is a server for jove sub processes.  It runs the command and 
   signals jove when there is some output ready to send to jove. By the 
   time we get here, out standard output goes to joves process input. */ 
 
#include "jove.h" 
 
#include <signal.h> 
#include <sys/ioctl.h> 
#include <sys/wait.h> 
#ifdef VENIX 
#include <sgtty.h> 
 
struct sgttb sg; 
#endif 
 
struct header { 
	int	pid; 
	int	nbytes; 
	char	buf[512]; 
} header; 
 
#define HEADSIZE	((sizeof header.pid) + sizeof (header.nbytes)) 
 
error(str) 
char	*str; 
{ 
	header.pid = getpid(); 
	header.nbytes = strlen(str); 
	strcpy(header.buf, str); 
	proc_write(&header, header.nbytes + 8); 
	exit(-2); 
} 
 
int	ppid, 
	InputFD, 
	JovesInput; 
 
p_inform() 
{ 
	long	nbytes; 
 
#ifdef VENIX 
	ioctl(JovesInput, TIOCQCNT, &sg); 
	nbytes = (long) sg.sg_ispeed; 
#else 
	ioctl(JovesInput, FIONREAD, (char *) &nbytes); 
#endif venix 
	if (nbytes > 0) 
		kill(ppid, INPUT_SIG); 
} 
 
 
/* This writes some data to jove, but if we are getting too far ahead of 
   jove, we sleep a second to give jove a chance to catch up.  */ 
 
proc_write(ptr, n) 
char	*ptr; 
{ 
	long	nbytes; 
 
#ifdef VENIX 
	ioctl(JovesInput, TIOCQCNT, &sg); 
	nbytes = (long) sg.sg_ispeed; 
#else 
	ioctl(JovesInput, FIONREAD, (char *) &nbytes); 
#endif venix 
	if (nbytes >= sizeof header) 
		kill(ppid, INPUT_SIG); 
 
	ignore(write(1, ptr, n)); 
	alarm(1); 
} 
 
read_pipe() 
{ 
	register int	n; 
	 
	signal(SIGALRM, p_inform); 
 
	while ((header.nbytes = read(InputFD, header.buf, sizeof header.buf)) > 0) { 
		n = HEADSIZE + header.nbytes; 
		proc_write(&header, n); 
	} 
} 
 
/* ARGSUSED */ 
main(argc, argv) 
char	*argv[]; 
{ 
	int	p[2]; 
	int	pid; 
 
	if (pipe(p) == -1) 
		error("Cannot pipe jove portsrv\n"); 
 
	ppid = getppid(); 
	switch (pid = fork()) { 
	case -1: 
		error("Cannot fork: portsrv\n"); 
 
	case 0: 
		/* We'll intercept childs output in p[0] */ 
		dup2(p[1], 1); 
		dup2(p[1], 2); 
		ignore(close(p[0])); 
		ignore(close(p[1])); 
			 
		setpgrp(getpid(), getpid()); 
		execv(argv[2], &argv[3]); 
		_exit(-4); 
 
	default: 
		ignore(close(0)); 
				/* Don't want this guy to read anything 
				   jove sends to our soon to be created 
				   child */ 
 
		JovesInput = atoi(argv[1]); 
		ignorf(signal(SIGINT, SIG_IGN)); 
		ignorf(signal(SIGQUIT, SIG_IGN)); 
		ignore(close(p[1])); 
 
		/* Tell jove the pid of the real child as opposed to us. */ 
		header.pid = getpid(); 
		header.nbytes = sizeof (int); 
		*(int *) header.buf = pid; 
		ignore(write(1, (char *) &header, sizeof pid + HEADSIZE)); 
		p_inform();	/* Inform jove */ 
 
		/* Read proc's output and send it to jove */ 
		InputFD = p[0]; 
		read_pipe(); 
		ignore(close(p[0])); 
		header.pid = getpid(); 
		header.nbytes = EOF;	/* Tell jove we are finished */ 
		ignore(write(1, (char *) &header, HEADSIZE)); 
		p_inform(); 
		/* Try to exit like our child did ... */ 
		{ 
			union wait	w; 
 
#ifndef VMUNIX 
			while (wait2(&w.w_status, 0) != pid) 
#else 
			while (wait3(&w.w_status, 0, 0) != pid) 
#endif 
				; 
			if (WIFEXITED(w)) 
				exit(w.w_retcode); 
			else if (WIFSIGNALED(w)) 
				kill(getpid(), w.w_termsig); 
		} 
	} 
} 
 
 
