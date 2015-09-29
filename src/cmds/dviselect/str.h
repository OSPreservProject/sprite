/*
 * Copyright (c) 1987 University of Maryland Department of Computer Science.
 * All rights reserved.  Permission to copy for any purpose is hereby granted
 * so long as this copyright notice remains intact.
 */

/*
 * Internal strings have both a length and a string area, so that they may
 * contain nulls.
 */
typedef struct string {
	int	s_len;
	char	*s_str;
} String;

#define s_eq(s1,s2) ((s1)->s_len == (s2)->s_len && _s_eq(s1, s2))

/*
 * Strings are created by appending characters to the current "string pool".
 * Once a string has been completed, it is fixed in place and cannot be
 * changed.  Until the string is complete, characters may be added to, and
 * removed from, the end.
 */
#define PoolExpandSize 1024	/* bytes per ExpandPool */

String	PoolString;		/* the current string */
char	*PoolPtr;		/* the current position in the pool */
int	PoolLen;		/* amount of space left in the pool */

#define s_addc(c) (--PoolLen >= 0 ? *PoolPtr++ = (c), PoolString.s_len++ \
				  : ExpandPool(c))
#define s_delc()  (PoolLen++, PoolPtr--, PoolString.s_len--)
#define s_kill()  (PoolLen += PoolString.s_len, \
		   PoolPtr -= PoolString.s_len, PoolString.s_len = 0)
