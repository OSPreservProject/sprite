/*
 * specs.h --
 *
 *	This file is included by gcc.c, and defines the compilation
 *	and linking specs for each of the target machines for which
 *	gcc can currently generate code.
 *
 * Copyright 1988 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *
 * $Header: specs.h,v 1.11 88/10/02 16:15:04 hilfingr Exp $ SPRITE (Berkeley)
 */

#ifndef _SPECS
#define _SPECS

/* This structure says how to run one compiler, and when to do so.  */

struct compiler
{
  char *suffix;			/* Use this compiler for input files
				   whose names end in this suffix.  */
  char *spec;			/* To use this compiler, pass this spec
				   to do_spec.  */
};

/*
 * One of the following structures exists for each machine for
 * which gcc can generate code.  It gives all the spec-related
 * information for compiling for tha target.
 */

typedef struct
{
    char *name;				/* Official name of this target
					 * machine (many -m switches may
					 * map to the same entry). */
    struct compiler *base_specs;	/* List of specs to use when compiling
					 * for this target machine.  Last
					 * compiler must be all zeros to
					 * indicate end of list. */
    char *link_base_spec;		/* Basic spec to use to link for this
					 * target. */
    char *cpp_predefines;		/* Spec to substitute for %p. */
    char *compile_spec;			/* Spec to substitute for %C. */
    char *asm_spec;			/* Spec to substitute for %a. */
    char *link_spec;			/* Spec to substitute for %l. */
    char *lib_spec;			/* Spec to substitute for %L. */
    char *startfile_spec;		/* Spec to substitute for %S. */
} target_specs;

/*
 * Spec information for individual machines:
 */

struct compiler default_compilers[] =
{
    {".c",
     "cpp %{nostdinc} %{C} %{v} %{D*} %{U*} %{I*} \
        -I/sprite/lib/include.new/%m.md %{M*} %{T} \
        -undef -D__GNUC__ %{ansi:-T -$ -D__STRICT_ANSI__}\
	%{!ansi:%p -Dunix -Dsprite}\
        %{O:-D__OPTIMIZE__} %{traditional} %{pedantic}\
	%{Wcomment} %{Wtrigraphs} %{Wall}\
        %i %{!M*:%{!E:%g.cpp}}%{E:%{o*}}%{M*:%{o*}}\n\
     %{!M*:%{!E:%C %g.cpp %{!Q:-quiet} -dumpbase %i %{Y*} %{d*} %{m*} %{f*}\
	%{g} %{O} %{W*} %{w} %{pedantic} %{ansi} %{traditional}\
	%{v:-version} %{gg:-symout %g.sym} %{pg:-p} %{p}\
        %{S:%{o*}%{!o*:-o %b.s}}%{!S:-o %g.s}\n\
     %{!S:%a %{R} %{j} %{J} %{h} %{d2} %{gg:-G %g.sym}\
	%g.s %{c:%{o*}%{!o*:-o %w%b.o}}%{!c:-o %d%w%b.o}\n }}}"},
    {".s",
     "%{!S:%a %{R} %{j} %{J} %{h} %{d2}\
	%i %{c:%{o*}%{!o*:-o %w%b.o}}%{!c:-o %d%w%b.o}\n }"},
    /* Mark end of table. */
    {0, 0}
};

char default_link_spec[] =
    "%{!c:%{!M*:%{!E:%{!S:%l %{o*}\
    %{A} %{d} %{e*} %{N} %{n} %{r} %{s} %{S} %{T*} %{t} %{u*} %{X} %{x} %{z}\
    %{y*} %{!nostdlib:%S} %{L*} -L/sprite/lib/%m.md \
    %o %{!nostdlib:%L}\n }}}}";


target_specs m68000_target =
{
    "sun2",					/* name */
    default_compilers,				/* base_specs */
    default_link_spec,				/* link_base_spec */
    "-Dmc68000 -Dsun2 %{funsigned-char: -D__CHAR_UNSIGNED__}",
						/* cpp_predefines */
    "cc1.68k -msoft-float -m68000",		/* compile_spec */
    "as -m68010",				/* asm_spec */
    "ld.new -X %{!e:-e start}",
						/* link_spec */
    "%{!p:%{!pg:-lc}}%{p:-lc_p}%{pg:-lc_p}",	/* lib_spec */
    ""						/* start_file_spec */
};

target_specs m68020_target =
{
    "sun3",					/* name */
    default_compilers,				/* base_specs */
    default_link_spec,				/* link_base_spec */
    "-Dmc68000 -Dsun3 %{funsigned-char: -D__CHAR_UNSIGNED__}",
						/* cpp_predefines */
    "cc1.68k -msoft-float -m68020",		/* compile_spec */
    "as -m68020",				/* asm_spec */
    "ld.new -X %{!e:-e start}",
						/* link_spec */
    "%{!p:%{!pg:-lc}}%{p:-lc_p}%{pg:-lc_p}",	/* lib_spec */
    ""						/* start_file_spec */
};

target_specs spur_target =
{
    "spur",					/* name */
    default_compilers,				/* base_specs */
    default_link_spec,				/* link_base_spec */
    "-Dspur %{funsigned-char: -D__CHAR_UNSIGNED__}",
						/* cpp_predefines */
    "cc1.spur -msoft-float",			/* compile_spec */
    "sas",					/* asm_spec */
    "sld -X -p %{!e:-e start}",			/* link_spec */
    "%{!p:%{!pg:-lc}}%{p:-lc_p}%{pg:-lc_p}",	/* lib_spec */
    ""						/* start_file_spec */
};

/*
 * Top-level table used to look up information for a particular target
 * machine.  The first entry will be used as the default target if
 * a target isn't specified in a command-line switch or environment
 * variable.
 */

typedef struct
{
    char *name;			/* Name of target machine (as it
				 * appears in "-m" switch. */
    target_specs *info;		/* Information about target machine. */
} target_machine;

target_machine target_machines[] = {
    {"sun3",	&m68020_target},	/* This is the default target. */
    {"68000",	&m68000_target},
    {"68010",	&m68000_target},
    {"sun2",	&m68000_target},
    {"68020",	&m68020_target},
    {"spur",	&spur_target},
    {0, 0}				/* Zeroes mark end of list. */
};

char *target_name;			/* Name of selected target. */
target_specs *target;			/* Info for selected target. */

#endif _SPECS
