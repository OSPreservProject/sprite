#
# Mkmf uses this file in conjunction with sed to modify md.mk
#

$a\
\
#\
# Special massaging of _mcount.c needed to create non-profiled .po file\
# and .o file with _mcount renamed to the mcount everyone expects\
#\
_mcount.po	: _mcount.o .NOTMAIN\
\	$(CP) _mcount.o _mcount.po\
_mcount.o	: .NOTMAIN\
\	$(CC) $(CFLAGS) -S $(.IMPSRC)\
\	$(MV) $(.PREFIX).s mcount.s\
\	$(SED) -e s/_mcount/mcount/g < mcount.s > $(.PREFIX).s\
\	$(AS) $(AFLAGS) -o $(.TARGET) $(.PREFIX).s\
\	$(RM) -f mcount.s
