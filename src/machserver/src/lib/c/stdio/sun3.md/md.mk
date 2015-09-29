#
# Prototype Makefile for machine-dependent directories.
#
# A file of this form resides in each ".md" subdirectory of a
# command.  Its name is typically "md.mk".  During makes in the
# parent directory, this file (or a similar file in a sibling
# subdirectory) is included to define machine-specific things
# such as additional source and object files.
#
# This Makefile is automatically generated.
# DO NOT EDIT IT OR YOU MAY LOSE YOUR CHANGES.
#
# Generated from /sprite/lib/mkmf/Makefile.md
# Wed Dec  4 12:34:53 PST 1991
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= perror.c StdioFileCloseProc.c StdioFileReadProc.c StdioFileWriteProc.c Stdio_Setup.c _cleanup.c clearerr.c fclose.c fdopen.c fflush.c fgetc.c fgets.c fileno.c fopen.c fprintf.c fputc.c fputs.c fread.c freopen.c fscanf.c fseek.c ftell.c fwrite.c gets.c getw.c printf.c puts.c putw.c rewind.c scanf.c setbuf.c setbuffer.c setlinebuf.c setvbuf.c sprintf.c sscanf.c tmpnam.c ungetc.c vfprintf.c vfscanf.c vprintf.c vsnprintf.c vsprintf.c StdioFileOpenMode.c
HDRS		= fileInt.h
MDPUBHDRS	= 
OBJS		= sun3.md/perror.o sun3.md/StdioFileCloseProc.o sun3.md/StdioFileReadProc.o sun3.md/StdioFileWriteProc.o sun3.md/Stdio_Setup.o sun3.md/_cleanup.o sun3.md/clearerr.o sun3.md/fclose.o sun3.md/fdopen.o sun3.md/fflush.o sun3.md/fgetc.o sun3.md/fgets.o sun3.md/fileno.o sun3.md/fopen.o sun3.md/fprintf.o sun3.md/fputc.o sun3.md/fputs.o sun3.md/fread.o sun3.md/freopen.o sun3.md/fscanf.o sun3.md/fseek.o sun3.md/ftell.o sun3.md/fwrite.o sun3.md/gets.o sun3.md/getw.o sun3.md/printf.o sun3.md/puts.o sun3.md/putw.o sun3.md/rewind.o sun3.md/scanf.o sun3.md/setbuf.o sun3.md/setbuffer.o sun3.md/setlinebuf.o sun3.md/setvbuf.o sun3.md/sprintf.o sun3.md/sscanf.o sun3.md/tmpnam.o sun3.md/ungetc.o sun3.md/vfprintf.o sun3.md/vfscanf.o sun3.md/vprintf.o sun3.md/vsnprintf.o sun3.md/vsprintf.o sun3.md/StdioFileOpenMode.o
CLEANOBJS	= sun3.md/perror.o sun3.md/StdioFileCloseProc.o sun3.md/StdioFileReadProc.o sun3.md/StdioFileWriteProc.o sun3.md/Stdio_Setup.o sun3.md/_cleanup.o sun3.md/clearerr.o sun3.md/fclose.o sun3.md/fdopen.o sun3.md/fflush.o sun3.md/fgetc.o sun3.md/fgets.o sun3.md/fileno.o sun3.md/fopen.o sun3.md/fprintf.o sun3.md/fputc.o sun3.md/fputs.o sun3.md/fread.o sun3.md/freopen.o sun3.md/fscanf.o sun3.md/fseek.o sun3.md/ftell.o sun3.md/fwrite.o sun3.md/gets.o sun3.md/getw.o sun3.md/printf.o sun3.md/puts.o sun3.md/putw.o sun3.md/rewind.o sun3.md/scanf.o sun3.md/setbuf.o sun3.md/setbuffer.o sun3.md/setlinebuf.o sun3.md/setvbuf.o sun3.md/sprintf.o sun3.md/sscanf.o sun3.md/tmpnam.o sun3.md/ungetc.o sun3.md/vfprintf.o sun3.md/vfscanf.o sun3.md/vprintf.o sun3.md/vsnprintf.o sun3.md/vsprintf.o sun3.md/StdioFileOpenMode.o
INSTFILES	= sun3.md/md.mk sun3.md/dependencies.mk Makefile local.mk
SACREDOBJS	= 
