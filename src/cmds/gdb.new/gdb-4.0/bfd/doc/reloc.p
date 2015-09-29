/* bfd_perform_relocation
The relocation routine returns as a status an enumerated type:
*/

typedef enum bfd_reloc_status {
/* No errors detected
*/

  bfd_reloc_ok,

/*
The relocation was performed, but there was an overflow.
*/

  bfd_reloc_overflow,

/*
The address to relocate was not within the section supplied
*/

  bfd_reloc_outofrange,

/*
Used by special functions
*/

  bfd_reloc_continue,

/*
Unused 
*/

  bfd_reloc_notsupported,

/*
Unsupported relocation size requested. 
*/

  bfd_reloc_other,

/*
The symbol to relocate against was undefined.
*/

  bfd_reloc_undefined,

/*
The relocation was performed, but may not be ok - presently generated
only when linking i960 coff files with i960 b.out symbols.
*/

  bfd_reloc_dangerous
   }
 bfd_reloc_status_enum_type;

/*
*/

typedef struct reloc_cache_entry 
{

/*
A pointer into the canonical table of pointers 
*/

  struct symbol_cache_entry **sym_ptr_ptr;

/*
offset in section                 
*/

  rawdata_offset address;

/*
addend for relocation value        
*/

  bfd_vma addend;    

/*
if sym is null this is the section 
*/

  struct sec *section;

/*
Pointer to how to perform the required relocation
*/

  CONST struct reloc_howto_struct *howto;
} arelent;

/*

 reloc_howto_type
The @code{reloc_howto_type} is a structure which contains all the
information that BFD needs to know to tie up a back end's data.
*/

typedef CONST struct reloc_howto_struct 
{ 
/* The type field has mainly a documetary use - the back end can to what
it wants with it, though the normally the back end's external idea of
what a reloc number would be would be stored in this field. For
example, the a PC relative word relocation in a coff environment would
have the type 023 - because that's what the outside world calls a
R_PCRWORD reloc.
*/

  unsigned int type;

/*
The value the final relocation is shifted right by. This drops
unwanted data from the relocation. 
*/

  unsigned int rightshift;

/*
The size of the item to be relocated - 0, is one byte, 1 is 2 bytes, 3
is four bytes.
*/

  unsigned int size;

/*
Now obsolete
*/

  unsigned int bitsize;

/*
Notes that the relocation is relative to the location in the data
section of the addend. The relocation function will subtract from the
relocation value the address of the location being relocated.
*/

  boolean pc_relative;

/*
Now obsolete
*/

  unsigned int bitpos;

/*
Now obsolete
*/

  boolean absolute;

/*
Causes the relocation routine to return an error if overflow is
detected when relocating.
*/

  boolean complain_on_overflow;

/*
If this field is non null, then the supplied function is called rather
than the normal function. This allows really strange relocation
methods to be accomodated (eg, i960 callj instructions).
*/

  bfd_reloc_status_enum_type (*special_function)();

/*
The textual name of the relocation type.
*/

  char *name;

/*
When performing a partial link, some formats must modify the
relocations rather than the data - this flag signals this.
*/

  boolean partial_inplace;

/*
The src_mask is used to select what parts of the read in data are to
be used in the relocation sum. Eg, if this was an 8 bit bit of data
which we read and relocated, this would be 0x000000ff. When we have
relocs which have an addend, such as sun4 extended relocs, the value
in the offset part of a relocating field is garbage so we never use
it. In this case the mask would be 0x00000000.
*/

  bfd_word src_mask;
/* The dst_mask is what parts of the instruction are replaced into the
instruction. In most cases src_mask == dst_mask, except in the above
special case, where dst_mask would be 0x000000ff, and src_mask would
be 0x00000000.
*/

  bfd_word dst_mask;           

/*
When some formats create PC relative instructions, they leave the
value of the pc of the place being relocated in the offset slot of the
instruction, so that a PC relative relocation can be made just by
adding in an ordinary offset (eg sun3 a.out). Some formats leave the
displacement part of an instruction empty (eg m88k bcs), this flag
signals the fact.
*/

  boolean pcrel_offset;
} reloc_howto_type;

/*

 HOWTO
The HOWTO define is horrible and will go away.
*/
#define HOWTO(C, R,S,B, P, BI, ABS, O, SF, NAME, INPLACE, MASKSRC, MASKDST, PC) \
  {(unsigned)C,R,S,B, P, BI, ABS,O,SF,NAME,INPLACE,MASKSRC,MASKDST,PC}

/*

 reloc_chain
*/
typedef unsigned char bfd_byte;

typedef struct relent_chain {
  arelent relent;
  struct   relent_chain *next;
} arelent_chain;

/*

If an output_bfd is supplied to this function the generated image
will be relocatable, the relocations are copied to the output file
after they have been changed to reflect the new state of the world.
There are two ways of reflecting the results of partial linkage in an
output file; by modifying the output data in place, and by modifying
the relocation record. Some native formats (eg basic a.out and basic
coff) have no way of specifying an addend in the relocation type, so
the addend has to go in the output data.  This is no big deal since in
these formats the output data slot will always be big enough for the
addend. Complex reloc types with addends were invented to solve just
this problem.
*/
 PROTO(bfd_reloc_status_enum_type,
                bfd_perform_relocation,
                        (bfd * abfd,
                        arelent *reloc_entry,
                        PTR data,
                        asection *input_section,
                        bfd *output_bfd));

/*
*/
