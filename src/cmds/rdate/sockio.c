/* sockio --- read and write routines for use with sockets */

/* Copyright (c) 1990, W. Keith Pyle, Austin, Texas */

/* ------------------------------------------------------------------------- */

read_socket(socket_fd, buffer, size)

int socket_fd;
register char *buffer;
register int size;

{
	register int bytes;
	register int n;

	bytes = 0;

	while (bytes < size) {

		if ((n = read(socket_fd, buffer + bytes, size - bytes)) < 0)
			return(n);
		
		bytes += n;
	}

	return(bytes);
}

/* ------------------------------------------------------------------------- */

write_socket(socket_fd, buffer, size)

int socket_fd;
register char *buffer;
register int size;

{
	register int bytes;
	register int n;

	bytes = 0;

	while (bytes < size) {

		if ((n = write(socket_fd, buffer + bytes, size - bytes)) < 0)
			return(n);
		
		bytes += n;
	}

	return(bytes);
}
