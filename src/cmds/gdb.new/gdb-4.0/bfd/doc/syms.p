/* @subsection typedef asymbol
An @code{asymbol} has the form:
*/

typedef struct symbol_cache_entry 
{
/* A pointer to the BFD which owns the symbol. This information is
necessary so that a back end can work out what additional (invisible to
the application writer) information is carried with the symbol. 
*/

  struct _bfd *the_bfd;

/*
The text of the symbol. The name is left alone, and not copied - the
application may not alter it. 
*/

   CONST char *name;

/*
The value of the symbol.
*/

   symvalue value;

/*
Attributes of a symbol:
*/

#define BSF_NO_FLAGS    0x00

/*
The symbol has local scope; @code{static} in @code{C}. The value is
the offset into the section of the data.
*/

#define BSF_LOCAL	0x01

/*
The symbol has global scope; initialized data in @code{C}. The value
is the offset into the section of the data.
*/

#define BSF_GLOBAL	0x02

/*
Obsolete
*/

#define BSF_IMPORT	0x04

/*
The symbol has global scope, and is exported. The value is the offset
into the section of the data.
*/

#define BSF_EXPORT	0x08

/*
The symbol is undefined. @code{extern} in @code{C}. The value has no meaning.
*/

#define BSF_UNDEFINED	0x10	

/*
The symbol is common, initialized to zero; default in @code{C}. The
value is the size of the object in bytes.
*/

#define BSF_FORT_COMM	0x20	

/*
A normal @code{C} symbol would be one of:
@code{BSF_LOCAL}, @code{BSF_FORT_COMM},  @code{BSF_UNDEFINED} or @code{BSF_EXPORT|BSD_GLOBAL}

The symbol is a debugging record. The value has an arbitary meaning.
*/

#define BSF_DEBUGGING	0x40

/*
The symbol has no section attached, any value is the actual value and
is not a relative offset to a section.
*/

#define BSF_ABSOLUTE	0x80

/*
Used by the linker
*/

#define BSF_KEEP        0x10000
#define BSF_KEEP_G      0x80000

/*
Unused
*/

#define BSF_WEAK        0x100000
#define BSF_CTOR        0x200000 
#define BSF_FAKE        0x400000 

/*
The symbol used to be a common symbol, but now it is allocated.
*/

#define BSF_OLD_COMMON  0x800000  

/*
The default value for common data.
*/

#define BFD_FORT_COMM_DEFAULT_VALUE 0

/*
In some files the type of a symbol sometimes alters its location
in an output file - ie in coff a @code{ISFCN} symbol which is also @code{C_EXT}
symbol appears where it was declared and not at the end of a section. 
This bit is set by the target BFD part to convey this information. 
*/

#define BSF_NOT_AT_END    0x40000

/*
Signal that the symbol is the label of constructor section.
*/

#define BSF_CONSTRUCTOR   0x1000000

/*
Signal that the symbol is a warning symbol. If the symbol is a warning
symbol, then the value field (I know this is tacky) will point to the
asymbol which when referenced will cause the warning.
*/

#define BSF_WARNING       0x2000000

/*
Signal that the symbol is indirect. The value of the symbol is a
pointer to an undefined asymbol which contains the name to use
instead.
*/

#define BSF_INDIRECT     0x4000000

/*
*/
  flagword flags;

/*
A pointer to the section to which this symbol is relative, or 0 if the
symbol is absolute or undefined. Note that it is not sufficient to set
this location to 0 to mark a symbol as absolute - the flag
@code{BSF_ABSOLUTE} must be set also.
*/

  struct sec *section;

/*
Back end special data. This is being phased out in favour of making
this a union.
*/

  PTR udata;	
} asymbol;

/*

 get_symtab_upper_bound
Returns the number of bytes required in a vector of pointers to
@code{asymbols} for all the symbols in the supplied BFD, including a
terminal NULL pointer. If there are no symbols in the BFD, then 0 is
returned.
*/
#define get_symtab_upper_bound(abfd) \
     BFD_SEND (abfd, _get_symtab_upper_bound, (abfd))

/*

 bfd_canonicalize_symtab
Supplied a BFD and a pointer to an uninitialized vector of pointers.
This reads in the symbols from the BFD, and fills in the table with
pointers to the symbols, and a trailing NULL. The routine returns the
actual number of symbol pointers not including the NULL.
*/

#define bfd_canonicalize_symtab(abfd, location) \
     BFD_SEND (abfd, _bfd_canonicalize_symtab,\
                  (abfd, location))

/*
 bfd_set_symtab
Provided a table of pointers to to symbols and a count, writes to the
output BFD the symbols when closed.
*/

 PROTO(boolean, bfd_set_symtab, (bfd *, asymbol **, unsigned int ));

/*

 bfd_print_symbol_vandf
Prints the value and flags of the symbol supplied to the stream file.
*/

 PROTO(void, bfd_print_symbol_vandf, (PTR file, asymbol *symbol));

/*

  bfd_make_empty_symbol
This function creates a new @code{asymbol} structure for the BFD, and
returns a pointer to it.

This routine is necessary, since each back end has private information
surrounding the @code{asymbol}. Building your own @code{asymbol} and
pointing to it will not create the private information, and will cause
problems later on.
*/
#define bfd_make_empty_symbol(abfd) \
     BFD_SEND (abfd, _bfd_make_empty_symbol, (abfd))
