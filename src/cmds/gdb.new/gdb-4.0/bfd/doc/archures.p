/* bfd_architecture
This enum gives the object file's CPU
architecture, in a global sense.  E.g. what processor family does it
belong to?  There is another field, which indicates what processor
within the family is in use.  The machine gives a number which
distingushes different versions of the architecture, containing for
example 2 and 3 for Intel i960 KA and i960 KB, and 68020 and 68030 for
Motorola 68020 and 68030.
*/

enum bfd_architecture 
{
  bfd_arch_unknown,   /* File arch not known */
  bfd_arch_obscure,   /* Arch known, not one of these */
  bfd_arch_m68k,      /* Motorola 68xxx */
  bfd_arch_vax,       /* DEC Vax */   
  bfd_arch_i960,      /* Intel 960 */
    /* The order of the following is important.
       lower number indicates a machine type that 
       only accepts a subset of the instructions
       available to machines with higher numbers.
       The exception is the "ca", which is
       incompatible with all other machines except 
       "core". */

#define bfd_mach_i960_core      1
#define bfd_mach_i960_ka_sa     2
#define bfd_mach_i960_kb_sb     3
#define bfd_mach_i960_mc        4
#define bfd_mach_i960_xa        5
#define bfd_mach_i960_ca        6

  bfd_arch_a29k,      /* AMD 29000 */
  bfd_arch_sparc,     /* SPARC */
  bfd_arch_mips,      /* MIPS Rxxxx */
  bfd_arch_i386,      /* Intel 386 */
  bfd_arch_ns32k,     /* National Semiconductor 32xxx */
  bfd_arch_tahoe,     /* CCI/Harris Tahoe */
  bfd_arch_i860,      /* Intel 860 */
  bfd_arch_romp,      /* IBM ROMP RS/6000 */
  bfd_arch_alliant,   /* Alliant */
  bfd_arch_convex,    /* Convex */
  bfd_arch_m88k,      /* Motorola 88xxx */
  bfd_arch_pyramid,   /* Pyramid Technology */
  bfd_arch_h8_300,    /* Hitachi H8/300 */
  bfd_arch_last
  };

/*
stuff

 bfd_prinable_arch_mach
Return a printable string representing the architecture and machine
type. The result is only good until the next call to
@code{bfd_printable_arch_mach}.  
*/
 PROTO(CONST char *,bfd_printable_arch_mach,
    (enum bfd_architecture arch, unsigned long machine));

/*

*i bfd_scan_arch_mach
Scan a string and attempt to turn it into an archive and machine type combination.  
*/
 PROTO(boolean, bfd_scan_arch_mach,
    (CONST char *, enum bfd_architecture *, unsigned long *));

/*

*i bfd_arch_compatible
This routine is used to determine whether two BFDs' architectures and machine types are
compatible.  It calculates the lowest common denominator between the
two architectures and machine types implied by the BFDs and sets the
objects pointed at by @var{archp} and @var{machine} if non NULL. 

This routine returns @code{true} if the BFDs are of compatible type,
otherwise @code{false}.
*/
 PROTO(boolean, bfd_arch_compatible,
     (bfd *abfd,
     bfd *bbfd,
     enum bfd_architecture *archp,
     unsigned long *machinep));

/*

 bfd_set_arch_mach
Set atch mach
*/
#define bfd_set_arch_mach(abfd, arch, mach) \
     BFD_SEND (abfd, _bfd_set_arch_mach,\
                    (abfd, arch, mach))
