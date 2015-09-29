
#define longcopy(source,dest,size)    \
{ \
    register long 	* to = source, * from = dest; \
    register int	count = size >> 2; \
    switch (count & 63) { \
	do { \
	  case 0:   *to++ = *from++;	  case 63:  *to++ = *from++; \
	  case 62:  *to++ = *from++;	  case 61:  *to++ = *from++; \
	  case 60:  *to++ = *from++;	  case 59:  *to++ = *from++; \
	  case 58:  *to++ = *from++;	  case 57:  *to++ = *from++; \
	  case 56:  *to++ = *from++;	  case 55:  *to++ = *from++; \
	  case 54:  *to++ = *from++;	  case 53:  *to++ = *from++; \
	  case 52:  *to++ = *from++;	  case 51:  *to++ = *from++; \
	  case 50:  *to++ = *from++;	  case 49:  *to++ = *from++; \
	  case 48:  *to++ = *from++;	  case 47:  *to++ = *from++; \
	  case 46:  *to++ = *from++;	  case 45:  *to++ = *from++; \
	  case 44:  *to++ = *from++;	  case 43:  *to++ = *from++; \
	  case 42:  *to++ = *from++;	  case 41:  *to++ = *from++; \
	  case 40:  *to++ = *from++;	  case 39:  *to++ = *from++; \
	  case 38:  *to++ = *from++;	  case 37:  *to++ = *from++; \
	  case 36:  *to++ = *from++;	  case 35:  *to++ = *from++; \
	  case 34:  *to++ = *from++;	  case 33:  *to++ = *from++; \
	  case 32:  *to++ = *from++;	  case 31:  *to++ = *from++; \
	  case 30:  *to++ = *from++;	  case 29:  *to++ = *from++; \
	  case 28:  *to++ = *from++;	  case 27:  *to++ = *from++; \
	  case 26:  *to++ = *from++;	  case 25:  *to++ = *from++; \
	  case 24:  *to++ = *from++;	  case 23:  *to++ = *from++; \
	  case 22:  *to++ = *from++;	  case 21:  *to++ = *from++; \
	  case 20:  *to++ = *from++;	  case 19:  *to++ = *from++; \
	  case 18:  *to++ = *from++;	  case 17:  *to++ = *from++; \
	  case 16:  *to++ = *from++;	  case 15:  *to++ = *from++; \
	  case 14:  *to++ = *from++;	  case 13:  *to++ = *from++; \
	  case 12:  *to++ = *from++;	  case 11:  *to++ = *from++; \
	  case 10:  *to++ = *from++;	  case 9:   *to++ = *from++; \
	  case 8:   *to++ = *from++;	  case 7:   *to++ = *from++; \
	  case 6:   *to++ = *from++;	  case 5:   *to++ = *from++; \
	  case 4:   *to++ = *from++;	  case 3:   *to++ = *from++; \
	  case 2:   *to++ = *from++;	  case 1:   *to++ = *from++; \
	} while ((count -= 64) > 0); \
    } \
}
