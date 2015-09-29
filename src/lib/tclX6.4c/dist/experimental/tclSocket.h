/*
 * tclSocket.h --
 *
 * This file documents the exported interface for the Socket
 * routines.
 *---------------------------------------------------------------------------
 * Copyright 1992 Lance Ellinghouse. lance@markv.com
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted, provided
 * that the above copyright notice appear in all copies.  Lance Ellinghouse
 * makes no representations about the suitability of this software for any 
 * purpose.  It is provided "as is" without express or implied warranty.
 */

/*
 * Version 1.2
 */

/*
 *
 * ChangeLog:
 *
 * Version 1.2 (02/27/92) (Sent to markd@sco.com on 03/17/92)
 *    Tcl_SocketCmd() readded due to demand for new naming.
 *    Tcl_SocketDelete() added.
 *
 * Version 1.1 (Posted to Net on 02/26/92)
 *    Tcl_SocketCmd() removed due to new layout of code
 *
 * Version 1.0
 *    Released to general use on UseNet
 *
 */

#include "tclExtdInt.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>


int  Tcl_SocketCmd _ANSI_ARGS_((ClientData clientData, Tcl_Interp *interp, int argc, char *argv[]));
void Tcl_SocketDelete _ANSI_ARGS_((ClientData clientData));

void Tcl_InitSocket _ANSI_ARGS_((Tcl_Interp *iPtr));

