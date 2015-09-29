/* 
 * tcldbm.c --
 *
 *       dbm interface Tcl command.
 *
 *---------------------------------------------------------------------------
 * Copyright 1992 Karl Lehenbauer & Mark Diekhans.
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted, provided
 * that the above copyright notice appear in all copies.  Karl Lehenbauer
 * and Mark Diekhans make no representations about the
 * suitability of this software for any purpose.  It is provided "as is"
 * without express or implied warranty.
 */

#include "tclExtdInt.h"
#include "tcl.h"

#include <dbm.h>

#define DBM_NOT_SEARCHING 0
#define DBM_START_SEARCH 1
#define DBM_SEARCHING 2
#define DBM_SEARCH_COMPLETE 3


/*
 *----------------------------------------------------------------------
 *
 * Tcl_DbmCmd --
 *
 * Results:
 *  Standard TCL results, may return the UNIX system error message.
 *
 *----------------------------------------------------------------------
 */
static int
Tcl_DbmCmd (clientData, interp, argc, argv)
    char        *clientData;
    Tcl_Interp  *interp;
    int          argc;
    char       **argv;
{
    static int dbmSearchState = DBM_NOT_SEARCHING;
    static datum key, value;


    if ((argv[1][0] == 'i') && (strcmp(argv[1], "init") == 0))
    {
        int dbmInitVal;

        if (argc != 3) {
            Tcl_AppendResult (interp, "bad # arg: ", argv[0],
                " init filename", (char *)NULL);
            return TCL_ERROR;
        }
        dbmInitVal = dbminit (argv[2]);
        dbmSearchState = DBM_START_SEARCH;
        sprintf (interp->result, "%d", dbmInitVal);
        return TCL_OK;
    }
    
    if ((argv[1][0] == 'f') && (strcmp(argv[1], "fetch") == 0))
    {
        if ((argc < 3) || (argc > 4)) {
            Tcl_AppendResult (interp, "bad # arg: ", argv[0],
                " fetch key [variable]", (char *)NULL);
            return TCL_ERROR;
        }
        dbmSearchState = DBM_SEARCHING;
        key.dptr = argv[2];
        key.dsize = strlen(argv[2]);
        value = fetch(key);

        if (value.dptr == NULL) {
            if (argc == 3) {
                Tcl_AppendResult (interp, "dbm variable '", argv[2],
                                  "doesn't exist.");
                return TCL_ERROR;
            }
            if (Tcl_SetVar (interp, argv[3], "", TCL_LEAVE_ERR_MSG) == NULL)
                return TCL_ERROR;
            strcpy (interp->result, "0");
            return TCL_OK;
        }
        value.dptr[value.dsize] = '\0';
        if (argc == 3) {
            Tcl_Return(interp, value.dptr, TCL_VOLATILE);
            return TCL_OK;
        }
        if (Tcl_SetVar (interp, argv[3], value.dptr, TCL_LEAVE_ERR_MSG)
               == NULL) return TCL_ERROR;
        strcpy (interp->result, "1");
        return TCL_OK;
    }

    if ((argv[1][0] == 's') && (strcmp (argv[1], "store") == 0))
    {
        datum key, value;
        int result;

        if (argc != 4) {
            Tcl_AppendResult (interp, "bad # arg: ", argv[0],
               " store key value", (char *)NULL);
            return TCL_ERROR;
        }
        dbmSearchState = DBM_NOT_SEARCHING;
        key.dptr = argv[2];
        key.dsize = strlen(argv[2]);
        value.dptr = argv[3];
        value.dsize = strlen(argv[3]);
        result = store(key, value);
        sprintf(interp->result, "%d", result);
        return TCL_OK;
    }
    
    if ((argv[1][0] == 'd') && (strcmp(argv[1], "delete") == 0))
    {
        datum key;
        int result;

        if (argc != 3) {
            sprintf(interp->result, "bad # arg: dbm delete key");
            return TCL_ERROR;
        }
        key.dptr = argv[2];
        key.dsize = strlen(argv[2]);
        result = delete(key);
        sprintf(interp->result, "%d", result);
        return TCL_OK;
    }
#define DBM_NOT_SEARCHING 0
#define DBM_START_SEARCH 1
#define DBM_SEARCHING 2
#define DBM_SEARCH_COMPLETE 3

    if ((argv[1][0] == 's') && (strcmp(argv[1], "startsearch") == 0))
    {
        datum key;

        if (argc != 2) {
            sprintf(interp->result, "bad # arg: dbm startsearch");
            return TCL_ERROR;
        }
        dbmSearchState = DBM_START_SEARCH;
        return TCL_OK;
    }

    if ((argv[1][0] == 'n') && (strcmp(argv[1], "nextelement") == 0))
    {
	static datum searchkey;

        if (argc != 3) {
            sprintf(interp->result, "bad # arg: dbm nextelement varname");
            return TCL_ERROR;
        }

        if (dbmSearchState == DBM_NOT_SEARCHING) {
            Tcl_AppendResult("you must call 'dbm startsearch' before ",
              "'dbm nextelement' with no intervening dbm operations");
            return TCL_ERROR;
        }

        if (dbmSearchState == DBM_START_SEARCH) 
        {
            dbmSearchState = DBM_SEARCHING;
            searchkey = firstkey();
        } else {
            searchkey = nextkey(searchkey);
        }

        if (searchkey.dptr == NULL) {
            dbmSearchState = DBM_SEARCH_COMPLETE;
            strcpy(interp->result, "0");
            return TCL_OK;
        }
        searchkey.dptr[searchkey.dsize] = '\0';
        strcpy(interp->result, "1");
        Tcl_SetVar(interp, argv[2], searchkey.dptr, 0);
        return TCL_OK;
    }
    
    sprintf(interp->result,
            "bad arg: dbm option must be \"init\", \"fetch\", \"store\", \"delete\", \"startsearch\" or \"nextelement\"");
    return TCL_ERROR;
}


/*
 *----------------------------------------------------------------------
 *
 * Tcl_InitDbmCmd --
 *      Initializes the TCL dbm command.
 *
 *----------------------------------------------------------------------
 */
void
Tcl_InitDbmCmd (interp)
    Tcl_Interp *interp;
{
    Tcl_CreateCommand (interp, "dbm", Tcl_DbmCmd, (ClientData)NULL,
                      (void (*)())NULL);
}


