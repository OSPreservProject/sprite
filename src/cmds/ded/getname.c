/* More or less lifted from ps.c per dlw@Berkeley.  This should be in libc! */

#ifndef lint
static char RCSid[] =
"$Header: getname.c,v 1.4 84/04/25 03:55:55 lepreau Exp $";
#endif

#include <pwd.h>
#include <grp.h>
#include <utmp.h>

struct	utmp utmp;

#ifndef NUID
# define NUID	1024	/* must not be a multiple of 5 */
#endif NUID

#ifndef NGID
# define NGID	301	/*   ''   */
#endif

#define	NMAX	(sizeof (utmp.ut_name))
#define NULL 0

extern char *strncpy();

struct nametable {
	char	nt_name[NMAX];
	int	nt_id;
};
struct nametable	unames[NUID];
/*
 * Doing it this way saves 4 byes per entry in the nametable, but loses
 * in that the return value gets clobbered next time.  Latter doesn't matter
 * to ded.
 */
char nplus1[NMAX+1];

struct nametable *
findslot (id, tbl, len)
	unsigned short	id;
	struct nametable *tbl;
	int		len;
{
	register struct nametable	*nt, *nt_start;

	/*
	 * find the id or an empty slot.
	 * return NULL if neither found.
	 */

	nt = nt_start = tbl + (id % (len - 20));
	while (nt->nt_name[0] && nt->nt_id != id)
	{
		if ((nt += 5) >= &tbl[len])
			nt -= len;
		if (nt == nt_start)
			return((struct nametable *)NULL);
	}
	return(nt);
}

/*
 * find uid in hashed table; add it if not found.
 * return pointer to name.
 */
char *
getname(uid)
	unsigned short	uid;
{
	register struct passwd		*pw;
	static int			init = 0;
	struct passwd			*getpwent();
	register struct nametable	*n;
	static lastuid = -1;

	if (uid == lastuid)
		return (nplus1);
	
	if ((n = findslot(uid, unames, NUID)) == NULL)
		return((char *)NULL);

	if (n->nt_name[0]) {	/* occupied? */
		(void) strncpy (nplus1, n->nt_name, NMAX);
		lastuid = uid;
		return (nplus1);
	}
	
	switch (init)
	{
		case 0:
			(void) setpwent();
			init = 1;
			/* intentional fall-thru */
		case 1:
			while (pw = getpwent())
			{
				if (pw->pw_uid < 0)
					continue;
				n = findslot((unsigned short)pw->pw_uid,
				    unames, NUID);
				if (n == NULL)
				{
					(void) endpwent();
					init = 2;
					return((char *)NULL);
				}
				if (n->nt_name[0])
					continue;   /* duplicate, not uid */
				(void) strncpy(n->nt_name, pw->pw_name, NMAX);
				n->nt_id = pw->pw_uid;
				if (pw->pw_uid == uid) {
					(void) strncpy (nplus1, n->nt_name, NMAX);
					lastuid = uid;
					return (nplus1);
				}
			}
			(void) endpwent();
			init = 2;
			/* intentional fall-thru */
		case 2:
			return ((char *)NULL);
	}
	/*NOTREACHED*/
}

#ifdef notyet
struct nametable	gnames[NGID];

/*
 * find gid in hashed table; add it if not found.
 * return pointer to name.
 */
char *
getgroup (gid)
	unsigned short	gid;
{
	register struct group	*gr;
	static int	init = 0;
	struct group	*getgrent();
	register struct nametable	*n;

	if ((n = findslot(gid, gnames, NGID)) == NULL)
		return((char *)NULL);

	if (n->nt_name[0])	/* occupied? */
		return(n->nt_name);

	switch (init)
	{
		case 0:
			(void) setgrent();
			init = 1;
			/* intentional fall-thru */
		case 1:
			while (gr = getgrent())
			{
				if (gr->gr_gid < 0)
					continue;
				n = findslot((unsigned short) gr->gr_gid,
				    gnames, NGID);
				if (n == NULL)
				{
					(void) endgrent();
					init = 2;
					return((char *)NULL);
				}
				if (n->nt_name[0])
					continue;	/* duplicate, not gid */
				(void) strncpy(n->nt_name, gr->gr_name, NMAX);
				n->nt_id = gr->gr_gid;
				if (gr->gr_gid == gid)
					return (n->nt_name);
			}
			(void) endgrent();
			init = 2;
			/* intentional fall-thru */
		case 2:
			return ((char *)NULL);
	}
	/*NOTREACHED*/
}
#endif notyet
