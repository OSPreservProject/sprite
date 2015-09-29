/* *i bfd_openr
Opens the file supplied (using @code{fopen}) with the target supplied, it
returns a pointer to the created BFD.

If NULL is returned then an error has occured.
Possible errors are no_memory, invalid_target or system_call error.
*/
 PROTO(bfd*, bfd_openr, (CONST char *filename,CONST char*target));

/*

*i bfd_fdopenr
bfd_fdopenr is to bfd_fopenr much like  fdopen is to fopen. It opens a BFD on
a file already described by the @var{fd} supplied. 

Possible errors are no_memory, invalid_target and system_call error.
*/
  PROTO(bfd *, bfd_fdopenr,
    (CONST char *filename, CONST char *target, int fd));

/*

 bfd_openw
Creates a BFD, associated with file @var{filename}, using the file
format @var{target}, and returns a pointer to it.

Possible errors are system_call_error, no_memory, invalid_target.
*/
 PROTO(bfd *, bfd_openw, (CONST char *filename, CONST char *target));

/*

 bfd_close
This function closes a BFD. If the BFD was open for writing, then
pending operations are completed and the file written out and closed.
If the created file is executable, then @code{chmod} is called to mark
it as such.

All memory attached to the BFD's obstacks is released. 

@code{true} is returned if all is ok, otherwise @code{false}.
*/
 PROTO(boolean, bfd_close,(bfd *));

/*

 bfd_create
This routine creates a new BFD in the manner of @code{bfd_openw}, but without
opening a file. The new BFD takes the target from the target used by
@var{template}. The format is always set to @code{bfd_object}.
*/

 PROTO(bfd *, bfd_create, (CONST char *filename, bfd *template));

/*

 bfd_alloc_size
Return the number of bytes in the obstacks connected to the supplied
BFD.
*/
 PROTO(bfd_size_type,bfd_alloc_size,(bfd *abfd));

/*
*/
