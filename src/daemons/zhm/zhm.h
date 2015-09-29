#ifndef __HM_H__
#define __HM_H__
/* This file is part of the Project Athena Zephyr Notification System.
 * It contains the hostmanager header file.
 *
 *      Created by:     David C. Jedlinsky
 *
 *      $Source: /afs/athena.mit.edu/astaff/project/zephyr/src/zhm/RCS/zhm.h,v $
 *      $Author: lwvanels $
 *      $Zephyr: /mit/zephyr/src.rw/zhm/RCS/zhm.h,v 1.13 90/10/19 07:11:48 raeburn Exp $
 *
 *      Copyright (c) 1987, 1991 by the Massachusetts Institute of Technology.
 *      For copying and distribution information, see the file
 *      "mit-copyright.h". 
 */

#include <zephyr/mit-copyright.h>
#include <zephyr/zephyr.h>
#include <zephyr/zsyslog.h>
#include <sys/socket.h>
#include <sys/param.h>
#include <netdb.h>
#ifdef lint
#include <sys/uio.h>			/* make lint shut up */
#endif /* lint */

#ifdef DEBUG
#define DPR(a) fprintf(stderr, a); fflush(stderr)
#define DPR2(a,b) fprintf(stderr, a, b); fflush(stderr)
#define Zperr(e) fprintf(stderr, "Error = %d\n", e)
#else
#define DPR(a)
#define DPR2(a,b)
#define Zperr(e)
#endif

#define ever (;;)

#define SERV_TIMEOUT 20
#define NOTICE_TIMEOUT 25
#define BOOTING 1
#define NOTICES 2

#define MAXRETRIES 2

extern char *malloc();
extern Code_t send_outgoing();
extern void init_queue(), retransmit_queue();

#ifdef vax
#define MACHINE "vax"
#define use_etext
#define ok
#endif /* vax */

#ifdef ibm032
#define MACHINE "rt"
#define adjust_size(size)	size -= 0x10000000
#define ok
#endif /* ibm032 */

#ifdef NeXT
#define MACHINE "NeXT"
#define ok
#endif /* NeXT */

#ifdef sun
#ifdef SUN2_ARCH
#define MACHINE "sun2"
#define ok
#endif /* SUN2_ARCH */

#ifdef SUN3_ARCH
#define MACHINE "sun3"
#define ok
#endif /* SUN3_ARCH */

#if defined (SUN4_ARCH) || defined (sparc)
#define MACHINE "sun4"
#define use_etext
#define ok
#endif /* SUN4_ARCH */

#if defined(sprite)
#if defined(ds3100)
#define adjust_size(size)	size -= 0x10000000
#endif
#if defined(sun4)
#define use_etext
#endif
#if defined(sun3)
#define adjust_size(size)	size -= 0x10000000
#endif
#define ok
#endif /* Sprite */

#ifndef ok
#if defined (m68k)
#define MACHINE "sun (unknown 68k)"
#else
#define MACHINE "sun (unknown)"
#endif
#define ok
#endif /* ! ok */
#endif /* sun */

#ifdef _AIX
#ifdef i386
#define	MACHINE	"ps2"
#define adjust_size(size)	size -= 0x400000
#endif
#ifdef _IBMR2
#define	MACHINE "IBM RS/6000"
#define	adjust_size(size)	size -= 0x20000000
#endif
#define	ok
#endif

#if defined(ultrix) && defined(mips)
#define MACHINE "decmips"
#define adjust_size(size)	size -= 0x10000000
#define ok
#endif /* ultrix && mips */


#ifdef use_etext
extern int etext;
#define adjust_size(size)	size -= (unsigned int) &etext;
#undef use_etext
#endif

#ifndef ok
#define MACHINE "unknown"
#endif
#undef ok

#endif
