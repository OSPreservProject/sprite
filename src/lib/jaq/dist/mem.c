/* 
 * mem.c--
 *
 *	Memory management for Jaquith archive package.
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
static char rcsid[] = "$Header: /sprite/lib/forms/RCS/mem.c,v 1.0 91/01/07 18:02:37 mottsmth Exp $ SPRITE (Berkeley)";
#endif /* not lint */

#include <stdio.h>
#include "jaquith.h"

typedef struct nodedef {
    char *name;
    int line;
    int size;
    int freeLink;
    char *ptr;
    unsigned char flags;
} NODE;

typedef struct tabdef {
    int inuse;
    int cnt;
    NODE *tab;
} TAB;

#define CHUNK 1000
#define BORDER "################"
#define BORDERSIZE 16
#define MAXNAMELEN 20
#define PTRFIELD 1
#define NAMEFIELD 2
#define SIZEFIELD 3
#define FREE 1
#define RETURNED 2
#define BITPATTERN '@'
#define NIL -1

#define REAL_ALLOC(x) malloc(x)
#define REAL_FREE(x) free(x)

static int maxBlkSize = 0x10000;
static int maxFreeSpace = 0;
static int traceFlags = 0;
static FILE *outStream = stderr;
static int freeHead = NIL;
static int freeTail = NIL;

int traceInit = 0;

static TAB blks = { 0, 0, (NODE *)NULL};
static int outCnt = 0;
static int outSize = 0;
static int totAlloc = 0;
static int totFree = 0;
static int curFree = 0;

static void CheckBlk   _ARGS_ ((NODE *nodePtr, int count));
static void PrintNodes _ARGS_ ((char *ownerName, NODE **nodeList, int count));
static void SortNodes  _ARGS_ ((NODE **nodeList, int count, int field));
static void Die        _ARGS_ (());


/*
 *----------------------------------------------------------------------
 *
 * Mem_Alloc--
 *
 *	Safe memory allocator. This gives us a place to localize
 * and trace memory use, if it should become necessary.
 *
 * Results:
 *	Ptr to block of specified size or NULL;
 *
 * Side effects:
 *	Allocates heap space.
 *
 *----------------------------------------------------------------------
 */

char *
Mem_Alloc(callerName, callerLine, callerSize)
    char *callerName;         /* calling procedure's name */
    int callerLine;           /* line no in caller's source */
    int callerSize;           /* requested block size */
{
    int i;
    NODE *newTab;
    NODE *nodePtr;
    char *blkPtr;
    int blkSize;

    blkSize = callerSize + 2*BORDERSIZE;

    if (!(traceFlags & TRACEMEM)) {
	blkPtr = (char *)REAL_ALLOC(blkSize);
	strncpy(blkPtr, BORDER, BORDERSIZE);
	blkPtr += BORDERSIZE;
	strncpy(blkPtr+callerSize, BORDER, BORDERSIZE);
	return(blkPtr);
    }

    if (callerSize == 0) {
	fprintf(outStream,"*** Mem_Alloc (%s:%d) size is zero.\n",
		callerName, callerLine);
    } else if (callerSize < 0) {
	fprintf(outStream,"*** Mem_Alloc (%s:%d) size is <= 0: %08x\n",
		callerName, callerLine, callerSize);
	Die();
    } else if (callerSize > maxBlkSize) {
	fprintf(outStream,"*** Mem_Alloc (%s:%d) size exceeds limit (%08x): %08x\n",
		callerName, callerLine, maxBlkSize, callerSize);
	Die();
    }

    if (blks.inuse >= blks.cnt) {
	fprintf(outStream, "*** Mem_Alloc (%s:%d) alloc %d (0x%x) byte table\n",
		callerName, callerLine,
		(blks.cnt+CHUNK)*sizeof(NODE),	(blks.cnt+CHUNK)*sizeof(NODE));
	newTab = (NODE *)REAL_ALLOC( (blks.cnt+CHUNK)*sizeof(NODE) );
	if (newTab == (NODE *)NULL) {
	    fprintf(outStream, "*** Mem_Alloc (%s:%d) couldn't get private table space\n",
		callerName, callerLine);
	    Die();
	}
	for (i=0; i<blks.cnt; i++) {
	    newTab[i] = blks.tab[i];
	}
	blks.cnt += CHUNK;
	if (blks.tab != (NODE *)NULL) {
	    REAL_FREE(blks.tab);
	}
	blks.tab = newTab;
    }

    totAlloc++;
    outSize += callerSize;
    outCnt++;
    nodePtr = blks.tab+blks.inuse;
    nodePtr->name = callerName;
    nodePtr->line = callerLine;
    nodePtr->size = callerSize;
    nodePtr->flags = 0;
    blkPtr = (char *)REAL_ALLOC(blkSize);
    strncpy(blkPtr, BORDER, BORDERSIZE);
    blkPtr += BORDERSIZE;
    nodePtr->ptr = blkPtr;
    strncpy(blkPtr+callerSize, BORDER, BORDERSIZE);
    blks.inuse++;

    if (traceFlags & TRACECALLS) {
	fprintf(outStream,"Mem_Alloc (%s:%d) ptr=0x%08x; size=0x%08x\n",
		callerName, callerLine, blkPtr, callerSize);
    }

    if (traceFlags & CHECKALLBLKS) {
	CheckBlk(blks.tab, blks.inuse);
    }

    if (traceFlags & TRACESTATS) {
	Mem_Report(callerName,callerLine,callerName,SORTBYADDR);
    }

    return(blkPtr);

} /* Mem_Alloc */


/*
 *----------------------------------------------------------------------
 *
 * Mem_Free--
 *
 *	Safe memory allocator. This gives us a place to localize
 * and trace memory use, if it should become necessary.
 *
 * Results:
 *	Ptr to block of specified size or NULL;
 *
 * Side effects:
 *	Allocates heap space.
 *
 *----------------------------------------------------------------------
 */

int
Mem_Free(callerName, callerLine, callerPtr)
    char *callerName;         /* calling procedure's name */
    int callerLine;           /* line no in caller's source */
    char *callerPtr;          /* block to be released */
{
    int blkNum;
    int i;
    char *blkPtr;
    NODE *nodePtr;

    blkPtr = callerPtr - BORDERSIZE;

    if (!(traceFlags & TRACEMEM)) {
	REAL_FREE(blkPtr);
	return 0;
    }

    for (blkNum=blks.inuse-1,nodePtr= &blks.tab[blks.inuse-1];
	 blkNum>=0; blkNum--,nodePtr--) {
	if (nodePtr->ptr == callerPtr) {
	    break;
	}
    }

    if (traceFlags & TRACECALLS) {
	fprintf(outStream,"Mem_Free  (%s:%d) ptr=0x%08x size=0x%08x\n",
		callerName, callerLine, callerPtr, 
		(blkNum < 0) ? -1 : nodePtr->size);
    }

    if (blkNum < 0) {
	if (!(traceFlags & IGNOREMISSINGBLK)) {
	    fprintf(outStream, "*** Mem_Free  (%s:%d) didn't find block ptr=0x%08x.\n",
		    callerName, callerLine, callerPtr);
	}
	if (!(traceFlags & IGNOREMISSINGBLK+WARNMISSINGBLK)) {
	    Die();
	}
	return(0);
    }

    if (nodePtr->flags & FREE) {
	fprintf(outStream,"*** Mem_Free  (%s:%d) block 0x%08x alloc'd by %s already free.\n",
		callerName, callerLine, callerPtr, nodePtr->name);
    }

    if (!(traceFlags & CHECKALLBLKS)) {
	CheckBlk(nodePtr, 1);
    }

    for (i=0; i<nodePtr->size; i++) {
	callerPtr[i] = BITPATTERN;
    }

    totFree++;
    outCnt--;
    outSize -= nodePtr->size;
    curFree += nodePtr->size;
    nodePtr->flags |= FREE;
    nodePtr->freeLink = NIL;

    if (traceFlags & CHECKALLBLKS) {
	CheckBlk(blks.tab, blks.inuse);
    }

    if (traceFlags & TRACESTATS) {
	Mem_Report(callerName,callerLine,callerName,SORTBYADDR);
    }

    /* enqueue block on free chain */
    if (freeHead == NIL) {
	freeHead = blkNum;
    } else {
	nodePtr = &blks.tab[freeTail];
	nodePtr->freeLink = blkNum;
    }
    freeTail = blkNum;

    while ((curFree > maxFreeSpace) && (freeHead != NIL)) {
	/* dequeue first block */
	nodePtr = &blks.tab[freeHead];
	CheckBlk(nodePtr, 1);
        freeHead = nodePtr->freeLink;	
	curFree -= nodePtr->size;
	nodePtr->flags |= RETURNED;
	REAL_FREE(nodePtr->ptr-BORDERSIZE);
    }

    return(0);

} /* Mem_Free */


/*
 *----------------------------------------------------------------------
 *
 * Mem_Report--
 *
 *	Report results of tracing
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

void
Mem_Report(callerName, callerLine, ownerName, reportFlags)
    char *callerName;         /* calling procedure's name */
    int callerLine;           /* line no in caller's source */
    char *ownerName;          /* blocker owner's name */
    int reportFlags;          /* output flags */
{
    int i,j;
    int nodeCnt = 0;
    NODE *nodePtr;
    NODE **arr;
    
    if (!(traceFlags & TRACEMEM)) {
	return;
    }

    for (i=0, nodePtr=blks.tab; i<blks.inuse; i++,nodePtr++) {
	if (((!(nodePtr->flags & FREE)) || (reportFlags & REPORTALLBLKS)) &&
	    ((strcmp(ownerName, ALLROUTINES) == 0) ||
	     (strcmp(nodePtr->name,ownerName) == 0))) {
	    nodeCnt++;
	}
    }

    arr = (NODE **)REAL_ALLOC(nodeCnt*sizeof(NODE *));
    if (arr == NULL) {
	fprintf(outStream,"Mem_Report: couldn't get private table space.\n");
	Die();
    }

    for (i=0, j=0,nodePtr=blks.tab; i<blks.inuse; i++,nodePtr++) {
	if (((!(nodePtr->flags & FREE)) || (reportFlags & REPORTALLBLKS)) &&
	    ((strcmp(ownerName, ALLROUTINES) == 0) ||
	     (strcmp(nodePtr->name,ownerName) == 0))) {
	    arr[j++] = nodePtr;
	}
    }

    fprintf(outStream,"- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -\n");    

    if (reportFlags & SORTBYREQ) {
	fprintf(outStream, "Mem_Report (%s:%d) blks for %s, ordered by request:\n",
		callerName, callerLine, ownerName);
	PrintNodes(ownerName, arr, nodeCnt);
    }
    if (reportFlags & SORTBYADDR) {
	fprintf(outStream, "Mem_Report (%s:%d) blks for %s, ordered by address:\n",
		callerName, callerLine, ownerName);
	SortNodes(arr, nodeCnt, PTRFIELD);
	PrintNodes(ownerName, arr, nodeCnt);
    }
    if (reportFlags & SORTBYOWNER) {
	fprintf(outStream, "Mem_Report (%s:%d) blks for %s, ordered by owner:\n",
		callerName, callerLine, ownerName);
	SortNodes(arr, nodeCnt, NAMEFIELD);
	PrintNodes(ownerName, arr, nodeCnt);
    }
    if (reportFlags & SORTBYSIZE) {
	fprintf(outStream, "Mem_Report (%s:%d) blks for %s, ordered by size:\n",
		callerName, callerLine, ownerName);
	SortNodes(arr, nodeCnt, SIZEFIELD);
	PrintNodes(ownerName, arr, nodeCnt);
    }
    REAL_FREE(arr);

    if (strcmp(ownerName, ALLROUTINES) != 0) {
	fprintf(outStream, "In use by all routines: %d (0x%x) blocks; %d (0x%x) bytes.\n",
		outCnt, outCnt, outSize, outSize);
    }

    fprintf(outStream,"Call counts: Mem_Alloc %d; Mem_Free %d\n",
	    totAlloc, totFree);
    fprintf(outStream,"- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -\n");
    fflush(outStream);

} /* Mem_Report */


/*
 *----------------------------------------------------------------------
 *
 * Mem_Report--
 *
 *	Report results of tracing
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static void
PrintNodes(ownerName, nodeList, nodeCnt)
    char *ownerName;          /* block owner's name */
    NODE **nodeList;          /* list of nodes to be printed */
    int  nodeCnt;             /* number of nodes */
{
    int i,odd;
    int localSize = 0; 
    int localCnt = 0;
    NODE *nodePtr1;
    NODE *nodePtr2;
    int half;
    char buf1[1024];
    char buf2[1024];
    char free1;
    char free2;

    half = nodeCnt >> 1;

    if (2*half == nodeCnt) {
	odd = 0;
    } else if (2*half == nodeCnt-1) {
	odd = 1;
    }

    if (nodeCnt > 0) {
	fprintf(outStream, "    owner               size      ptr        owner             size      ptr\n");
    }

    for (i=0; i<half; i+=2) {
	nodePtr1 = nodeList[i];
	nodePtr2 = nodeList[i+i];
	free1 = ' ';
	free2 = ' ';
	if (!(nodePtr1->flags & FREE)) {
	    localSize += nodePtr1->size;
	    localCnt++;
	    free1 = '!';
	}
	if (!(nodePtr2->flags & FREE)) {
	    localSize += nodePtr2->size;
	    localCnt++;
	    free2 = '!';
	}
	strcpy(buf1, nodePtr1->name);
	sprintf(buf1+strlen(buf1),":%d",nodePtr1->line);
	strcpy(buf2, nodePtr2->name);
	sprintf(buf2+strlen(buf2),":%d",nodePtr2->line);
	fprintf(outStream, "%-*s %c%08x %08x  %-*s %c%08x %08x\n",
		MAXNAMELEN, buf1, free1, nodePtr1->size, nodePtr1->ptr,
		MAXNAMELEN, buf2, free2, nodePtr2->size, nodePtr2->ptr);
    } 

    if (odd == 1) {
	nodePtr1 = nodeList[nodeCnt-1];
	strcpy(buf1, nodePtr1->name);
	sprintf(buf1+strlen(buf1),":%d",nodePtr1->line);
	free1 = ' ';
	if (!(nodePtr1->flags & FREE)) {
	    localSize += nodePtr1->size;
	    localCnt++;
	    free1 = '!';
	}
	fprintf(outStream, "%-*s %c%08x %08x\n",
		MAXNAMELEN, buf1, free1, nodePtr1->size, nodePtr1->ptr);
    }
    fprintf(outStream, "In use by %s - %d (0x%x) blocks; %d (0x%x) bytes.\n\n",
	    ownerName, localCnt, localCnt, localSize, localSize);

} /* PrintNodes */



/*
 *----------------------------------------------------------------------
 *
 * Mem_Control --
 *
 *	Control diagnostic memory stuff
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Sets tracing flags.
 *
 *----------------------------------------------------------------------
 */

void
Mem_Control(callerBlkSize, callerStream, callerFlags, callerFree)
    int callerBlkSize;        /* Maximum block size */
    FILE *callerStream;       /* Diagnostic output stream */
    int callerFlags;          /* tracing flags. see mem.h */
    int callerFree;           /* max freed block space before returning to REALFREE */

{
    if (callerBlkSize >= 0) {
	maxBlkSize = callerBlkSize;
    }

    if (callerStream != (FILE *)NULL) {
	outStream = callerStream;
    }

    if ((traceFlags & IGNOREMISSINGBLK) &&
	(traceFlags & WARNMISSINGBLK)) {
	fprintf(stderr,"conflicting trace flags.\n");
	exit(0);
    }

    maxFreeSpace = callerFree;
    traceFlags = callerFlags;

    if ((!(traceFlags & TRACEMEM)) && (blks.tab != NULL)) {
	free(blks.tab);
	blks.tab = (NODE *)NULL;
	blks.cnt = 0;
	outStream = stderr;
    }
    
    traceInit = 1;
    
} /* Mem_Control */


/*
 *----------------------------------------------------------------------
 *
 * SortNodes
 *
 *	Quickie sort routine
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static void
SortNodes(nodeList, nodeCnt, field)
    NODE **nodeList;          /* list of nodes */
    int nodeCnt;              /* number of nodes */
    int field;                /* field to sort on */
{
    int i,j,gap;
    NODE *temp;

    for (gap=nodeCnt/2; gap>0; gap /= 2) {
	for (i=gap; i<nodeCnt; i++) {
	    if (field == PTRFIELD) {
		for (j=i-gap;
		     j>=0 && nodeList[j]->ptr>nodeList[j+gap]->ptr;
		     j-=gap) {
		    temp = nodeList[j];
		    nodeList[j] = nodeList[j+gap];
		    nodeList[j+gap] = temp;
		}
	    } else if (field == NAMEFIELD) {
		for (j=i-gap;
		     j>=0 && (strcmp(nodeList[j]->name,nodeList[j+gap]->name) >0);
		     j-=gap) {
		    temp = nodeList[j];
		    nodeList[j] = nodeList[j+gap];
		    nodeList[j+gap] = temp;
		}
	    } else if (field == SIZEFIELD) {
		for (j=i-gap;
		     j>=0 && nodeList[j]->size>nodeList[j+gap]->size;
		     j-=gap) {
		    temp = nodeList[j];
		    nodeList[j] = nodeList[j+gap];
		    nodeList[j+gap] = temp;
		}
	    }
	}
    }

} /* SortNodes */


/*
 *----------------------------------------------------------------------
 *
 * CheckBlk--
 *
 *	Verify that border hasn't been overwritten.
 *      For free blocks verify that no new bits have appeared.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static void
CheckBlk(nodePtr, count)
    NODE *nodePtr;            /* block list */
    int count;                /* number of blocks */
{
    int i;

    while (count-- > 0) {
	if (!(nodePtr->flags & FREE) &&
	    ((bcmp(nodePtr->ptr-BORDERSIZE,BORDER,BORDERSIZE) != 0) || 
	    (bcmp(nodePtr->ptr+nodePtr->size,BORDER,BORDERSIZE) != 0))) {
	    fprintf(outStream, "*** CheckBlk: owner=%s; line=%d, ptr=0x%08x; size=0x%08x: border overwritten!\n",
		nodePtr->name, nodePtr->line, nodePtr->ptr, nodePtr->size);
	    Die();
	}
	if ((nodePtr->flags & FREE) && (!(nodePtr->flags & RETURNED))) {
	    for (i=0; i<nodePtr->size; i++) {
		if (nodePtr->ptr[i] != BITPATTERN) {
		    fprintf(outStream, "*** CheckBlk: owner=%s; line=%d, ptr=0x%08x; size=0x%08x: freed block modified!\n",
		    nodePtr->name, nodePtr->line, nodePtr->ptr, nodePtr->size);
		    Die();
		}
	    }
	}
	nodePtr++;
    }

} /* CheckBlk */


static void
Die()
{
    if (outStream != NULL) {
	fflush(outStream);
    }
    fprintf("%s", 0); /* double ptr whammy */
    exit(-1); /* just in case */
}
