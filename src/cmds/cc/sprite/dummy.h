/*
 * dummy.h --
 *
 * This is an empty include file.  It is the target of a symbolic link
 * in ../../cpp/config.h.  It turns out that some of the files in cpp
 * include config.h, even though they don't need to.  The "real" config.h
 * files are target-machine specific, so ../../cpp/config.h was made
 * to point here to keep cpp target-independent.
 */

#define BITS_PER_UNIT   8
#define TARGET_BELL     '\007'
#define TARGET_BS       '\010'
#define TARGET_FF       '\014'
#define TARGET_NEWLINE  '\012'
#define TARGET_CR       '\015'
#define TARGET_TAB      '\011'
#define TARGET_VT       '\013'

