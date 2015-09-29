/*
 * stdio.h --
 *
 *	This header file declares the stdio library facilities.  They
 *	provide a general stream facility and routines for formatted
 *	input and output.
 *
 * Copyright 1986, 1988 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *
 * $Header: /sprite/src/lib/include/RCS/stdio.h,v 1.26 90/12/07 23:46:20 rab Exp $ SPRITE (Berkeley)
 */

#ifndef _STDIO_H
#define _STDIO_H

/* 
 * sprite.h is needed for typedefs that are used in some function
 * prototypes.  Unfortunately, some user programs define conflicting
 * typedefs.  Because practically everyone uses stdio.h, we should
 * give advance warning before forcing users to use the typedefs from
 * sprite.h.  This must be done before we can turn on function
 * prototypes for Sprite user program.  (Or, change the prototypes so 
 * that they don't use the Sprite typedefs.)
 */
#include <cfuncproto.h>

#ifdef KERNEL
#include <sprite.h>
#endif

#ifndef EOF
#define EOF (-1)
#endif

#ifndef NULL
#define NULL 0
#endif

#ifndef _CLIENTDATA
typedef int *ClientData;
#define _CLIENTDATA
#endif

#ifndef _VA_LIST
#define _VA_LIST
typedef char *va_list;
#endif

/*
 * The main data structure used by the stdio module is a FILE.  This
 * describes a byte-sequential communication channel.  The channel
 * includes buffer storage and the names of three stream-dependent
 * procedures:
 *
 * The procedure readProc is called when another byte of data is needed
 * and the buffer is empty (readCount == 0).  It should read more data
 * into the buffer, reset readCount and lastAccess, and set the STDIO_EOF
 * flag and/or status field if any problem occurred while reading the data.
 *
 * The writeProc procedure is similar to readProc, except that it is
 * called when the buffer has filled (writeCount just became zero);
 * its job is to write out the contents of the buffer and reset
 * lastAccess and writeCount.  If the flush parameter is non-zero, then
 * the procedure is being called as part of fflush, and it MUST empty
 * the buffer.  Otherwise, the procedure may, if it chooses, increase
 * the size of the buffer and return without actually writing anything.
 *
 * The third procedure, closeProc, is called when the stream is closed
 * (writeProc is also called on close, before closeProc).  CloseProc
 * should take any client-specific closing actions, such as closing
 * the file corresponding to the stream or freeing the buffer space
 * for the stream.  Its return value will be the return value from
 * the fclose call.  If an error occurs while closing the stream, then
 * the FILE structure should not be de-allocated, since the client will
 * need to get at information in it to find out what went wrong.
 *
 * The procedures have the following calling sequences:
 *
 *	void readProc(stream)
 *	    FILE *stream;
 *	{
 *	}

 *	void writeProc(stream, flush)
 *	    FILE *stream;
 *	    Boolean flush;
 *	{
 *	}

 *	int closeProc(stream)
 *	    FILE *stream;
 *	{
 *	}
 *
 * See StdIoFileReadProc, StdIoFileWriteProc, and StdIoFileCloseProc for
 * examples of these procedures.
 */

typedef struct _file {
    unsigned char *lastAccess;	/* Place (in buffer) from which last input
				 * or output byte was read or written
				 * (respectively). */
    int readCount;		/* # of characters that may be read from
				 * buffer before calling readProc to refill
				 * the buffer. */
    int writeCount;		/* # of characters that may be written into
				 * buffer before calling writeProc to empty
				 * the buffer.  WriteProc is called immediately
				 * when the buffer fills, so that this value
				 * is never zero unless the stream is not
				 * currently being used for writing. */
    unsigned char *buffer;	/* Pointer to storage for characters.  NULL
				 * means storage hasn't been allocated yet. */
    int bufSize;		/* Total number of bytes of storage available
				 * in buffer. 0 means storage for buffer hasn't
				 * been allocated yet. */
    void (*readProc)_ARGS_((struct _file *));
				/* Procedure called to refill buffer. */
    void (*writeProc)_ARGS_((struct _file *, Boolean));
				/* Procedure called to empty buffer. */
    int (*closeProc)_ARGS_((struct _file *));
				/* Procedure called to close stream.  NULL
				 * means no procedure to call. */
    ClientData clientData;	/* Additional data for the use of the
				 * procedures above,  e.g. the stream ID used
				 * in kernel calls. */
    int status;			/* Non-zero means an error has occurred while
				 * emptying or filling the buffer.  This field
				 * is set by readProc and writeProc. */
    int flags;			/* Miscellaneous flags.  See below for values.
				 */
    struct _file *nextPtr;	/* For file streams, this is used to link all
				 * file streams together (NULL means end of
				 * list).  For other types of streams, it can
				 * be used for anything desired by the
				 * stream implementor. */
} FILE;

/* Flags for FILEs:
 *
 * STDIO_READ:		Means that this stream is used for input.
 * STDIO_WRITE:		Means that this stream is used for output.
 * STDIO_EOF:		Means that an end-of-file has been encountered
 *			on this stream.  All future reads will fail.
 * STDIO_LINEBUF:	Means that this stream is line-buffered:  flush when
 *			a newline is output or stdin is read.
 * STDIO_NOT_OUR_BUF:  	Means that the buffer for the stream belongs to someone
 *	    	  	else and should not be freed by the stdio library.
 * 
 */

#define STDIO_READ		1
#define STDIO_WRITE		2
#define STDIO_EOF		4
#define STDIO_LINEBUF		8
#define STDIO_NOT_OUR_BUF	16

/*
 *----------------------------------------------------------------------
 *
 * getc --
 * getchar --
 * putc --
 * putchar --
 *
 *	These four macros are used to input the next character from
 *	a FILE or output the next character to a FILE.  Normally they
 *	just move a character to or from a buffer, but if the buffer is
 *	full (or empty) then they call a slow procedure to empty (or fill)
 *	the buffer.
 *
 *	These macros are somewhat gross. putc is a ternary operator
 *	to allow people to say things like
 *
 *	    if (a)
 *	    	  putc(stdout, '\n');
 *	    else ...
 *
 *	If it were a complex expression, the compiler would complain.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Information is modified in stream's buffer.
 *
 *----------------------------------------------------------------------
 */

#ifndef lint
#define getc(stream) 							\
    (((stream)->readCount <= 0) ?   	    	    	    	    	\
	    fgetc(stream) :						\
	    ((stream)->readCount -= 1,		 	    	    	\
	     (stream)->lastAccess += 1,				    	\
	     *((stream)->lastAccess)))

#define putc(c, stream)	    	    		    	    	    	\
    ((((stream)->writeCount <= 1) || ((stream)->flags & STDIO_LINEBUF)) ? \
	    fputc(c, stream) :  		    	    	    	\
	    ((stream)->writeCount -= 1,	    	    	    	    	\
	     (stream)->lastAccess += 1,	    	    	    	    	\
	     *(stream)->lastAccess = c))
#else
_EXTERN int getc _ARGS_((FILE stream));
_EXTERN int putc _ARGS_((int c, FILE stream));
#endif

#define getchar() getc(stdin)

#define putchar(c) putc(c, stdout)

/*
 *----------------------------------------------------------------------
 *
 * ferror --
 * feof --
 *
 *	These two macros return information about whether an error
 *	or end-of-file condition has occurred on a stream.
 *
 * Results:
 *	ferror returns 0 if no error has occurred on the stream;
 *	if an error has occurred then it returns the error code.
 *	feof returns 0 if no end-of-file has been encountered on
 *	the stream, and TRUE (non-zero) if an end-of-file has been
 *	encountered.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

#define ferror(stream) ((stream)->status)
#define feof(stream) ((stream)->flags & STDIO_EOF)


/*
 *----------------------------------------------------------------------
 *
 * Miscellaneous additional things exported by stdio:
 *
 *----------------------------------------------------------------------
 */

/*
 * Handles for standard input and output channels.
 */

extern FILE stdioInFile, stdioOutFile, stdioErrFile;
#define stdin	(&stdioInFile)
#define stdout	(&stdioOutFile)
#define stderr	(&stdioErrFile)

/*
 * Default buffer size:
 */

#define BUFSIZ			4096

/*
 * Flags to setvbuf:
 */

#define _IOFBF		1
#define _IOLBF		2
#define _IONBF		3

/*
 * Relative position indicator for fseek:
 */

#define SEEK_SET	0
#define SEEK_CUR	1
#define SEEK_END	2

/*
 *----------------------------------------------------------------------
 *
 * Procedures exported by the stdio module:
 * (Note that these declarations are missing the "const" modifiers
 * found in the ANSI version...)
 * 
 *----------------------------------------------------------------------
 */

_EXTERN void	clearerr _ARGS_((FILE *stream));
_EXTERN int	fclose _ARGS_((FILE *stream));
_EXTERN FILE *	fdopen _ARGS_((int streamID, char *access));
_EXTERN int	fflush _ARGS_((FILE *stream));
_EXTERN int	fgetc _ARGS_((FILE *stream));
_EXTERN char *	fgets _ARGS_((char *bufferPtr, int maxChars, FILE *stream));
_EXTERN int	fileno _ARGS_((FILE *stream));
_EXTERN FILE *	fopen _ARGS_((_CONST char *fileName, _CONST char *access));
_EXTERN int	fputc _ARGS_((int c, FILE *stream));
_EXTERN int	fputs _ARGS_((char *string, FILE *stream));
_EXTERN int	fread _ARGS_((char *bufferPtr, int size, int numItems,
			      FILE *stream));
_EXTERN FILE *	freopen _ARGS_((_CONST char *fileName,
                                _CONST char *access, FILE *stream));
_EXTERN long	fseek _ARGS_((FILE *stream, int offset, int base));
_EXTERN long	ftell _ARGS_((FILE *stream));
_EXTERN int	fwrite _ARGS_((char *bufferPtr, int size, int numItems,
			       FILE *stream));
_EXTERN char *	gets _ARGS_((char *bufferPtr));
_EXTERN int	getw _ARGS_((FILE *stream));
_EXTERN void	perror _ARGS_((_CONST char *msg));
_EXTERN FILE *	popen _ARGS_((_CONST char *cmd, char *mode));
_EXTERN int	pclose _ARGS_((FILE *ptr));
_EXTERN int      remove _ARGS_((_CONST char *filename));
_EXTERN int      rename _ARGS_((_CONST char *oldname, _CONST char *newname));

#ifdef KERNEL
/*
 * Special-case declarations for kernels:
 * Printf returns void because the old Sys_Printf did.
 * Varargs declarations aren't easy to do across all machines, so 
 * we'll punt on them for now.
 */
_EXTERN void	printf _ARGS_(());
_EXTERN int	fprintf _ARGS_(());
_EXTERN int	scanf _ARGS_(());
_EXTERN char *	sprintf _ARGS_(());
_EXTERN int	sscanf _ARGS_(());
_EXTERN int	fscanf _ARGS_(());
_EXTERN int	vfprintf _ARGS_(());
_EXTERN int	vfscanf _ARGS_(());
_EXTERN int	vprintf _ARGS_(());
_EXTERN char *	vsprintf _ARGS_(());
#else /* KERNEL */
/* 
 * User-mode declarations for the routines in the special-case section:
 * Note that the prototype declarations are actually no-ops until 
 * _ARGS_ is turned on for user code.  Also, the varargs declarations 
 * are only a first cut; there no guarantee they'll actually work when 
 * _ARGS_ is turned on.
 */
_EXTERN int	printf _ARGS_((_CONST char *format, ...));
_EXTERN int	fprintf _ARGS_((FILE *stream, _CONST char *format, ...));
_EXTERN int	scanf _ARGS_((_CONST char *format, ...));
_EXTERN char *	sprintf _ARGS_((char *s, _CONST char *format, ...));
_EXTERN int	sscanf _ARGS_((char *s, _CONST char *format, ...));
_EXTERN int	fscanf _ARGS_((FILE *stream, _CONST char *format, ...));
_EXTERN int	vfprintf _ARGS_((FILE *stream,
                                 _CONST char *format, va_list args));
_EXTERN int	vfscanf _ARGS_((FILE *stream,
                                _CONST char *format, va_list args));
_EXTERN int	vprintf _ARGS_((_CONST char *format, va_list args));
_EXTERN char *	vsprintf _ARGS_((char *string,
                                 _CONST char *format, va_list args));
#endif /* KERNEL */

_EXTERN int	puts _ARGS_((_CONST char *string));
_EXTERN int	putw _ARGS_((int w, FILE *stream));
_EXTERN void	rewind _ARGS_((FILE *stream));
_EXTERN void	setbuf _ARGS_((FILE *stream, char *buf));
_EXTERN void	setbuffer _ARGS_((FILE *stream, char *buf, int size));
_EXTERN void	setlinebuf _ARGS_((FILE *stream));
_EXTERN int	setvbuf _ARGS_((FILE *stream, char *buf, int mode, int size));
_EXTERN FILE *	tmpfile _ARGS_((void));
_EXTERN char *	tmpnam _ARGS_((char *s));
_EXTERN char *	tempnam _ARGS_((char *dir, char *pfx));
_EXTERN int	ungetc _ARGS_((int c, FILE *stream));
_EXTERN void	_cleanup _ARGS_((void));

_EXTERN void	Stdio_Setup _ARGS_((FILE *stream, int readable, int writable,
				unsigned char *buffer, int bufferSize,
				void (*readProc)(FILE * file),
				void (*writeProc)(FILE * file, Boolean flush),
				int (*closeProc)(FILE * file),
				ClientData clientData));

#endif /* _STDIO_H */
