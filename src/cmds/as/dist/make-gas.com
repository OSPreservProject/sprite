$!
$!	Command file to build a GNU assembler on VMS
$!
$ if "''p1'" .eqs. "LINK" then goto Link
$ gcc/debug/define="VMS" as.c
$ gcc/debug/define=("VMS", "error=as_fatal") xrealloc.c
$ gcc/debug/define=("VMS", "error=as_fatal") xmalloc.c
$ gcc/debug/define=("VMS", "error=as_fatal") hash.c
$ gcc/debug/define="VMS" hex-value.c
$ gcc/debug/define="VMS" atof-generic.c
$ gcc/debug/define="VMS" append.c
$ gcc/debug/define="VMS" messages.c
$ gcc/debug/define="VMS" expr.c
$ gcc/debug/define="VMS" app.c
$ gcc/debug/define="VMS" frags.c
$ gcc/debug/define="VMS" input-file.c
$ gcc/debug/define="VMS" input-scrub.c
$ gcc/debug/define="VMS" output-file.c
$ gcc/debug/define="VMS" read.c
$ gcc/debug/define="VMS" subsegs.c
$ gcc/debug/define="VMS" symbols.c
$ gcc/debug/define="VMS" write.c
$ gcc/debug/define="VMS" version.c
$ gcc/debug/define="VMS" flonum-const.c
$ gcc/debug/define="VMS" flonum-copy.c
$ gcc/debug/define="VMS" flonum-mult.c
$ gcc/debug/define="VMS" strstr.c
$ gcc/debug/define="VMS" bignum-copy.c
$ gcc/debug/define="VMS" gdb.c
$ gcc/debug/define="VMS" gdb-file.c
$ gcc/debug/define="VMS" gdb-symbols.c
$ gcc/debug/define="VMS" gdb-blocks.c
$ gcc/debug/define="VMS" obstack.c
$ gcc/debug/define="VMS" gdb-lines.c
$ gcc/debug/define="VMS" vax.c
$ gcc/debug/define="VMS" atof-vax.c
$ gcc/debug/define="VMS" vms.c
$ Link:
$ link/exec=gcc-as sys$input:/opt
!
!	Linker options file for GNU assembler
!
as,xrealloc,xmalloc,hash,hex-value,atof-generic,append,messages,expr,app,-
frags,input-file,input-scrub,output-file,read,subsegs,symbols,write,-
version,flonum-const,flonum-copy,flonum-mult,strstr,bignum-copy,gdb,-
gdb-file,gdb-symbols,gdb-blocks,obstack,gdb-lines,vax,atof-vax,vms,-
gnu_cc:[000000]gcclib/lib,sys$share:vaxcrtl/lib
