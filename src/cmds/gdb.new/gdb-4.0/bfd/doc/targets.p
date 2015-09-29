/* bfd_target
@node bfd_target
@subsection bfd_target
This structure contains everything that BFD knows about a target.
It includes things like its byte order, name, what routines to call
to do various operations, etc.   

Every BFD points to a target structure with its "xvec" member. 

Shortcut for declaring fields which are prototyped function pointers,
while avoiding anguish on compilers that don't support protos.
*/

#define SDEF(ret, name, arglist) \
                PROTO(ret,(*name),arglist)
#define SDEF_FMT(ret, name, arglist) \
                PROTO(ret,(*name[bfd_type_end]),arglist)

/*
These macros are used to dispatch to functions through the bfd_target
vector. They are used in a number of macros further down in @file{bfd.h}, and
are also used when calling various routines by hand inside the BFD
implementation.  The "arglist" argument must be parenthesized; it
contains all the arguments to the called function.
*/

#define BFD_SEND(bfd, message, arglist) \
               ((*((bfd)->xvec->message)) arglist)

/*
For operations which index on the BFD format 
*/

#define BFD_SEND_FMT(bfd, message, arglist) \
            (((bfd)->xvec->message[(int)((bfd)->format)]) arglist)

/*
This is the struct which defines the type of BFD this is.  The
"xvec" member of the struct @code{bfd} itself points here.  Each module
that implements access to a different target under BFD, defines
one of these.

FIXME, these names should be rationalised with the names of the
entry points which call them. Too bad we can't have one macro to
define them both! 
*/

typedef struct bfd_target
{

/*
identifies the kind of target, eg SunOS4, Ultrix, etc 
*/

  char *name;

/*
The "flavour" of a back end is a general indication about the contents
of a file.
*/

  enum target_flavour_enum {
    bfd_target_aout_flavour_enum,
    bfd_target_coff_flavour_enum,
    bfd_target_ieee_flavour_enum,
    bfd_target_oasys_flavour_enum,
    bfd_target_srec_flavour_enum} flavour;

/*
The order of bytes within the data area of a file.
*/

  boolean byteorder_big_p;

/*
The order of bytes within the header parts of a file.
*/

  boolean header_byteorder_big_p;

/*
This is a mask of all the flags which an executable may have set -
from the set @code{NO_FLAGS}, @code{HAS_RELOC}, ...@code{D_PAGED}.
*/

  flagword object_flags;       

/*
This is a mask of all the flags which a section may have set - from
the set @code{SEC_NO_FLAGS}, @code{SEC_ALLOC}, ...@code{SET_NEVER_LOAD}.
*/

  flagword section_flags;

/*
The pad character for filenames within an archive header.
*/

  char ar_pad_char;            

/*
The maximum number of characters in an archive header.
*/

 unsigned short ar_max_namelen;

/*
The minimum alignment restriction for any section.
*/

  unsigned int align_power_min;

/*
Entries for byte swapping for data. These are different to the other
entry points, since they don't take BFD as first arg.  Certain other handlers
could do the same.
*/

  SDEF (bfd_vma,      bfd_getx64, (bfd_byte *));
  SDEF (void,         bfd_putx64, (bfd_vma, bfd_byte *));
  SDEF (bfd_vma, bfd_getx32, (bfd_byte *));
  SDEF (void,         bfd_putx32, (bfd_vma, bfd_byte *));
  SDEF (bfd_vma, bfd_getx16, (bfd_byte *));
  SDEF (void,         bfd_putx16, (bfd_vma, bfd_byte *));

/*
Byte swapping for the headers
*/

  SDEF (bfd_vma,   bfd_h_getx64, (bfd_byte *));
  SDEF (void,          bfd_h_putx64, (bfd_vma, bfd_byte *));
  SDEF (bfd_vma,  bfd_h_getx32, (bfd_byte *));
  SDEF (void,          bfd_h_putx32, (bfd_vma, bfd_byte *));
  SDEF (bfd_vma,  bfd_h_getx16, (bfd_byte *));
  SDEF (void,          bfd_h_putx16, (bfd_vma, bfd_byte *));

/*
Format dependent routines, these turn into vectors of entry points
within the target vector structure; one for each format to check.

Check the format of a file being read.  Return bfd_target * or zero. 
*/

  SDEF_FMT (struct bfd_target *, _bfd_check_format, (bfd *));

/*
Set the format of a file being written.  
*/

  SDEF_FMT (boolean,            _bfd_set_format, (bfd *));

/*
Write cached information into a file being written, at bfd_close. 
*/

  SDEF_FMT (boolean,            _bfd_write_contents, (bfd *));

/*
The following functions are defined in @code{JUMP_TABLE}. The idea is
that the back end writer of @code{foo} names all the routines
@code{foo_}@var{entry_point}, @code{JUMP_TABLE} will built the entries
in this structure in the right order.

Core file entry points
*/

  SDEF (char *, _core_file_failing_command, (bfd *));
  SDEF (int,    _core_file_failing_signal, (bfd *));
  SDEF (boolean, _core_file_matches_executable_p, (bfd *, bfd *));

/*
Archive entry points
*/

 SDEF (boolean, _bfd_slurp_armap, (bfd *));
 SDEF (boolean, _bfd_slurp_extended_name_table, (bfd *));
 SDEF (void,   _bfd_truncate_arname, (bfd *, CONST char *, char *));
 SDEF (boolean, write_armap, (bfd *arch, 
                              unsigned int elength,
                              struct orl *map,
                              int orl_count, 
                              int stridx));

/*
Standard stuff.
*/

  SDEF (boolean, _close_and_cleanup, (bfd *));
  SDEF (boolean, _bfd_set_section_contents, (bfd *, sec_ptr, PTR,
                                            file_ptr, bfd_size_type));
  SDEF (boolean, _bfd_get_section_contents, (bfd *, sec_ptr, PTR, 
                                            file_ptr, bfd_size_type));
  SDEF (boolean, _new_section_hook, (bfd *, sec_ptr));

/*
Symbols and reloctions
*/

 SDEF (unsigned int, _get_symtab_upper_bound, (bfd *));
  SDEF (unsigned int, _bfd_canonicalize_symtab,
           (bfd *, struct symbol_cache_entry **));
  SDEF (unsigned int, _get_reloc_upper_bound, (bfd *, sec_ptr));
  SDEF (unsigned int, _bfd_canonicalize_reloc, (bfd *, sec_ptr, arelent **,
                                               struct symbol_cache_entry**));
  SDEF (struct symbol_cache_entry  *, _bfd_make_empty_symbol, (bfd *));
  SDEF (void,     _bfd_print_symbol, (bfd *, PTR, struct symbol_cache_entry  *,
                                      bfd_print_symbol_enum_type));
#define bfd_print_symbol(b,p,s,e) BFD_SEND(b, _bfd_print_symbol, (b,p,s,e))
  SDEF (alent *,   _get_lineno, (bfd *, struct symbol_cache_entry  *));

  SDEF (boolean,   _bfd_set_arch_mach, (bfd *, enum bfd_architecture,
                                       unsigned long));

  SDEF (bfd *,  openr_next_archived_file, (bfd *arch, bfd *prev));
  SDEF (boolean, _bfd_find_nearest_line,
        (bfd *abfd, struct sec  *section,
         struct symbol_cache_entry  **symbols,bfd_vma offset,
        CONST char **file, CONST char **func, unsigned int *line));
  SDEF (int,    _bfd_stat_arch_elt, (bfd *, struct stat *));

  SDEF (int,    _bfd_sizeof_headers, (bfd *, boolean));

  SDEF (void, _bfd_debug_info_start, (bfd *));
  SDEF (void, _bfd_debug_info_end, (bfd *));
  SDEF (void, _bfd_debug_info_accumulate, (bfd *, struct sec  *));

/*
Special entry points for gdb to swap in coff symbol table parts
*/

  SDEF(void, _bfd_coff_swap_aux_in,(
       bfd            *abfd ,
       PTR             ext,
       int             type,
       int             class ,
       PTR             in));

  SDEF(void, _bfd_coff_swap_sym_in,(
       bfd            *abfd ,
       PTR             ext,
       PTR             in));

  SDEF(void, _bfd_coff_swap_lineno_in,  (
       bfd            *abfd,
       PTR            ext,
       PTR             in));

} bfd_target;

/*

*i bfd_find_target
Returns a pointer to the transfer vector for the object target
named target_name.  If target_name is NULL, chooses the one in the
environment variable GNUTARGET; if that is null or not defined then
the first entry in the target list is chosen.  Passing in the
string "default" or setting the environment variable to "default"
will cause the first entry in the target list to be returned,
and "target_defaulted" will be set in the BFD.  This causes
@code{bfd_check_format} to loop over all the targets to find the one
that matches the file being read.  
*/
 PROTO(bfd_target *, bfd_find_target,(CONST char *, bfd *));

/*

*i bfd_target_list
This function returns a freshly malloced NULL-terminated vector of the
names of all the valid BFD targets. Do not modify the names 
*/
 PROTO(CONST char **,bfd_target_list,());

/*
*/

