/*
 * sunSR.h --
 *
 *     Constants for the Sun status register.
 *
 * Copyright (C) 1985 Regents of the University of California
 * All rights reserved.
 *
 * $Header$ SPRITE (Berkeley)
 *
 */

#ifndef _SUNSR
#define _SUNSR

/*
 *  Definition of bits in the 68010 status register (SR)
 */

/* Trace mode mask */
#define	SUN_SR_TRACEMODE	0x8000

/* Supervisor state mask */
#define	SUN_SR_SUPSTATE		0x2000

/* Interrupt level mask */
#define	SUN_SR_INTMASK		0x0700
 
/* Condition codes mask */
#define	SUN_SR_CC		0x001F

/*
 *  Masks for eight interrupt priority levels:
 *   lowest = 0,   highest = 7.
 *
 */

#define	SUN_SR_PRIO_0		0x0000
#define	SUN_SR_PRIO_1		0x0100
#define	SUN_SR_PRIO_2		0x0200
#define	SUN_SR_PRIO_3		0x0300
#define	SUN_SR_PRIO_4		0x0400
#define	SUN_SR_PRIO_5		0x0500
#define	SUN_SR_PRIO_6		0x0600
#define	SUN_SR_PRIO_7		0x0700

/*
 *  The CPU must be in the supervisor state when the priority level 
 *  is changed, hence the supervisor bit is set in these constants.
 */
#define	SUN_SR_HIGHPRIO		0x2700
#define	SUN_SR_LOWPRIO		0x2000

/* User priority */
#define	SUN_SR_USERPRIO		0x0000

#endif _SUNSR
