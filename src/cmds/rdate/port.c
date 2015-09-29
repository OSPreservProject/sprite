/* port --- routines to deal with sockets and ports for user programs */

/* Copyright (c) 1990, W. Keith Pyle, Austin, Texas */

#include "rdate.h"

/* ------------------------------------------------------------------------- */

open_port(host, port)

char *host;
int port;

{
	int socket_fd;

	struct hostent *hentry;
	struct sockaddr_in sa;

	/* Make sure there is a host name specified */

	if (host == NULL || *host == '\0') {

		errno = EDESTADDRREQ;
		return(-1);
	}

	/* Get the host file entry for the named host */

	if ((hentry = gethostbyname(host)) == NULL) {

		errno = ECONNREFUSED;
		return(-1);
	}

	/* Clear the socket structure */

	bzero((char *)&sa, sizeof(sa));

	/* Put the host's address in the socket structure */

	bcopy(hentry->h_addr, (char *)&sa.sin_addr, hentry->h_length);

	/* Set the address type and port */

	sa.sin_family = hentry->h_addrtype;
	sa.sin_port = htons((unsigned short)port);

	/* Open the socket */

	if ((socket_fd = socket(hentry->h_addrtype, SOCK_STREAM, 0)) < 0)
		return(-1);
	
	/* Connect to the port */

	if (connect(socket_fd, (struct sockaddr *)&sa, sizeof(sa)) < 0)
		return(-1);
	
	/* Return the socket descriptor */

	return(socket_fd);
}
