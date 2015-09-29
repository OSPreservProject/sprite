/*
 * sockInt.h --
 *
 *	Internal declarations of the socket-related routines.
 *	The data structures defined in this filed are used to manage
 *	sockets.
 *
 * Copyright 1987 Regents of the University of California
 * All rights reserved.
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *
 *
 * $Header: /sprite/src/daemons/ipServer/RCS/sockInt.h,v 1.6 89/08/15 19:55:38 rab Exp $ SPRITE (Berkeley)
 */

#ifndef _IPS_SOCKINT
#define _IPS_SOCKINT

#include "sprite.h"
#include "dev/net.h"
#include "netInet.h"
#include "fs.h"
#include "list.h"
#include "proc.h"

#ifdef KERNEL
#include "../fs/fsInt.h"
#include "../fs/fsSocket.h"
#endif

/*
 * Sock_State defines the various states that a socket can be in at
 * any one moment.
 */
typedef enum {
    UNUSED,		/* Not in use. */
    CREATED,		/* The socket has been created by opening the
			 * socket pseudo-device. */
    HAVE_LOCAL_ADDR,	/* A bind operation has been performed. */
    READY,		/* The socket can send and receive datagrams
			 * (datagram only). */
    LISTENING,		/* A stream socket is waiting for connection
			 * requests from remote hosts. */
    CONNECTING,		/* A stream socket trying to connect to a remote host.*/
    CONNECTED,		/* Stream socket: the connection attempt succeeded.
    			 * Datagram socket: the local and remote addresses
			 * have been defined. */
    DISCONNECTING,	/* In the process of disconnecting from a remote host */
    DISCONNECTED,	/* The connection has been closed. */
} Sock_State;


/*
 * There are two buffer queues for a socket. Data in the queue are
 * held in a linked list.
 */
typedef struct {
    List_Links		links;		/* List of Sock_BufDataInfo items. */
    int			size;		/* # of bytes in the bufffer. */
    int			maxSize;	/* Max. # of bytes allowed. */
} Sock_BufInfo;


/*
 * Each data buffer in a socket buffer queue is described by this structure.
 */

typedef struct {
    List_Links	links;			/* All data are linked together */
    Net_InetSocketAddr	address;	/* For reads, who sent the packet. */
    int		len;			/* Number of bytes stored. */
    Address	bufPtr;			/* Ptr to the data buffer. */
    Address	base;			/* Base address of the Mem_Alloc'd
					 * memory used for bufPtr. This is
					 * the address given to Mem_Free when
					 * info is no longer needed. */
} Sock_BufDataInfo;

/*
 * Information about the peudo-device request buffer associated with
 * each socket.  This is set up and used when the pseudo-device connection
 * is just being created.  It is passed to the Sock_Open routine, and the
 * location of the request buffer is used for cleanup and for error checking.
 */

typedef struct SockPdevState {
    int protoIndex;		/* Type for Sock_SharedInfo */
    int reqBufSize;		/* Size of the request buffer */
    char *requestBuf;		/* The request buffer itself */
} SockPdevState;

/*
 * Information about a socket that is shared among processes using the socket.
 *  1) current state of use, protocol type.
 *  2) pointer to protocol-dependent information.
 *  3) queues that hold data to be sent to another socket
 *     and data that has been received from another socket.
 *  4) info about the local and remote addresses assigned to this socket.
 *
 */
typedef struct Sock_SharedInfo {
    List_Links		protoLinks;	/* Links to others of same protocol. */
    int			protoIndex;	/* One of RAW_PROTO_INDEX,
					 * UDP_PROTO_INDEX, TCP_PROTO_INDEX. */
    int			protocol;	/* For raw sockets: the IP protocol #
					 * that must be present in incoming
					 * packet in order to be received;
					 * protocol # used in IP header on
					 * output. */
    ClientData		protoData;	/* Protocol-specific information. */
    int			reqBufSize;	/* Size of the pdev request buffer */
    char 	       *requestBuf;	/* The pdev request buffer.  Packet
					 * data is put here by the kernel, and
					 * we format packets in place (without
					 * copies) before outputing to the net*/
    Sock_State		state;		/* Defined above. */
    int			options;	/* Manipulated via IOC_NET_GET_OPTION/
					 * SET_OPTION in <dev/net.h>. */
    Ioc_Owner		owner;		/* Current owner (process or family). */
    int			flags;		/* Defined below. */
    int			clientCount;	/* # of users using this socket. */
    ReturnStatus	error;		/* Asynchronous error status from
					 * ICMP or from the protocol-dependent
					 * layer. */
    Sock_BufInfo	recvBuf;	/* Data from the network. */
    Sock_BufInfo	sendBuf;	/* Data to be sent out. */

    Net_InetSocketAddr	local;		/* Address for this host. */
    Net_InetSocketAddr	remote;		/* Address for remote host. */

    Net_InetSocketAddr	sentTo;		/* Destination of the most-recently
					 * sent packet (only if not connected.)
					 */
    int			linger;		/* # of seconds to wait upon closing. */
    struct Sock_SharedInfo *parentPtr;	/* Used to wakeup a socket waiting to
					 * accept a new connection. */
    List_Links		clientList;	/* List of clients using this socket.
					 * Used to notify when the select state
					 * changes. */
    Boolean		justEstablished; /* Used by a connection-based
					 * protocol's select routine to notify
					 * a waiter that the connection has
					 * just become established. */
} Sock_SharedInfo;

/*
 * Sock_SharedInfo flags:
 *	SOCK_STOP_RECV		- don't accept any more data.
 *	SOCK_STOP_SEND		- can't send data any more.
 *	SOCK_URGENT_DATA_NEXT	- urgent data is at the next logical byte
 *				  in the receive buffer.
 *	SOCK_RAW_HAVE_LOCAL_ADDR - for raw sockets, the local address field
 *				   contains a usable address.
 *	SOCK_HAVE_BOUND_ADDR	 - An address has been bound to the socket
 *				   with a bind operation. This flag is used
 *				   to prevent mutiple bind operations on the
 *				   same socket.
 */

#define SOCK_STOP_RECV			0x01
#define SOCK_STOP_SEND			0x02
#define SOCK_URGENT_DATA_NEXT		0x04
#define SOCK_RAW_HAVE_LOCAL_ADDR	0x08
#define	SOCK_HAVE_BOUND_ADDR		0x10


/*
 * SOCK_MAX_LINGER_TIME	- linger at most 2 minutes.
 */
#define	SOCK_MAX_LINGER_TIME	120

/*
 * Information about a socket that must be kept for each process using the
 * socket.
 */
typedef struct Sock_PrivInfo {
    List_Links		links;		/* Used to attach this element to the
					 * socket's clientList. */
    Sock_SharedInfo	*sharePtr;	/* Common socket info. */
#ifndef KERNEL
    int			streamID;	/* Pdev stream ID. */
#else
    struct FsSocketIOHandle	*sockHandlePtr;	/* Back pointer to FS handle */
#endif
    int			fsFlags;	/* Flags from Fs_Open. */
    Proc_PID		pid;		/* Process using this socket. */
    int			hostID;		/* Host where process is based. */
    int			userID;		/* User-ID of the process. */
    int			clientID;	/* Client ID for the pdev.  */
    int			recvFlags;	/* Flags from IOC_NET_RECV_FLAGS ioctl.
					 * Needed to emulate recv, recvfrom,
					 * recvmsg. Defined in <dev/net.h>. */
    Net_InetSocketAddr	recvFrom;	/* Source of the packet that was just
					 * read. Needed for recvfrom, recvmsg
					 * emulation. */
    Net_SendInfo	sendInfo;	/* Info from IOC_NET_SEND_INFO ioctl.
					 * Needed to emulate send,sendto,
					 * sendmsg. */
    Boolean		sendInfoValid;	/* If TRUE, the sendInfo is useful. */
} Sock_PrivInfo;


#endif /* _IPS_SOCKINT */
