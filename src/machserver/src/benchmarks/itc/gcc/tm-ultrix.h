#include "tm-vax.h"

/* Note: I thought I found that STRUCTURE_SIZE_BOUNDARY needs to be 32
   by default, and that is why I made this file.
   Then I tested pcc on Ultrix once more and found the same results as on BSD.
   So this file now does not change anything.
   The only reason it still exists is to contain this note.

   Can anyone tell me fully what is going on?  */

/* 
#undef STRUCTURE_SIZE_BOUNDARY
#define STRUCTURE_SIZE_BOUNDARY (TARGET_VAXC_ALIGNMENT ? 8 : 32)
*/
