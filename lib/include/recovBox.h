/*
 * recovBox.h --
 *
 *	External definitions needed by users of the fast recovery mechanism.
 *
 * Copyright (C) 1985 Regents of the University of California
 * All rights reserved.
 *
 *
 * $Header: /sprite/src/lib/include/RCS/recovBox.h,v 1.13 92/09/21 11:55:42 mgbaker Exp $ SPRITE (Berkeley)
 */

#include <sysStats.h>

#ifndef _RECOV_BOX
#define _RECOV_BOX

/*
 * Parameters to Sys_StatsStub interface (should turn into system call
 * interface!!!!) to the recovery box.  Only values from 0 to 99 may be
 * used by the recovery box and associated functions.  The higher numbers
 * are used in the kernel recovery module and defined in recov.h.
 */

#define	RECOV_INIT_OBJ_TYPE		0
#define	RECOV_INSERT_OBJ		1
#define	RECOV_INSERT_ARRAY		2
#define	RECOV_DELETE_OBJ		3
#define	RECOV_UPDATE_OBJ		4
#define	RECOV_RETURN_OBJ		5
#define	RECOV_RETURN_ARRAY		6
#define	RECOV_RETURN_CONTENTS		7
#define	RECOV_GET_OBJECT_SIZE		8
#define RECOV_MAP_TYPE			9
#define RECOV_MAP_OBJ_NUM		10
#define RECOV_TEST_ADD_DELETE		11
#define RECOV_PRINT_REBOOT_TIMES	12
#define RECOV_PRINT_SIZE		13
#define RECOV_TOGGLE_CHECKSUM		14
#define RECOV_TEST_ADD			15

/*
 * For testing and timings.  Used with RECOV_TEST_ADD_DELETE test.
 */
typedef struct Recov_Timings {
    int		iterations;
    int		numHandles;
} Recov_Timings;

/*
 * An object's ID consists of its type number followed by its ID for that type.
 */
typedef	struct	Recov_ObjectID {
    int		typeID;
    int		objectNumber;
} Recov_ObjectID;

/*
 * There is one entry in the table of contents for each object type.  Each
 * entry states the size of the objects for that type, the maximum possible
 * number of objects for that type, and the current number for that type.
 * The index in the table of contents of this entry gives the number of the
 * object type.  There is also a field called "applicationTypeID" used by
 * applications to access this type across crashes without needing to know
 * the order in which types were allocated.  The application can hard-code
 * this number and use it to get the kernel's typeID for this type upon
 * restart.
 */
typedef struct	Recov_ContentsEntry {
    int			objectSize;		/* Size of these objects. */
    int			applicationTypeID;	/* Used by application. */
    int			maxNumObjects;		/* Max num of these objects. */
    int			currentNum;		/* Current num allocated. */
    int			firstFree;		/* First index free list. */
} Recov_ContentsEntry;

/*
 **************************************************************************
 * The following structures are all arguments to the interface to maniuplate
 * objects in the recovery box.
 **************************************************************************
 */

/*
 * Information necessary to create a new object type.  All the objects will
 * be of the same size, and you must specify the maximum number that will
 * ever exist at once.  The applicationTypeID, described above, is passed
 * in via this structure.
 */
typedef struct Recov_InitObjTypeArgs {
    int                 objectSize;	/* Size of new objects. */
    int                 maxNumObjects;	/* Maximum number of objects allowed. */
    int                 objectType;     /* Out field. */
    int			applicationTypeID;	/* Application's ID to map to
						 * to this type. Can be unset
						 * (<= 0) or set (> 0).  If set,
						 * an error will be  returned
						 * if it's not unique. */
    int			doChecksum;	/* If 1, do checksum.  If 0, don't. */
} Recov_InitObjTypeArgs;

/*
 * Information necessary to insert a new object.  The field "objectPtr"
 * points to contents of the new object to insert.
 */
typedef struct Recov_InsertObjArgs {
    int                 typeID;		/* Type number of this object. */
    Address             objectPtr;	/* In field: Ptr to object to insert. */
    Recov_ObjectID      objectID;       /* Out field: new object's ID. */
    int			applicationObjectNum;
					/* In field: Num that application
					 * can use to get real objectID.
					 * This is so that different
					 * applications can point at
					 * the same object without knowing the
					 * order of inserts, etc.  Also, the
					 * application can guarantee the
					 * uniqueness of the number.  Can be
					 * unset (<= 0) or set (>0).  If set,
					 * an error will be returned if there's
					 * already an object of the same type
					 * with this object ID.  */
} Recov_InsertObjArgs;

/*
 * Information necessary to insert a set of new objects.  This is a way
 * of inserting more than one object at a time without interference from
 * other processes doing similar object inserts.  If the routine returns
 * with SUCCESS, then all the objects were inserted.  If it returns FAILURE,
 * then no objects were inserted.  If it panics, the results are undefined.
 */
typedef struct Recov_InsertArrayArgs {
    int                 typeID;		/* Type number of this object. */
    int			numObjs;	/* Number of objects to insert. */
    char		*obuffer;	/* In field: objects to insert. */
    int			*applObjNums;	/* In field: array of application's
					 * object numbers for objects.  May
					 * be NIL. */
    Recov_ObjectID	*objIDBuffer;	/* Out field: array of new objects'
					 * object IDs. */
} Recov_InsertArrayArgs;

/*
 * Information necessary to delete an object.
 */
typedef struct Recov_DeleteObjArgs {
    Recov_ObjectID      objectID; 	/* ID of the object to delete. */
} Recov_DeleteObjArgs;

/*
 * Information necessary to update an object.  The field "objectPtr" points
 * to the new version of the object.
 */
typedef struct Recov_UpdateObjArgs {
    Address             objectPtr;	/* In field: Ptr to object to insert. */
    Recov_ObjectID      objectID;	/* ID of the object to update. */
} Recov_UpdateObjArgs;

/*
 * Information necessary to return an object.  The field "objectPtr" must
 * point to allocated memory of at least the size of this type of object.
 */
typedef struct Recov_ReturnObjArgs {
    Address             objectPtr;	/* Out: Buffer for returning object. */
    Recov_ObjectID      objectID;	/* ID of the object to delete. */
} Recov_ReturnObjArgs;

/*
 * Information necessary to return all the objects of a given type.
 * There are 2 buffers returned:  the first is the array of objects of the
 * given type, and the second is an array giving the ID's of the objects
 * of the corresponding index in the first buffer.
 * The "length" fields specify the current allocated lengths of the
 * return buffers.  Upon successful return, the "length" fields will
 * specify how much of each buffer was used.  Upon failure, the length 
 * fields will specify the needed space for the buffers if the reason for
 * failure was that they weren't large enough.
 */
typedef	struct Recov_ReturnArrayArgs {
    int			typeID;		/* Type number of objects to return. */ 
    int			olength;	/* In/out field for buffer length. */
    char		*obuffer;	/* Out field for object array. */
    int			ilength;	/* In/out field for id buffer length. */
    char		*ibuffer;	/* Out field for object id array. */
    int			alength;	/* In/out field for appl. obj. nums. */
    char		*abuffer;	/* Out field for appl. obj. numbers. */
} Recov_ReturnArrayArgs;

/*
 * Information necessary to return the table of contents for the recovery box.
 * The field "length" specifies the current allocated length of the
 * return buffer.  Upon successful return, the "length" field will
 * specify how much of that buffer was used.  Upon failure, the length 
 * field will specify the needed space for the buffer if the reason for
 * failure was that it wasn't large enough.
 */
typedef struct Recov_ReturnTableArgs {
    int			length;		/* In/out field for buffer length. */
    char		*buffer;	/* Out field for contents buffer. */
} Recov_ReturnTableArgs;

/*
 * Information necessary to return the size of a given type of object in
 * the box.
 */
typedef	struct Recov_GetObjectSizeArgs {
    int			typeID;		/* In field to give object type. */
    int			objectSize;	/* Out field to return size. */
} Recov_GetObjectSizeArgs;

/*
 * Information necessary to map the application's type ID to the real type
 * ID used by the recovery box.
 */
typedef	struct Recov_MapTypeArgs {
    int			applicationTypeID;	/* In field: object type. */
    int			typeID;			/* Out field: real type. */
} Recov_MapTypeArgs;

/*
 * Information necessary to map the application's object number to the real
 * object number used by the recovery box.
 */
typedef	struct Recov_MapObjectNumArgs {
    int			realTypeID;	/* In: recov box's type for object */
    int			applicationObjectNum;	/* In: Obj numD to map from. */
    int			realObjectNum;		/* Out field: real obj num. */
} Recov_MapObjectNumArgs;


/*
 * Defines to make the system call interface look like a library interface,
 * since the sys call interface uses the gross Sys_Stats stuff.
 */
#define	RecovBox_InitObjType(/* (Recov_InitObjTypeArgs *) */ initArgsPtr) \
    Sys_Stats(SYS_RECOV_BOX, RECOV_INIT_OBJ_TYPE, (Address) initArgsPtr)

#define	RecovBox_InsertObj(/* (Recov_InsertObjArgs *) */ insertArgsPtr)	\
    Sys_Stats(SYS_RECOV_BOX, RECOV_INSERT_OBJ, (Address) insertArgsPtr)

#define	RecovBox_InsertArray(/* (Recov_InsertObjArgs *) */ insertArrayArgsPtr)\
    Sys_Stats(SYS_RECOV_BOX, RECOV_INSERT_ARRAY, (Address) insertArrayArgsPtr)

#define	RecovBox_DeleteObj(/* (Recov_DeleteObjArgs *) */ deleteArgsPtr)	\
    Sys_Stats(SYS_RECOV_BOX, RECOV_DELETE_OBJ, (Address) deleteArgsPtr)

#define	RecovBox_UpdateObj(/* (Recov_UpdateObjArgs *) */ updateArgsPtr)	\
    Sys_Stats(SYS_RECOV_BOX, RECOV_UPDATE_OBJ, (Address) updateArgsPtr)

#define	RecovBox_ReturnObj(/* (Recov_ReturnObjArgs *) */ returnObjArgsPtr) \
    Sys_Stats(SYS_RECOV_BOX, RECOV_RETURN_OBJ, (Address) returnObjArgsPtr)

#define	RecovBox_ReturnArray(/* (Recov_ReturnArrayArgs *) */returnArrayArgsPtr)\
    Sys_Stats(SYS_RECOV_BOX, RECOV_RETURN_ARRAY, (Address) returnArrayArgsPtr)

#define	RecovBox_ReturnContents(/* (Recov_ReturnTableArgs *) */ \
    returnContentsArgsPtr)\
    Sys_Stats(SYS_RECOV_BOX, RECOV_RETURN_CONTENTS,	\
    (Address) returnContentsArgsPtr)

#define RecovBox_GetObjectSize(/* (Recov_GetObjectSizeArgs *) */ \
    getObjectSizeArgsPtr) \
    Sys_Stats(SYS_RECOV_BOX, RECOV_GET_OBJECT_SIZE, \
    (Address) getObjectSizeArgsPtr)

#define RecovBox_MapType(/*(Recov_MapTypeArgs *)*/ mapTypeArgsPtr) \
    Sys_Stats(SYS_RECOV_BOX, RECOV_MAP_TYPE, (Address) mapTypeArgsPtr)

#define RecovBox_MapObjectNum(/*(Recov_MapObjectNumArgs *)*/ \
    mapObjectNumArgsPtr) \
    Sys_Stats(SYS_RECOV_BOX, RECOV_MAP_OBJ_NUM, (Address) mapObjectNumArgsPtr)


#endif /* _RECOV_BOX */
