/***********************************************************************
 *
 * PROJECT:	  PMake
 * MODULE:	  Prefix -- Exportation
 * FILE:	  export.c
 *
 * AUTHOR:  	  Adam de Boor: Jul  5, 1989
 *
 * ROUTINES:
 *	Name	  	    Description
 *	----	  	    -----------
 *	Export_Prefix	    Export a directory as a prefix
 *	Export_Send 	    Send list of exported prefixes to the local
 *	    	    	    daemon.
 *	Export_Init 	    Initialize the module.
 *	Export_Dump 	    Dump list of exported prefixes into a buffer.
 *
 * REVISION HISTORY:
 *	Date	  Name	    Description
 *	----	  ----	    -----------
 *	7/ 5/89	  ardeb	    Initial version
 *
 * DESCRIPTION:
 *	Functions to handle exporting of various prefixes. Doesn't take
 *	the place of the standard mount daemon. Just responds to queries
 *	about where a prefix is defined.
 *
 * 	Copyright (c) Berkeley Softworks 1989
 * 	Copyright (c) Adam de Boor 1989
 *
 * 	Permission to use, copy, modify, and distribute this
 * 	software and its documentation for any non-commercial purpose
 *	and without fee is hereby granted, provided that the above copyright
 * 	notice appears in all copies.  Neither Berkeley Softworks nor
 * 	Adam de Boor makes any representations about the suitability of this
 * 	software for any purpose.  It is provided "as is" without
 * 	express or implied warranty.
 *
 ***********************************************************************/
#ifndef lint
static char *rcsid =
"$Id: export.c,v 1.9 89/11/14 17:16:05 adam Exp $";
#endif lint

#include    "prefix.h"
#include    "rpc.h"

#include    <ctype.h>
#include    <netdb.h>
#include    <arpa/inet.h>

typedef struct {
    char    	*fsname;    	/* Name of file system being exported */
    char    	*prefix;    	/* Prefix under which it's being exported */
    int	    	active;	    	/* Actively exported. An inactively
				 * exported prefix is one that the user
				 * wants to delete, but can't because
				 * some client is using the FS. Setting this
				 * false keeps ExportLocate from responding */
} Export;

static Lst  exports;


/***********************************************************************
 *				ExportLocate
 ***********************************************************************
 * SYNOPSIS:	    Handle a PREFIX_LOCATE broadcast call
 * CALLED BY:	    Rpc module
 * RETURN:	    Local directory, if we're exporting the prefix
 * SIDE EFFECTS:    None
 *
 * STRATEGY:
 *
 * REVISION HISTORY:
 *	Name	Date		Description
 *	----	----		-----------
 *	ardeb	7/ 6/89		Initial Revision
 *
 ***********************************************************************/
static void
ExportLocate(from, msg, dataLen, data, serverData)
    struct sockaddr_in	    *from;  	/* Source of call */
    Rpc_Opaque	    	    msg;    	/* Message token for reply */
    int	    	    	    dataLen;	/* Length of passed data */
    Rpc_Opaque	    	    data;   	/* Name of prefix being sought */
    Rpc_Opaque	    	    serverData;	/* JUNK */
{
    dprintf("LOCATE from %s for %s\n", InetNtoA(from->sin_addr),
	    data);
	    
    if (exports) {
	LstNode 	    ln;
	Export  	    *ep;
	
	for (ln = Lst_First(exports); ln != NILLNODE; ln = Lst_Succ(ln)) {
	    ep = (Export *)Lst_Datum(ln);

	    if ((strcmp(ep->prefix, (char *)data) == 0) && ep->active) {
		/*
		 * We've got it -- answer the call (follow the fold
		 * and stray no more! Stray no more! ...)
		 */
		Rpc_Return(msg, strlen(ep->fsname)+1, ep->fsname);
		break;
	    }
	}
    }
}


/***********************************************************************
 *				Export_Prefix
 ***********************************************************************
 * SYNOPSIS:	    Arrange to export a directory under a prefix
 * CALLED BY:	    main
 * RETURN:	    Nothing
 * SIDE EFFECTS:    An Export structure is added to the exports list
 *
 * STRATEGY:
 *
 * REVISION HISTORY:
 *	Name	Date		Description
 *	----	----		-----------
 *	ardeb	7/ 6/89		Initial Revision
 *
 ***********************************************************************/
void
Export_Prefix(fsname, prefix)
    char    	*fsname;    	/* Filesystem being exported */
    char    	*prefix;    	/* Name under which it's being exported */
{
    Export  	*ep;
    FILE    	*ef;
    LstNode 	ln;
    
    if (exports == NULL) {
	exports = Lst_Init(FALSE);
    }

    dprintf("export %s as %s\n", fsname, prefix);

    /*
     * See if we're replacing an existing prefix...
     */
    for (ln = Lst_First(exports); ln != NILLNODE; ln = Lst_Succ(ln)) {
	ep = (Export *)Lst_Datum(ln);

	if (strcmp(prefix, ep->prefix) == 0) {
	    break;
	}
    }
    if (ln == NILLNODE) {
	/*
	 * Nope -- allocate a new Export record and stick it on the end
	 * of the list.
	 */
	ep = (Export *)malloc(sizeof(Export));
	(void)Lst_AtEnd(exports, (ClientData)ep);
    }

    /*
     * Save the parameters in the record
     */
    ep->fsname = fsname;
    ep->prefix = prefix;
    ep->active = TRUE;

    /*
     * Make sure the filesystem is exported, warning if it isn't.
     */
    ef = fopen(EXPORTS, "r");
    if (ef == NULL) {
	Message("/etc/exports doesn't exist -- %s won't be allowed out",
		fsname);
    } else {
	char   	line[512];
	int 	fslen = strlen(fsname);

	while (fgets(line, sizeof(line), ef) != NULL) {
	    if ((strncmp(line, fsname, fslen) == 0) &&
		isspace(line[fslen]))
	    {
		fclose(ef);
		return;
	    }
	}
	fclose(ef);
	Message("%s not in /etc/exports -- won't be allowed out", fsname);
    }
}
    

/***********************************************************************
 *				ExportExport
 ***********************************************************************
 * SYNOPSIS:	    Export another directory as a prefix
 * CALLED BY:	    PREFIX_EXPORT
 * RETURN:	    Nothing
 * SIDE EFFECTS:    An Export record is added to the exports list
 *
 * STRATEGY:
 *
 * REVISION HISTORY:
 *	Name	Date		Description
 *	----	----		-----------
 *	ardeb	7/ 7/89		Initial Revision
 *
 ***********************************************************************/
static void
ExportExport(from, msg, len, data, serverData)
    struct sockaddr_int	*from;
    Rpc_Message	    	msg;
    int	    	    	len;
    Rpc_Opaque	    	data;	/* Buffer holding both dir (first string)
				 * and prefix, separated by a null character */
    Rpc_Opaque	    	serverData;	/* Data we gave (UNUSED) */
{
    char    	*cp;
    char	*args = (char *)data;
    char	*fsname;
    char	*prefix;

    cp = args + strlen(args) + 1;

    fsname = (char *)malloc(strlen(args) + 1);
    strcpy(fsname, args);

    prefix = (char *)malloc(strlen(cp) + 1);
    strcpy(prefix, cp);

    Export_Prefix(fsname, prefix);

    Rpc_Return(msg, 0, NULL);
}


/***********************************************************************
 *				Export_Send
 ***********************************************************************
 * SYNOPSIS:	    Send all exported prefixes to the local daemon
 * CALLED BY:	    main
 * RETURN:	    Nothing
 * SIDE EFFECTS:    None
 *
 * STRATEGY:
 *
 * REVISION HISTORY:
 *	Name	Date		Description
 *	----	----		-----------
 *	ardeb	7/ 7/89		Initial Revision
 *
 ***********************************************************************/
void
Export_Send(server)
    struct sockaddr_in	    *server;
{
    LstNode	    ln;	    	/* Node in exports list */
    Export  	    *ep;    	/* Current exported prefix */
    char    	    buf[512];	/* Transmission buffer */
    int	    	    len;    	/* Length of parameters */
    struct timeval  retry;  	/* Retransmission interval */
    Rpc_Stat	    status; 	/* Status of call */

    retry.tv_sec = 1;
    retry.tv_usec = 0;
    
    if (exports) {
	for (ln = Lst_First(exports); ln != NILLNODE; ln = Lst_Succ(ln)) {
	    ep = (Export *)Lst_Datum(ln);

	    len = strlen(ep->fsname);
	    strcpy(buf, ep->fsname);
	    
	    strcpy(buf+len+1,ep->prefix);

	    len += strlen(ep->prefix) + 2;

	    status = Rpc_Call(prefixSock, server, PREFIX_EXPORT,
			     len, buf,
			     0, NULL,
			     2, &retry);
	    if (status != RPC_SUCCESS) {
		fprintf(stderr, "Couldn't export %s as %s (%s)\n", ep->fsname,
			ep->prefix, Rpc_ErrorMessage(status));
	    }
	}
    }
}


/***********************************************************************
 *				Export_Dump
 ***********************************************************************
 * SYNOPSIS:	    Dump the list of exported prefixes into a buffer.
 * CALLED BY:	    DumpPrefix
 * RETURN:	    *leftPtr adjusted to reflect space remaining.
 * SIDE EFFECTS:    None
 *
 * STRATEGY:
 *	Each exported prefix is placed in the buffer as:
 *	    x<fsname>:<prefix>\n
 *
 * REVISION HISTORY:
 *	Name	Date		Description
 *	----	----		-----------
 *	ardeb	7/ 8/89		Initial Revision
 *
 ***********************************************************************/
void
Export_Dump(buf, leftPtr)
    char    	*buf;	    /* Place to start storing */
    int	    	*leftPtr;   /* IN/OUT: remaining room in buf */
{
    LstNode 	ln;
    Export  	*ep;

    if (exports) {
	for (ln = Lst_First(exports); ln != NILLNODE; ln = Lst_Succ(ln)) {
	    int	    entlen;
	    
	    ep = (Export *)Lst_Datum(ln);
	    entlen = 1+strlen(ep->fsname)+1+strlen(ep->prefix)+1+2;

	    if (*leftPtr >= entlen) {
		sprintf(buf, "x%s:%s:%c\n", ep->fsname, ep->prefix,
			ep->active ? '1' : '0');
		buf += entlen;
		*leftPtr -= entlen;
	    }
	}
    }
}


/***********************************************************************
 *				ExportUnmountResponse
 ***********************************************************************
 * SYNOPSIS:	    Handle a response to our PREFIX_UNMOUNT call
 * CALLED BY:	    Rpc_Broadcast
 * RETURN:	    FALSE (keep broadcasting)
 * SIDE EFFECTS:    *msgPtr advanced.
 *
 * STRATEGY:
 *
 * REVISION HISTORY:
 *	Name	Date		Description
 *	----	----		-----------
 *	ardeb	7/11/89		Initial Revision
 *
 ***********************************************************************/
static Boolean
ExportUnmountResponse(from, len, data, msgPtr, serverData)
    struct sockaddr_in	*from;	    /* Source of reply */
    int	    	    	len;	    /* Length of returned data */
    Rpc_Opaque	    	data;	    /* Returned data (s/b none) */
    char    	    	**msgPtr;   /* Place to store hostname (message
				     * being formed for return to caller) */
    Rpc_Opaque	    	serverData; /* Data we gave (UNUSED) */
{
    struct hostent  	*he;

    he = gethostbyaddr(&from->sin_addr, sizeof(from->sin_addr), AF_INET);

    sprintf(*msgPtr, "%s, ", he ? he->h_name : InetNtoA(from->sin_addr));
    *msgPtr += strlen(*msgPtr);

    return(FALSE);
}


/***********************************************************************
 *				ExportDelete
 ***********************************************************************
 * SYNOPSIS:	    Delete an exported prefix
 * CALLED BY:	    PREFIX_NOEXPORT
 * RETURN:	    Nothing
 * SIDE EFFECTS:    If the prefix exists in the export record, it is
 *	    	    deleted.
 *
 * STRATEGY:
 *
 * REVISION HISTORY:
 *	Name	Date		Description
 *	----	----		-----------
 *	ardeb	7/ 8/89		Initial Revision
 *
 ***********************************************************************/
static void
ExportDelete(from, msg, len, data, serverData)
    struct sockaddr_in	*from;	    /* Source of data */
    Rpc_Message		msg;	    /* Message for reply/return */
    int			len;	    /* Length of prefix (including null) */
    Rpc_Opaque		data;	    /* Null-terminated prefix */
    Rpc_Opaque 	    	serverData; /* Data we gave (UNUSED) */
{
    char	    	ansbuf[1024];	/* Buffer for response (1K
					 * enough?) */
    char    	    	*answer = "Ok";	/* Assume ok */

    if (exports) {
	LstNode	    	ln; 	/* Node for current exported prefix */
	Export	    	*ep;	/* Exported prefix being checked */

	for (ln = Lst_First(exports); ln != NILLNODE; ln = Lst_Succ(ln)) {
	    ep = (Export *)Lst_Datum(ln);

	    if (strcmp(ep->prefix, (char *)data) == 0) {
		struct timeval	retry;	    	/* Retrans interval for
						 * broadcast */
		char	    	*cp;	    	/* Pointer to pass to
						 * Rpc_Broadcast */
		struct sockaddr_in server;  	/* Nothingness to tell
						 * Rpc_Broadcast to go
						 * everywhere */
		Rpc_Stat    	status;	    	/* Status of broadcast */
		
		/*
		 * Tell the world the prefix is going away, after marking the
		 * prefix as inactive, preventing us from exporting it
		 * during the broadcast.
		 */
		ep->active = FALSE;

		server.sin_family = AF_INET;
		server.sin_port = htons(PREFIX_PORT);
		server.sin_addr.s_addr = INADDR_ANY;

		/*
		 * 1-second intervals, so we don't keep the user waiting
		 * too long, there being no shortcut out of the broadcast.
		 */
		retry.tv_sec = 1;
		retry.tv_usec = 0;
		
		cp = ansbuf;
		
		status = Rpc_Broadcast(prefixSock, &server, PREFIX_UNMOUNT,
				       strlen(ep->prefix)+1,
				       (Rpc_Opaque)ep->prefix,
				       0, (Rpc_Opaque)NULL,
				       2, &retry,
				       ExportUnmountResponse,
				       (Rpc_Opaque)&cp);

		if (status != RPC_SUCCESS && status != RPC_TIMEDOUT) {
		    answer = Rpc_ErrorMessage(status);
		    dprintf("Rpc_Broadcast: %s\n", answer);
		} else if (cp != ansbuf) {
		    /*
		     * Objection! The name(s) of the objector(s) is in
		     * answer, separated by commas. Make a sentence out of
		     * the response and ship it back, leaving the prefix
		     * marked inactive.
		     */
		    cp[-2] = ' '; /* Blow away final comma */
		    /*
		     * Tack on end o' message, nuking extraneous space
		     */
		    strcpy(&cp[-1], "still using it");
		    answer = ansbuf;
		} else {
		    /*
		     * Remove the prefix from the export list
		     */
		    (void)Lst_Remove(exports, ln);
		    
		    /*
		     * Free all associated memory
		     */
		    free(ep->prefix);
		    free(ep->fsname);
		    free((char *)ep);
		}
	    }
	}
    } else {
	answer = "no prefixes exported";
    }
    Rpc_Return(msg, strlen(answer)+1, answer);
}

/***********************************************************************
 *				Export_Init
 ***********************************************************************
 * SYNOPSIS:	    Initialize this module
 * CALLED BY:	    main
 * RETURN:	    Nothing
 * SIDE EFFECTS:    Our RPC servers are registered
 *
 * STRATEGY:
 *
 * REVISION HISTORY:
 *	Name	Date		Description
 *	----	----		-----------
 *	ardeb	7/ 6/89		Initial Revision
 *
 ***********************************************************************/
void
Export_Init()
{
    Rpc_ServerCreate(prefixSock, PREFIX_LOCATE, ExportLocate, NULL, NULL,
		     NULL);
    Rpc_ServerCreate(prefixSock, PREFIX_EXPORT, ExportExport, NULL, NULL,
		     NULL);
    Rpc_ServerCreate(prefixSock, PREFIX_NOEXPORT, ExportDelete, NULL, NULL,
		     NULL);
    
}

