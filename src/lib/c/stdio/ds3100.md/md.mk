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
# Mon Jun  8 14:33:03 PDT 1992
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= fileno.c fread.c vfprintf.c Stdio_Setup.c fgetc.c getw.c putw.c clearerr.c fclose.c fflush.c fgets.c fopen.c fputc.c fputs.c fseek.c fwrite.c gets.c puts.c setvbuf.c sscanf.c ungetc.c _cleanup.c fdopen.c ftell.c perror.c tmpnam.c StdioFileReadProc.c fprintf.c freopen.c fscanf.c getchar.c printf.c putchar.c rewind.c scanf.c setbuf.c setbuffer.c setlinebuf.c sprintf.c vfscanf.c vsprintf.c vprintf.c StdioFileOpenMode.c vsnprintf.c StdioFileCloseProc.c StdioFileWriteProc.c
HDRS		= fileInt.h
MDPUBHDRS	= 
OBJS		= ds3100.md/StdioFileCloseProc.o ds3100.md/StdioFileOpenMode.o ds3100.md/StdioFileReadProc.o ds3100.md/StdioFileWriteProc.o ds3100.md/Stdio_Setup.o ds3100.md/_cleanup.o ds3100.md/clearerr.o ds3100.md/fclose.o ds3100.md/fdopen.o ds3100.md/fflush.o ds3100.md/fgetc.o ds3100.md/fgets.o ds3100.md/fileno.o ds3100.md/fopen.o ds3100.md/fprintf.o ds3100.md/fputc.o ds3100.md/fputs.o ds3100.md/fread.o ds3100.md/freopen.o ds3100.md/fscanf.o ds3100.md/fseek.o ds3100.md/ftell.o ds3100.md/fwrite.o ds3100.md/getchar.o ds3100.md/gets.o ds3100.md/getw.o ds3100.md/perror.o ds3100.md/printf.o ds3100.md/putchar.o ds3100.md/puts.o ds3100.md/putw.o ds3100.md/rewind.o ds3100.md/scanf.o ds3100.md/setbuf.o ds3100.md/setbuffer.o ds3100.md/setlinebuf.o ds3100.md/setvbuf.o ds3100.md/sprintf.o ds3100.md/sscanf.o ds3100.md/tmpnam.o ds3100.md/ungetc.o ds3100.md/vfprintf.o ds3100.md/vfscanf.o ds3100.md/vprintf.o ds3100.md/vsnprintf.o ds3100.md/vsprintf.o
CLEANOBJS	= ds3100.md/fileno.o ds3100.md/fread.o ds3100.md/vfprintf.o ds3100.md/Stdio_Setup.o ds3100.md/fgetc.o ds3100.md/getw.o ds3100.md/putw.o ds3100.md/clearerr.o ds3100.md/fclose.o ds3100.md/fflush.o ds3100.md/fgets.o ds3100.md/fopen.o ds3100.md/fputc.o ds3100.md/fputs.o ds3100.md/fseek.o ds3100.md/fwrite.o ds3100.md/gets.o ds3100.md/puts.o ds3100.md/setvbuf.o ds3100.md/sscanf.o ds3100.md/ungetc.o ds3100.md/_cleanup.o ds3100.md/fdopen.o ds3100.md/ftell.o ds3100.md/perror.o ds3100.md/tmpnam.o ds3100.md/StdioFileReadProc.o ds3100.md/fprintf.o ds3100.md/freopen.o ds3100.md/fscanf.o ds3100.md/getchar.o ds3100.md/printf.o ds3100.md/putchar.o ds3100.md/rewind.o ds3100.md/scanf.o ds3100.md/setbuf.o ds3100.md/setbuffer.o ds3100.md/setlinebuf.o ds3100.md/sprintf.o ds3100.md/vfscanf.o ds3100.md/vsprintf.o ds3100.md/vprintf.o ds3100.md/StdioFileOpenMode.o ds3100.md/vsnprintf.o ds3100.md/StdioFileCloseProc.o ds3100.md/StdioFileWriteProc.o
INSTFILES	= ds3100.md/md.mk ds3100.md/dependencies.mk Makefile tags TAGS
SACREDOBJS	= 
