/* 
 * shm.c --
 *
 *	These routines map system V shared memory calls into 4.3 BSD
 *	calls.
 *
 * Copyright 1990 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 */

/*
From dillon@postgres.Berkeley.EDU Fri Jun 23 17:17:30 1989
	Here is the support file I wrote for the postgres that implements
most of the shared memory calls through mmap().  The line used to compile
the module is:

	cc -I/usr/include -I/usr/att/usr/include -c port.c

	The module is not 100% transparent but works well enough that most
programs which use the shared memory calls don't know the difference.

	From working with both the shared memory calls and the mmap() calls
It is clear that the mmap() calls are not only superior, but integrate well
into the system whereas the shared memory calls are really nothing more than
a huge hack.  For example, on SUNs one must compile in the exact amount of
memory to reserved for shared memory and this memory cannot be used by the
VM.  What a waste!  The sequent's Dynix OS, on the otherhand, and the mmap()
call in general has no such restrictions.

	I'm going blind into this.  Hopefully you have access to both the
mmap includes and the AT&T shared memory includes.

				Luck,


					-Matt
*/

#ifndef lint
static char rcsid[] = "$Header: /sprite/src/lib/c/etc/RCS/shm.c,v 1.5 90/06/27 11:17:58 shirriff Exp $ SPRITE (Berkeley)";
#endif /* not lint */
#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ipc.h>
#include <sys/shm.h>	/* att-include file	*/
#include <sys/mman.h>
#include <assert.h>

#ifndef __STDC__
#define void char
#endif

#define LEN 64

extern int errno;

typedef struct {
    void  *base;
    long  size;
    short exists;
    int key;
    char name[LEN];
} SHMIDS;

int _shmTrace = 0;

/*
 * In sprite there is no limit on the number of file descriptors.
 * Set this to 64 because that is how many ultrix allows.
 */

#ifndef _NFILE
#define _NFILE 64
#endif

static SHMIDS ShmIds[_NFILE];

/*
 *  shmget(key, size, shmflg)
 *
 *  key might be IPC_PRIVATE in which case a 'private' segment is created
 *  shmflg = file permissions
 *  size = size of shared memory segment
 */

shmget(key, size, shmflg)
key_t key;
int size, shmflg;
{
    int fd;
    char buf[LEN];
    int fileflags = O_RDWR;
    extern int errno;
    int i;

    if (_shmTrace) {
	printf("call: shmget(%d, %d, %d)\n", key, size, shmflg);
    }
    if ((shmflg & IPC_CREAT) || (shmflg & IPC_PRIVATE))
	fileflags |= O_CREAT;
    if (shmflg & IPC_NOWAIT)
	fileflags |= O_NDELAY;
    if (shmflg & IPC_EXCL)
	fileflags |= O_EXCL;
    if (shmflg & IPC_ALLOC) {
	fprintf(stderr, "shmget: IPC_ALLOC flag not implemented\n");
	exit(1);
    }

    if (key == IPC_PRIVATE)
	sprintf(buf, "/tmp/post.phm.%d", getpid());
    else
	sprintf(buf, "/tmp/post.shm.%d", key);
    fd = open(buf, fileflags, (shmflg & 0777) | 0600);
    if (fd < 0) {
	return -1;
    }
    if ((fileflags & O_CREAT) || key == IPC_PRIVATE) {
	lseek(fd,(long)size-1,0);
	read(fd,buf,1);
	lseek(fd,(long)size-1,0);
	write(fd,buf,1);
    } else {
	size = lseek(fd,0L,2);
	lseek(fd,0L,0);	/* not really needed, in for conformity */
	if (_shmTrace) {
	    printf("Segment already has length %d\n", size);
	}
    }
    if (key == IPC_PRIVATE) {
	unlink(buf);
    } else {
	/*
	 * See if we already have an entry in the table.
	 */
	for (i=0; i< _NFILE; i++) {
	    if (key == ShmIds[i].key && ShmIds[i].exists) {
		close(fd);
		if (_shmTrace) {
		    printf("Segment already exists as %d\n", i);
		}
		return i;
	    }
	}
    }
    ShmIds[fd].size = size;
    ShmIds[fd].base = (void *)0L;
    ShmIds[fd].exists = 1;
    ShmIds[fd].key = key;
    strncpy(ShmIds[fd].name, buf, LEN);
    if (_shmTrace) {
	printf("shmget returning %d\n", fd);
    }
    return(fd);
}

shmop()
{
    assert(0);
}

shmctl(shmid, cmd, buf)
int shmid, cmd;
struct shmid_ds *buf;	/* sys/shm.h att-includes */
{
    struct stat stat;
    int result = 0;

    if (_shmTrace) {
	printf("call: shmctl(%d, %d, %d)\n", shmid, cmd, buf);
    }
    assert(shmid >= 0 && shmid < _NFILE);
    assert(ShmIds[shmid].exists);

    fstat(shmid, &stat);

    switch(cmd) {
    case IPC_STAT:
	buf->shm_perm.mode= stat.st_mode;
	buf->shm_perm.uid = stat.st_uid;
	buf->shm_perm.gid= stat.st_gid;
	buf->shm_perm.cuid = stat.st_uid;
	buf->shm_perm.cgid= stat.st_gid;
	buf->shm_perm.key = shmid;
	buf->shm_atime = stat.st_atime;
	buf->shm_ctime = stat.st_ctime;
	/* fill in more??? */
	break;
    case IPC_SET:	/* set only uid, guid, and low 9 bits of mode   */
	fchmod(shmid, (stat.st_mode & ~0777) | (buf->shm_perm.mode & 0777));
	fchown(shmid, buf->shm_perm.uid, buf->shm_perm.gid);
	break;
    case IPC_RMID:   /* delete the shared memory identifier		*/
	if (ShmIds[shmid].key != IPC_PRIVATE) {
	    unlink(ShmIds[shmid].name);
	}
	close(shmid);
	ShmIds[shmid].exists = 0;
	ShmIds[shmid].key = -1;
	break;
    }
    return(result);
}

char *
shmat(shmid, shmaddr, shmflg)
int shmid;
int shmaddr;
int shmflg;
{
    void *base;
    long size;
    long pgmask = getpagesize() - 1;

    if (_shmTrace) {
	printf("call: shmat(%d, %d, %d)\n", shmid, shmaddr, shmflg);
    }
    assert(!shmaddr);
    assert(!shmflg);

    assert(shmid >= 0 && shmid < _NFILE);
    assert(ShmIds[shmid].exists);

    if (ShmIds[shmid].base)		/* already mapped! */
	return((char *)ShmIds[shmid].base);
    base = (void *)(((int)sbrk(0) + pgmask) & ~pgmask);
    size = (ShmIds[shmid].size + pgmask) & ~pgmask;
    if (_shmTrace) {
	printf("Mapping: length = %d\n", size);
    }
    if (_shmTrace) {
	printf("shmat: calling mmap(%x, %d, %d, %d, %d, %d)\n", base, size,
		PROT_READ|PROT_WRITE, MAP_SHARED, shmid, 0);
    }
    base = (void *)mmap(base, size, PROT_READ|PROT_WRITE, MAP_SHARED,
	    shmid, 0);
    if (base < 0) {
	if (_shmTrace) {
	    printf("mmap failed: base = %d, errno = %d\n", base, errno);
	}
	return((char *)-1);
    }
    ShmIds[shmid].base = base;
    if (_shmTrace) {
	printf("shmat returning 0x%x\n", base);
    }
    return((char *)base);
}

int
shmdt(shmaddr)
void *shmaddr;
{
    short i;

    if (_shmTrace) {
	printf("call: shmdt(%d)\n", shmaddr);
    }
    assert(shmaddr);

    for (i = 0; i < _NFILE; ++i) {
	if (ShmIds[i].exists && ShmIds[i].base == shmaddr) {
	    if (munmap(ShmIds[i].base, ShmIds[i].size) < 0) {
		fprintf(stderr, "munmap failed!\n");
		exit(1);
	    }
	    ShmIds[i].base = 0;
	    break;
	}
    }
    return(0);
}



