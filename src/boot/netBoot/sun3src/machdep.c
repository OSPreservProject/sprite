
/*
 * @(#)machdep.c 1.1 86/09/27
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

#include "../h/systypes.h"
#include "../sun3/machdep.h"

/*
 *	these are the routines in machdep
 *	addr is usually a u_long
 *	entry is a ??_size
 *	routines for mapping and registers return ??_size
 *	most of the set* routines return what was there before
 *	%'ed routines return nothing
		cx_size		getcxreg()
		cx_size		setcxreg(entry)
		sm_size		getsmreg(addr)
		sm_size		setsmreg(addr,entry)
		pg_size		getpgreg(addr)
		pg_size		setpgreg(addr,entry)
		berr_size	getberrreg()
		int  %  	map(virt, size, phys, space)
 *
 */
/*
 * get/set fc3 are to eliminate the assembly all over the place,
 * and to let users do their own self abuse
 */
u_long
getfc3(size,addr)
register u_long			size, *addr;
{
	MOVL(#3, d0);
	MOVC(d0, sfc);
	if (size == sizeof(u_char))
		MOVSB(a5@, d7);
	else if (size == sizeof(u_short))
		MOVSW(a5@, d7);
	else if (size == sizeof(u_long))
		MOVSL(a5@, d7);
	return(size);
}

u_long
setfc3(size,addr,entry)
register u_long			size, *addr,entry;
{
	MOVL(#3, d0);
	MOVC(d0, dfc);
	if (size == sizeof(u_char))
		MOVSB(d6, a5@);
	else if (size == sizeof(u_short))
		MOVSW(d6, a5@);
	else if (size == sizeof(u_long))
		MOVSL(d6, a5@);
}
cx_size
getcxreg()
{
	return((cx_size) getfc3(sizeof(cx_size), CX_OFF));
}
cx_size
setcxreg(entry)
register cx_size		entry;
{
	register cx_size	ret = getcxreg();

	setfc3(sizeof(cx_size), CX_OFF, entry);
	return(ret);
}

sm_size
getsmreg(addr)
register u_long			addr;
{
	addr = ((addr & ~SEGMASK) + SM_OFF) & ADDRMASK;
	return((sm_size) getfc3(sizeof(sm_size), addr));
}

sm_size
setsmreg(addr,entry)
register u_long			addr;
register sm_size		entry;
{
	register sm_size	ret = getsmreg(addr);

	addr = ((addr & ~SEGMASK) + SM_OFF) & ADDRMASK;
	setfc3(sizeof(sm_size), addr, entry);
	return(ret);
}

pg_size
getpgreg(addr)
register u_long			addr;
{
	addr = ((addr & ~PAGEMASK) + PG_OFF) & ADDRMASK;
	return((pg_size) getfc3(sizeof(pg_size), addr));
}

pg_size
setpgreg(addr,entry)
register u_long			addr;
register pg_size		entry;
{
	register pg_size	ret = getpgreg(addr);

	addr = ((addr & ~PAGEMASK) + PG_OFF) & ADDRMASK;
	setfc3(sizeof(pg_size), addr, entry);
	return(ret);
}

map(virt, size, phys, space)
register u_long				virt, size, phys;
register enum pm_type			space;
{
	pg_t				page;
	register struct pg_field	*pgp = &page.pg_field;
	register			i;

	pgp->pg_valid = 1;
	pgp->pg_permission = PMP_ALL;
	pgp->pg_space = space;

	phys = BTOP(phys);
	size = BTOP(size);

	for (i = 0; i < size; i++){		/* for each page, */
		pgp->pg_pagenum = phys++;
		setpgreg(virt + PTOB(i), page.pg_whole);
	}
}


berr_size
getberrreg()
{
	return((berr_size) getfc3(sizeof(berr_size), BERR_OFF));
}



