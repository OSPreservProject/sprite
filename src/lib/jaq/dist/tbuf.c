/* 
 * tbuf.c--
 *
 *	Routines to handle tape buffer stuff
 *
 * Copyright 1992 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 */

#ifndef lint
static char rcsid[] = "$Header: /sprite/lib/forms/RCS/tbuf.c,v 1.0 91/01/07 18:02:37 mottsmth Exp $ SPRITE (Berkeley)";
#endif /* not lint */

#include "jaquith.h"

#define MINFREEVOLS 2
#define BUFSIZE 4096

extern int jDebug;            /* Internal debugging only */
extern int syserr;            /* Our personal record of errno */

/*
 * Layout for building tar headers
 */
#define TBLOCK 512
#define NAMSIZ 100
#define	TUNMLEN	32
#define	TGNMLEN	32
#define	PRFXLEN	155
#define	TMAGIC		"ustar  "  /* 7 chars and a null */
#define	LF_OLDNORMAL	'\0'	/* Normal disk file, Unix compat */
#define	LF_NORMAL	'0'	/* Normal disk file */
#define	LF_LINK		'1'	/* Link to previously dumped file */
#define	LF_SYMLINK	'2'	/* Symbolic link */
#define	LF_CHR		'3'	/* Character special file */
#define	LF_BLK		'4'	/* Block special file */
#define	LF_DIR		'5'	/* Directory */
#define	LF_FIFO		'6'	/* FIFO special file */
#define	LF_CONTIG	'7'	/* Contiguous file */
#define	LF_RMTLINK	'R'	/* Sprite Remote link */

typedef struct THdr {
    char name[NAMSIZ];        /* file name */
    char mode[8];             /* protection type */
    char uid[8];              /* user id # */
    char gid[8];              /* group id # */
    char size[12];            /* size in bytes */
    char mtime[12];           /* last modified time */
    char chksum[8];           /* checksum with this field == spaces */
    char linkflag;            /* file type */
    char linkname[NAMSIZ];    /* target file name if this is a sym link */
    char magic[6];            /* TMAGIC */
    char version[2];          /* "00" */
    char uname[TUNMLEN];      /* user name */
    char gname[TGNMLEN];      /* group name */
    char devmajor[8];         /* major device byte if this is a device */
    char devminor[8];         /* minor device byte */
    char prefix[PRFXLEN];     /* prefix to file name */
} THdr;


/*
 *----------------------------------------------------------------------
 *
 * TBuf_Pad --
 *
 *	Pad the buffer to the specified unit.
 *
 * Results:
 *	Returns # of bytes written.
 *
 * Side effects:
 *	None.
 *
 * Note:
 *      This is written so it can pad out to a block larger than
 *      T_TBLOCK, even though this isn't needed presently.
 *
 *----------------------------------------------------------------------
 */

int
TBuf_Pad(tBufStream, len, blockSize)
    int tBufStream;           /* output file descriptor */
    int len;                  /* current size of buffer */
    int blockSize;            /* desired blocksize */
{
    char nullBuf[T_TBLOCK];
    char null = '\0';
    int left;
    int total;

    total = left = blockSize - (len % blockSize);
    if (total == blockSize) {
	return 0;
    }
    strncpy(nullBuf, &null, sizeof(nullBuf));

    while (left > 0) {
	len = (left > sizeof(nullBuf)) ? sizeof(nullBuf) : left;
	if ((len=write(tBufStream, nullBuf, len)) < 1) {
	    syserr = errno;
	    return T_IOFAILED;
	}
        left -= len;
    }

    return total;

}


/*
 *----------------------------------------------------------------------
 *
 * TBuf_Open --
 *
 *	Open Tape buffer.
 *
 * Results:
 *	file descriptor of open buffer file.
 *
 * Side effects:
 *	None.
 *
 * Note:
 *      A return code of T_SUCCESS just means the attempt was ok.
 *      Still need to check stream ptr to see if file was there.
 *
 *----------------------------------------------------------------------
 */

int
TBuf_Open(path, tBufId, tBufStreamPtr, tHdrStreamPtr, mode) 
    char *path;               /* path to tape buffer file */
    int tBufId;               /* tBuf number (name) of file */
    int *tBufStreamPtr;       /* receiving descriptor for tbuf */
    int *tHdrStreamPtr;       /* receiving descriptor for hdr */
    int mode;                 /* open mode: O_RDONLY, O_WRONLY, O_RDWR */
{
    char fullPath[T_MAXPATHLEN];
    int pathLen;

    strcpy(fullPath, path);
    strcat(fullPath, "/");
    pathLen = strlen(fullPath);
    sprintf(fullPath+pathLen, "tbuf.%d%c", tBufId, '\0');

    if ((*tBufStreamPtr=open(fullPath, mode, 0)) < 0) {
	/*
	 * File doesn't exist; we have to make it
	 */
	if (errno == ENOENT) {
	    if (mode & (O_RDWR | O_WRONLY)) {
		if ((*tBufStreamPtr=open(fullPath, mode+O_CREAT, 0600)) < 0) {
		    syserr = errno;
		    fprintf(stderr, "TBuf_Open: couldn't create %s",
			    fullPath);
		    return T_BUFFAILED;
		}
	    } else {
		*tBufStreamPtr = -1;
		return T_SUCCESS;
	    }
	} else {
	    syserr = errno;
	    fprintf(stderr, "Tbuf_Open: failed to open %s: %d",
		    fullPath, syserr);
	    return T_BUFFAILED;
	}
    }

    if (tHdrStreamPtr == (int *)NULL) {
	return T_SUCCESS;
    }

    sprintf(fullPath+pathLen, "thdr.%d%c", tBufId, '\0');
    if ((*tHdrStreamPtr=open(fullPath, mode, 0)) < 0) {
	/*
	 * File doesn't exist; we have to make it
	 */
	if (errno == ENOENT) {
	    if ((*tHdrStreamPtr=open(fullPath, mode+O_CREAT, 0600)) < 0) {
	    syserr = errno;
		fprintf(stderr, "TBuf_Open: couldn't create %s",
			fullPath);
		return T_BUFFAILED;		
	    }
	} else {
	    syserr = errno;
	    fprintf(stderr, "Tbuf_Open: failed to open %s: %d",
		    fullPath, syserr);
	    return T_BUFFAILED;
	}
    }

    return T_SUCCESS;
}


/*
 *----------------------------------------------------------------------
 *
 * TBuf_Seek --
 *
 *	Seek to end minus 2 dummy blocks that tar requires.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
TBuf_Seek(tBufStream, len)
    int tBufStream;           /* output file descriptor */
    int len;                  /* length of file */
{
    int retCode;

    retCode = (int)lseek(tBufStream, (off_t)len, L_SET);
    syserr = errno;
    return (retCode == -1) ? T_INDXFAILED : T_SUCCESS;
}


/*
 *----------------------------------------------------------------------
 *
 * TBuf_Close --
 *
 *	Close Tape buffer.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
TBuf_Close(tBufStream, tHdrStream, tBufSize)
    int tBufStream;           /* output file descriptor */
    int tHdrStream;           /* output header descriptor */
    int tBufSize;             /* for appending null tar records */
{
    int pad = 0;

    if (tBufSize >= 0) {
	pad = TBuf_Terminate(tBufStream, tBufSize);
	fsync(tBufStream);
    }
    close(tBufStream);

    if (tHdrStream != -1) {
	if (tBufSize >= 0) {
	    fsync(tHdrStream);
	}
	close(tHdrStream);
    }

    return pad;

}


/*
 *----------------------------------------------------------------------
 *
 * TBuf_Terminate --
 *
 *	Append final blocks onto end of file to satisfy tar.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
TBuf_Terminate(tBufStream, curSize)
    int tBufStream;           /* output file descriptor */
    int curSize;              /* current size in bytes */
{
    char null = '\0';
    char nullBuf[2*T_TBLOCK]; /* tar requires exactly 2 null blocks */

    strncpy(nullBuf, &null, sizeof(nullBuf));

    if (write(tBufStream, nullBuf, sizeof(nullBuf)) < 1) {
        syserr = errno;
        return -1;
    }

    return sizeof(nullBuf);
}



/*
 *----------------------------------------------------------------------
 *
 * TBuf_WriteTarHdr --
 *
 *	Build a tbuf header in tar format.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
TBuf_WriteTarHdr(destStream, hdrPtr)
    int destStream;           /* destination */
    T_FileStat *hdrPtr;       /* source info structure */
{
    char null = '\0';
    unsigned char *outPtr;
    unsigned char outBuf[T_TBLOCK];
    THdr *fmtBuf = (THdr *)outBuf;
    char *slashPtr;
    int fileNameLen;
    int sum;
    int i;
    int devmajor;
    int devminor;
    char linkFlag;

    strncpy(outBuf, &null, T_TBLOCK);

    /*
     * if the name doesn't fit in the regular NAMSIZ space
     * then we have to go to extended format. This means putting
     * the simple filename in the NAMSIZ slot and the preceding part
     * of the pathname in the prefix slot.  Sure hope you have a POSIX
     * compliant tar program that understands this format.
     */
    fileNameLen = strlen(hdrPtr->fileName);
    if (fileNameLen > NAMSIZ) {
	slashPtr = hdrPtr->fileName+NAMSIZ-1;
	while ((*slashPtr != '/') && (slashPtr > hdrPtr->fileName)) {
	    slashPtr--;
	}
	if (slashPtr == hdrPtr->fileName) {
	    return 0;
	}
	strncpy(fmtBuf->name, slashPtr+1, NAMSIZ);
	strncpy(fmtBuf->prefix, hdrPtr->fileName, slashPtr-hdrPtr->fileName);
    } else {
	strncpy(fmtBuf->name, hdrPtr->fileName, NAMSIZ);
    }

    switch (hdrPtr->mode & S_IFMT) {
    case S_IFREG:
	if (*hdrPtr->linkName) {
	    linkFlag = LF_LINK;
	} else {
	    linkFlag = LF_NORMAL;
	}
	break;
    case S_IFDIR:
	linkFlag = LF_DIR;
	break;
    case S_IFCHR:
	linkFlag = LF_CHR;
	devmajor = (hdrPtr->rdev>>8) & 0xFF;
	devminor = (hdrPtr->rdev) & 0xFF;
	sprintf(fmtBuf->devmajor, "%6o %c%6o %c",
		devmajor, null,	devminor, null);
	break;
    case S_IFBLK:
	linkFlag = LF_BLK;
	devmajor = (hdrPtr->rdev>>8) & 0xFF;
	devminor = (hdrPtr->rdev) & 0xFF;
	sprintf(fmtBuf->devmajor, "%6o %c%6o %c",
		devmajor, null,	devminor, null);
	break;
    case S_IFLNK:
	linkFlag = LF_SYMLINK;
	break;
    case S_IFIFO:
	linkFlag = LF_FIFO;
	break;
    case S_IFRLNK:
	linkFlag = LF_RMTLINK;
	break;
    default:
	linkFlag = LF_NORMAL;
	fprintf(stderr,"TBuf_WriteTarHdr: %s: unexpected file type 0%o\n", 
		hdrPtr->fileName, hdrPtr->mode & S_IFMT);
	break;
    }

    sprintf(outBuf+NAMSIZ, "%6o %c%6o %c%6o %c%11o %11o %8s%c%s",
	    hdrPtr->mode & (~S_IFMT), null,
	    hdrPtr->uid, null,
	    hdrPtr->gid, null,
	    hdrPtr->size,
	    hdrPtr->mtime,
	    " ",
	    linkFlag,
	    "        ");

    strncpy(fmtBuf->linkname, hdrPtr->linkName, NAMSIZ);
    strncpy(fmtBuf->magic, TMAGIC, sizeof(TMAGIC));
    strncpy(fmtBuf->uname, hdrPtr->uname, TUNMLEN);
    strncpy(fmtBuf->gname, hdrPtr->gname, TGNMLEN);

    for (sum=0,i=0,outPtr=outBuf; i<T_TBLOCK; i++) {
	sum += (int)*outPtr++;
    }
    sprintf(fmtBuf->chksum, "%6o", sum);

    write(destStream, outBuf, T_TBLOCK);

    return T_TBLOCK;
}


/*
 *----------------------------------------------------------------------
 *
 * TBuf_ParseTarHdr --
 *
 *	Read a formatted header into pieces.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
TBuf_ParseTarHdr(inBuf, statInfoPtr)
    char *inBuf;              /* source buffer of formatted hdr */
    T_FileStat *statInfoPtr;       /* destination info structure */
{
    THdr *fmtBuf = (THdr *)inBuf;
    int sum;
    int chksum;
    int i;
    int devmajor;
    int devminor;
    char tmpPrefix[PRFXLEN+1];
    char saveChar;

    sscanf(fmtBuf->chksum, "%o", &chksum);
    strncpy(fmtBuf->chksum, "        ", sizeof(fmtBuf->chksum));
    for (sum=0,i=0; i<sizeof(THdr); i++) {
	sum += (int)*inBuf++;
    }
    if (sum != chksum) {
	fprintf(stderr,"Checksum failed for file %s: read %d, actual %d\n",
		fmtBuf->name, chksum, sum);
	return T_BUFFAILED;
    }

    /*
     * The name and prefix fields must be combined.
     * Furthermore, there's no guarantee that they're null-terminated,
     * but uname and gname are.
     */
    if (*fmtBuf->prefix) {
	strncpy(tmpPrefix, fmtBuf->prefix, PRFXLEN);
	saveChar = *(fmtBuf->name+NAMSIZ);
	*(fmtBuf->name+NAMSIZ) = '\0';
	statInfoPtr->fileName = Str_Cat(3, tmpPrefix, "/", fmtBuf->name);
	*(fmtBuf->name+NAMSIZ) = saveChar;    
    } else {
	saveChar = *(fmtBuf->name+NAMSIZ);
	*(fmtBuf->name+NAMSIZ) = '\0';
	statInfoPtr->fileName = Str_Dup(fmtBuf->name);
	*(fmtBuf->name+NAMSIZ) = saveChar;    
    }
    sscanf(fmtBuf->mode, "%o", &statInfoPtr->mode);
    sscanf(fmtBuf->uid, "%o", &statInfoPtr->uid);
    sscanf(fmtBuf->gid, "%o", &statInfoPtr->gid);
    sscanf(fmtBuf->size, "%o", &statInfoPtr->size);
    sscanf(fmtBuf->mtime, "%o", &statInfoPtr->mtime);
    statInfoPtr->rdev = 0;

    switch (fmtBuf->linkflag) {
    case LF_NORMAL:
	statInfoPtr->mode |= S_IFREG;
	break;
    case LF_DIR:
	statInfoPtr->mode |= S_IFDIR;
	break;
    case LF_CHR:
	statInfoPtr->mode |= S_IFCHR;
	sscanf(fmtBuf->devmajor, "%o", &devmajor);
	sscanf(fmtBuf->devminor, "%o", &devminor);
	statInfoPtr->rdev = (devmajor<<8) | devminor;
	break;
    case LF_BLK:
	statInfoPtr->mode |= S_IFBLK;
	sscanf(fmtBuf->devmajor, "%o", &devmajor);
	sscanf(fmtBuf->devminor, "%o", &devminor);
	statInfoPtr->rdev = (devmajor<<8) | devminor;
	break;
    case LF_SYMLINK:
	statInfoPtr->mode |= S_IFLNK;
	break;
    case LF_FIFO:
	statInfoPtr->mode |= S_IFIFO;
	break;
    case LF_RMTLINK:
	statInfoPtr->mode |= S_IFRLNK;
	break;
    default:
	fprintf(stderr,"ParseTarHdr: unknown file type: '%c'\n", fmtBuf->linkflag);
	statInfoPtr->mode |= S_IFREG;
	break;
    }
    saveChar = *(fmtBuf->linkname+NAMSIZ);
    *(fmtBuf->linkname+NAMSIZ) = '\0';
    statInfoPtr->linkName = Str_Dup(fmtBuf->linkname);
    *(fmtBuf->linkname+NAMSIZ) = saveChar;
    statInfoPtr->uname = Str_Dup(fmtBuf->uname);
    statInfoPtr->gname = Str_Dup(fmtBuf->gname);

    return T_SUCCESS;
}


/*
 *----------------------------------------------------------------------
 *
 * TBuf_FindFile --
 *
 *	Locate a file by scanning through tbuf
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Leaves the file pointer positioned to read the data.
 *
 *----------------------------------------------------------------------
 */

int
TBuf_FindFile(tBufStream, statInfoPtr)
    int tBufStream;           /* open tbuf descriptor */
    T_FileStat *statInfoPtr;  /* file to be located */
{
    char *fileName = statInfoPtr->fileName;
    int offset = statInfoPtr->offset;
    char buf[T_TBLOCK];
    T_FileStat tmpStatInfo;
    int retCode = T_SUCCESS;

    if (jDebug) {
	fprintf(stderr,"FindFile: %d %s\n", offset, fileName);
    }

    /* seek to beginning of file in buffer */
    if (TBuf_Seek(tBufStream, offset) != T_SUCCESS) {
	fprintf(stderr,"FindFile: seek %d errno %d\n", offset, errno);
	return T_BUFFAILED;
    }

    /* get a tar hdr */
    if (read(tBufStream, buf, sizeof(buf)) != sizeof(buf)) {
	fprintf(stderr,"FindFile: read errno %d\n", errno);
	return T_BUFFAILED;
    }

    /* now see if we got a tar hdr */
    if ((retCode=TBuf_ParseTarHdr(buf, &tmpStatInfo)) != T_SUCCESS) {
	if (jDebug) {
	    fprintf(stderr,"FindFile: bad tar hdr\n");
	}
	return retCode;
    }

    /* sanity check: filename better match */
    if (strcmp(fileName, tmpStatInfo.fileName) != 0) {
	if (jDebug) {
	    fprintf(stderr,"FindFile: %s != %s\n",
		    fileName, tmpStatInfo.fileName);
	}
	return T_BUFFAILED;
    }

    /* we don't have these in the index so we need to get them here */
    statInfoPtr->gid = tmpStatInfo.gid;
    statInfoPtr->uid = tmpStatInfo.uid;

    return T_SUCCESS;
}



/*
 *----------------------------------------------------------------------
 *
 * TBuf_Delete --
 *
 *	Delete a sequence of tape buffers
 *
 * Results:
 *	return code
 *
 * Side effects:
 *	buffer is destroyed.
 *
 * Note:
 *      This is a hack. No guarantee that space is actually
 *      freed after we remove the tbuf; someone might be using it.
 *
 *----------------------------------------------------------------------
 */

int
TBuf_Delete(rootPath, arch, firstTBuf, lastTBuf)
    char *rootPath;           /* path to this archive */
    char *arch;               /* archive name */
    int firstTBuf;            /* low number tbuf */
    int lastTBuf;             /* high number tbuf */
{
    char fullPath[T_MAXPATHLEN];
    int tBufId;

    if (firstTBuf == -1) {
	if (Admin_RemoveLRU(rootPath, arch, &firstTBuf) != T_SUCCESS) {
	    return T_FAILURE;
	}
	lastTBuf = firstTBuf;
    }

    for (tBufId=firstTBuf; tBufId<=lastTBuf; tBufId++) {
        sprintf(fullPath, "%s/%s/tbuf.%d", rootPath, arch, tBufId);
        unlink(fullPath);
        sprintf(fullPath, "%s/%s/thdr.%d", rootPath, arch, tBufId);
        unlink(fullPath);
        sprintf(fullPath, "%s/%s/meta.%d", rootPath, arch, tBufId);
        unlink(fullPath);
    }

    return T_SUCCESS;
}
