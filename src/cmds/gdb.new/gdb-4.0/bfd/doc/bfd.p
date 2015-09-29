/* @section @code{typedef bfd}

A BFD is has type @code{bfd}; objects of this type are the cornerstone
of any application using @code{libbfd}. References though the BFD and
to data in the BFD give the entire BFD functionality.

Here is the struct used to define the type @code{bfd}.  This contains
the major data about the file, and contains pointers to the rest of
the data.
*/

struct _bfd 
{
/*   The filename the application opened the BFD with.
*/

  CONST char *filename;                

/*
A pointer to the target jump table.
*/

  struct bfd_target *xvec;

/*

To avoid dragging too many header files into every file that
includes @file{bfd.h}, IOSTREAM has been declared as a "char *", and MTIME
as a "long".  Their correct types, to which they are cast when used,
are "FILE *" and "time_t".  

The iostream is the result of an fopen on the filename.
*/

  char *iostream;

/*
Is the file being cached @xref{File Caching}.
*/

  boolean cacheable;

/*
Marks whether there was a default target specified when the BFD was
opened. This is used to select what matching algorithm to use to chose
the back end.
*/

  boolean target_defaulted;

/*
The caching routines use these to maintain a least-recently-used list of
BFDs (@pxref{File Caching}).
*/

  struct _bfd *lru_prev, *lru_next;

/*
When a file is closed by the caching routines, BFD retains state
information on the file here:
*/

  file_ptr where;              

/*
and here:
*/

  boolean opened_once;

/*
*/
  boolean mtime_set;
/* File modified time 
*/

  long mtime;          

/*
Reserved for an unimplemented file locking extension.
*/

int ifd;

/*
The format which belongs to the BFD.
*/

  bfd_format format;

/*
The direction the BFD was opened with
*/

  enum bfd_direction {no_direction = 0,
                       read_direction = 1,
                       write_direction = 2,
                       both_direction = 3} direction;

/*
Format_specific flags
*/

  flagword flags;              

/*
Currently my_archive is tested before adding origin to anything. I
believe that this can become always an add of origin, with origin set
to 0 for non archive files.  
*/

  file_ptr origin;             

/*
Remember when output has begun, to stop strange things happening.
*/

  boolean output_has_begun;

/*
Pointer to linked list of sections
*/

  struct sec  *sections;

/*
The number of sections 
*/

  unsigned int section_count;

/*
Stuff only useful for object files:
The start address.
*/

  bfd_vma start_address;
/* Used for input and output
*/

  unsigned int symcount;
/* Symbol table for output BFD
*/

  struct symbol_cache_entry  **outsymbols;             

/*
Architecture of object machine, eg m68k 
*/

  enum bfd_architecture obj_arch;

/*
Particular machine within arch, e.g. 68010
*/

  unsigned long obj_machine;

/*
Stuff only useful for archives:
*/

  PTR arelt_data;              
  struct _bfd *my_archive;     
  struct _bfd *next;           
  struct _bfd *archive_head;   
  boolean has_armap;           

/*
Used by the back end to hold private data.
*/

  PTR tdata;

/*
Used by the application to hold private data
*/

  PTR usrdata;

/*
Where all the allocated stuff under this BFD goes (@pxref{Memory Usage}).
*/

  struct obstack memory;
};

/*

 bfd_set_start_address

Marks the entry point of an output BFD. Returns @code{true} on
success, @code{false} otherwise.
*/

 PROTO(boolean, bfd_set_start_address,(bfd *, bfd_vma));

/*

  bfd_get_mtime

Return cached file modification time (e.g. as read from archive header
for archive members, or from file system if we have been called
before); else determine modify time, cache it, and return it.  
*/

 PROTO(long, bfd_get_mtime, (bfd *));

/*

 stuff
*/


#define bfd_sizeof_headers(abfd, reloc) \
     BFD_SEND (abfd, _bfd_sizeof_headers, (abfd, reloc))

#define bfd_find_nearest_line(abfd, section, symbols, offset, filename_ptr, func, line_ptr) \
     BFD_SEND (abfd, _bfd_find_nearest_line,  (abfd, section, symbols, offset, filename_ptr, func, line_ptr))

#define bfd_debug_info_start(abfd) \
        BFD_SEND (abfd, _bfd_debug_info_start, (abfd))

#define bfd_debug_info_end(abfd) \
        BFD_SEND (abfd, _bfd_debug_info_end, (abfd))

#define bfd_debug_info_accumulate(abfd, section) \
        BFD_SEND (abfd, _bfd_debug_info_accumulate, (abfd, section))

#define bfd_stat_arch_elt(abfd, stat) \
        BFD_SEND (abfd, _bfd_stat_arch_elt,(abfd, stat))

#define bfd_coff_swap_aux_in(a,e,t,c,i) \
        BFD_SEND (a, _bfd_coff_swap_aux_in, (a,e,t,c,i))

#define bfd_coff_swap_sym_in(a,e,i) \
        BFD_SEND (a, _bfd_coff_swap_sym_in, (a,e,i))

#define bfd_coff_swap_lineno_in(a,e,i) \
        BFD_SEND ( a, _bfd_coff_swap_lineno_in, (a,e,i))

/*
*/
