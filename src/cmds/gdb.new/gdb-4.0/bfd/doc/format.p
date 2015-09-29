/* *i bfd_check_format
This routine is supplied a BFD and a format. It attempts to verify if
the file attached to the BFD is indeed compatible with the format
specified (ie, one of @code{bfd_object}, @code{bfd_archive} or
@code{bfd_core}).

If the BFD has been set to a specific @var{target} before the call,
only the named target and format combination will be checked. If the
target has not been set, or has been set to @code{default} then all
the known target backends will be interrogated to determine a match.

The function returns @code{true} on success, otherwise @code{false}
with one of the following error codes: 
@table @code
@item 
invalid_operation
if @code{format} is not one of @code{bfd_object}, @code{bfd_archive}
or @code{bfd_core}.
@item system_call_error
if an error occured during a read -  even some file mismatches can
cause system_call_errros
@item file_not_recognised
none of the backends recognised the file format
@item file_ambiguously_recognized
more than one backend recognised the file format.
@end table
*/
 PROTO(boolean, bfd_check_format, (bfd *abfd, bfd_format format));

/*

*i bfd_set_format
This function sets the file format of the supplied BFD to the format
requested. If the target set in the BFD does not support the format
requested, the format is illegal or the BFD is not open for writing
than an error occurs.
*/
 PROTO(boolean,bfd_set_format,(bfd *, bfd_format));

/*

*i bfd_format_string
This function takes one argument, and enumerated type (bfd_format) and
returns a pointer to a const string "invalid", "object", "archive",
"core" or "unknown" depending upon the value of the enumeration.
*/
 PROTO(CONST char *, bfd_format_string, (bfd_format));

/*
*/
