/* bfd_get_next_mapent
What this does
*/
 PROTO(symindex, bfd_get_next_mapent, (bfd *, symindex, carsym **));

/*

 bfd_set_archive_head

Used whilst processing archives. Sets the head of the chain of BFDs
contained in an archive to @var{new_head}. (see chapter on archives)
*/

 PROTO(boolean, bfd_set_archive_head, (bfd *output, bfd *new_head));

/*

 bfd_get_elt_at_index
Return the sub bfd contained within the archive at archive index n.
*/

 PROTO(bfd *, bfd_get_elt_at_index, (bfd *, int));

/*

 bfd_openr_next_archived_file
Initially provided a BFD containing an archive and NULL, opens a BFD
on the first contained element and returns that. Subsequent calls to
bfd_openr_next_archived_file should pass the archive and the previous
return value to return a created BFD to the next contained element.
NULL is returned when there are no more.
*/

 PROTO(bfd*, bfd_openr_next_archived_file,
               (bfd *archive, bfd *previous));

/*
*/

