/* 
 * recovFast.c --
 *
 *	The routines here deal with fast restart and recovery from an
 *	area of memory preserved across crashes.
 *
 * Copyright 1987 Regents of the University of California
 * All rights reserved.
 */

#ifndef lint
static char rcsid[] = "$Header$ SPRITE (Berkeley)";
#endif /* not lint */


#include <sprite.h>
#include <recov.h>
#include <mach.h>
#include <machConst.h>
#include <sync.h>
#include <stdio.h>
#include <vm.h>
#include <stdlib.h>

#define	NUM_RESTART_TYPES	10

/*
 * Info per/object about what its checksum is, if it's allocated, or what
 * the next free index is if it's not allocated.
 */
typedef	struct	ObjInfo {
    short	applObjNum;
    union	{
	unsigned short	checksum;
	unsigned short	nextFree;
    } info;
} ObjInfo;

/*
 * Per-type information kept in the table of contents.  The objects for a type
 * are stored as if in an array, indexed by the object's number.  The bitmap
 * for a type shows the allocation of objects in the object array.  It also
 * gives the application's object number for the object, if there is one.
 * The value in the array for an object can be 0 (unallocated), or -1 (allocated
 * without an application object number, or else give the application's
 * object number.
 */
typedef struct	RestartTypeInfo {
    int		objectSize;		/* Size of this type's objects. */
    int		maxNumObjects;		/* Max possible objects at once. */
    int		currentNum;		/* Current num of objects for type. */
    int		applicationTypeID;	/* Application's type ID. */
    int		firstFree;		/* First index in free obj buffer. */
    ObjInfo	*objInfoAddr;		/* Address of checksum/free array. */
    char	*objectsAddr;		/* Address of object storage. */
    unsigned	short	(*Checksum)();	/* Function to use to generate checksum.
					 * No checksum done if NIL. */
} RestartTypeInfo;

typedef struct	RestartHeader {
    int			initialized;
    int			nextTypeID;
    RestartTypeInfo	contents[NUM_RESTART_TYPES];
} RestartHeader;


RestartHeader	*restartTablePtr;

static	int	spaceUsed;

/*
 * Used for saving past values of checksum routines for the different
 * object types when the checksum is toggled on and off for testing.
 */
Address	checksumList[NUM_RESTART_TYPES] =
	{(Address) NIL, (Address) NIL,
	(Address) NIL, (Address) NIL,
	(Address) NIL, (Address) NIL,
	(Address) NIL, (Address) NIL,
	(Address) NIL, (Address) NIL};

Boolean	recov_RestartDebug = FALSE;

static unsigned short GetFreeIndex _ARGS_((int typeID));
static void AddFreeIndex _ARGS_((int typeID, int objNum));

static Sync_Lock restartTableLock;
#define	LOCKPTR	(&restartTableLock)


/*
 *----------------------------------------------------------------------
 *
 * Recov_InitRecovBox --
 *
 *	Initialize fast recovery code.
 *
 * Results:
 *	Whether or not box was already initialized.  (TRUE if done already.)
 *
 * Side effects:
 *	Recovery area and data structures are initialized.
 *
 *----------------------------------------------------------------------
 */
Boolean
Recov_InitRecovBox()
{
    Boolean	alreadyInit = FALSE;
    int		nextID;

    restartTablePtr = (RestartHeader *) mach_RestartTablePtr;
    if (recov_RestartDebug) {
	printf("mach_RestartTablePtr is at 0x%x\n");
    }
    if (restartTablePtr == (RestartHeader *) NIL ||
	    restartTablePtr == (RestartHeader *) NULL) {
	panic("Recov_InitRecovBox called with no restart table available.");
    }
    if ((Mach_GetRestartTableSize() & (VMMACH_PAGE_SIZE -1)) != 0) {
	panic(
	"Recov_Init_RecovBox: restart table not multiple of page size.\n");
    }
    if (((unsigned int) restartTablePtr & (VMMACH_PAGE_SIZE - 1)) != 0) {
	panic(
	"Recov_Init_RecovBox: restart table not aligned on page boundary.\n");
    }
    /*
     * Check if we've been initialized before.
     */
    if (!restartTablePtr->initialized) {
	/* Uninitialized - set everything to zero. */
	bzero((char *) restartTablePtr, Mach_GetRestartTableSize());
	spaceUsed = sizeof (RestartHeader);
	if (recov_RestartDebug) {
	    printf("Recov_InitRecovBox: initialized, addr 0x%x.\n",
		    (char *) restartTablePtr);
	}
    } else {
	alreadyInit = TRUE;
	/* The nextTypeID is stored in table and needs no initialization. */
	nextID = restartTablePtr->nextTypeID;
	/* set spaceUsed */
	if (nextID == 0) {
	    spaceUsed = 0;
	} else {
	    spaceUsed = (unsigned int)
		    restartTablePtr->contents[nextID - 1].objectsAddr +
		    (restartTablePtr->contents[nextID - 1].maxNumObjects *
		    restartTablePtr->contents[nextID - 1].objectSize) -
		    (unsigned int) restartTablePtr;
	}

	if (recov_RestartDebug) {
	    printf("Recov_InitRecovBox: already initialized, addr 0x%x.\n",
		    (char *) restartTablePtr);
	}
    }

    restartTablePtr->initialized = 1;

    return alreadyInit;
}

/*
 *----------------------------------------------------------------------
 *
 * Recov_ToggleChecksum --
 *
 *	Toggle whether checksum is done or not for type .
 *
 * Results:
 *	Number of objects, given their size and the fact that there must be
 *	a bitmap of their allocation, etc.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
void
Recov_ToggleChecksum(typeID)
    int		typeID;
{
    unsigned	short	(*save)();

    if (!recov_Transparent) {
	printf("Recov_ToggleChecksum: no transparent recovery.\n");
	return;
    }
    save = restartTablePtr->contents[typeID].Checksum;
    restartTablePtr->contents[typeID].Checksum =
	    (unsigned short (*)()) checksumList[typeID];
    checksumList[typeID] = (Address) save;
    if (save != (unsigned short (*)()) NIL) {
	printf("Checksum turned off.\n");
    } else {
	printf("Checksum turned on.\n");
    }

    return;
}


/*
 *----------------------------------------------------------------------
 *
 * GetFreeIndex --
 *
 *	Get the index of the next free object space. Take it off free list.
 *
 * Results:
 *	The index of the free object space.
 *
 * Side effects:
 *	Takes object off free list.
 *
 *----------------------------------------------------------------------
 */
static unsigned short
GetFreeIndex(typeID)
    int		typeID;
{
    unsigned short	freeIndex;
    RestartTypeInfo	*typeInfoPtr;

    if (!recov_Transparent) {
	panic("GetFreeIndex: no transparent recovery.");
    }
    typeInfoPtr = &(restartTablePtr->contents[typeID]);
    freeIndex = typeInfoPtr->firstFree;
    typeInfoPtr->firstFree = typeInfoPtr->objInfoAddr[freeIndex].info.nextFree;

    return freeIndex;
}


/*
 *----------------------------------------------------------------------
 *
 * AddFreeIndex --
 *
 *	Add an object to the free list.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Object goes at head of free list.
 *
 *----------------------------------------------------------------------
 */
static void
AddFreeIndex(typeID, objNum)
    int		typeID;
    int		objNum;
{
    RestartTypeInfo	*typeInfoPtr;

    if (!recov_Transparent) {
	panic("AddFreeIndex: no transparent recovery.");
    }
    typeInfoPtr = &(restartTablePtr->contents[typeID]);
    typeInfoPtr->objInfoAddr[objNum].info.nextFree = typeInfoPtr->firstFree;
    typeInfoPtr->firstFree = objNum;

    return;
}


/*
 *----------------------------------------------------------------------
 *
 * Recov_MaxNumObjects --
 *
 *	Figure out how many objects of this new size can be put into the table.
 *	If it's a fast restart, figure out how many total objects would fit
 *	into the table assuming this is the first type.
 *
 * Results:
 *	Number of objects, given their size and the fact that there must be
 *	a bitmap of their allocation, etc.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
int
Recov_MaxNumObjects(objectSize, restart)
    int		objectSize;
    Boolean	restart;	/* If this is a fast restart. */
{
    int		spaceLeft;
    int		maxNumObjects;

    if (!recov_Transparent) {
	printf("Recov_MaxNumObjects: no transparent recovery.\n");
	return 0;
    }
    if (!restartTablePtr->initialized) {
	panic("Recov_MaxNumObjects called before Recov_InitRecovBox.");
    }
    /*
     * If this is a fast restart, spaceUsed includes everything in the table
     * already and we'd get told we had room for almost no objects, but what
     * we want on a fast restart is how many objects (old and new) the table
     * will hold, so we do the calculation differently.  But this only works
     * for the first type allocated after the restart!
     */
    if (restart) {
	spaceLeft = Mach_GetRestartTableSize() - sizeof (RestartHeader);
    } else {
	spaceLeft = Mach_GetRestartTableSize() - spaceUsed;
    }
    /*
     * This includes space for the objects and their checksum/freelist array
     * and a word alignment.
     */
    maxNumObjects = (spaceLeft - sizeof (int)) / (objectSize + sizeof (int));

    return maxNumObjects;
}

/*
 *----------------------------------------------------------------------
 *
 * Recov_PrintSpace --
 *
 *	Print out how much space is taken by what in the recovery box.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Prints out the result.
 *
 *----------------------------------------------------------------------
 */
void
Recov_PrintSpace(objectSize)
    int		objectSize;
{
    int		spaceLeft;
    int		maxNumObjects;

    if (!recov_Transparent) {
	printf("Recov_PrintSpace: no transparent recovery.\n");
	return;
    }
    if (!restartTablePtr->initialized) {
	panic("Recov_PrintSpace called before Recov_InitRecovBox.");
    }
    /*
     */
    printf("Sizes:\n");
    printf("Header:\t%d\t0x%x\n", sizeof (RestartHeader),
	    sizeof (RestartHeader));
    spaceLeft = Mach_GetRestartTableSize() - sizeof (RestartHeader);
    printf("ObjSpace:\t%d\t0x%x\n", spaceLeft, spaceLeft);
    spaceLeft = Mach_GetRestartTableSize() - spaceUsed;
    printf("SpaceLeft:\t%d\t0x%x\n", spaceLeft, spaceLeft);
    /*
     * This includes space for the objects and their allocation bitmap
     * and a word alignment.
     */
    maxNumObjects = (spaceLeft - sizeof (int)) / (objectSize + sizeof (int));
    printf("This is room for %d (0x%x) objs of size %d with allocation map.\n",
	    maxNumObjects, maxNumObjects, objectSize);

    return;
}


/*
 *----------------------------------------------------------------------
 *
 * Recov_InitType --
 *
 *	Initialize fast recovery area for a new object type.
 *
 * Results:
 *	SUCCESS or FAILURE.  If successful we also return in objectTypeIDPtr
 *	the type ID for this new object type if there are no errors, or
 *	an error code (-1) if this does not work.  Reasons for failure are too
 *	many object types already, not enough space for the desired number
 *	of objects or an object size of zero, or else that the given
 *	application type ID is positive (set) and turns out not to be unique.
 *
 * Side effects:
 *	A portion of the recovery area is set aside.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
Recov_InitType(objectSize, maxNumObjects, applicationTypeID, objectTypePtr,
Checksum)
    int			objectSize;
    int			maxNumObjects;
    int			applicationTypeID;
    int			*objectTypePtr;	/* OUT: new object typeId
    					 * if successful. */
    unsigned short	(*Checksum)();	/* No Checksum if 0 or NIL, use
					 * default if == 1. */
{
    int			spaceLeft;
    unsigned int	mask;
    int			i;
    RestartTypeInfo	*typeInfoPtr;
    int			typeID;

    LOCK_MONITOR;

    if (recov_RestartDebug) {
	printf("Recov_InitType: objectSize: %d, maxNumObjects: %d\n",
		objectSize, maxNumObjects);
    }

    if (!recov_Transparent) {
	printf("Recov_InitType: no transparent recovery.\n");
	UNLOCK_MONITOR;
	return FAILURE;
    }
	
    if (objectSize <= 0) {
	printf("Recov_InitType: Cannot accept 0-sized object type.\n");
	UNLOCK_MONITOR;
	return FAILURE;
    }

    /*
     * Check if there are still slots for new object types.
     */
    *objectTypePtr = -1;
    if (restartTablePtr->nextTypeID >= NUM_RESTART_TYPES) {
	printf("Recov_InitType: No space for a new restart object type.\n");
	UNLOCK_MONITOR;
	return FAILURE;
    }
    typeID = restartTablePtr->nextTypeID;

    if (applicationTypeID > 0) {
	for (i = 0; i < typeID; i++) {
	    if (restartTablePtr->contents[i].applicationTypeID ==
		    applicationTypeID) {
		printf("Recov_InitType: applicationTypeID %d is not unique.\n",
			applicationTypeID);
		UNLOCK_MONITOR;
		return FAILURE;
	    }
	}
    }
    /*
     * Check if there's space for the desired number of objects.
     */
    spaceLeft = Mach_GetRestartTableSize() - spaceUsed;
    if (recov_RestartDebug) {
	printf("Recov_InitType: space used: 0x%x, space left now: 0x%x\n",
		spaceUsed, spaceLeft);
    }

    /*
     * Is there enough space for the new objects plus a bitmap
     * of their allocation?  (Plus a word for alignment for the bitmap.)
     */
    if (Recov_MaxNumObjects(objectSize, 0) < maxNumObjects) {
	printf("Recov_InitType: Not enough space left for request.\n");
	UNLOCK_MONITOR;
	return FAILURE;
    }
    
    /*
     * Allocate new object type space.
     */
    typeInfoPtr = &(restartTablePtr->contents[typeID]);
    typeInfoPtr->objectSize = objectSize;
    typeInfoPtr->applicationTypeID = applicationTypeID;
    typeInfoPtr->maxNumObjects = maxNumObjects;
    typeInfoPtr->currentNum = 0;
    typeInfoPtr->objInfoAddr = (ObjInfo *) (((char *) restartTablePtr) +
	    spaceUsed + sizeof (int));
    /* Word-align the checksum/freelist array. */
    mask = sizeof (int) - 1;
    typeInfoPtr->objInfoAddr = (ObjInfo *)
	    (((unsigned int) typeInfoPtr->objInfoAddr) & ~mask);
    typeInfoPtr->objectsAddr = (char *)
	    (((char *) typeInfoPtr->objInfoAddr) +
	    (maxNumObjects * sizeof (ObjInfo)));

    if (recov_RestartDebug) {
	printf("Recov_InitType: objInfoAddr: 0x%x, objectsAddr: 0x%x\n",
		typeInfoPtr->objInfoAddr, typeInfoPtr->objectsAddr);
    }
    
    spaceUsed = (unsigned int) typeInfoPtr->objectsAddr +
	    (maxNumObjects * objectSize) - (unsigned int) restartTablePtr;
    if (Checksum == (unsigned short (*)()) 0) {
	typeInfoPtr->Checksum = (unsigned short (*)()) NIL;
    } else if (Checksum == (unsigned short (*)()) 1) {
	typeInfoPtr->Checksum = Recov_Checksum;
    } else {
	typeInfoPtr->Checksum = Checksum;
    }

    /* Set up free list. */
    typeInfoPtr->firstFree = 0;
    for (i = 0; i < maxNumObjects - 1; i++) {
	typeInfoPtr->objInfoAddr[i].info.nextFree = i + 1;
    }
    /* Point back to beginning of array. */
    typeInfoPtr->objInfoAddr[maxNumObjects - 1].info.nextFree = 0;

    *objectTypePtr = typeID;

    if (recov_RestartDebug) {
	printf("Recov_InitType: new object type %d\n", *objectTypePtr);
    }
    restartTablePtr->nextTypeID++;
    
    UNLOCK_MONITOR;
    return SUCCESS;
}


/*
 *----------------------------------------------------------------------
 *
 * Recov_GetObjectSize --
 *
 *	Return the object size for a given type of objects.
 *
 * Results:
 *	The size of an object of the given type. -1 if there is no such
 *	type.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
int
Recov_GetObjectSize(typeID)
    int		typeID;		/* Type of object to insert. */
{
    int		objectSize;

    LOCK_MONITOR;
    if (!recov_Transparent) {
	printf("Recov_GetObjectSize: no transparent recovery.\n");
	UNLOCK_MONITOR;
	return -1;
    }
    if (typeID < 0 || typeID >= restartTablePtr->nextTypeID) {
	UNLOCK_MONITOR;
	return -1;
    }

    objectSize = restartTablePtr->contents[typeID].objectSize;
    UNLOCK_MONITOR;
    return objectSize;
}


/*
 *----------------------------------------------------------------------
 *
 * Recov_InsertObject --
 *
 *	Insert an object into the recovery area.
 *
 * Results:
 *	SUCCESS or FAILURE.  Reasons for failure are that the object type
 *	doesn't exist or that the area for this object type is already
 *	filled.
 *
 * Side effects:
 *	If successful, a new object is recorded in the recovery area.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
Recov_InsertObject(typeID, objectPtr, applicationObjectNum, objectIDPtr)
    int			typeID;		/* Type of object to insert. */
    ClientData		objectPtr;	/* The object. */
    int			applicationObjectNum;	/* The appl's number for obj. */
    Recov_ObjectID	*objectIDPtr;	/* OUT: ID of new object. */
{
    int			i;
    RestartTypeInfo	*restartTypeInfoPtr;
    ObjInfo		*objInfoAddr;
    int			objectSize;
    char		*addr;
    int			freeIndex;

    LOCK_MONITOR;

    objectIDPtr->typeID = -1;
    objectIDPtr->objectNumber = -1;

    if (!recov_Transparent) {
	printf("Recov_InsertObject: no transparent recovery.\n");
	UNLOCK_MONITOR;
	return FAILURE;
    }
    if (typeID < 0 || typeID >= restartTablePtr->nextTypeID) {
	printf("Recov_InsertObject: No such fast restart object type.\n");
	UNLOCK_MONITOR;
	return FAILURE;
    }
    restartTypeInfoPtr = &(restartTablePtr->contents[typeID]);
    if (restartTypeInfoPtr->currentNum >=
	    restartTypeInfoPtr->maxNumObjects) {
	printf("Recov_InsertObject: No space for another such object.\n");
	UNLOCK_MONITOR;
	return FAILURE;
    }

    /* Get a free index. */
    freeIndex = GetFreeIndex(typeID);

    objInfoAddr = restartTypeInfoPtr->objInfoAddr;
    if (applicationObjectNum <= 0) {
	objInfoAddr[freeIndex].applObjNum = -1;
    } else {
	/* Should I really check?  This will make it a lot slower. */
	for (i = 0; i < restartTypeInfoPtr->maxNumObjects; i++) {
	    if (objInfoAddr[i].applObjNum == applicationObjectNum) {
		printf("Recov_InsertObject: Object num already used.\n");
		UNLOCK_MONITOR;
		return FAILURE;
	    }
	}
	objInfoAddr[freeIndex].applObjNum = applicationObjectNum;
    }

    objectSize = restartTypeInfoPtr->objectSize;
    addr = restartTypeInfoPtr->objectsAddr + (freeIndex * objectSize);

    if (restartTypeInfoPtr->Checksum != (unsigned short (*)()) NIL) {
	volatile unsigned short	sum;
	/* Volatile should change when we check the checksum!!! */

	sum = (*(restartTypeInfoPtr->Checksum))(objectSize,
		(Address) objectPtr);
    }

    objectIDPtr->typeID = typeID;
    objectIDPtr->objectNumber = freeIndex;
    restartTypeInfoPtr->currentNum++;

    bcopy((char *) objectPtr, addr, objectSize);
    

    UNLOCK_MONITOR;
    return SUCCESS;
}


/*
 *----------------------------------------------------------------------
 *
 * Recov_InsertObjects --
 *
 *	Insert a set of objects into the recovery area at once.  If the
 *	objNumBuffer is not NIL, then this array gives the application's
 *	object numbers to record.
 *
 * Results:
 *	SUCCESS or FAILURE.  Reasons for failure are that the object type
 *	doesn't exist or that the area for this object type is already
 *	filled.  The objects' IDs are returned in the out-going buffer.
 *
 * Side effects:
 *	If successful, new objects are recorded in the recovery area.
 *	If the routine returns SUCCESS, then all the objects have been
 *	inserted.  If it returns FAILURE, then no objects have been inserted.
 *	If it panics, then the results are undefined.
 *	
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
Recov_InsertObjects(typeID, numObjs, obuffer, objNumBuffer, objIDBuffer)
    int			typeID;		/* Type of object to insert. */
    int			numObjs;	/* Number of objects to insert. */
    char		*obuffer;	/* The objects. */
    int			*objNumBuffer;	/* The appl's numbers for objs. */
    Recov_ObjectID	*objIDBuffer;	/* OUT: IDs of new objects. */
{
    int			i, freeIndex;
    RestartTypeInfo	*restartTypeInfoPtr;
    ObjInfo		*objInfoAddr;
    int			objectSize;
    char		*addr;
    Recov_ObjectID	*objectIDPtr;
    char		*objectPtr;

    LOCK_MONITOR;

    if (!recov_Transparent) {
	printf("Recov_InsertObjects: no transparent recovery.\n");
	UNLOCK_MONITOR;
	return FAILURE;
    }

    /* Should I bother to do this? */
    for (i = 0; i < numObjs; i++) {
	objIDBuffer[i].typeID = -1;
	objIDBuffer[i].objectNumber = -1;
    }

    if (typeID < 0 || typeID >= restartTablePtr->nextTypeID) {
	printf("Recov_InsertObjects: No such fast restart object type.\n");
	UNLOCK_MONITOR;
	return FAILURE;
    }
    restartTypeInfoPtr = &(restartTablePtr->contents[typeID]);
    if (restartTypeInfoPtr->currentNum + numObjs >
	    restartTypeInfoPtr->maxNumObjects) {
	printf("Recov_InsertObjects: Not enough space for new objects.\n");
	UNLOCK_MONITOR;
	return FAILURE;
    }

    objInfoAddr = restartTypeInfoPtr->objInfoAddr;

    if (objNumBuffer != (int *) NIL) {
	/* Should I really check?  This will make it a lot slower. */
	for (i = 0; i < numObjs; i++) {
	    for (i = 0; i < restartTypeInfoPtr->maxNumObjects; i++) {
		if (objInfoAddr[i].applObjNum == objNumBuffer[i]) {
		    printf("Recov_InsertObjects: Object num %d already used.\n",
			    objNumBuffer[i]);
		    UNLOCK_MONITOR;
		    return FAILURE;
		}
	    }
	}
    }

    objectSize = restartTypeInfoPtr->objectSize;
    /* Get first empty slot. */
    freeIndex = GetFreeIndex(typeID);

    for (i = 0; i < numObjs; i++) {
	if (objNumBuffer != (int *) NIL) {
	    objInfoAddr[freeIndex].applObjNum = objNumBuffer[i];
	} else {
	    objInfoAddr[freeIndex].applObjNum = -1;
	}
	objectIDPtr = &(objIDBuffer[i]);
	objectIDPtr->typeID = typeID;
	objectIDPtr->objectNumber = freeIndex;
	restartTypeInfoPtr->currentNum++;

	objectPtr = obuffer + (objectSize * i);
	addr = restartTypeInfoPtr->objectsAddr + (freeIndex * objectSize);

	if (restartTypeInfoPtr->Checksum != (unsigned short (*)()) NIL) {
	    volatile unsigned short	sum;
	    /* Volatile should change when we check the checksum!!! */

	    sum = (*(restartTypeInfoPtr->Checksum))(objectSize,
		    (Address) objectPtr);
	}

	bcopy((char *) objectPtr, addr, objectSize);
	/* Find next empty slot. */
	freeIndex = GetFreeIndex(typeID);
    }

    UNLOCK_MONITOR;
    return SUCCESS;
}


/*
 *----------------------------------------------------------------------
 *
 * Recov_DeleteObject --
 *
 *	Delete an object from the recovery area.
 *
 * Results:
 *	SUCCESS or FAILURE.  Reasons for failure are that the object type
 *	doesn't exist or that the object doesn't exist.
 *
 * Side effects:
 *	If successful, an object space is freed up.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
Recov_DeleteObject(objectID)
    Recov_ObjectID	objectID;	/* ID of object to remove. */
{
    ObjInfo		*objInfoAddr;
    char		*addr;
    int			objectSize;
    RestartTypeInfo	*restartTypeInfoPtr;

    LOCK_MONITOR;

    if (!recov_Transparent) {
	printf("Recov_DeleteObject: no transparent recovery.\n");
	UNLOCK_MONITOR;
	return FAILURE;
    }
    restartTypeInfoPtr =
	    &(restartTablePtr->contents[objectID.typeID]);
    if (objectID.typeID < 0 || objectID.typeID >= restartTablePtr->nextTypeID) {
	printf("Recov_DeleteObject: bad typeID.\n");
	UNLOCK_MONITOR;
	return FAILURE;
    }
    if (objectID.objectNumber < 0 ||
	    objectID.objectNumber > restartTypeInfoPtr->maxNumObjects) {
	printf("Recov_DeleteObject: bad object number.\n");
	UNLOCK_MONITOR;
	return FAILURE;
    }
    if (restartTypeInfoPtr->objectSize <= 0) {
	printf("Recov_DeleteObject: bad object type.\n");
	UNLOCK_MONITOR;
	return FAILURE;
    }
    objInfoAddr = restartTypeInfoPtr->objInfoAddr;
    if (objInfoAddr[objectID.objectNumber].applObjNum == 0) {
	printf("Recov_DeleteObject: object doesn't exist.\n");
	UNLOCK_MONITOR;
	return FAILURE;
    }
    objInfoAddr[objectID.objectNumber].applObjNum = 0;
    AddFreeIndex(objectID.typeID, objectID.objectNumber);
    objectSize = restartTypeInfoPtr->objectSize;
    restartTypeInfoPtr->currentNum--;
    if (restartTypeInfoPtr->currentNum < 0) {
	UNLOCK_MONITOR;
	panic("Recov_DeleteObject: less than zero objects of a type.\n");
    }
    addr = restartTypeInfoPtr->objectsAddr +
	    (objectID.objectNumber * objectSize);
    /* Not really necessary, but helpful for debugging. */
    bzero(addr, objectSize);

    UNLOCK_MONITOR;
    return SUCCESS;
}

/*
 *----------------------------------------------------------------------
 *
 * Recov_UpdateObject --
 *
 *	Update an object in the recovery area.
 *
 * Results:
 *	SUCCESS or FAILURE.  Reasons for failure are that the object type
 *	doesn't exist or that the object doesn't exist.
 *
 * Side effects:
 *	If successful, an object is updated.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
Recov_UpdateObject(objectPtr, objectID)
    ClientData		objectPtr;	/* New value of object. */
    Recov_ObjectID	objectID;	/* ID of object to update. */
{
    ObjInfo		*objInfoAddr;
    char		*addr;
    RestartTypeInfo	*restartTypeInfoPtr;
    int			objectSize;

    LOCK_MONITOR;

    if (!recov_Transparent) {
	printf("Recov_UpdateObject: no transparent recovery.\n");
	UNLOCK_MONITOR;
	return FAILURE;
    }
    restartTypeInfoPtr =
	    &(restartTablePtr->contents[objectID.typeID]);
    if (objectID.typeID < 0 || objectID.typeID >= restartTablePtr->nextTypeID) {
	printf("Recov_UpdateObject: bad typeID.\n");
	UNLOCK_MONITOR;
	return FAILURE;
    }
    if (objectID.objectNumber < 0 ||
	    objectID.objectNumber > restartTypeInfoPtr->maxNumObjects) {
	printf("Recov_UpdateObject: bad object number.\n");
	UNLOCK_MONITOR;
	return FAILURE;
    }
    if (restartTypeInfoPtr->objectSize <= 0) {
	printf("Recov_UpdateObject: bad object type.\n");
	UNLOCK_MONITOR;
	return FAILURE;
    }
    objInfoAddr = restartTypeInfoPtr->objInfoAddr;
    if (objInfoAddr[objectID.objectNumber].applObjNum == 0) {
	printf("Recov_UpdateObject: object doesn't exist.\n");
	UNLOCK_MONITOR;
	return FAILURE;
    }
    objectSize = restartTypeInfoPtr->objectSize;
    addr = restartTypeInfoPtr->objectsAddr +
	    (objectID.objectNumber * objectSize);

    if (restartTypeInfoPtr->Checksum != (unsigned short (*)()) NIL) {
	volatile unsigned short	sum;
	/* XXX Volatile should change when we check the checksum!!! */

	sum = (*(restartTypeInfoPtr->Checksum))(objectSize,
		(Address) objectPtr);
    }

    bcopy((char *) objectPtr, addr, objectSize);

    UNLOCK_MONITOR;
    return SUCCESS;
}

/*
 *----------------------------------------------------------------------
 *
 * Recov_ReturnObject --
 *
 *	Return the value of a single object.
 *
 * Results:
 *	SUCCESS or FAILURE.  Reasons for failure are that the object type
 *	doesn't exist or that the object doesn't exist.
 *
 * Side effects:
 *	If successful, an object is returned.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
Recov_ReturnObject(objectPtr, objectID, checksum)
    ClientData		objectPtr;	/* Ptr to return object. */
    Recov_ObjectID	objectID;	/* ID of object to return. */
    Boolean		checksum;
{
    ObjInfo		*objInfoAddr;
    char		*addr;
    RestartTypeInfo	*restartTypeInfoPtr;
    int			objectSize;

    LOCK_MONITOR;

    if (!recov_Transparent) {
	printf("Recov_ReturnObject: no transparent recovery.\n");
	UNLOCK_MONITOR;
	return FAILURE;
    }
    restartTypeInfoPtr =
	    &(restartTablePtr->contents[objectID.typeID]);
    if (objectID.typeID < 0 || objectID.typeID >= restartTablePtr->nextTypeID) {
	printf("Recov_ReturnObject: bad typeID.\n");
	UNLOCK_MONITOR;
	return FAILURE;
    }
    if (objectID.objectNumber < 0 ||
	    objectID.objectNumber > restartTypeInfoPtr->maxNumObjects) {
	printf("Recov_ReturnObject: bad object number.\n");
	UNLOCK_MONITOR;
	return FAILURE;
    }
    if (restartTypeInfoPtr->objectSize <= 0) {
	printf("Recov_ReturnObject: bad object type.\n");
	UNLOCK_MONITOR;
	return FAILURE;
    }
    objInfoAddr = restartTypeInfoPtr->objInfoAddr;
    if (objInfoAddr[objectID.objectNumber].applObjNum == 0) {
	printf("Recov_ReturnObject: object doesn't exist.\n");
	UNLOCK_MONITOR;
	return FAILURE;
    }
    objectSize = restartTypeInfoPtr->objectSize;
    addr = restartTypeInfoPtr->objectsAddr +
	    (objectID.objectNumber * objectSize);

    if (checksum && restartTypeInfoPtr->Checksum !=
	    (unsigned short (*)()) NIL) {
	volatile unsigned short	sum;
	/* XXX Volatile should change when we check the checksum!!! */

	sum = (*(restartTypeInfoPtr->Checksum))(objectSize, (Address) addr);
    }
    bcopy(addr, (char *) objectPtr, objectSize);

    UNLOCK_MONITOR;
    return SUCCESS;
}

/*
 *----------------------------------------------------------------------
 *
 * Recov_ReturnObjects --
 *
 *	Return an array of all the objects of a particular type.  Also return
 *	an array of their object IDs if the id buffer is not NIL.  Finally,
 *	return an array of the corresponding application object numbers if the
 *	application num buffer is not NIL.
 *
 * Results:
 *	SUCCESS or FAILURE.  Reasons for failure are that the object type
 *	doesn't exist or that not enough space was handed to the routine.
 *	The lengthPtrs return the actual length of the buffers filled in.
 *
 * Side effects:
 *	If successful, an array of objects is returned in the object buffer
 *	and an array of object IDs in the object ID buffer (if not NIL),
 *	and an array of application's object numbers in the application object
 *	num buffer (if not NIL).
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
Recov_ReturnObjects(typeID, olengthPtr, obuffer, ilengthPtr, ibuffer,
	alengthPtr, abuffer)
    int		typeID;		/* Type of objects to return. */
    int		*olengthPtr;	/* IN/OUT: length of object buffer. */
    char	*obuffer;	/* OUT: Buffer to put objects into. */
    int		*ilengthPtr;	/* IN/OUT: length of object ID buffer. */
    char	*ibuffer;	/* OUT: Buffer to put object IDs into. */
    int		*alengthPtr;	/* IN/OUT: length of appl. obj. num buffer. */
    char	*abuffer;	/* OUT: Buffer to put appl. obj. nums into. */
{
    ObjInfo		*objInfoAddr;
    char		*addr;
    RestartTypeInfo	*restartTypeInfoPtr;
    int			i;
    int			destNum;
    int			osizeNeeded;
    int			isizeNeeded = 0;
    int			asizeNeeded = 0;

    LOCK_MONITOR;

    if (!recov_Transparent) {
	printf("Recov_ReturnObjects: no transparent recovery.\n");
	UNLOCK_MONITOR;
	return FAILURE;
    }
    restartTypeInfoPtr =
	    &(restartTablePtr->contents[typeID]);
    if (typeID < 0 || typeID >= restartTablePtr->nextTypeID) {
	printf("Recov_ReturnObjects: bad typeID.\n");
	UNLOCK_MONITOR;
	return FAILURE;
    }
    if (restartTypeInfoPtr->objectSize <= 0) {
	printf("Recov_ReturnObjects: bad object type.\n");
	UNLOCK_MONITOR;
	return FAILURE;
    }
    osizeNeeded = restartTypeInfoPtr->currentNum *
	    restartTypeInfoPtr->objectSize;
    if (*olengthPtr < osizeNeeded) {
	printf("Recov_ReturnObjects: not enough space in object buffer.\n");
	*olengthPtr = osizeNeeded;
	UNLOCK_MONITOR;
	return FAILURE;
    }
    if (ibuffer != (char *) NIL) {
	isizeNeeded = restartTypeInfoPtr->currentNum *
		sizeof (Recov_ObjectID);
	if (*ilengthPtr < isizeNeeded) {
	    printf("Recov_ReturnObjects: not enough space in id buffer.\n");
	    *ilengthPtr = isizeNeeded;
	    UNLOCK_MONITOR;
	    return FAILURE;
	}
    }
    if (abuffer != (char *) NIL) {
	asizeNeeded = restartTypeInfoPtr->currentNum * sizeof (int);
	if (*alengthPtr < asizeNeeded) {
	printf("Recov_ReturnObjects: not enough space in appl. num buffer.\n");
	    *alengthPtr = asizeNeeded;
	    UNLOCK_MONITOR;
	    return FAILURE;
	}
    }
    bzero(obuffer, *olengthPtr);	/* Easier debugging if all bzeroed. */
    *olengthPtr = osizeNeeded;
    if (ibuffer != (char *) NIL) {
	bzero(ibuffer, *ilengthPtr);
	*ilengthPtr = isizeNeeded;
    }
    if (abuffer != (char *) NIL) {
	bzero(abuffer, *alengthPtr);
	*alengthPtr = asizeNeeded;
    }


    objInfoAddr = restartTypeInfoPtr->objInfoAddr;
    destNum = 0;
    addr = restartTypeInfoPtr->objectsAddr;
    for (i = 0; i < restartTypeInfoPtr->maxNumObjects; i++) {
	if (objInfoAddr[i].applObjNum != 0) {
	    
	    if (restartTypeInfoPtr->Checksum != (unsigned short (*)()) NIL) {
		volatile unsigned short	sum;
		/* Volatile should change when we check the checksum!!! */

		sum = (*(restartTypeInfoPtr->Checksum))
			(restartTypeInfoPtr->objectSize, (Address) addr);
	    }

	    /* copy object */
	    bcopy(addr, obuffer + (destNum * restartTypeInfoPtr->objectSize),
		    restartTypeInfoPtr->objectSize);
	    if (ibuffer != (char *) NIL) {
		((Recov_ObjectID *) ibuffer)[destNum].typeID = typeID;
		((Recov_ObjectID *) ibuffer)[destNum].objectNumber = i;
	    }
	    if (abuffer != (char *) NIL) {
		((int *) abuffer)[destNum] = objInfoAddr[i].applObjNum;
	    }
	    destNum++;
	}
	addr += restartTypeInfoPtr->objectSize;
    }
    if (destNum > restartTypeInfoPtr->currentNum) {
	UNLOCK_MONITOR;
	panic("Recov_ReturnObjects: miscalculation of number of objects.");
    }

    UNLOCK_MONITOR;
    return SUCCESS;
}

/*
 *----------------------------------------------------------------------
 *
 * Recov_ReturnContents --
 *
 *	Return the table of contents.
 *
 * Results:
 *	SUCCESS or FAILURE.  The procedure can return failure if not enough
 *	buffer space is passed to it.
 *
 * Side effects:
 *	If successful, the table of contents is copied into the out param.
 *	The lengthPtr is filled in with the needed length of the buffer on
 *	both SUCCESS and FAILURE.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
Recov_ReturnContents(lengthPtr, buffer)
    int		*lengthPtr;	/* IN/OUT: length of buffer. */
    char	*buffer;	/* OUT: Buffer to put table into. */
{
    int			i;
    int			sizeNeeded;
    Recov_ContentsEntry *bufPtr;
    RestartTypeInfo	*restartTypeInfoPtr;

    LOCK_MONITOR;

    if (!recov_Transparent) {
	printf("Recov_ReturnContents: no transparent recovery.\n");
	UNLOCK_MONITOR;
	return FAILURE;
    }
    for (i = 0; i < NUM_RESTART_TYPES; i++) {
	if (restartTablePtr->contents[i].objectSize <= 0) {
	    break;
	}
    }
    sizeNeeded = i * sizeof (Recov_ContentsEntry);
    if (sizeNeeded > *lengthPtr) {
	*lengthPtr = sizeNeeded;
	printf("Recov_ReturnContents: not enough buffer space.\n");
	UNLOCK_MONITOR;
	return FAILURE;
    }
    *lengthPtr = sizeNeeded;

    bufPtr = (Recov_ContentsEntry *) buffer;
    for (i = 0; i < NUM_RESTART_TYPES; i++) {
	if (restartTablePtr->contents[i].objectSize <= 0) {
	    break;
	}
	restartTypeInfoPtr = &(restartTablePtr->contents[i]);
	bufPtr->objectSize = restartTypeInfoPtr->objectSize;
	bufPtr->maxNumObjects = restartTypeInfoPtr->maxNumObjects;
	bufPtr->applicationTypeID =
		restartTypeInfoPtr->applicationTypeID;
	bufPtr->currentNum = restartTypeInfoPtr->currentNum;
	bufPtr->firstFree = restartTypeInfoPtr->firstFree;
	bufPtr++;
    }


    UNLOCK_MONITOR;
    return SUCCESS;
}

/*
 *----------------------------------------------------------------------
 *
 * Recov_NumObjects --
 *
 *	Return the current number of objects for given type.
 *
 * Results:
 *	The number, or -1 if something went wrong.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
int
Recov_NumObjects(typeID)
    int		typeID;	/* IN: type of object to count. */
{
    LOCK_MONITOR;

    if (!recov_Transparent) {
	printf("Recov_NumObjects: no transparent recovery.\n");
	UNLOCK_MONITOR;
	return -1;
    }
    if (typeID < 0 || typeID >= restartTablePtr->nextTypeID) {
	return -1;
    }

    UNLOCK_MONITOR;
    return restartTablePtr->contents[typeID].currentNum;
}


/*
 *----------------------------------------------------------------------
 *
 * Recov_MapType --
 *
 *	Return the real type used by the kernel given the application's
 *	type number;
 *
 * Results:
 *	SUCCESS or FAILURE.  The procedure can return failure no such
 *	application type is found.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
Recov_MapType(applicationTypeID, typeIDPtr)
    int		applicationTypeID;	/* IN: Application's type number. */
    int		*typeIDPtr;		/* OUT: Buffer to put type into. */
{
    int		i;

    LOCK_MONITOR;
    if (!recov_Transparent) {
	printf("Recov_MapType: no transparent recovery.\n");
	UNLOCK_MONITOR;
	return FAILURE;
    }
    for (i = 0; i < restartTablePtr->nextTypeID; i++) {
	if (restartTablePtr->contents[i].applicationTypeID ==
		applicationTypeID) {
	    *typeIDPtr = i;
	    UNLOCK_MONITOR;
	    return SUCCESS;
	}
    }
    *typeIDPtr = -1;

    UNLOCK_MONITOR;
    return FAILURE;
}

/*
 *----------------------------------------------------------------------
 *
 * Recov_MapObjectNum --
 *
 *	Return the real object number used by the kernel given the application's
 *	object number.
 *
 * Results:
 *	SUCCESS or FAILURE.  The procedure can return failure no such
 *	application type or object number are found.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
Recov_MapObjectNum(typeID, applicationObjectNum, objectNumPtr)
    int		typeID;			/* The real type ID of the object. */
    int		applicationObjectNum;	/* IN: Application's object num. */
    int		*objectNumPtr;		/* OUT: Buffer to put obj num into. */
{
    ObjInfo	*objInfoAddr;
    int		i;

    if (!recov_Transparent) {
	printf("Recov_MapObjectNum: no transparent recovery.\n");
	UNLOCK_MONITOR;
	return FAILURE;
    }
    if (typeID < 0 || typeID >= restartTablePtr->nextTypeID) {
	*objectNumPtr = -1;
	return FAILURE;
    }
    LOCK_MONITOR;
    objInfoAddr = restartTablePtr->contents[typeID].objInfoAddr;
    for (i = 0; i < restartTablePtr->contents[typeID].maxNumObjects; i++) {
	if (objInfoAddr[i].applObjNum == applicationObjectNum) {
	    *objectNumPtr = i;
	    UNLOCK_MONITOR;
	    return SUCCESS;
	}
    }

    *objectNumPtr = -1;
    UNLOCK_MONITOR;
    return FAILURE;
}


/*
 *----------------------------------------------------------------------
 *
 * Recov_Checksum --
 *
 *	Compute the 16-bit one's complement of the 1's complement sum of
 *	of all words in the buffer.
 *
 *	Note: It is assumed that the length of the buffer is at most
 *	128K bytes long. It also helps if the buffer is word-aligned.
 *
 * Results:
 *	The 1's complement checksum in network byte-order.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
unsigned short
Recov_Checksum(len, bufPtr)
    register int len;		/* The number of bytes to checksum. */
    Address bufPtr;		/* What to checksum. */
{
    register unsigned short *wordPtr = (unsigned short *) bufPtr;
    register unsigned int sum = 0;

    
    /*
     * The basic algorithm 16-bit 1's complement addition is 
     *  1) add the two unsigned 16-bit quantities, 
     *  2) if there was a carry out of the high-order bit, 
     *	   it is added to the sum.
     * To detect a carry out of the high-order bit, the sum is stored
     * in a 32-bit word. As an optimization, we delay step 2 until
     * all the words have been added together. At that point, the
     * upper-half of the sum contains the sum of the carries from the
     * additions. This value is then added to the lower half and if that
     * operation causes a carry, then 1 is added to the sum.
     *
     * The optimization does place a limit on how many bytes can be
     * summed without causing an overflow of the 32-bit sum. In the worst
     * case, a maximum of 64K additions of 16-bit values can be added
     * without overflow.
     * 
     * The summation is done in an unrolled loop. Once we have less than 
     * 32 bytes to sum then it must be done in smaller loops.
     */

    while (len >= 32) {
	sum += *wordPtr++;
	sum += *wordPtr++;
	sum += *wordPtr++;
	sum += *wordPtr++;

	sum += *wordPtr++;
	sum += *wordPtr++;
	sum += *wordPtr++;
	sum += *wordPtr++;

	sum += *wordPtr++;
	sum += *wordPtr++;
	sum += *wordPtr++;
	sum += *wordPtr++;

	sum += *wordPtr++;
	sum += *wordPtr++;
	sum += *wordPtr++;
	sum += *wordPtr++;

	len -= 32;
    }
    while (len >= 2) {
	sum += *wordPtr++;
	len -= 2;
    }

    if (len == 1) {
#if BYTE_ORDER == LITTLE_ENDIAN
	sum += (*wordPtr) & 0x00ff;
#else
	sum += (*wordPtr) & 0xff00;
#endif
    }

    /*
     * The most signficant bits of "sum" contains the carries from
     * the overflow of the summing. Add this overflow back into
     * the least significant 16 bits of the sum and do it a second
     * time in case there's a carry from the first time.
     */
    if (sum > 0xffff) {

	sum = ((sum >> 16) & 0xffff) + (sum & 0xffff);
	/*
	 * See if there was a carry from the addition. The overflow will
	 * be at most 1.
	 */
	if (sum > 0xffff) {
	    sum++;
	}
    }

    return((~sum & 0xffff));
}

/*
 *----------------------------------------------------------------------
 *
 * Recov_Cmd --
 *
 *	Process user system call for doing recovery box operations.
 *	This is called via Sys_StatsStub and therefore has the same
 *	interface and user address pointers passed in.
 *
 * Results:
 *	SUCCESS or FAILURE.
 *
 * Side effects:
 *	May modify contents of recovery box.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
Recov_Cmd(option, argPtr)
    int		option;
    Address	argPtr;
{
    Recov_InitObjTypeArgs	initArg;
    Recov_InsertObjArgs		insertArg;
    Recov_InsertArrayArgs	insertArrayArg;
    Recov_DeleteObjArgs		deleteArg;
    Recov_UpdateObjArgs		updateArg;
    Recov_ReturnObjArgs		returnArg;
    Recov_ReturnArrayArgs	returnArrayArg;
    Recov_ReturnTableArgs	returnContentsArg;
    Recov_GetObjectSizeArgs	getObjectSizeArg;
    Recov_MapTypeArgs		mapTypeArg;
    Recov_MapObjectNumArgs	mapObjNumArg;
    ReturnStatus		status = SUCCESS;

    if (option != RECOV_TOGGLE_CHECKSUM &&
	    option != RECOV_PRINT_REBOOT_TIMES && option != RECOV_BULK_REOPEN
	    && option != RECOV_SINGLE_REOPEN &&
	    option != RECOV_DO_SERVER_DRIVEN &&
	    option != RECOV_NO_SERVER_DRIVEN &&
	    (argPtr == (Address) NIL || argPtr == (Address) 0 ||
	    argPtr == (Address) USER_NIL)) {
	return GEN_INVALID_ARG;
    }
    /* option is the command */
    switch (option) {
    case RECOV_INIT_OBJ_TYPE: {
	status = Vm_CopyIn(sizeof (initArg), argPtr,
		(Address) &initArg);
	if (status != SUCCESS) {
	    break;
	}
	if (initArg.doChecksum != 0 && initArg.doChecksum != 1) {
	    status = GEN_INVALID_ARG;
	    break;
	}
	status = Recov_InitType(initArg.objectSize,
		initArg.maxNumObjects, initArg.applicationTypeID,
		&(initArg.objectType),
		(unsigned short (*)())initArg.doChecksum);
	if (status != SUCCESS) {
	    break;
	}
	status = Vm_CopyOut(sizeof (int),
		(Address) &(initArg.objectType),
		(Address) &(((Recov_InitObjTypeArgs *)
		argPtr)->objectType));
	break;
    }
    case RECOV_INSERT_OBJ: {
	char	*objectPtr;
	int	objectSize;

	status = Vm_CopyIn(sizeof (insertArg), argPtr,
		(Address) &insertArg);
	if (status != SUCCESS) {
	    break;
	}
	if ((insertArg.objectPtr == (Address) NIL ||
		insertArg.objectPtr == (Address) 0 ||
		insertArg.objectPtr == (Address) USER_NIL)) {
	    status = GEN_INVALID_ARG;
	    break;
	}

	objectSize = Recov_GetObjectSize(insertArg.typeID);
	if (objectSize == -1) {
	    status = GEN_INVALID_ARG;
	    break;
	}
	objectPtr = malloc(objectSize);

	status = Vm_CopyIn(objectSize, insertArg.objectPtr,
		(Address) objectPtr);
	if (status != SUCCESS) {
	    free(objectPtr);
	    break;
	}
	status = Recov_InsertObject(insertArg.typeID,
		(ClientData) objectPtr, insertArg.applicationObjectNum,
		&(insertArg.objectID));
	if (status != SUCCESS) {
	    free(objectPtr);
	    break;
	}
	status = Vm_CopyOut(sizeof (Recov_ObjectID),
		(Address) &(insertArg.objectID),
		(Address) &(((Recov_InsertObjArgs *)
		argPtr)->objectID));
	free(objectPtr);
	break;
    }
    case RECOV_INSERT_ARRAY: {
	char		*objectBuffer;
	int		objectSize;
	int		*objNumBuffer;
	Recov_ObjectID	*objIDBuffer;
	int		numObjs;

	status = Vm_CopyIn(sizeof (insertArrayArg), argPtr,
		(Address) &insertArrayArg);
	if (status != SUCCESS) {
	    break;
	}
	if (insertArrayArg.obuffer == (Address) NIL ||
		insertArrayArg.obuffer == (Address) 0 ||
		insertArrayArg.obuffer == (Address) USER_NIL) {
	    status = GEN_INVALID_ARG;
	    break;
	}
	objectSize = Recov_GetObjectSize(insertArrayArg.typeID);
	if (objectSize == -1) {
	    status = GEN_INVALID_ARG;
	    break;
	}
	numObjs = insertArrayArg.numObjs;
	objectBuffer = malloc(objectSize * numObjs);
	status = Vm_CopyIn(objectSize * numObjs,
		(Address) insertArrayArg.obuffer,
		(Address) objectBuffer);
	if (status != SUCCESS) {
	    free(objectBuffer);
	    break;
	}
	if (insertArrayArg.applObjNums == (int *) NIL ||
		insertArrayArg.applObjNums == (int *) 0 ||
		insertArrayArg.applObjNums == (int *) USER_NIL) {
	    objNumBuffer = (int *) NIL;
	} else {
	    objNumBuffer = (int *) malloc(sizeof (int) * numObjs);
	    status = Vm_CopyIn(sizeof (int) * numObjs,
		    (Address) insertArrayArg.applObjNums,
		    (Address) objNumBuffer);
	    if (status != SUCCESS) {
		free(objectBuffer);
		free((char *) objNumBuffer);
		break;
	    }
	}
	if (insertArrayArg.objIDBuffer == (Recov_ObjectID *) NIL ||
		insertArrayArg.objIDBuffer == (Recov_ObjectID *) 0 ||
		insertArrayArg.objIDBuffer ==
		(Recov_ObjectID *) USER_NIL) {
	    status = GEN_INVALID_ARG;
	    free(objectBuffer);
	    if (objNumBuffer != (int *) NIL) {
		free((char *) objNumBuffer);
	    }
	    break;
	} else {
	    objIDBuffer = (Recov_ObjectID *)
		    malloc(sizeof (Recov_ObjectID) * numObjs);
	}
	status = Recov_InsertObjects(insertArrayArg.typeID, numObjs,
		objectBuffer, objNumBuffer, objIDBuffer);
	if (status != SUCCESS) {
	    free(objectBuffer);
	    if (objNumBuffer != (int *) NIL) {
		free((char *) objNumBuffer);
	    }
	    free((char *) objIDBuffer);
	    break;
	}
	status = Vm_CopyOut(sizeof (Recov_ObjectID) * numObjs,
		(Address) objIDBuffer,
		(Address) insertArrayArg.objIDBuffer);
	free((char *) objectBuffer);
	if (objNumBuffer != (int *) NIL) {
	    free((char *) objNumBuffer);
	}
	free((char *) objIDBuffer);
	break;
    }
    case RECOV_DELETE_OBJ: {
	status = Vm_CopyIn(sizeof (deleteArg), argPtr,
		(Address) &deleteArg);
	if (status != SUCCESS) {
	    break;
	}
	status = Recov_DeleteObject(deleteArg.objectID);
	break;
    }
    case RECOV_UPDATE_OBJ: {
	char	*objectPtr;
	int	objectSize;

	status = Vm_CopyIn(sizeof (updateArg), argPtr,
		(Address) &updateArg);
	if (status != SUCCESS) {
	    break;
	}
	if ((updateArg.objectPtr == (Address) NIL ||
		updateArg.objectPtr == (Address) 0 ||
		updateArg.objectPtr == (Address) USER_NIL)) {
	    status = GEN_INVALID_ARG;
	    break;
	}

	objectSize = Recov_GetObjectSize(updateArg.objectID.typeID);
	if (objectSize == -1) {
	    status = GEN_INVALID_ARG;
	    break;
	}
	objectPtr = malloc(objectSize);

	status = Vm_CopyIn(objectSize, updateArg.objectPtr,
		(Address) objectPtr);
	if (status != SUCCESS) {
	    free(objectPtr);
	    break;
	}
	status = Recov_UpdateObject((ClientData) objectPtr,
		updateArg.objectID);
	free(objectPtr);
	break;
    }
    case RECOV_RETURN_OBJ: {
	char	*objectPtr;
	int	objectSize;

	status = Vm_CopyIn(sizeof (returnArg), argPtr,
		(Address) &returnArg);
	if (status != SUCCESS) {
	    break;
	}
	if (returnArg.objectPtr == (Address) NIL ||
		returnArg.objectPtr == (Address) 0 ||
		returnArg.objectPtr == (Address) USER_NIL) {
	    status = GEN_INVALID_ARG;
	    break;
	}

	objectSize = Recov_GetObjectSize(returnArg.objectID.typeID);
	if (objectSize == -1) {
	    status = GEN_INVALID_ARG;
	    break;
	}
	objectPtr = malloc(objectSize);

	status = Recov_ReturnObject((ClientData) objectPtr,
		returnArg.objectID, TRUE);
	if (status != SUCCESS) {
	    free(objectPtr);
	    break;
	}
	status = Vm_CopyOut(objectSize, (Address) objectPtr,
		(Address) ((Recov_ReturnObjArgs *) argPtr)->objectPtr);
	free(objectPtr);
		
	break;
    }
    case RECOV_RETURN_ARRAY: {
	char	*newobuffer;
	char	*newibuffer;
	char	*newabuffer;

	status = Vm_CopyIn(sizeof (returnArrayArg), argPtr,
		(Address) &returnArrayArg);
	if (status != SUCCESS) {
	    break;
	}
	if ((returnArrayArg.obuffer == (Address) NIL ||
		returnArrayArg.obuffer == (Address) 0 ||
		returnArrayArg.obuffer == (Address) USER_NIL)) {
	    status = GEN_INVALID_ARG;
	    break;
	}
	newobuffer = malloc(returnArrayArg.olength);

	if ((returnArrayArg.ibuffer == (Address) NIL ||
		returnArrayArg.ibuffer == (Address) 0 ||
		returnArrayArg.ibuffer == (Address) USER_NIL)) {
	    returnArrayArg.ibuffer = (Address) NIL;
	    newibuffer = (char *) NIL;
	} else {
	    if (returnArrayArg.ilength <= 0) {
		free(newobuffer);
		status = GEN_INVALID_ARG;
		break;
	    }
	    newibuffer = malloc(returnArrayArg.ilength);
	}
	    
	if ((returnArrayArg.abuffer == (Address) NIL ||
		returnArrayArg.abuffer == (Address) 0 ||
		returnArrayArg.abuffer == (Address) USER_NIL)) {
	    returnArrayArg.abuffer = (Address) NIL;
	    newabuffer = (char *) NIL;
	} else {
	    if (returnArrayArg.alength <= 0) {
		free(newobuffer);
		if (newibuffer != (char *) NIL) {
		    free(newibuffer);
		}
		status = GEN_INVALID_ARG;
		break;
	    }
	    newabuffer = malloc(returnArrayArg.alength);
	}
		
	status = Recov_ReturnObjects(returnArrayArg.typeID,
		&returnArrayArg.olength, newobuffer,
		&returnArrayArg.ilength,
		newibuffer, &returnArrayArg.alength, newabuffer);
	if (status != SUCCESS) {
	    free(newobuffer);
	    if (newibuffer != (char *) NIL) {
		free(newibuffer);
	    }
	    if (newabuffer != (char *) NIL) {
		free(newabuffer);
	    }
	    break;
	}
	status = Vm_CopyOut(sizeof (int),
		(Address) &returnArrayArg.olength,
		(Address) &(((Recov_ReturnArrayArgs *)
		argPtr)->olength));
	if (status != SUCCESS) {
	    free(newobuffer);
	    if (newibuffer != (char *) NIL) {
		free(newibuffer);
	    }
	    if (newabuffer != (char *) NIL) {
		free(newabuffer);
	    }
	    break;
	}
	status = Vm_CopyOut(returnArrayArg.olength,
		(Address) newobuffer,
		(Address) (((Recov_ReturnArrayArgs *)
		argPtr)->obuffer));

	if (status != SUCCESS) {
	    free(newobuffer);
	    if (newibuffer != (char *) NIL) {
		free(newibuffer);
	    }
	    if (newabuffer != (char *) NIL) {
		free(newabuffer);
	    }
	    break;
	}
	if (newibuffer != (char *) NIL) {
	    status = Vm_CopyOut(sizeof (int),
		    (Address) &returnArrayArg.ilength, (Address)
		    &(((Recov_ReturnArrayArgs *) argPtr)->ilength));
	    if (status != SUCCESS) {
		free(newobuffer);
		free(newibuffer);
		if (newabuffer != (char *) NIL) {
		    free(newabuffer);
		}
		break;
	    }
	    status = Vm_CopyOut(returnArrayArg.ilength,
		    (Address) newibuffer,
		    (Address)
		    (((Recov_ReturnArrayArgs *) argPtr)->ibuffer));
	    if (status != SUCCESS) {
		free(newobuffer);
		free(newibuffer);
		if (newabuffer != (char *) NIL) {
		    free(newabuffer);
		}
		break;
	    }
	}
	if (newabuffer != (char *) NIL) {
	    status = Vm_CopyOut(sizeof (int),
		    (Address) &returnArrayArg.alength, (Address)
		    &(((Recov_ReturnArrayArgs *) argPtr)->alength));
	    if (status != SUCCESS) {
		free(newobuffer);
		if (newibuffer != (char *) NIL) {
		    free(newibuffer);
		}
		free(newabuffer);
		break;
	    }
	    status = Vm_CopyOut(returnArrayArg.alength,
		    (Address) newabuffer,
		    (Address)
		    (((Recov_ReturnArrayArgs *) argPtr)->abuffer));
	}

	free(newobuffer);
	if (newibuffer != (char *) NIL) {
	    free(newibuffer);
	}
	if (newabuffer != (char *) NIL) {
	    free(newabuffer);
	}
	break;
    }
    case RECOV_RETURN_CONTENTS: {
	char	*newbuffer;
	int	length;

	status = Vm_CopyIn(sizeof (returnContentsArg), argPtr,
		(Address) &returnContentsArg);
	if (status != SUCCESS) {
	    break;
	}
	if ((returnContentsArg.buffer == (Address) NIL ||
		returnContentsArg.buffer == (Address) 0 ||
		returnContentsArg.buffer == (Address) USER_NIL)) {
	    status = GEN_INVALID_ARG;
	    break;
	}
	length = returnContentsArg.length;
	newbuffer = malloc(length);
	status = Recov_ReturnContents(&length, newbuffer);
	if (status != SUCCESS) {
	    free(newbuffer);
	    break;
	}
	status = Vm_CopyOut(sizeof (int), (Address) &length,
		(Address) &(((Recov_ReturnTableArgs *)
		argPtr)->length));
	if (status != SUCCESS) {
	    free(newbuffer);
	    break;
	}
	status = Vm_CopyOut(length, (Address) newbuffer,
		(Address) (((Recov_ReturnTableArgs *) argPtr)->buffer));
	break;
    }
    case RECOV_GET_OBJECT_SIZE: {
	status = Vm_CopyIn(sizeof (getObjectSizeArg), argPtr,
		(Address) &getObjectSizeArg);
	if (status != SUCCESS) {
	    break;
	}

	getObjectSizeArg.objectSize =
		Recov_GetObjectSize(getObjectSizeArg.typeID);
	if (getObjectSizeArg.objectSize == -1) {
	    status = GEN_INVALID_ARG;
	    break;
	}
	status = Vm_CopyOut(sizeof (getObjectSizeArg),
		(Address) &getObjectSizeArg, argPtr);
	break;
    }
    case RECOV_MAP_TYPE: {
	status = Vm_CopyIn(sizeof (mapTypeArg), argPtr,
		(Address) &mapTypeArg);
	if (status != SUCCESS) {
	    break;
	}
	status = Recov_MapType(mapTypeArg.applicationTypeID,
		&(mapTypeArg.typeID));
	if (status != SUCCESS) {
	    break;
	}
	status = Vm_CopyOut(sizeof (mapTypeArg),
		(Address) &mapTypeArg, argPtr);
	break;
    }
    case RECOV_MAP_OBJ_NUM: {
	status = Vm_CopyIn(sizeof (mapObjNumArg), argPtr,
		(Address) &mapObjNumArg);
	if (status != SUCCESS) {
	    break;
	}
	status = Recov_MapObjectNum(mapObjNumArg.realTypeID,
		mapObjNumArg.applicationObjectNum,
		&(mapObjNumArg.realObjectNum));
	if (status != SUCCESS) {
	    break;
	}
	status = Vm_CopyOut(sizeof (mapObjNumArg),
		(Address) &mapObjNumArg, argPtr);
	break;
    }
    case RECOV_TOGGLE_CHECKSUM: {
	Recov_ToggleChecksum((int) argPtr);
	status = SUCCESS;
	break;
    }
    case RECOV_BULK_REOPEN: {
	recov_BulkHandles = TRUE;
	status = SUCCESS;
	break;
    }
    case RECOV_SINGLE_REOPEN: {
	recov_BulkHandles = FALSE;
	status = SUCCESS;
	break;
    }
    case RECOV_IGNORE_CLEAN: {
	if (recov_SkipCleanFiles) {
	    printf("Cannot both skip and ignore reopening clean files.\n");
	    status = FAILURE;
	} else {
	    recov_IgnoreCleanFiles = TRUE;
	    status = SUCCESS;
	}
	break;
    }
    case RECOV_REOPEN_CLEAN: {
	recov_IgnoreCleanFiles = FALSE;
	recov_SkipCleanFiles = FALSE;
	status = SUCCESS;
	break;
    }
    case RECOV_SKIP_CLEAN: {
	if (recov_IgnoreCleanFiles) {
	    printf("Cannot both skip and ignore reopening clean files.\n");
	    status = FAILURE;
	} else {
	    recov_SkipCleanFiles = TRUE;
	    status = SUCCESS;
	}
	break;
    }
    case RECOV_DO_SERVER_DRIVEN: {
	recov_ClientIgnoreServerDriven = FALSE;
	printf("Client is now responding to server-driven recovery.\n");
	status = SUCCESS;
	break;
    }
    case RECOV_NO_SERVER_DRIVEN: {
	recov_ClientIgnoreServerDriven = TRUE;
	printf("Client is now ignoring server-driven recovery.\n");
	status = SUCCESS;
	break;
    }
    case RECOV_PRINT_SIZE: {
	int	objSize;

	objSize = (int) argPtr;
	Recov_PrintSpace(objSize);
	status = SUCCESS;
	break;
    }
    default:
	status = GEN_INVALID_ARG;
	break;
    }

    return status;
}
