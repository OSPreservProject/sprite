/*
 * memcmp --- compare strings.
 *
 * We use our own routine since it has to act like strcmp() for return
 * value, and the BSD manual says bcmp() only returns zero/non-zero.
 */

int
memcmp (s1, s2, l)
register char *s1, *s2;
register int l;
{
	for (; l--; s1++, s2++) {
		if (*s1 != *s2)
			return (*s1 - *s2);
	}
	return (*--s1 - *--s2);
}
