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
# Mon Jun  8 14:33:35 PDT 1992
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= fileno.c fread.c vfprintf.c Stdio_Setup.c fgetc.c getw.c putw.c clearerr.c fclose.c fflush.c fgets.c fopen.c fputc.c fputs.c fseek.c fwrite.c gets.c puts.c setvbuf.c sscanf.c ungetc.c _cleanup.c fdopen.c ftell.c perror.c tmpnam.c StdioFileReadProc.c fprintf.c freopen.c fscanf.c getchar.c printf.c putchar.c rewind.c scanf.c setbuf.c setbuffer.c setlinebuf.c sprintf.c vfscanf.c vsprintf.c vprintf.c StdioFileOpenMode.c vsnprintf.c StdioFileCloseProc.c StdioFileWriteProc.c
HDRS		= fileInt.h
MDPUBHDRS	= 
OBJS		= symm.md/fileno.o symm.md/fread.o symm.md/vfprintf.o symm.md/Stdio_Setup.o symm.md/fgetc.o symm.md/getw.o symm.md/putw.o symm.md/clearerr.o symm.md/fclose.o symm.md/fflush.o symm.md/fgets.o symm.md/fopen.o symm.md/fputc.o symm.md/fputs.o symm.md/fseek.o symm.md/fwrite.o symm.md/gets.o symm.md/puts.o symm.md/setvbuf.o symm.md/sscanf.o symm.md/ungetc.o symm.md/_cleanup.o symm.md/fdopen.o symm.md/ftell.o symm.md/perror.o symm.md/tmpnam.o symm.md/StdioFileReadProc.o symm.md/fprintf.o symm.md/freopen.o symm.md/fscanf.o symm.md/getchar.o symm.md/printf.o symm.md/putchar.o symm.md/rewind.o symm.md/scanf.o symm.md/setbuf.o symm.md/setbuffer.o symm.md/setlinebuf.o symm.md/sprintf.o symm.md/vfscanf.o symm.md/vsprintf.o symm.md/vprintf.o symm.md/StdioFileOpenMode.o symm.md/vsnprintf.o symm.md/StdioFileCloseProc.o symm.md/StdioFileWriteProc.o
CLEANOBJS	= symm.md/fileno.o symm.md/fread.o symm.md/vfprintf.o symm.md/Stdio_Setup.o symm.md/fgetc.o symm.md/getw.o symm.md/putw.o symm.md/clearerr.o symm.md/fclose.o symm.md/fflush.o symm.md/fgets.o symm.md/fopen.o symm.md/fputc.o symm.md/fputs.o symm.md/fseek.o symm.md/fwrite.o symm.md/gets.o symm.md/puts.o symm.md/setvbuf.o symm.md/sscanf.o symm.md/ungetc.o symm.md/_cleanup.o symm.md/fdopen.o symm.md/ftell.o symm.md/perror.o symm.md/tmpnam.o symm.md/StdioFileReadProc.o symm.md/fprintf.o symm.md/freopen.o symm.md/fscanf.o symm.md/getchar.o symm.md/printf.o symm.md/putchar.o symm.md/rewind.o symm.md/scanf.o symm.md/setbuf.o symm.md/setbuffer.o symm.md/setlinebuf.o symm.md/sprintf.o symm.md/vfscanf.o symm.md/vsprintf.o symm.md/vprintf.o symm.md/StdioFileOpenMode.o symm.md/vsnprintf.o symm.md/StdioFileCloseProc.o symm.md/StdioFileWriteProc.o
INSTFILES	= symm.md/md.mk symm.md/dependencies.mk Makefile tags TAGS
SACREDOBJS	= 
