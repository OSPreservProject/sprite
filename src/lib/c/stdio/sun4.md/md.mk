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
# Mon Jun  8 14:33:25 PDT 1992
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= fileno.c fread.c vfprintf.c Stdio_Setup.c fgetc.c getw.c putw.c clearerr.c fclose.c fflush.c fgets.c fopen.c fputc.c fputs.c fseek.c fwrite.c gets.c puts.c setvbuf.c sscanf.c ungetc.c _cleanup.c fdopen.c ftell.c perror.c tmpnam.c StdioFileReadProc.c fprintf.c freopen.c fscanf.c getchar.c printf.c putchar.c rewind.c scanf.c setbuf.c setbuffer.c setlinebuf.c sprintf.c vfscanf.c vsprintf.c vprintf.c StdioFileOpenMode.c vsnprintf.c StdioFileCloseProc.c StdioFileWriteProc.c
HDRS		= fileInt.h
MDPUBHDRS	= 
OBJS		= sun4.md/fileno.o sun4.md/fread.o sun4.md/vfprintf.o sun4.md/Stdio_Setup.o sun4.md/fgetc.o sun4.md/getw.o sun4.md/putw.o sun4.md/clearerr.o sun4.md/fclose.o sun4.md/fflush.o sun4.md/fgets.o sun4.md/fopen.o sun4.md/fputc.o sun4.md/fputs.o sun4.md/fseek.o sun4.md/fwrite.o sun4.md/gets.o sun4.md/puts.o sun4.md/setvbuf.o sun4.md/sscanf.o sun4.md/ungetc.o sun4.md/_cleanup.o sun4.md/fdopen.o sun4.md/ftell.o sun4.md/perror.o sun4.md/tmpnam.o sun4.md/StdioFileReadProc.o sun4.md/fprintf.o sun4.md/freopen.o sun4.md/fscanf.o sun4.md/getchar.o sun4.md/printf.o sun4.md/putchar.o sun4.md/rewind.o sun4.md/scanf.o sun4.md/setbuf.o sun4.md/setbuffer.o sun4.md/setlinebuf.o sun4.md/sprintf.o sun4.md/vfscanf.o sun4.md/vsprintf.o sun4.md/vprintf.o sun4.md/StdioFileOpenMode.o sun4.md/vsnprintf.o sun4.md/StdioFileCloseProc.o sun4.md/StdioFileWriteProc.o
CLEANOBJS	= sun4.md/fileno.o sun4.md/fread.o sun4.md/vfprintf.o sun4.md/Stdio_Setup.o sun4.md/fgetc.o sun4.md/getw.o sun4.md/putw.o sun4.md/clearerr.o sun4.md/fclose.o sun4.md/fflush.o sun4.md/fgets.o sun4.md/fopen.o sun4.md/fputc.o sun4.md/fputs.o sun4.md/fseek.o sun4.md/fwrite.o sun4.md/gets.o sun4.md/puts.o sun4.md/setvbuf.o sun4.md/sscanf.o sun4.md/ungetc.o sun4.md/_cleanup.o sun4.md/fdopen.o sun4.md/ftell.o sun4.md/perror.o sun4.md/tmpnam.o sun4.md/StdioFileReadProc.o sun4.md/fprintf.o sun4.md/freopen.o sun4.md/fscanf.o sun4.md/getchar.o sun4.md/printf.o sun4.md/putchar.o sun4.md/rewind.o sun4.md/scanf.o sun4.md/setbuf.o sun4.md/setbuffer.o sun4.md/setlinebuf.o sun4.md/sprintf.o sun4.md/vfscanf.o sun4.md/vsprintf.o sun4.md/vprintf.o sun4.md/StdioFileOpenMode.o sun4.md/vsnprintf.o sun4.md/StdioFileCloseProc.o sun4.md/StdioFileWriteProc.o
INSTFILES	= sun4.md/md.mk sun4.md/dependencies.mk Makefile tags TAGS
SACREDOBJS	= 
