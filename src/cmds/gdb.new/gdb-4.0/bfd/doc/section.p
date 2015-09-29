/* The shape of a section struct:
*/

typedef struct sec {

/*
The name of the section, the name isn't a copy, the pointer is
the same as that passed to bfd_make_section.
*/

    CONST char *name;

/*
The next section in the list belonging to the BFD, or NULL.
*/

    struct sec *next;

/*
The field flags contains attributes of the section. Some of these
flags are read in from the object file, and some are synthesized from
other information. 
*/

flagword flags;

/*
*/

#define SEC_NO_FLAGS   0x000

/*
Tells the OS to allocate space for this section when loaded.
This would clear for a section containing debug information only.
*/

#define SEC_ALLOC      0x001

/*
Tells the OS to load the section from the file when loading.
This would be clear for a .bss section 
*/

#define SEC_LOAD       0x002

/*
The section contains data still to be relocated, so there will be some
relocation information too.
*/

#define SEC_RELOC      0x004

/*
Obsolete ? 
*/

#define SEC_BALIGN     0x008

/*
A signal to the OS that the section contains read only data.
*/

#define SEC_READONLY   0x010

/*
The section contains code only.
*/

#define SEC_CODE       0x020

/*
The section contains data only.
*/

#define SEC_DATA        0x040

/*
The section will reside in ROM.
*/

#define SEC_ROM        0x080

/*
The section contains constructor information. This section type is
used by the linker to create lists of constructors and destructors
used by @code{g++}. When a back end sees a symbol which should be used
in a constructor list, it creates a new section for the type of name
(eg @code{__CTOR_LIST__}), attaches the symbol to it and builds a
relocation. To build the lists of constructors, all the linker has to
to is catenate all the sections called @code{__CTOR_LIST__} and
relocte the data contained within - exactly the operations it would
peform on standard data.
*/

#define SEC_CONSTRUCTOR 0x100

/*
The section is a constuctor, and should be placed at the end of the ..
*/

#define SEC_CONSTRUCTOR_TEXT 0x1100

/*
*/
#define SEC_CONSTRUCTOR_DATA 0x2100

/*
*/
#define SEC_CONSTRUCTOR_BSS  0x3100

/*

The section has contents - a bss section could be
@code{SEC_ALLOC} | @code{SEC_HAS_CONTENTS}, a debug section could be
@code{SEC_HAS_CONTENTS}
*/

#define SEC_HAS_CONTENTS 0x200

/*
An instruction to the linker not to output sections containing
this flag even if they have information which would normally be written.
*/

#define SEC_NEVER_LOAD 0x400

/*

The base address of the section in the address space of the target.
*/

   bfd_vma vma;

/*
The size of the section in bytes of the loaded section. This contains
a value even if the section has no contents (eg, the size of @code{.bss}).
*/

   bfd_size_type size;    

/*
If this section is going to be output, then this value is the
offset into the output section of the first byte in the input
section. Eg, if this was going to start at the 100th byte in the
output section, this value would be 100. 
*/

   bfd_vma output_offset;

/*
The output section through which to map on output.
*/

   struct sec *output_section;

/*
The alignment requirement of the section, as an exponent - eg 3
aligns to 2^3 (or 8) 
*/

   unsigned int alignment_power;

/*
If an input section, a pointer to a vector of relocation records for
the data in this section.
*/

   struct reloc_cache_entry *relocation;

/*
If an output section, a pointer to a vector of pointers to
relocation records for the data in this section.
*/

   struct reloc_cache_entry **orelocation;

/*
The number of relocation records in one of the above 
*/

   unsigned reloc_count;

/*
Which section is it 0..nth     
*/

   int index;                      

/*
Information below is back end specific - and not always used or
updated 

File position of section data   
*/

   file_ptr filepos;      
/* File position of relocation info        
*/

   file_ptr rel_filepos;

/*
File position of line data              
*/

   file_ptr line_filepos;

/*
Pointer to data for applications        
*/

   PTR userdata;

/*
*/
   struct lang_output_section *otheruserdata;

/*
Attached line number information        
*/

   alent *lineno;
/* Number of line number records   
*/

   unsigned int lineno_count;

/*
When a section is being output, this value changes as more
linenumbers are written out 
*/

   file_ptr moving_line_filepos;

/*
what the section number is in the target world 
*/

   unsigned int target_index;

/*
*/
   PTR used_by_bfd;

/*
If this is a constructor section then here is a list of the
relocations created to relocate items within it.
*/

   struct relent_chain *constructor_chain;

/*
The BFD which owns the section.
*/

   bfd *owner;

/*
*/
} asection ;

/*

 bfd_get_section_by_name
Runs through the provided @var{abfd} and returns the @code{asection}
who's name matches that provided, otherwise NULL. @xref{Sections}, for more information.
*/

 PROTO(asection *, bfd_get_section_by_name,
    (bfd *abfd, CONST char *name));

/*

 bfd_make_section
This function creates a new empty section called @var{name} and attaches it
to the end of the chain of sections for the BFD supplied. An attempt to
create a section with a name which is already in use, returns the old
section by that name instead.

Possible errors are:
@table @code
@item invalid_operation
If output has already started for this BFD.
@item no_memory
If obstack alloc fails.
@end table
*/

 PROTO(asection *, bfd_make_section, (bfd *, CONST char *name));

/*

 bfd_set_section_flags
Attempts to set the attributes of the section named in the BFD
supplied to the value. Returns true on success, false on error.
Possible error returns are:
@table @code
@item invalid operation
The section cannot have one or more of the attributes requested. For
example, a .bss section in @code{a.out} may not have the
@code{SEC_HAS_CONTENTS} field set.
@end table
*/

 PROTO(boolean, bfd_set_section_flags,
       (bfd *, asection *, flagword));

/*

 bfd_map_over_sections
Calls the provided function @var{func} for each section attached to
the BFD @var{abfd}, passing @var{obj} as an argument. The function
will be called as if by 

@example
  func(abfd, the_section, obj);
@end example
*/

 PROTO(void, bfd_map_over_sections,
            (bfd *abfd, void (*func)(), PTR obj));

/*

This is the prefered method for iterating over sections, an
alternative would be to use a loop:

@example
   section *p;
   for (p = abfd->sections; p != NULL; p = p->next)
      func(abfd, p, ...)
@end example

 bfd_set_section_size
Sets @var{section} to the size @var{val}. If the operation is ok, then
@code{true} is returned, else @code{false}. 

Possible error returns:
@table @code
@item invalid_operation
Writing has started to the BFD, so setting the size is invalid
@end table 
*/

 PROTO(boolean, bfd_set_section_size,
     (bfd *, asection *, bfd_size_type val));

/*

 bfd_set_section_contents
Sets the contents of the section @var{section} in BFD @var{abfd} to
the data starting in memory at @var{data}. The data is written to the
output section starting at offset @var{offset} for @var{count} bytes.

Normally @code{true} is returned, else @code{false}. Possible error
returns are:
@table @code
@item no_contents
The output section does not have the @code{SEC_HAS_CONTENTS}
attribute, so nothing can be written to it.
@item and some more too
@end table
This routine is front end to the back end function @code{_bfd_set_section_contents}.
*/

 PROTO(boolean, bfd_set_section_contents,
         (bfd *abfd,        
         asection *section,
         PTR data,
         file_ptr offset,
         bfd_size_type count));

/*

 bfd_get_section_contents
This function reads data from @var{section} in BFD @var{abfd} into
memory starting at @var{location}. The data is read at an offset of
@var{offset} from the start of the input section, and is read for
@var{count} bytes.

If the contents of a constuctor with the @code{SEC_CONSTUCTOR} flag
set are requested, then the @var{location} is filled with zeroes.

If no errors occur, @code{true} is returned, else @code{false}.
Possible errors are:

@table @code
@item unknown yet
@end table
*/

 PROTO(boolean, bfd_get_section_contents, 
        (bfd *abfd, asection *section, PTR location,
         file_ptr offset, bfd_size_type count));

/*
*/

