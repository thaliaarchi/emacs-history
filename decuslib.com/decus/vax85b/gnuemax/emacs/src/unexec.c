/* A
 * unexec.c - Convert a running program into an a.out file. 
 *  
 * Author:	Spencer W. Thomas 
 * 		Computer Science Dept. 
 * 		University of Utah 
 * Date:	Tue Mar  2 1982 
 * Copyright (c) 1982 Spencer W. Thomas 
 * 
 * Synopsis: 
 *	unexec( new_name, a_name, data_start, bss_start ) 
 *	char *new_name, *a_name; 
 *	unsigned data_start, bss_start; 
 * 
 * Takes a snapshot of the program and makes an a.out format file in the 
 * file named by the string argument new_name. 
 * If a_name is non-NULL, the symbol table will be taken from the given file. 
 *  
 * The boundaries within the a.out file may be adjusted with the data_start  
 * and bss_start arguments.  Either or both may be given as 0 for defaults. 
 *  
 * Data_start gives the boundary between the text segment and the data 
 * segment of the program.  The text segment can contain shared, read-only 
 * program code and literal data, while the data segment is always unshared 
 * and unprotected.  Data_start gives the lowest unprotected address.  Since 
 * the granularity of write-protection is on 1k page boundaries on the VAX, a 
 * given data_start value which is not on a page boundary is rounded down to 
 * the beginning of the page it is on.  The default when 0 is given leaves the 
 * number of protected pages the same as it was before. 
 *  
 * Bss_start indicates how much of the data segment is to be saved in the 
 * a.out file and restored when the program is executed.  It gives the lowest 
 * unsaved address, and is rounded up to a page boundary.  The default when 0 
 * is given assumes that the entire data segment is to be stored, including 
 * the previous data and bss as well as any additional storage allocated with 
 * break (2). 
 *  
 * This routine is expected to only work on a VAX running 4.1 bsd UNIX and 
 * will probably require substantial conversion effort for other systems. 
 * In particular, it depends on the fact that a process' _u structure is 
 * accessible directly from the user program. 
 * 
 * If you make improvements I'd like to get them too. 
 * harpo!utah-cs!thomas, thomas@Utah-20 
 * 
 */ 
#include <stdio.h> 
#include <sys/param.h> 
#include <sys/dir.h> 
#include <sys/user.h> 
#include <sys/stat.h> 
#include <a.out.h> 
/* #include <core.h> */ 
 
#if !defined(vax) && !defined(m68000)	/* this is ridiculous, it */ 
					/* won''t work anywhere else */ 
#define	N_BADMAG(x) ((x.a_magic != A_MAGIC1) && (x.a_magic != A_MAGIC2) &&\ 
		     (x.a_magic != A_MAGIC3) && (x.a_magic != A_MAGIC4) &&\ 
		     (x.a_magic != A_MAGIC5) && (x.a_magic != A_MAGIC6) ) 
 
#define N_TXTOFF(hdr)	(sizeof hdr) 
#define	OMAGIC	A_MAGIC1	/* Magic number of Writeable text file */ 
#define UPAGES	020 
#endif 
 
#ifdef vax 
#define	UADDR	(0x80000000 - ctob(UPAGES))	/* where _u starts */ 
 
#define PSIZE	    10240 
#endif 
#ifdef m68000 
#define UADDR	(0x1000000 - ctob(UPAGES)) 
 
#define PSIZE	8192 
#endif 
 
#ifndef emacs 
#include <stdio.h> 
#define PERROR perror 
#define ERROR fprintf 
#define ERRORF stderr, 
#else 
#define PERROR(file) error ("Failure operating on %s", file) 
#define ERROR error 
#define ERRORF 
#endif 
 
static struct user u; 
static struct exec hdr, ohdr; 
 
/* **************************************************************** 
 * unexec 
 * 
 * driving logic. 
 */ 
unexec( new_name, a_name, data_start, bss_start ) 
char *new_name, *a_name; 
unsigned data_start, bss_start; 
{ 
    int new, a_out = -1; 
 
    if ( a_name && (a_out = open( a_name, 0 )) < 0 ) 
    { 
	PERROR( a_name ); 
	return -1; 
    } 
    if ( (new = creat (new_name, 0666)) < 0) 
    { 
	PERROR( new_name ); 
	return -1; 
    } 
 
    read_u(); 
    if (make_hdr( new, a_out, data_start, bss_start ) < 0 || 
	copy_text_and_data( new ) < 0 || 
	copy_sym( new, a_out ) < 0) 
    { 
	close( new ); 
	/* unlink( new_name );	    	/* Failed, unlink new a.out */ 
	return -1;	 
    } 
 
    close( new ); 
    if ( a_out >= 0 ) 
	close( a_out ); 
    mark_x( new_name ); 
    return 0; 
} 
 
/* **************************************************************** 
 * read_u 
 * 
 * Get the u structure (from memory.) 
 */ 
static 
read_u() 
{ 
    u = *(struct user *)UADDR;    	/* Simple, when it's in core... */ 
} 
 
/* **************************************************************** 
 * make_hdr 
 * 
 * Make the header in the new a.out from the header in core. 
 * Modify the text and data sizes. 
 */ 
static int 
make_hdr( new, a_out, data_start, bss_start ) 
int new, a_out; 
unsigned data_start, bss_start; 
{ 
    /* Get symbol table info from header of a.out file if given one. */ 
    if ( a_out >= 0 ) 
    { 
	if ( read( a_out, &hdr, sizeof hdr ) != sizeof hdr ) 
	{ 
	    PERROR( "Couldn't read header from a.out file" ); 
	    return -1; 
	} 
 
	if N_BADMAG( hdr ) 
	{ 
	    ERROR( ERRORF "a.out file doesn't have legal magic number\n" ); 
	    return -1; 
	} 
     
	/* Check for agreement between a.out file and program. */ 
    /* This is not valid when the symbol table can come from 
     * an oload incremental load file... */ 
#    ifdef CHECK_A_OUT 
	if (  hdr.a_magic != u.u_exdata.ux_mag  || 
 
	      ( (hdr.a_magic != OMAGIC) && 
	        (hdr.a_text != u.u_exdata.ux_tsize || 
	         hdr.a_data != u.u_exdata.ux_dsize) )  || 
 
	      ( (hdr.a_magic == OMAGIC) && 
		(hdr.a_data + hdr.a_text != u.u_exdata.ux_dsize) )  || 
 
	      hdr.a_entry != u.u_exdata.ux_entloc  ) 
	{ 
	    ERROR( ERRORF 
		"unexec: Program didn't come from given a.out\n" ); 
	    return -1; 
	} 
#    endif CHECK_A_OUT 
    } 
    else 
	hdr.a_syms = 0;			/* No a.out, so no symbol info. */ 
 
    /* Construct header from user structure. */ 
    hdr.a_magic = u.u_exdata.ux_mag; 
    if(N_BADMAG(hdr)) 
      { 
	ERROR(ERRORF "unexec: bad magic number %d.  Check u. layout\n"); 
	return -1; 
      } 
    hdr.a_text = u.u_exdata.ux_tsize; 
    hdr.a_data = u.u_exdata.ux_dsize; 
    hdr.a_bss = u.u_exdata.ux_bsize; 
			    /* hdr.a_syms is set above. */ 
    hdr.a_entry = u.u_exdata.ux_entloc; 
    hdr.a_trsize = 0; 
    hdr.a_drsize = 0; 
 
    /* Save hdr before adjustments, for msgs. */ 
    ohdr = hdr; 
 
    /* Adjust data/bss boundary. */ 
    if ( bss_start != 0 ) 
    { 
	bss_start = (bss_start+01777) & ~01777;	      /* (Up) to page bdry. */ 
	if ( bss_start > ctob( u.u_dsize ) + ctob( u.u_tsize ) ) 
	{ 
	    ERROR( ERRORF 
		"unexec: Specified bss_start( %u ) is past end of program.\n", 
		bss_start ); 
	    return -1; 
	} 
 
	hdr.a_data = bss_start - hdr.a_text;  /* Data between text and bss. */ 
	hdr.a_bss =  ctob( u.u_dsize ) - hdr.a_data;   /* Remainder is bss. */ 
    } 
    else			/* Default - All data is inited now! */ 
    { 
	hdr.a_data = ctob( u.u_dsize ); 
	hdr.a_bss = 0; 
	bss_start = ctob( u.u_dsize ) + ctob( u.u_tsize );	/* At end. */ 
    } 
 
    /* Adjust text/data boundary. */ 
    if ( data_start != 0 ) 
    { 
	data_start = data_start & ~01777;	/* (Down) to page boundary. */ 
	hdr.a_text = data_start;		   /* Size of text segment. */ 
	hdr.a_data = bss_start - hdr.a_text;  /* Data between text and bss. */ 
    } 
    else 
    { 
	data_start = hdr.a_text; 
    } 
 
    /* Chatty... */ 
#ifndef emacs 
    printf( "Text/Data boundary was %u, is now %u\n", 
	ohdr.a_text, hdr.a_text ); 
    printf( "Data/Bss boundary was %u, is now %u\n", 
	ohdr.a_text + ohdr.a_data, hdr.a_text + hdr.a_data ); 
#endif 
 
    if ( data_start > bss_start )	/* Can't have negative data size. */ 
    { 
	ERROR( ERRORF 
	    "unexec: data_start(%u) can't be greater than bss_start( %u ).\n", 
	    data_start, bss_start ); 
	return -1; 
    } 
#ifndef emacs 
    printf( "Data segment size (excluding bss) was %u, is now %u\n", 
	ohdr.a_data, hdr.a_data ); 
#endif 
    if ( write( new, &hdr, sizeof hdr ) != sizeof hdr ) 
    { 
	PERROR( "Couldn't write header to new a.out file" ); 
	return -1; 
    } 
    return 0; 
} 
 
/* **************************************************************** 
 * copy_text_and_data 
 * 
 * Copy the text and data segments from memory to the new a.out 
 */ 
static int 
copy_text_and_data( new ) 
int new; 
{ 
    int ret; 
    char buf[80]; 
 
    lseek( new, (long)N_TXTOFF(hdr), 0 ); 
 
    ret = write (new, 0, hdr.a_text + hdr.a_data); 
    if (ret != hdr.a_text + hdr.a_data) 
      { 
 	sprintf(buf, "Write failure in unexec: \ 
ret %d s.b. %u mgc %d txt %u dta %u", 
 	  ret, hdr.a_text + hdr.a_data, hdr.a_magic, hdr.a_text, hdr.a_data); 
 	PERROR(buf); 
	return -1; 
      } 
    return 0; 
} 
 
/* **************************************************************** 
 * copy_sym 
 * 
 * Copy the relocation information and symbol table from the a.out to the new 
 */ 
static int 
copy_sym( new, a_out ) 
int new, a_out; 
{ 
    char page[PSIZE]; 
    int n; 
 
    if ( a_out < 0 ) 
	return 0; 
 
    lseek( a_out, (long)N_SYMOFF(ohdr), 0 );	/* Position a.out to symtab.*/ 
    while ( (n = read( a_out, page, PSIZE )) > 0 ) 
    { 
	if ( write( new, page, n ) != n ) 
	{ 
	    PERROR( "Error writing symbol table to new a.out" ); 
	    ERROR( ERRORF "new a.out should be ok otherwise\n" ); 
	    return 0; 
	} 
    } 
    if ( n < 0 ) 
    { 
	PERROR( "Error reading symbol table from a.out,\n" ); 
	ERROR( ERRORF "new a.out should be ok otherwise\n" ); 
    } 
    return 0; 
} 
 
/* **************************************************************** 
 * mark_x 
 * 
 * After succesfully building the new a.out, mark it executable 
 */ 
static 
mark_x( name ) 
char *name; 
{ 
    struct stat sbuf; 
    int um; 
 
    um = umask( 777 ); 
    umask( um ); 
    if ( stat( name, &sbuf ) == -1 ) 
    { 
	PERROR ( "Can't stat new a.out" ); 
	ERROR( ERRORF "Setting protection to %o\n", 0777 & ~um ); 
	sbuf.st_mode = 0777; 
    } 
    sbuf.st_mode |= 0111 & ~um; 
    if ( chmod( name, sbuf.st_mode ) == -1 ) 
	PERROR( "Couldn't change mode of new a.out to executable" ); 
 
} 
