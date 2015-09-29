/*-
 * makeBoot.c --
 *	Program to take an executable and make it down-loadable by the
 *	Sun PROM monitor boot code.
 *
 * Copyright (c) 1987 by the Regents of the University of California
 *
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *
 *
 */
#ifndef lint
static char rcsid[] =
"$Header$ SPRITE (Berkeley)";
#endif lint

#include    <sys/exec.h>
#include    <sys/file.h>
#include    <errno.h>
extern int  errno;
#include    <sys/types.h>
#include    <sys/socket.h>
#include    <netdb.h>

/*-
 *-----------------------------------------------------------------------
 * main --
 *	The main function. Takes a program as the first argument and an
 *	output file as the second argument. Strips and installs the program
 *	in the output file in the boot directory with the header removed.
 *	The program should be linked to start at 0x4000 (or where ever it
 *	relocates itself to).
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	The output file is created.
 *
 *-----------------------------------------------------------------------
 */
main(argc, argv)
    int	    	  argc;
    char	  **argv;
{
    char	  buffer[1024];
    int	    	  i;
    int	    	  in;
    int	    	  out;
    char    	  *outName;
    unsigned short *sp;
    int	    	  nb;
    struct exec	  header;
    int	    	  totalBytes;
    extern char	  *malloc();
    struct hostent *he;

    if (argc < 3) {
	printf("usage: %s <program> (<output> | -h <host>)\n", argv[0]);
	exit(1);
    }
    
    if (strcmp(argv[2], "-h") == 0) {
	he = gethostbyname(argv[3]);
	if (he == 0) {
	    printf("host %s unknown\n", argv[3]);
	    exit(1);
	} else if (he->h_addrtype != AF_INET) {
	    printf("host not on internet\n");
	    exit(1);
	} else {
	    char  	*q;
	    char  	addrBuf[sizeof("AABBCCDD")];
	    char  	*x = "0123456789ABCDEF";
	    short 	i;
	    char	*p;

	    outName = malloc(sizeof("AABBCCDD") + strlen(BOOTDIR) + 1);

	    q = he->h_addr;
	    sprintf(outName, "%s/", BOOTDIR);
	    p = outName + strlen(outName);
	    for(i = 4; i > 0; i--) {
		*p++ = x[(*q >> 4) & 0xF];
		*p++ = x[(*q++) & 0xF];
	    }
	    *p++ = '\0';
	}
    } else if (argv[2][0] == '/') {
	outName = argv[2];
    } else {
	outName = malloc(strlen(argv[2]) + strlen(BOOTDIR) + 2);
	sprintf(outName, "%s/%s", BOOTDIR, argv[2]);
    }
    
    in = open(argv[1], O_RDONLY, 0);
    if (in < 0) {
	perror(argv[1]);
	exit(1);
    } else {
	out = open(outName, O_CREAT|O_WRONLY|O_EXCL, 0666);
	if ((out < 0) && (errno == EEXIST)) {
	    char  line[80];

	    printf("%s exists, overwrite? ", outName);
	    gets(line);
	    if ((line[0] != 'Y') && (line[0] != 'y')) {
		printf("aborted\n");
		exit(1);
	    }
	    (void)unlink(outName);
	    out = open(outName, O_WRONLY|O_CREAT|O_TRUNC, 0666);
	}
	fchmod(out, 0644);
    }

    /*
     * Strip off header and figure out how much data should really be there.
     */
    nb = read(in, &header, sizeof(header));
    if (nb < 0) {
	printf("Couldn't read header of %s\n", argv[1]);
	exit(1);
    }

    totalBytes = header.a_text + header.a_data;
    
#ifndef min
#define min(a,b) (((a)<(b))?(a):(b))
#endif /* min */

    do {
	nb = read(in, buffer, min(sizeof(buffer),totalBytes));
	totalBytes -= nb;
	write(out, buffer, nb);
    } while ((nb > 0) && (totalBytes > 0));

    if (totalBytes < 0) {
	printf("Copied too much? (%d bytes)\n", -totalBytes);
    } else if (totalBytes > 0) {
	extern char *sys_errlist[];
	
	printf("Couldn't copy entire file: %s (%d bytes left)\n",
	       sys_errlist[errno],
	       totalBytes);
    }
    close(in);
    close(out);
}
