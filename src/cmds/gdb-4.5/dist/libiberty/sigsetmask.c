/* Version of sigsetmask.c
   Copyright (C) 1991 Free Software Foundation, Inc.

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

/* steve chamberlain	
   sac@cygnus.com

*/


/* Set the current signal mask to the set provided, and return the 
   previous value */

#include <ansidecl.h>
#include <signal.h>

sigset_t
DEFUN(sigsetmask,(set),
      sigset_t *set)
{
    sigset_t old;
    
    (void) sigprocmask(SIG_SETMASK, set, &old);
    return old;
}
