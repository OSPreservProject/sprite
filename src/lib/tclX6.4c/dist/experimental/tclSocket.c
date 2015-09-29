/*
 * tclSocket.c --
 *
 * This file defines all the Socket system calls to be usable with Tcl
 *---------------------------------------------------------------------------
 * Copyright 1992 Lance Ellinghouse. lance@markv.com
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted, provided
 * that the above copyright notice appear in all copies. Lance Ellinghouse
 * makes no representations about the suitability of this software for any 
 * purpose.  It is provided "as is" without express or implied warranty.
 */

/*
 * Version 1.3: 
 */

/*
 * ChangeLog:
 *
 * Version 1.3: 
 *     03/17/92: (Sent to markd@sco.com on 03/17/92)
 *     Cleaned up the error return messages
 *     Fixed a few minor bugs
 *     02/27/92:
 *     Added Tcl_SocketCmd() back in and redid the calling structure
 *         to be "socket cmd" instead of just "cmd". This makes it more
 *         Tcl like.
 *     Added Tcl_SocketDelete()
 *     Modified Tcl_InitSocket() to use Tcl_SocketCmd() and Tcl_SocketDelete()
 *     address(sin_addr.s_addr) changed to address(sin_addr) to make things
 *         easier and more in line with Tcl.
 *
 * Version 1.2: 02/16/91 (Posted to Net on 02/26/92)
 *     Changed layout of code to make it more modular and flexible.
 *     Added "rcmd" for super-user access to run remote commands
 *           (Not Fully Tested)
 *     Added "rexec" for access to run remote commands
 *           (Not Fully Tested)
 *     Added "gethostbyaddr"
 *           (Not Fully Tested)
 *     Added "getpeername"
 *     Added "getsockname"
 *     Added "shutdown"
 *           (Not Fully Tested)
 *     Fixed a number of bugs
 *
 * Version 1.1: 02/14/91
 *     Changed all Inet address handling to use and return octet format
 *     where ever possible. Also changed all returned port numbers to
 *     be in HOST format instead of NETWORK format. These changes make it
 *     more in the spirit of Tcl.
 *
 * Version 1.0:
 *     Released to general usage on UseNet
 *
 */

#include "tclSocket.h"


/*
 *----------------------------------------------------------------------
 *
 *  [file#] socket socket {socket_family} {socket_type} {protocol}
 *      socket_family = AF_INET (default)
 *      socket_type = SOCK_STREAM (default), SOCK_DGRAM, SOCK_RAW
 *      protocol = socket protocol. Default = 0
 *
 *  socket bind {file#} {server_addr}
 *      file# = value returned by socket
 *      server_addr = socket_address. This is a array name (see below for description)
 *
 *  socket listen {file#} {backlog}
 *      file# = value returned by socket
 *      backlog = number of outstanding connections (default and max is 5)
 *
 *  socket connect {file#} {server_addr}
 *      file# = value returned by socket
 *      server_addr = socket_address. This is a array name (see below for description)
 *
 *  [newfile#] socket accept {file#} {client_addr}
 *      newfile# = New socket descriptor from client
 *      file# = value returned by socket
 *      client_addr = socket_address of client connection. This is an array name.
 *
 *  [nvalue] socket htonl {value}
 *  [nvalue] socket htons {value}
 *  [value]  socket ntohl {nvalue}
 *  [value]  socket ntohs {nvalue}
 *      value = value in HOST order
 *      nvalue = value in NETWORK order
 *
 *  socket gethostbyname {hostent_var_name} {hostname}
 *      hostent_var_name = host_ent. This is the name of an array as defined below in host_ent
 *      hostname = name of host being looked up
 *
 *  [hostname] socket gethostname
 *      hostname = the current machine's hostname as known by the system
 *
 *  socket getprotobyname {protoent_var_name} {protoname}
 *      protoent_var_name = proto_ent. This is the name of an array as defined below in proto_ent
 *      protoname = name to look up in the protocol's list. i.e. TCP, UDP, etc
 *
 *  socket getservbyname {servent_var_name} {service_name} {protocol}
 *      servent_var_name = serv_ent. This is the name of an array as defined below in serv_ent
 *      service_name = name of service to look up
 *      protocol = name of protocol you wish to use, i.e. TCP, UDP
 *
 *  [file#] socket rcmd host_var_name port_num local_user_name remote_user_name command
 *      file# = stream file handle connected to remote command's STDIN and STDOUT and STDERR
 *      host_var_name = host name to connect to. Will be replaced with full hostname
 *      port_num = port number to connect to. I.E. rshd or others gotten by getservbyname?
 *      local_user_name = user name for local user
 *      remote_user_name = user name to run command under
 *      command = command to execute on remote machine
 *  ***NOTE: only the super-user can run this command correctly
 *
 *  [file#] socket rexec host_var_name port_num user passwd command
 *      file# = stream file handle connected to remote command's STDIN and STDOUT and STDERR
 *      host_var_name = host name to connect to. Will be replaced with full hostname
 *      port_num = port number to connect to. I.E. rshd or others gotten by getservbyname?
 *      user = user name to run command under
 *      passwd = password for user (can be "" and will be looked up
 *              in user's .netrc or prompted for it.
 *      command = command to execute on remote machine
 *
 *  socket getpeername file# peer_socket_var
 *      file# = open socket file handle
 *      peer_socket_var = socket_address of peer socket
 *
 *  socket getsockname file# sock_addr_var
 *      file# = open socket file handle
 *      sock_addr_var = socket_address of peer socket
 *
 *  socket shutdown file# how
 *      file# = open socket file handle
 *      how = how to shut down. NO_READ or NO_WRITE or NO_RDWR
 *
 ************ Array and structure definitions
 *
 *  socket_address = This is used to pass socket addresses around.
 *                  It is an array with each index a specific entry.
 *                  The index values are below along with allowable
 *                  entries:
 *      sin_family = AF_INET (default)
 *      sin_addr = INADDR_ANY or x.x.x.x notation 
 *                           (as returned by gethostbyname$(haddr) )
 *      sin_port = port number in host order
 *
 *  host_ent = This is used to store the result of gethostbyname.
 *                  It is an array with each index a specifiv entry.
 *                  The index values are below.
 *      h_name = Official name of host
 *      h_aliases = This is a LIST of aliases for the Official hostname
 *      h_addrtype = host's address type. Currently same as AF_INET
 *      h_length = length in bytes of address.. This can be used in other places as needed.
 *      h_addr = primary address for this host (in x.x.x.x notation)
 *      h_addr_list = LIST of address aliases for this host. h_addr is NOT in this list.
 *
 *  proto_ent = This is used to store the result of getprotobyname.
 *                  It is an array with each index a specifiv entry.
 *                  The index values are below.
 *      p_name = Official name of protocol
 *      p_aliases = This is a LIST of aliases for the Official protoname
 *      p_proto = This is the Protocol number that can be passed to "socket" and such.
 *
 *  serv_ent = This is used to store the result of getservbyname.
 *                  It is an array with each index a specifiv entry.
 *                  The index values are below.
 *      s_name = Official name of service
 *      s_aliases = LIST of aliases for the Official service name
 *      s_port = port number to connect to (in host byte order)
 *      s_proto = protocol name to use
 *
 ************
 *
 * Results:
 *  A standard Tcl result. Defined above in '[]'s.
 *
 *----------------------------------------------------------------------
 */

static char *socketCmdName = (char *)NULL;

static int  Get_sin(Tcl_Interp *iptr, char *cmdName, char *varName,
                    struct sockaddr_in *sin, int *length)
{
        int     result = TCL_ERROR;
        char    *value;
        char    rtnval[50];

        /* Now load all values! */
        if (sin->sin_family == AF_INET) {
            strcpy(rtnval,"AF_INET");
        }
        if ((value = Tcl_SetVar2(iptr,varName,"sin_family",rtnval,TCL_LEAVE_ERR_MSG))==(char *)NULL) {
            result = TCL_ERROR;
            goto exitPoint;
        }

        strcpy(rtnval,inet_ntoa(sin->sin_addr));
        if ((value = Tcl_SetVar2(iptr,varName,"sin_addr",rtnval,TCL_LEAVE_ERR_MSG))==(char *)NULL) {
            result = TCL_ERROR;
            goto exitPoint;
        }

        sprintf(rtnval,"%hu",ntohs(sin->sin_port));
        if ((value = Tcl_SetVar2(iptr,varName,"sin_port",rtnval,TCL_LEAVE_ERR_MSG))==(char *)NULL) {
            result = TCL_ERROR;
            goto exitPoint;
        }
        *length = sizeof(struct sockaddr_in);
        result = TCL_OK;
exitPoint:
        return (result);
}

static int  Set_sin(Tcl_Interp *iptr, char *cmdName, char *varName,
                    struct sockaddr_in *sin, char *lenParm, int *length)
{
        int     result = TCL_OK;
        char    *value;

        if ((value = Tcl_GetVar2(iptr,varName,"sin_family",TCL_LEAVE_ERR_MSG))==(char *)NULL) {
            result = TCL_ERROR;
            goto exitPoint;
        } else {
            if (STREQU(value, "AF_INET")) {
                sin->sin_family = AF_INET;
            }
        }
        if ((value = Tcl_GetVar2(iptr,varName,"sin_addr",TCL_LEAVE_ERR_MSG))==(char *)NULL) {
            result = TCL_ERROR;
            goto exitPoint;
        } else {
            if (STREQU(value, "INADDR_ANY")) {
                sin->sin_addr.s_addr = INADDR_ANY;
            } else {
                sin->sin_addr.s_addr = inet_addr(value);
            }
        }
        if ((value = Tcl_GetVar2(iptr,varName,"sin_port",TCL_LEAVE_ERR_MSG))==(char *)NULL) {
            result = TCL_ERROR;
            goto exitPoint;
        } else {
            sin->sin_port = htons((u_short)atoi(value));
        }
        if (lenParm != (char *)NULL) {
            *length = atoi(lenParm);
        } else {
            *length = sizeof(struct sockaddr_in);
        }
        result = TCL_OK;
exitPoint:
        return (result);
}


    /* ARGSUSED */
static int cmd_accept(dummy, iptr, argc, argv)
    ClientData dummy;   /* Not used */
    Tcl_Interp *iptr;   /* Current interpreter */
    int argc;           /* Number of arguments */
    char **argv;        /* Argument strings */
{
        int                 length = 0;
        struct sockaddr_in  sin;
        OpenFile            *origPtr=(OpenFile *)NULL,
                            *newPtr=(OpenFile *)NULL;
        int                 newFD;
        char                fdName[15];
        int                 result;
        Interp              *iPtr=(Interp *)iptr;

        
        if (argc != 3) {
            Tcl_AppendResult(iptr, socketCmdName, " ", argv[0],
                                ": wrong # of arguments: ", socketCmdName, " ",
                                argv[0],
                                " file# client_addr_var", (char *)NULL);
            result = TCL_ERROR;
            goto exitPoint;
        }
        if (TclGetOpenFile(iptr,argv[1],&origPtr)!=TCL_OK) {
            Tcl_AppendResult(iptr, socketCmdName, " ", argv[0],
                                ": ", argv[1], " is not a valid file descriptor",
                                (char *)NULL);
            result = TCL_ERROR;
            goto exitPoint;
        }
        if (origPtr->readable != 1 || origPtr->writable != 1) {
            Tcl_AppendResult(iptr, socketCmdName, " ", argv[0],
                                ": ", argv[1],
                                " is not readable and writeable. Possibly not a true socket",
                                (char *)NULL);
            result = TCL_ERROR;
            goto exitPoint;
        }
        if (argc==4)
            length = atoi(Tcl_GetVar(iptr,argv[3],0));
        else
            length = sizeof(struct sockaddr_in);

        if ((newFD = accept(fileno(origPtr->f), &sin, &length)) < 0) {
            Tcl_AppendResult(iptr, socketCmdName, " ", argv[0],
                                ": failed: ", Tcl_UnixError(iptr), (char *)NULL);
            result = TCL_ERROR;
            goto exitPoint;
        }
        newPtr = (OpenFile *)ckalloc(sizeof(OpenFile));
        newPtr->f = NULL;
        newPtr->f2 = NULL;
        newPtr->readable = 0;
        newPtr->writable = 0;
        newPtr->numPids = 0;
        newPtr->pidPtr = NULL;
        newPtr->errorId = 0;
        newPtr->f = fdopen(newFD,"a+");
        if (newPtr->f == (FILE *)NULL) {
            Tcl_AppendResult(iptr, socketCmdName, " ", argv[0],
                                ": fdopen failed: ", Tcl_UnixError(iptr), (char *)NULL);
            result = TCL_ERROR;
            goto exitPoint;
        }
        newPtr->readable = newPtr->writable = 1;
        TclMakeFileTable(iPtr,newFD);
        if (iPtr->filePtrArray[newFD] != (OpenFile *)NULL) {
            Tcl_AppendResult(iptr, socketCmdName, " ", argv[0], "socket already open error",
                (char *)NULL);
            result = TCL_ERROR;
            goto exitPoint;
        }
        iPtr->filePtrArray[newFD] = newPtr;
        setvbuf(newPtr->f, (char *)NULL, _IONBF, 0);
        sprintf(fdName,"file%d",newFD);
        Tcl_SetResult(iptr,fdName,TCL_VOLATILE);

        result = Get_sin(iptr, argv[0], argv[2], &sin, &length);
        if (result == TCL_ERROR) {
            newPtr = (OpenFile *)NULL;
            goto exitPoint;
        }

        result = TCL_OK;
exitPoint:
        if (result == TCL_ERROR)
            if (newPtr != (OpenFile *)NULL)
                ckfree((char *)newPtr);
        return(result);
}

    /* ARGSUSED */
static int cmd_bind(dummy, iptr, argc, argv)
    ClientData dummy;   /* Not used */
    Tcl_Interp *iptr;   /* Current interpreter */
    int argc;           /* Number of arguments */
    char **argv;        /* Argument strings */
{
        char                *p1,
                            *p2,
                            *value;
        int                 length = 0;
        struct sockaddr_in  sin;
        OpenFile            *filePtr=(OpenFile *)NULL;
        int                 result;

        if (argc < 3 || argc > 4) {
            Tcl_AppendResult(iptr, socketCmdName, " ", argv[0],
                                ": wrong # of arguments: ", socketCmdName, 
                                " ", argv[0], " file# server_addr",
                                (char *)NULL);
            result = TCL_ERROR;
            goto exitPoint;
        }
        if (TclGetOpenFile(iptr,argv[1],&filePtr)!=TCL_OK) {
            Tcl_AppendResult(iptr, socketCmdName, " ", argv[0],
                                ": ", argv[1], " is not a valid file descriptor",
                                (char *)NULL);
            result = TCL_ERROR;
            goto exitPoint;
        }
        if (filePtr->readable != 1 || filePtr->writable != 1) {
            Tcl_AppendResult(iptr, socketCmdName, " ", argv[0],
                        ": ", argv[1], " is not readable and writeable. Possibly not a true socket",
                        (char *)NULL);
            result = TCL_ERROR;
            goto exitPoint;
        }
        sin.sin_family = AF_INET;
        sin.sin_addr.s_addr = INADDR_ANY;
        sin.sin_port = 0;
        result = Set_sin(iptr, argv[0], argv[2], &sin, (argc==4?argv[3]:NULL), &length);
        if (result == TCL_ERROR) {
            goto exitPoint;
        }
        if (bind(fileno(filePtr->f),&sin,length) < 0) {
            Tcl_AppendResult(iptr, socketCmdName, " ", argv[0],
                        ": failed: ", Tcl_UnixError(iptr),
                        (char *)NULL);
            result = TCL_ERROR;
            goto exitPoint;
        }
        Tcl_ResetResult(iptr);
        result = TCL_OK;
exitPoint:
        return(result);
}

    /* ARGSUSED */
static int cmd_connect(dummy, iptr, argc, argv)
    ClientData dummy;   /* Not used */
    Tcl_Interp *iptr;   /* Current interpreter */
    int argc;           /* Number of arguments */
    char **argv;        /* Argument strings */
{
        int                 length = 0;
        struct sockaddr_in  sin;
        OpenFile            *filePtr=(OpenFile *)NULL;
        int                 result;

        if (argc != 3) {
            Tcl_AppendResult(iptr, socketCmdName, " ", argv[0],
                            ": wrong # of arguments: ",
                            socketCmdName, " ", argv[0], " file# server_addr",
                            (char *)NULL);
            result = TCL_ERROR;
            goto exitPoint;
        }
        if (TclGetOpenFile(iptr,argv[1],&filePtr)!=TCL_OK) {
            Tcl_AppendResult(iptr, socketCmdName, " ", argv[0],
                            ": ", argv[1], " is not a valid file descriptor",
                            (char *)NULL);
            result = TCL_ERROR;
            goto exitPoint;
        }
        if (filePtr->readable != 1 || filePtr->writable != 1) {
            Tcl_AppendResult(iptr, socketCmdName, " ", argv[0],
                            ": ", argv[1], " is not readable and writeable. Possibly not a true socket",
                            (char *)NULL);
            result = TCL_ERROR;
            goto exitPoint;
        }
        sin.sin_family = AF_INET;
        sin.sin_addr.s_addr = INADDR_ANY;
        sin.sin_port = 0;
        result = Set_sin(iptr, argv[0], argv[2], &sin, (argc==4?argv[3]:NULL), &length);
        if (result == TCL_ERROR) {
            goto exitPoint;
        }
        if (connect(fileno(filePtr->f),&sin,length) < 0) {
            Tcl_AppendResult(iptr, socketCmdName, " ", argv[0],
                            ": failed: ", Tcl_UnixError(iptr),
                            (char *)NULL);
            result = TCL_ERROR;
            goto exitPoint;
        }
        Tcl_ResetResult(iptr);
        result = TCL_OK;
exitPoint:
        return(result);
}

static int SetHostInfo(iptr, cmd_name, host, varName)
    Tcl_Interp      *iptr;
    char            *cmd_name;
    struct hostent  *host;
    char            *varName;
{
        struct in_addr  saddr;
        char            rtnval[50];
        char            *list;
        char            *tmplist;
        char            *value;
        int             index;
        int             result;

        if ((value = Tcl_SetVar2(iptr,varName,"h_name",host->h_name,TCL_LEAVE_ERR_MSG))==(char *)NULL) {
            result = TCL_ERROR;
            goto exitPoint;
        }
        list = ckalloc(5);
        strcpy(list,"");
        index = 0;
        while (host->h_aliases[index] != (char *)NULL) {
            tmplist = ckalloc(strlen(list)+strlen(host->h_aliases[index])+5);
            if (index == 0)
                sprintf(tmplist,"%s%s",list,host->h_aliases[index]);
            else
                sprintf(tmplist,"%s %s",list,host->h_aliases[index]);
            ckfree(list);
            list=tmplist;
            index++;
        }
        if ((value = Tcl_SetVar2(iptr,varName,"h_aliases",list,TCL_LEAVE_ERR_MSG))==(char *)NULL) {
            ckfree(list);
            result = TCL_ERROR;
            goto exitPoint;
        }
        ckfree(list);
        sprintf(rtnval,"%d",host->h_addrtype);
        if ((value = Tcl_SetVar2(iptr,varName,"h_addrtype",rtnval,TCL_LEAVE_ERR_MSG))==(char *)NULL) {
            result = TCL_ERROR;
            goto exitPoint;
        }
        sprintf(rtnval,"%d",host->h_length);
        if ((value = Tcl_SetVar2(iptr,varName,"h_length",rtnval,TCL_LEAVE_ERR_MSG))==(char *)NULL) {
            result = TCL_ERROR;
            goto exitPoint;
        }
        bcopy(host->h_addr_list[0], (char *)&saddr, host->h_length);
        strcpy(rtnval,inet_ntoa(saddr));
        if ((value = Tcl_SetVar2(iptr,varName,"h_addr",rtnval,TCL_LEAVE_ERR_MSG))==(char *)NULL) {
            result = TCL_ERROR;
            goto exitPoint;
        }
        list = ckalloc(5);
        strcpy(list,"");
        index = 1;
        while (host->h_addr_list[index] != (char *)NULL) {
            bcopy(host->h_addr_list[index], (char *)&saddr, host->h_length);
            strcpy(rtnval,inet_ntoa(saddr));
            tmplist = ckalloc(strlen(list)+strlen(rtnval)+5);
            if (index == 1)
                sprintf(tmplist,"%s%s",list,rtnval);
            else
                sprintf(tmplist,"%s %s",list,rtnval);
            ckfree(list);
            list=tmplist;
            index++;
        }
        if ((value = Tcl_SetVar2(iptr,varName,"h_addr_list",list,TCL_LEAVE_ERR_MSG))==(char *)NULL) {
            ckfree(list);
            result = TCL_ERROR;
            goto exitPoint;
        }
        ckfree(list);

        result = TCL_OK;
exitPoint:
        return(result);
}

    /* ARGSUSED */
static int cmd_gethostby(dummy, iptr, argc, argv)
    ClientData dummy;   /* Not used */
    Tcl_Interp *iptr;   /* Current interpreter */
    int argc;           /* Number of arguments */
    char **argv;        /* Argument strings */
{
        struct hostent  *host;
        extern int      h_errno;
        char            rtnval[50];
        char            *varName;
        char            *value;
        int             index;
        int             result;
        int             is_gethostbyname = FALSE;

        if (STREQU(argv[0],"gethostbyname"))
            is_gethostbyname = TRUE;
        else
            is_gethostbyname = FALSE;

        if (argc != 3) {
            Tcl_AppendResult(iptr, socketCmdName, " ", argv[0], 
                                    ": wrong # of arguments: ",
                                    socketCmdName, " ", argv[0], (char *)NULL);
            if (is_gethostbyname == TRUE)
                Tcl_AppendResult(iptr, 
                                    " host_ent_var_name hostname",
                                    (char *)NULL);
            else
                Tcl_AppendResult(iptr, 
                                    " host_ent_var_name IPAddress",
                                    (char *)NULL);
            result = TCL_ERROR;
            goto exitPoint;
        }
        varName = argv[1];
tryingAgain:
        if (is_gethostbyname == TRUE)
            host = gethostbyname (argv[2]);
        else
            host = gethostbyaddr (argv[2], strlen(argv[2]), AF_INET);
        if (host == (struct hostent *)NULL) {
            Tcl_AppendResult(iptr, socketCmdName, " ", argv[0],
                                    ": Error: ", (char *)NULL);
            switch (h_errno) {
                case TRY_AGAIN:
                    /* Sleep for 2 sec and then try again.. 
                        afterall that is what it is telling us to do */
                    sleep(2);
                    goto tryingAgain;
                    break;
                case HOST_NOT_FOUND:
                    Tcl_AppendResult(iptr, "Host Not Found.", (char *)NULL);
                    break;
                case NO_RECOVERY:
                    Tcl_AppendResult(iptr, "DNS unrecoverable failure.", (char *)NULL);
                    break;
                case NO_ADDRESS:
                    Tcl_AppendResult(iptr, "This host has NO address.", (char *)NULL);
                    break;
                default:
                    Tcl_AppendResult(iptr, "Unknown Error #", (char *)NULL);
                    sprintf(rtnval,"%d.", h_errno);
                    Tcl_AppendResult(iptr, rtnval, (char *)NULL);
                    break;
            }
            result = TCL_ERROR;
            goto exitPoint;
        }
        
        result = SetHostInfo(iptr, argv[0], host, varName);
        if (result == TCL_OK)
            Tcl_ResetResult(iptr);
exitPoint:
        endhostent();
        return(result);
}

    /* ARGSUSED */
static int cmd_gethostname(dummy, iptr, argc, argv)
    ClientData dummy;   /* Not used */
    Tcl_Interp *iptr;   /* Current interpreter */
    int argc;           /* Number of arguments */
    char **argv;        /* Argument strings */
{
        char    rtnval[MAXHOSTNAMELEN+1];
        int     result;

        if (argc != 1) {
            Tcl_AppendResult(iptr, socketCmdName, " ", argv[0],
                                ": wrong # of arguments: ",
                                socketCmdName, " ", argv[0],
                                (char *)NULL);
            result = TCL_ERROR;
            goto exitPoint;
        }
        rtnval[0]='\0';
        gethostname(rtnval,MAXHOSTNAMELEN);
        Tcl_SetResult(iptr,rtnval,TCL_VOLATILE);
        result = TCL_OK;
exitPoint:
        return (result);
}

    /* ARGSUSED */
static int cmd_getprotobyname(dummy, iptr, argc, argv)
    ClientData dummy;   /* Not used */
    Tcl_Interp *iptr;   /* Current interpreter */
    int argc;           /* Number of arguments */
    char **argv;        /* Argument strings */
{
        struct protoent     *proto;
        char                rtnval[50];
        char                *varName;
        char                *list;
        char                *tmplist;
        char                *value;
        int                 index;
        int                 result;

        if (argc != 3) {
            Tcl_AppendResult(iptr, socketCmdName, " ", argv[0],
                                ": wrong # of arguments: ", 
                                socketCmdName, " ", argv[0], " protoent_var_name protocol_name",
                                (char *)NULL);
            result = TCL_ERROR;
            goto exitPoint;
        }
        varName = argv[1];
        if ((proto=getprotobyname(argv[2]))==(struct hostent *)NULL) {
            Tcl_AppendResult(iptr, socketCmdName, " ", argv[0],
                                ": Error: Protocol \"", argv[2], "\" not found.",
                                (char *)NULL);
            result = TCL_ERROR;
            goto exitPoint;
        }
        
        if ((value = Tcl_SetVar2(iptr,varName,"p_name",proto->p_name,TCL_LEAVE_ERR_MSG))==(char *)NULL) {
            result = TCL_ERROR;
            goto exitPoint;
        }
        list = ckalloc(5);
        strcpy(list,"");
        index = 0;
        while (proto->p_aliases[index] != (char *)NULL) {
            tmplist = ckalloc(strlen(list)+strlen(proto->p_aliases[index])+5);
            if (index == 0)
                sprintf(tmplist,"%s%s",list,proto->p_aliases[index]);
            else
                sprintf(tmplist,"%s %s",list,proto->p_aliases[index]);
            ckfree(list);
            list=tmplist;
            index++;
        }
        if ((value = Tcl_SetVar2(iptr,varName,"p_aliases",list,TCL_LEAVE_ERR_MSG))==(char *)NULL) {
            ckfree(list);
            result = TCL_ERROR;
            goto exitPoint;
        }
        ckfree(list);
        sprintf(rtnval,"%d",proto->p_proto);
        if ((value = Tcl_SetVar2(iptr,varName,"p_proto",rtnval,TCL_LEAVE_ERR_MSG))==(char *)NULL) {
            result = TCL_ERROR;
            goto exitPoint;
        }

        Tcl_ResetResult(iptr);
        result = TCL_OK;
exitPoint:
        endprotoent();
        return(result);
}

    /* ARGSUSED */
static int cmd_getservbyname(dummy, iptr, argc, argv)
    ClientData dummy;   /* Not used */
    Tcl_Interp *iptr;   /* Current interpreter */
    int argc;           /* Number of arguments */
    char **argv;        /* Argument strings */
{
        struct servent      *serv;
        char                rtnval[50];
        char                *varName;
        char                *list;
        char                *tmplist;
        char                *value;
        int                 index;
        int                 result;

        if (argc != 4) {
            Tcl_AppendResult(iptr, socketCmdName, " ", argv[0],
                                ": wrong # of arguments: ",
                                socketCmdName, " ", argv[0],
                                " servent_var_name service protocol_name",
                                (char *)NULL);
            result = TCL_ERROR;
            goto exitPoint;
        }
        varName = argv[1];
        if ((serv=getservbyname(argv[2], argv[3]))==(struct servent *)NULL) {
            Tcl_AppendResult(iptr, socketCmdName, " ", argv[0],
                                ": Error: Service \"", argv[2],
                                "\" for Protocol \"", argv[3],
                                "\" not found.",
                                (char *)NULL);
            result = TCL_ERROR;
            goto exitPoint;
        }
        
        if ((value = Tcl_SetVar2(iptr,varName,"s_name",serv->s_name,TCL_LEAVE_ERR_MSG))==(char *)NULL) {
            result = TCL_ERROR;
            goto exitPoint;
        }
        list = ckalloc(5);
        strcpy(list,"");
        index = 0;
        while (serv->s_aliases[index] != (char *)NULL) {
            tmplist = ckalloc(strlen(list)+strlen(serv->s_aliases[index])+5);
            if (index == 0)
                sprintf(tmplist,"%s%s",list,serv->s_aliases[index]);
            else
                sprintf(tmplist,"%s %s",list,serv->s_aliases[index]);
            ckfree(list);
            list=tmplist;
            index++;
        }
        if ((value = Tcl_SetVar2(iptr,varName,"s_aliases",list,TCL_LEAVE_ERR_MSG))==(char *)NULL) {
            ckfree(list);
            result = TCL_ERROR;
            goto exitPoint;
        }
        ckfree(list);
        sprintf(rtnval,"%d",ntohs((ushort)serv->s_port));
        if ((value = Tcl_SetVar2(iptr,varName,"s_port",rtnval,TCL_LEAVE_ERR_MSG))==(char *)NULL) {
            result = TCL_ERROR;
            goto exitPoint;
        }
        if ((value = Tcl_SetVar2(iptr,varName,"s_proto",serv->s_proto,TCL_LEAVE_ERR_MSG))==(char *)NULL) {
            result = TCL_ERROR;
            goto exitPoint;
        }

        Tcl_ResetResult(iptr);
        result = TCL_OK;
exitPoint:
        endservent();
        return(result);
}

    /* ARGSUSED */
static int cmd_htonl(dummy, iptr, argc, argv)
    ClientData dummy;   /* Not used */
    Tcl_Interp *iptr;   /* Current interpreter */
    int argc;           /* Number of arguments */
    char **argv;        /* Argument strings */
{
        char    rtnval[30];
        int     result;
        unsigned long   arg=0, value=0;

        if (argc != 2) {
            Tcl_AppendResult(iptr, socketCmdName, " ", argv[0],
                                ": wrong # of arguments: ",
                                socketCmdName, " ", argv[0], " number",
                                (char *)NULL);
            result = TCL_ERROR;
            goto exitPoint;
        }

        arg = (unsigned long)atol(argv[1]);
        value = htonl(arg);
        sprintf(rtnval,"%lu",value);

        Tcl_SetResult(iptr,rtnval,TCL_VOLATILE);
        result = TCL_OK;
exitPoint:
        return(result);
}

    /* ARGSUSED */
static int cmd_htons(dummy, iptr, argc, argv)
    ClientData dummy;   /* Not used */
    Tcl_Interp *iptr;   /* Current interpreter */
    int argc;           /* Number of arguments */
    char **argv;        /* Argument strings */
{
        char    rtnval[30];
        int     result;
        unsigned short  arg=0, value=0;
        if (argc != 2) {
            Tcl_AppendResult(iptr, socketCmdName, " ", argv[0],
                                ": wrong # of arguments: ",
                                socketCmdName, " ", argv[0], " number",
                                (char *)NULL);
            result = TCL_ERROR;
            goto exitPoint;
        }

        arg = (unsigned short)atoi(argv[1]);
        value = htons(arg);
        sprintf(rtnval,"%hu",value);

        Tcl_SetResult(iptr,rtnval,TCL_VOLATILE);
        result = TCL_OK;
exitPoint:
        return(result);
}

    /* ARGSUSED */
static int cmd_listen(dummy, iptr, argc, argv)
    ClientData dummy;   /* Not used */
    Tcl_Interp *iptr;   /* Current interpreter */
    int argc;           /* Number of arguments */
    char **argv;        /* Argument strings */
{
        OpenFile    *filePtr=(OpenFile *)NULL;
        int         backlog=5;
        int         result;

        if (argc < 2 || argc > 3) {
            Tcl_AppendResult(iptr, socketCmdName, " ", argv[0],
                                ": wrong # of arguments: ",
                                socketCmdName, " ", argv[0], " file# [backlog]",
                                (char *)NULL);
            result = TCL_ERROR;
            goto exitPoint;
        }
        if (TclGetOpenFile(iptr,argv[1],&filePtr)!=TCL_OK) {
            Tcl_AppendResult(iptr, socketCmdName, " ", argv[0],
                                ": ", argv[1], "s is not a valid file descriptor",
                                (char *)NULL);
            result = TCL_ERROR;
            goto exitPoint;
        }
        if (filePtr->readable != 1 || filePtr->writable != 1) {
            Tcl_AppendResult(iptr, socketCmdName, " ", argv[0],
                        ": ", argv[1], " is not readable and writeable. Possibly not a true socket",
                        (char *)NULL);
            result = TCL_ERROR;
            goto exitPoint;
        }
        if (argc == 3) {
            backlog = atoi(argv[2]);
            if (backlog < 0)
                backlog = 0;
            if (backlog > 5)
                backlog = 5;
        }
        if (listen(fileno(filePtr->f),backlog) < 0) {
            Tcl_AppendResult(iptr, socketCmdName, " ", argv[0],
                        ": failed: ", Tcl_UnixError(iptr),
                        (char *)NULL);
            result = TCL_ERROR;
            goto exitPoint;
        }
        Tcl_ResetResult(iptr);
        result = TCL_OK;
exitPoint:
        return(result);
}

    /* ARGSUSED */
static int cmd_ntohl(dummy, iptr, argc, argv)
    ClientData dummy;   /* Not used */
    Tcl_Interp *iptr;   /* Current interpreter */
    int argc;           /* Number of arguments */
    char **argv;        /* Argument strings */
{
        char    rtnval[30];
        int     result;
        unsigned long   arg=0, value=0;
        if (argc != 2) {
            Tcl_AppendResult(iptr, socketCmdName, " ", argv[0],
                                ": wrong # of arguments: ",
                                socketCmdName, " ", argv[0], " number",
                                (char *)NULL);
            result = TCL_ERROR;
            goto exitPoint;
        }

        arg = (unsigned long)atol(argv[1]);
        value = ntohl(arg);
        sprintf(rtnval,"%lu",value);

        Tcl_SetResult(iptr,rtnval,TCL_VOLATILE);
        result = TCL_OK;
exitPoint:
        return(result);
}

    /* ARGSUSED */
static int cmd_ntohs(dummy, iptr, argc, argv)
    ClientData dummy;   /* Not used */
    Tcl_Interp *iptr;   /* Current interpreter */
    int argc;           /* Number of arguments */
    char **argv;        /* Argument strings */
{
        char    rtnval[30];
        int     result;
        unsigned short  arg=0, value=0;
        if (argc != 2) {
            Tcl_AppendResult(iptr, socketCmdName, " ", argv[0],
                                ": wrong # of arguments: ",
                                socketCmdName, " ", argv[0], " number",
                                (char *)NULL);
            result = TCL_ERROR;
            goto exitPoint;
        }

        arg = (unsigned short)atoi(argv[1]);
        value = ntohs(arg);
        sprintf(rtnval,"%hu",value);

        Tcl_SetResult(iptr,rtnval,TCL_VOLATILE);
        result = TCL_OK;
exitPoint:
        return(result);
}

    /* ARGSUSED */
static int cmd_socket(dummy, iptr, argc, argv)
    ClientData dummy;   /* Not used */
    Tcl_Interp *iptr;   /* Current interpreter */
    int argc;           /* Number of arguments */
    char **argv;        /* Argument strings */
{
        int         domain=AF_INET;
        int         type=SOCK_STREAM;
        int         protocol=0;
        int         s;
        OpenFile    *filePtr=(OpenFile *)NULL;
        int         result;
        Interp      *iPtr=(Interp *)iptr;

        filePtr = (OpenFile *)ckalloc(sizeof(OpenFile));
        filePtr->f = NULL;
        filePtr->f2 = NULL;
        filePtr->readable = 0;
        filePtr->writable = 0;
        filePtr->numPids = 0;
        filePtr->pidPtr = NULL;
        filePtr->errorId = 0;
        switch (argc) {
            case 4:
                protocol=atoi(argv[3]);
            case 3:
                if (STREQU(argv[2],"SOCK_STREAM")) {
                    type=SOCK_STREAM;
                } else if (STREQU(argv[2],"SOCK_DGRAM")) {
                    type=SOCK_DGRAM;
                } else if (STREQU(argv[2],"SOCK_RAW")) {
                    type=SOCK_RAW;
                }
            case 2:
                if (STREQU(argv[1],"AF_INET")) {
                    domain=AF_INET;
                }
        }
        s=socket(domain,type,protocol);
        if (s < 0) {
            Tcl_AppendResult(iptr, socketCmdName, " ", argv[0], ": failed: ",
                Tcl_UnixError(iptr), (char *)NULL);
            result = TCL_ERROR;
            goto exitPoint;
        }
        filePtr->f = fdopen(s,"a+");
        if (filePtr->f == (FILE *)NULL) {
            Tcl_AppendResult(iptr, socketCmdName, " ", argv[0], ": fdopen failed: ",
                Tcl_UnixError(iptr), (char *)NULL);
            close(s);
            result = TCL_ERROR;
            goto exitPoint;
        }
        filePtr->readable = filePtr->writable = 1;
        TclMakeFileTable(iPtr,s);
        if (iPtr->filePtrArray[s] != (OpenFile *)NULL) {
            Tcl_AppendResult(iptr, socketCmdName, " ", argv[0], ": socket already open error",
                (char *)NULL);
            result = TCL_ERROR;
            goto exitPoint;
        }
        iPtr->filePtrArray[s] = filePtr;
        setvbuf(filePtr->f, (char *)NULL, _IONBF, 0);
        sprintf(iPtr->result,"file%d",s);
        result = TCL_OK;
exitPoint:
        if (result == TCL_ERROR)
            if (filePtr != (OpenFile *)NULL)
                ckfree((char *)filePtr);
        return(result);
}

    /* ARGSUSED */
static int cmd_rexecrcmd(dummy, iptr, argc, argv)
    ClientData dummy;   /* Not used */
    Tcl_Interp *iptr;   /* Current interpreter */
    int argc;           /* Number of arguments */
    char **argv;        /* Argument strings */
{
        int             s;
        OpenFile        *filePtr=(OpenFile *)NULL;
        int             result;
        Interp          *iPtr=(Interp *)iptr;
        char            hostname[MAXHOSTNAMELEN];
        char            *h;
        unsigned short  port_num=0;
        char            *value;
        int             is_rexec=FALSE;

        filePtr = (OpenFile *)ckalloc(sizeof(OpenFile));
        filePtr->f = NULL;
        filePtr->f2 = NULL;
        filePtr->readable = 0;
        filePtr->writable = 0;
        filePtr->numPids = 0;
        filePtr->pidPtr = NULL;
        filePtr->errorId = 0;

        if (STREQU(argv[0], "rexec"))
            is_rexec = TRUE;
        else
            is_rexec = FALSE;

        if (argc != 6) {
            Tcl_AppendResult(iptr, socketCmdName, " ", argv[0],
                                    ": wrong # of arguments: ",
                                    socketCmdName, " ", argv[0],
                                    (char *)NULL);
            if (is_rexec == TRUE)
                Tcl_AppendResult(iptr,
                                " host_var_name port user passwd cmd",
                                (char *)NULL);
            else
                Tcl_AppendResult(iptr,
                                " host_var_name port locuser remuser cmd",
                                (char *)NULL);
            result = TCL_ERROR;
            goto exitPoint;
        }
        if ((value = Tcl_GetVar(iptr,argv[1],TCL_LEAVE_ERR_MSG))==(char *)NULL) {
            result = TCL_ERROR;
            goto exitPoint;
        } else {
            strcpy(hostname,value);
        }
        port_num = (unsigned short)atoi(argv[2]);
        h = hostname;
        if (is_rexec == TRUE)
            s=rexec(&h,port_num,argv[3],argv[4],argv[5],(int *)0);
        else
            s=rcmd(&h,port_num,argv[3],argv[4],argv[5],(int *)0);
        if (s < 0) {
            Tcl_AppendResult(iptr, socketCmdName, " ", argv[0], ": failed: ",
                Tcl_UnixError(iptr), (char *)NULL);
            result = TCL_ERROR;
            goto exitPoint;
        }
        filePtr->f = fdopen(s,"a+");
        if (filePtr->f == (FILE *)NULL) {
            Tcl_AppendResult(iptr, socketCmdName, " ", argv[0], ": fdopen failed: ",
                Tcl_UnixError(iptr), (char *)NULL);
            close(s);
            result = TCL_ERROR;
            goto exitPoint;
        }
        filePtr->readable = filePtr->writable = 1;
        TclMakeFileTable(iPtr,s);
        if (iPtr->filePtrArray[s] != (OpenFile *)NULL) {
            Tcl_AppendResult(iptr, socketCmdName, " ", argv[0], ":socket already open error",
                (char *)NULL);
            result = TCL_ERROR;
            goto exitPoint;
        }
        iPtr->filePtrArray[s] = filePtr;
        setvbuf(filePtr->f, (char *)NULL, _IONBF, 0);
        sprintf(iPtr->result,"file%d",s);
        result = TCL_OK;
        if ((value = Tcl_SetVar(iptr,argv[1],hostname,TCL_LEAVE_ERR_MSG))==(char *)NULL) {
            result = TCL_ERROR;
            goto exitPoint;
        }
exitPoint:
        if (result == TCL_ERROR)
            if (filePtr != (OpenFile *)NULL)
                ckfree((char *)filePtr);
        return(result);
}

    /* ARGSUSED */
static int cmd_getnameinfo(dummy, iptr, argc, argv)
    ClientData dummy;   /* Not used */
    Tcl_Interp *iptr;   /* Current interpreter */
    int argc;           /* Number of arguments */
    char **argv;        /* Argument strings */
{
        int                 length = 0;
        struct sockaddr_in  sin;
        int                 result;
        OpenFile            *origPtr = (OpenFile *)NULL;
        int                 is_getpeername = FALSE;
        
        if (STREQU(argv[0],"getpeername"))
            is_getpeername = TRUE;
        else
            is_getpeername = FALSE;

        if (argc != 3) {
            Tcl_AppendResult(iptr, socketCmdName, " ", argv[0],
                                ": wrong # of arguments: ",
                                socketCmdName, " ", argv[0], " file# addr_var",
                                (char *)NULL);
            result = TCL_ERROR;
            goto exitPoint;
        }
        if (TclGetOpenFile(iptr,argv[1],&origPtr)!=TCL_OK) {
            Tcl_AppendResult(iptr, socketCmdName, " ", argv[0],
                                ": ", argv[1], " is not a valid file descriptor",
                                (char *)NULL);
            result = TCL_ERROR;
            goto exitPoint;
        }
        if (origPtr->readable != 1 || origPtr->writable != 1) {
            Tcl_AppendResult(iptr, socketCmdName, " ", argv[0],
                        ": ", argv[1], " is not readable and writeable. Possibly not a true socket",
                                (char *)NULL);
            result = TCL_ERROR;
            goto exitPoint;
        }
        if (argc==4)
            length = atoi(Tcl_GetVar(iptr,argv[3],0));
        else
            length = sizeof(struct sockaddr_in);

        if (is_getpeername == TRUE)
            result = getpeername(fileno(origPtr->f), &sin, &length);
        else
            result = getsockname(fileno(origPtr->f), &sin, &length);

        if (result < 0) {
            Tcl_AppendResult(iptr, socketCmdName, " ", argv[0],
                        ": failed: ", Tcl_UnixError(iptr),
                                (char *)NULL);
            result = TCL_ERROR;
            goto exitPoint;
        }

        result = Get_sin(iptr, argv[0], argv[2], &sin, &length);
        if (result == TCL_ERROR) {
            goto exitPoint;
        }
        result = TCL_OK;
exitPoint:
        return(result);
}
    
    /* ARGSUSED */
static int cmd_shutdown(dummy, iptr, argc, argv)
    ClientData dummy;   /* Not used */
    Tcl_Interp *iptr;   /* Current interpreter */
    int argc;           /* Number of arguments */
    char **argv;        /* Argument strings */
{
        int                 result;
        OpenFile            *origPtr = (OpenFile *)NULL;
        int                 how;
        
        if (argc != 3) {
            Tcl_AppendResult(iptr, socketCmdName, " ", argv[0],
                            ": wrong # of arguments: ",
                            socketCmdName, " ", argv[0], " file# NO_READ|NO_WRITE|NO_RDWR",
                            (char *)NULL);
            result = TCL_ERROR;
            goto exitPoint;
        }
        if (STREQU(argv[2],"NO_READ"))
            how = 0;
        else if (STREQU(argv[2],"NO_WRITE"))
            how = 1;
        else if (STREQU(argv[2],"NO_RDWR"))
            how = 2;
        else {
            Tcl_AppendResult(iptr, socketCmdName, " ", argv[0],
                                ": ", argv[2], " not a valid option. Valid options are: NO_READ, NO_WRITE, NO_RDWR",
                                (char *)NULL);
            result = TCL_ERROR;
            goto exitPoint;

        }
        if (TclGetOpenFile(iptr,argv[1],&origPtr)!=TCL_OK) {
            Tcl_AppendResult(iptr, socketCmdName, " ", argv[0],
                                ": ", argv[1], " is not a valid file descriptor",
                                (char *)NULL);
            result = TCL_ERROR;
            goto exitPoint;
        }
        if (origPtr->readable != 1 || origPtr->writable != 1) {
            Tcl_AppendResult(iptr, socketCmdName, " ", argv[0],
                        ": ", argv[1], " is not readable and writeable. Possibly not a true socket",
                                (char *)NULL);
            result = TCL_ERROR;
            goto exitPoint;
        }

        result = shutdown(fileno(origPtr->f), how);

        if (result < 0) {
            Tcl_AppendResult(iptr, socketCmdName, " ", argv[0],
                        ": failed: ", Tcl_UnixError(iptr),
                                (char *)NULL);
            result = TCL_ERROR;
            goto exitPoint;
        }

        result = TCL_OK;
exitPoint:
        return(result);
}

static struct socket_cmd_ {
        char                *cmd;
        Tcl_CmdProc         *cmd_proc;
        ClientData          cmdData;
        Tcl_CmdDeleteProc   *cmdDelete;
        } socket_cmd[] = {
            "accept",           cmd_accept,         (ClientData)NULL, (Tcl_CmdDeleteProc *)NULL,
            "bind",             cmd_bind,           (ClientData)NULL, (Tcl_CmdDeleteProc *)NULL,
            "connect",          cmd_connect,        (ClientData)NULL, (Tcl_CmdDeleteProc *)NULL,
            "gethostbyname",    cmd_gethostby,      (ClientData)NULL, (Tcl_CmdDeleteProc *)NULL,
            "gethostbyaddr",    cmd_gethostby,      (ClientData)NULL, (Tcl_CmdDeleteProc *)NULL,
            "gethostname",      cmd_gethostname,    (ClientData)NULL, (Tcl_CmdDeleteProc *)NULL,
            "getpeername",      cmd_getnameinfo,    (ClientData)NULL, (Tcl_CmdDeleteProc *)NULL,
            "getprotobyname",   cmd_getprotobyname, (ClientData)NULL, (Tcl_CmdDeleteProc *)NULL,
            "getservbyname",    cmd_getservbyname,  (ClientData)NULL, (Tcl_CmdDeleteProc *)NULL,
            "getsockname",      cmd_getnameinfo,    (ClientData)NULL, (Tcl_CmdDeleteProc *)NULL,
            "htonl",            cmd_htonl,          (ClientData)NULL, (Tcl_CmdDeleteProc *)NULL,
            "htons",            cmd_htons,          (ClientData)NULL, (Tcl_CmdDeleteProc *)NULL,
            "listen",           cmd_listen,         (ClientData)NULL, (Tcl_CmdDeleteProc *)NULL,
            "ntohl",            cmd_ntohl,          (ClientData)NULL, (Tcl_CmdDeleteProc *)NULL,
            "ntohs",            cmd_ntohs,          (ClientData)NULL, (Tcl_CmdDeleteProc *)NULL,
            "rexec",            cmd_rexecrcmd,      (ClientData)NULL, (Tcl_CmdDeleteProc *)NULL,
            "rcmd",             cmd_rexecrcmd,      (ClientData)NULL, (Tcl_CmdDeleteProc *)NULL,
            "shutdown",         cmd_shutdown,       (ClientData)NULL, (Tcl_CmdDeleteProc *)NULL,
            "socket",           cmd_socket,         (ClientData)NULL, (Tcl_CmdDeleteProc *)NULL,
            (char *)NULL,       (Tcl_CmdProc *)NULL,(ClientData)NULL, (Tcl_CmdDeleteProc *)NULL
        };

int Tcl_SocketCmd(ClientData clientData, Tcl_Interp *interp,
                  int argc, char *argv[])
{
    register struct socket_cmd_ *cmd;
    cmd = &socket_cmd[0];

    socketCmdName = argv[0];
    if (argc < 2)
        goto error_rtn;
    while (cmd->cmd != (char *)NULL) {
        if (STREQU(argv[1],cmd->cmd)) {
            return((*cmd->cmd_proc)(clientData,interp,(argc-1),&argv[1]));
        }
        cmd++;
    }
error_rtn:
    Tcl_AppendResult(interp, argv[0],
                ": '", argv[1], "' not a valid command: valid commands are:",
                (char *)NULL);
    cmd = &socket_cmd[0];
    while (cmd->cmd != (char *)NULL) {
        Tcl_AppendResult(interp, " ", cmd->cmd, (char *)NULL);
        cmd++;
    }
    return(TCL_ERROR);
}

void Tcl_SocketDelete(ClientData clientData)
{
    register struct socket_cmd_ *cmd;
    cmd = &socket_cmd[0];

    while (cmd->cmd != (char *)NULL) {
        if (cmd->cmdDelete != (Tcl_CmdDeleteProc *)NULL)
            (*cmd->cmdDelete)(clientData);
        cmd++;
    }
}

void Tcl_InitSocket(Tcl_Interp *iPtr)
{
    Tcl_CreateCommand(iPtr,"socket",Tcl_SocketCmd,(ClientData)NULL,Tcl_SocketDelete);
}



