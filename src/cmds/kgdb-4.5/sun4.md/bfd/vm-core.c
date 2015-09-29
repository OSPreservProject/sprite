/* BFD back end for kernel core files on sprite
   Copyright (C) 1988, 1989, 1991 Free Software Foundation, Inc.
   Written by John Gilmore of Cygnus Support.

This file is part of BFD, the Binary File Descriptor library.

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

/* To use this file on a particular host, configure the host with these
   parameters in the config/h-HOST file:

	HDEFINES=-DVM_CORE
	HDEPFILES=vm-core.o

 */

#include "bfd.h"
#include "sysdep.h"
#include "libbfd.h"
#include "libaout.h"           /* BFD a.out internal data structures */

#include <stdio.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/dir.h>
#include <signal.h>
#include <machine/reg.h>
#include <kernel/dbg.h>

#include <sys/user.h>		/* After a.out.h  */
#include <sys/file.h>

#include <errno.h>
#define NOGAP

typedef struct {
  StopInfo stopInfo;
  Dbg_DumpBounds bounds;
} CoreState;

struct vm_core_struct 
{
  asection *code_section;
  asection *data_section;
  asection *stack_section;
  asection *cache_section;

  CoreState coreState;
} *rawptr;


#define core_state(bfd) (&((bfd)->tdata.vm_core_data->coreState))
#define core_codesec(bfd) ((bfd)->tdata.vm_core_data->code_section)
#define core_datasec(bfd) ((bfd)->tdata.vm_core_data->data_section)
#define core_stacksec(bfd) ((bfd)->tdata.vm_core_data->stack_section)
#define core_cachesec(bfd) ((bfd)->tdata.vm_core_data->cache_section)

StopInfo *
bfd_get_vm_core_stopinfo(abfd)
     bfd *abfd;
{
  return (&(abfd)->tdata.vm_core_data->coreState.stopInfo);
}

Dbg_DumpBounds *
bfd_get_vm_core_bounds(abfd)
     bfd *abfd;
{
  return (&(abfd)->tdata.vm_core_data->coreState.bounds);
}

Mach_RegState *
bfd_get_vm_core_regs(abfd)
     bfd *abfd;
{
  return (&(abfd)->tdata.vm_core_data->coreState.stopInfo.regs);
}

Mach_RegState *
bfd_get_vm_core_fpregs(abfd)
     bfd *abfd;
{
  return (&(abfd)->tdata.vm_core_data->coreState.stopInfo.regs);
}

/* Handle sprite kernel kgcore dump file.  */

/* ARGSUSED */
bfd_target *
vm_core_file_p (abfd)
     bfd *abfd;

{
  int val;
  CoreState coreState;
  /* This struct is just for allocating two things with one zalloc, so
     they will be freed together, without violating alignment constraints. */


  val = bfd_read ((void *)&coreState, 1, sizeof coreState, abfd);
  if (val != sizeof coreState)
    return 0;			/* Too small to be a core file */

#if 0
  /* Sanity check perhaps??? */
  if (u.u_dsize > 0x1000000)	/* Remember, it's in pages... */
    return 0;
  if (u.u_ssize > 0x1000000)
    return 0;
  /* Check that the size claimed is no greater than the file size. FIXME. */
#endif

  /* OK, we believe you.  You're a core file (sure, sure).  */

  /* Allocate both the upage and the struct core_data at once, so
     a single free() will free them both.  */
  rawptr = (struct vm_core_struct *)bfd_zalloc (abfd, sizeof (struct vm_core_struct));
  if (rawptr == NULL) {
    bfd_error = no_memory;
    return 0;
  }
  
  abfd->tdata.vm_core_data = rawptr;

  rawptr->coreState = coreState; /*Copy the uarea into the tdata part of the bfd */

  /* Create the sections.  This is raunchy, but bfd_close wants to free
     them separately.  */

  core_stacksec(abfd) = (asection *) zalloc (sizeof (asection));
  if (core_stacksec (abfd) == NULL) {
  loser:
    bfd_error = no_memory;
    free ((void *)rawptr);
    return 0;
  }
  core_datasec (abfd) = (asection *) zalloc (sizeof (asection));
  if (core_datasec (abfd) == NULL) {
  loser1:
    free ((void *)core_stacksec (abfd));
    goto loser;
  }
  core_cachesec (abfd) = (asection *) zalloc (sizeof (asection));
  if (core_cachesec (abfd) == NULL) {
  loser2:
    free ((void *)core_datasec (abfd));
    goto loser1;
  }
  core_codesec(abfd) = (asection *) zalloc (sizeof (asection));
  if (core_stacksec (abfd) == NULL) {
    free ((void *)rawptr);
    goto loser2;
  }

  core_stacksec (abfd)->name = ".stack";
  core_datasec (abfd)->name = ".data";
  core_cachesec (abfd)->name = ".cache";
  core_codesec (abfd)->name = ".code";

  core_stacksec (abfd)->flags = SEC_ALLOC + SEC_LOAD + SEC_HAS_CONTENTS;
  core_datasec (abfd)->flags = SEC_ALLOC + SEC_LOAD + SEC_HAS_CONTENTS;
  core_cachesec (abfd)->flags = SEC_ALLOC + SEC_HAS_CONTENTS;
  core_codesec (abfd)->flags = SEC_ALLOC + SEC_LOAD + SEC_CODE + SEC_HAS_CONTENTS;

  core_datasec (abfd)->_raw_size = coreState.bounds.kernelDataSize;
  core_stacksec (abfd)->_raw_size = coreState.bounds.kernelStacksSize;
  core_cachesec (abfd)->_raw_size = coreState.bounds.fileCacheSize;
  core_codesec (abfd)->_raw_size = coreState.bounds.kernelCodeSize;

  /* What a hack... we'd like to steal it from the exec file,
     since the upage does not seem to provide it.  FIXME.  */
  core_datasec (abfd)->vma = coreState.bounds.kernelDataStart;
  core_stacksec (abfd)->vma = coreState.bounds.kernelStacksStart;
  core_cachesec (abfd)->vma = coreState.bounds.fileCacheStart;
  core_codesec (abfd)->vma = coreState.bounds.kernelCodeStart;

  core_codesec (abfd)->filepos = sizeof coreState;
#ifndef NOGAP
  core_datasec (abfd)->filepos = coreState.bounds.kernelDataStart - coreState.bounds.kernelCodeStart + (sizeof coreState);
  core_stacksec (abfd)->filepos = coreState.bounds.kernelStacksStart - coreState.bounds.kernelCodeStart + (sizeof coreState);
  core_cachesec (abfd)->filepos = coreState.bounds.fileCacheStart - coreState.bounds.kernelCodeStart + (sizeof coreState);
#else
  core_datasec (abfd)->filepos = coreState.bounds.kernelCodeSize + (sizeof coreState);
  core_stacksec (abfd)->filepos = coreState.bounds.kernelDataSize + coreState.bounds.kernelCodeSize + (sizeof coreState);
  core_cachesec (abfd)->filepos = coreState.bounds.kernelStacksSize + coreState.bounds.kernelDataSize + coreState.bounds.kernelCodeSize + (sizeof coreState);
#endif

  /* Align to word at least */
  core_stacksec (abfd)->alignment_power = 2;
  core_datasec (abfd)->alignment_power = 2;
  core_cachesec (abfd)->alignment_power = 2;
  core_codesec (abfd)->alignment_power = 2;

  abfd->sections = core_stacksec (abfd);
  core_stacksec (abfd)->next = core_datasec (abfd);
  core_datasec (abfd)->next = core_codesec (abfd);
  core_codesec (abfd)->next = core_cachesec (abfd);
  abfd->section_count = 4;

  return abfd->xvec;
}

char *
vm_core_file_failing_command (abfd)
     bfd *abfd;
{
  return 0;
}

/* ARGSUSED */
int
vm_core_file_failing_signal (abfd)
     bfd *abfd;
{
  return abfd->tdata.vm_core_data->coreState.stopInfo.trapType;
}

/* ARGSUSED */
boolean
vm_core_file_matches_executable_p  (core_bfd, exec_bfd)
     bfd *core_bfd, *exec_bfd;
{
  return true;		/* FIXME, We have no way of telling at this point */
}

/* No archive file support via this BFD */
#define	vm_openr_next_archived_file	bfd_generic_openr_next_archived_file
#define	vm_generic_stat_arch_elt		bfd_generic_stat_arch_elt
#define	vm_slurp_armap			bfd_false
#define	vm_slurp_extended_name_table	bfd_true
#define	vm_write_armap			(PROTO (boolean, (*),	\
     (bfd *arch, unsigned int elength, struct orl *map, \
      unsigned int orl_count, int stridx))) bfd_false
#define	vm_truncate_arname		bfd_dont_truncate_arname
#define	aout_32_openr_next_archived_file	bfd_generic_openr_next_archived_file

#define	vm_close_and_cleanup		bfd_generic_close_and_cleanup
#define	vm_set_section_contents		(PROTO(boolean, (*),	\
         (bfd *abfd, asection *section, PTR data, file_ptr offset,	\
         bfd_size_type count))) bfd_false
#define	vm_get_section_contents		bfd_generic_get_section_contents
#define	vm_new_section_hook		(PROTO (boolean, (*),	\
	(bfd *, sec_ptr))) bfd_true
#define	vm_get_symtab_upper_bound	bfd_0u
#define	vm_get_symtab			(PROTO (unsigned int, (*), \
        (bfd *, struct symbol_cache_entry **))) bfd_0u
#define	vm_get_reloc_upper_bound		(PROTO (unsigned int, (*), \
	(bfd *, sec_ptr))) bfd_0u
#define	vm_canonicalize_reloc		(PROTO (unsigned int, (*), \
	(bfd *, sec_ptr, arelent **, struct symbol_cache_entry**))) bfd_0u
#define	vm_make_empty_symbol		(PROTO (		\
	struct symbol_cache_entry *, (*), (bfd *))) bfd_false
#define	vm_print_symbol			(PROTO (void, (*),	\
	(bfd *, PTR, struct symbol_cache_entry  *,			\
	 bfd_print_symbol_type))) bfd_false
#define	vm_get_lineno			(PROTO (alent *, (*),	\
	(bfd *, struct symbol_cache_entry *))) bfd_nullvoidptr
#define	vm_set_arch_mach			(PROTO (boolean, (*),	\
	(bfd *, enum bfd_architecture, unsigned long))) bfd_false
#define	vm_find_nearest_line		(PROTO (boolean, (*),	\
        (bfd *abfd, struct sec  *section,				\
         struct symbol_cache_entry  **symbols,bfd_vma offset,		\
         CONST char **file, CONST char **func, unsigned int *line))) bfd_false
#define	vm_sizeof_headers		(PROTO (int, (*),	\
	(bfd *, boolean))) bfd_0

#define vm_bfd_debug_info_start		bfd_void
#define vm_bfd_debug_info_end		bfd_void
#define vm_bfd_debug_info_accumulate	(PROTO (void, (*),	\
	(bfd *, struct sec *))) bfd_void
#define vm_bfd_get_relocated_section_contents bfd_generic_get_relocated_section_contents
#define vm_bfd_relax_section bfd_generic_relax_section
/* If somebody calls any byte-swapping routines, shoot them.  */
void
swap_abort()
{
  abort(); /* This way doesn't require any declaration for ANSI to fuck up */
}
#define	NO_GET	((PROTO(bfd_vma, (*), (         bfd_byte *))) swap_abort )
#define	NO_PUT	((PROTO(void,    (*), (bfd_vma, bfd_byte *))) swap_abort )

bfd_target vm_core_vec =
  {
    "vm-core",
    bfd_target_unknown_flavour,
    true,			/* target byte order */
    true,			/* target headers byte order */
    (HAS_RELOC | EXEC_P |	/* object flags */
     HAS_LINENO | HAS_DEBUG |
     HAS_SYMS | HAS_LOCALS | DYNAMIC | WP_TEXT | D_PAGED),
    (SEC_HAS_CONTENTS | SEC_ALLOC | SEC_LOAD | SEC_RELOC), /* section flags */
    ' ',						   /* ar_pad_char */
    16,							   /* ar_max_namelen */
    3,							   /* minimum alignment power */
    NO_GET, NO_PUT, NO_GET, NO_PUT, NO_GET, NO_PUT, /* data */
    NO_GET, NO_PUT, NO_GET, NO_PUT, NO_GET, NO_PUT, /* hdrs */

    {_bfd_dummy_target, _bfd_dummy_target,
     _bfd_dummy_target, vm_core_file_p},
    {bfd_false, bfd_false,	/* bfd_create_object */
     bfd_false, bfd_false},
    {bfd_false, bfd_false,	/* bfd_write_contents */
     bfd_false, bfd_false},
    
    JUMP_TABLE(vm)
};
