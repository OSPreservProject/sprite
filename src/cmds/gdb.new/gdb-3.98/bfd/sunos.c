/* BFD backend for SunOS binaries.
   Copyright (C) 1990-1991 Free Software Foundation, Inc.
   Written by Cygnus Support.

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

/* $Id: sunos.c,v 1.21 1991/07/31 16:58:01 gnu Exp $ */

#define ARCH 32
#define TARGETNAME "a.out-sunos-big"
#define VECNAME sunos_big_vec
#include "aoutf1.h"
