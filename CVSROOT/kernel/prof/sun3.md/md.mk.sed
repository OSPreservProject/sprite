#
# Mkmf uses this file in conjunction with sed to modify md.mk
#

$a\
\
#\
# Special massaging of _mcount.c needed to create non-profiled .po file\
# and .o file with _mcount renamed to the mcount everyone expects\
#\
$(TM).md/_mcount.po	: $(TM).md/_mcount.o .NOTMAIN\
\	$(CP) $(TM).md/_mcount.o $(TM).md/_mcount.po\
$(TM).md/_mcount.o	: .NOTMAIN\
\	$(CC) $(CFLAGS) -S $(.IMPSRC)\
\	$(SED) -e s/_mcount/mcount/g < _mcount.s > $(TM).md/_mcount.s\
\	$(AS) $(AFLAGS) -o $(.TARGET) $(TM).md/_mcount.s\
\	$(RM) -f _mcount.s $(TM).md/_mcount.s
