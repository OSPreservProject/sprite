#
# System Makefile for:
#	Sun3 running Sprite and a Bourne Shell that does error-checking
# switching.
#
# These are the variables used to specify the nature of the system on which
# pmake is running. These names may later be used in #if expressions for
# conditional reading of the enclosed portion of the Makefile
#
sun=	 Machine is a sun
mc68000= so it has a Motorola 68000-family chip.
sprite=	 It runs Sprite.
mc68020= It's a Sun3 so it has a 68020 microprocessor.

.SUFFIXES : .out .a .ln .o .c .cc .F .f .e .r .y .l .s .cl .p .h \
	    .c,v .cc,v .y,v .l,v .s,v .h,v
.INCLUDES : .h
.LIBS	: .a

#
# Don't assume any dependencies for files without suffixes.
#
.NULL	: .h

YACC	= yacc
YFLAGS	=
LEX	= lex
LFLAGS	=
CC	= cc
RM	= rm
MV	= mv
#if defined(vax) || defined(sun)
AS	= as
#else
AS	= as -
#endif
PC	= pc
PFLAGS	=
CFLAGS	=
AFLAGS	=
RC	= f77
RFLAGS	=
FC	= f77
EFLAGS	=
FFLAGS	=
LOADLIBES=
CO	= co
COFLAGS	=
CI	= ci
CIFLAGS	=
AR	= ar
ARFLAGS	= r
LD	= ld

.c,v.c .y,v.y .l,v.l .s,v.s .h,v.h :
	$(CO) $(COFLAGS) $(.IMPSRC) $(.TARGET)

.c.o :
	$(CC) $(CFLAGS) -c $(.IMPSRC)

.p.o :
	$(PC) $(PFLAGS) -c $(.IMPSRC)

.cl.o :
	class -c $(.IMPSRC)

.e.o .r.o .F.o .f.o :
	$(FC) $(RFLAGS) $(EFLAGS) $(FFLAGS) -c $(.IMPSRC)

.s.o :
	$(AS) $(AFLAGS) -o $(.TARGET) $(.IMPSRC)

.y.o :
	$(YACC) $(YFLAGS) $(.IMPSRC)
	$(CC) $(CFLAGS) -c y.tab.c
	$(RM) y.tab.c
	$(MV) y.tab.o $(.TARGET)

.l.o :
	$(LEX) $(LFLAGS) $(.IMPSRC)
	$(CC) $(CFLAGS) -c lex.yy.c
	$(RM) lex.yy.c
	$(MV) lex.yy.o $(.TARGET)

.y.c :
	$(YACC) $(YFLAGS) $(.IMPSRC)
	$(MV) y.tab.c $(.TARGET)

.l.c :
	$(LEX) $(LFLAGS) $(.IMPSRC)
	$(MV) lex.yy.c $(.TARGET)

.s.out .c.out .o.out :
	$(CC) $(CFLAGS) $(.IMPSRC) $(LOADLIBES) -o $(.TARGET)

.f.out .F.out .r.out .e.out :
	$(FC) $(EFLAGS) $(RFLAGS) $(FFLAGS) $(.IMPSRC) \
		$(LOADLIBES) -o $(.TARGET)
	$(RM) -f $(.PREFIX).o

.y.out :
	$(YACC) $(YFLAGS) $(.IMPSRC)
	$(CC) $(CFLAGS) y.tab.c $(LOADLIBES) -ly -o $(.TARGET)
	$(RM) y.tab.c

.l.out :
	$(LEX) $(LFLAGS) $(.IMPSRC)
	$(CC) $(CFLAGS) lex.yy.c $(LOADLIBES) -ll -o $(.TARGET)
	$(RM) lex.yy.c

#
# System search-path specifications.
#
.PATH.h: /sprite/lib/include /sprite/lib/include/$(MACHINE).md
.PATH.a: /sprite/lib/$(MACHINE).md
