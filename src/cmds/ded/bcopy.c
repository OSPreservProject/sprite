bcopy(from, to, len)
register char *from, *to;
register int len;
{

    while (len--)
    	*to++ = *from++;
}
