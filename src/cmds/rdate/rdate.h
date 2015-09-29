/* rdate.h --- include file for remote server date setting utility */

/* Copyright (c) 1990, W. Keith Pyle, Austin, Texas */

#if defined(sun) || defined(ultrix)
#define HAS_ADJTIME
#endif

/* Sprite doesn't have adjtime */
#undef HAS_ADJTIME

#if defined(hpux)
#define bcopy(from, to, count) memcpy(to, from, count)
#define bzero(to, count) memset(to, 0, count)
#endif

#if defined(SYSV) && defined(HAS_ADJTIME)
#undef HAS_ADJTIME
#endif

#include <stdio.h>
#include <errno.h>
#include <time.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/uio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#ifdef 386IX
#include <net/errno.h>
#endif

#define TIME_PORT	37

#define FALSE	0
#define TRUE	1

void exit();
