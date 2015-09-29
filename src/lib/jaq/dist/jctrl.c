/* 
 * jctrl.c --
 *
 *	Perform sysadmin duties on Jaquith robot.
 *
 * Copyright 1991 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *
 * Quote:
 *      "We would like to apologize for the way in which politicians are
 *      represented in this programme.  It was never our intention to imply
 *      that politicians are weak-kneed, political time-servers who are more
 *      concerned with their personal vendettas and private power struggles
 *      than the problems of government, nor to suggest at any point that
 *      they sacrifice their credibility by denying free debate on vital
 *      matters in the mistaken impression that party unity comes before
 *      the well-being of the people they supposedly represent, nor to imply
 *      at any stage that they are squabbling little toadies without an ounce
 *      of concern for the vital social problems of today.  Nor indeed do we
 *      intend that viewers should consider them as crabby ulcerous little
 *      self-seeking vermin with furry legs and an excessive addiction to
 *      alcohol and certain explicit sexual practices which some people might
 *      find offensive.  We are sorry if this impression has come across."
 *      -- Monty Python
 */

#ifndef lint
static char rcsid[] = "$Header: /sprite/lib/forms/RCS/jquery.c,v 1.0 91/01/07 18:02:37 mottsmth Exp $ SPRITE (Berkeley)";
#endif /* not lint */

#include "jaquith.h"
#include "option.h"

static char printBuf[T_MAXSTRINGLEN];

static FILE *memDbg = NULL;   /* stream for memory tracing */

int jDebug;                   /* Internal debugging only */
int syserr = 0;               /* Our personal record of errno */

#define DEF_VOL -1
#define DEF_SLOT -1
#define DEF_SLOT2 -1
#define DEF_DEV ""
#define DEF_VERBOSE 0
#define DEF_CMD ""
#define DEF_MSG ""

#define NUMCMDS 9
#define CMD_INSERT    0x01
#define CMD_REMOVE    0x02
#define CMD_LOAD      0x03
#define CMD_UNLOAD    0x04
#define CMD_OPENDOOR  0x05
#define CMD_DISPMSG   0x06
#define CMD_MOVE      0x07
#define CMD_READLABEL 0x08
#define CMD_LISTVOLS  0x09

#define NEEDVOL   0x01
#define NEEDSLOT  0x02
#define NEEDSLOT2 0x04
#define NEEDDEV   0x08

typedef struct cmd {
    char *name;               /* cmd name */
    int id;                   /* cmd identifier */
    int flags;                /* cmd's required args */
} Cmd;

static void  CheckOptions     _ARGS_ ((Parms *parmsPtr, int *cmdPtr));
static void  InsertVolume     _ARGS_ ((int volId, int slot));
static void  RemoveVolume     _ARGS_ ((int volId));
static void  LoadVolume       _ARGS_ ((int volId, char *dev));
static void  UnloadVolume     _ARGS_ ((int volId, char *dev));
static void  MoveVolume       _ARGS_ ((int srcSlot, int destSlot));
static void  ReadVolLabel     _ARGS_ ((int srcSlot));
static void  OpenDoor         _ARGS_ (());
static void  BuildVolList     _ARGS_ (());
static Cmd  *FindCmdInList    _ARGS_ ((char *cmd));
static int   FindVolumeInList _ARGS_ ((int volId));
static int   FindDeviceInList _ARGS_ ((char *reader));
static int   FindSlotInList   _ARGS_ ((int slot));
static int   PruneDirectory   _ARGS_ ((int minTBuf,int maxTBuf,
				       char *dirPath, char *curPath));

static int robotStream = -1;
static DevConfig *devList = NULL;
static VolConfig *volList= NULL;
static int devCnt = 0;
static int volCnt = 0;

typedef struct parmTag {
    char *cmd;
    int volId;
    int slot;
    int slot2;
    char *dev;
    char *root;
    int verbose;
    char *robot;
    char *devFile;
    char *volFile;
    char *msg;
} Parms;

Parms parms = {
    DEF_CMD,
    DEF_VOL,
    DEF_SLOT,
    DEF_SLOT2,
    DEF_DEV,
    DEF_ROOT,
    DEF_VERBOSE,
    DEF_ROBOT,
    DEF_DEVFILE,
    DEF_VOLFILE,
    DEF_MSG
};

Option optionArray[] = {
    {OPT_STRING, "cmd", (char *)&parms.cmd, "Main command option"},
    {OPT_INT, "vol", (char *)&parms.volId, "Volume id. Required."},
    {OPT_INT, "slot", (char *)&parms.slot, "Volume's home location."},
    {OPT_INT, "slot2", (char *)&parms.slot2, "Destination for move cmd."},
    {OPT_STRING, "dev", (char *)&parms.dev, "Device name."},
    {OPT_STRING, "root", (char *)&parms.root, "root of index tree"},
    {OPT_TRUE, "v", (char *)&parms.verbose, "Verbose mode"},

    {OPT_STRING, "robot", (char *)&parms.robot, "Robot device name"},
    {OPT_STRING, "devfile", (char *)&parms.devFile, "Device config file"},
    {OPT_STRING, "volfile", (char *)&parms.volFile, "Volume config file"},
    {OPT_STRING, "msg", (char *)&parms.msg, "Display message on jukebox"}
};
int numOptions = sizeof(optionArray) / sizeof(Option);


/*
 *----------------------------------------------------------------------
 *
 * jctrl --
 *
 *	Main driver for manual jukebox manipulations
 *
 * Results:
 *	none.
 *
 * Side effects:
 *	none.
 *
 *----------------------------------------------------------------------
 */

int
main(argc, argv)
    int argc;                 /* See Option Array */
    char *argv[];
{
    int retCode;
    int cmd;

/*    memDbg = fopen("jctrl.mem","w"); */
    MEM_CONTROL(8192, memDbg, TRACEMEM+TRACECALLS, 4096);

    argc = Opt_Parse(argc, argv, optionArray, numOptions, 0);

    CheckOptions(&parms, &cmd);

    if ((cmd != CMD_OPENDOOR) &&
	(cmd != CMD_DISPMSG) &&
	(cmd != CMD_LISTVOLS)) {
	/* Obtain list of known volumes */
	Admin_ReadVolConfig(parms.volFile, NULL, &volCnt);
	volList = (VolConfig *)MEM_ALLOC("main",volCnt*sizeof(VolConfig));
	if (Admin_ReadVolConfig(parms.volFile, volList, &volCnt) != T_SUCCESS) {
	    sprintf(printBuf,"Can't read %s: %s\nContinue? [y/n] ",
		    parms.volFile, sys_errlist[syserr]);
	    if (!Utils_GetOk(printBuf)) {
		exit(-1);
	    }
	}

	/* Obtain list of known devices */    
	Admin_ReadDevConfig(parms.devFile, NULL, &devCnt);
	devList = (DevConfig *)MEM_ALLOC("main",devCnt*sizeof(DevConfig));
	if (Admin_ReadDevConfig(parms.devFile, devList, &devCnt) != T_SUCCESS) {
	    sprintf(printBuf,"Can't read %s: %s\nContinue? [y/n] ", 
		    parms.devFile, sys_errlist[syserr]);
	    if (!Utils_GetOk(printBuf)) {
		exit(-1);
	    }
	}
    }

    /* Open robot */
    if (Dev_InitRobot(parms.robot, &robotStream) != T_SUCCESS) {
	sprintf(printBuf,"Couldn't open robot: %s\n", sys_errlist[syserr]);
	Utils_Bailout(printBuf, BAIL_PRINT);
    }

    switch (cmd) {
    case CMD_INSERT:
	InsertVolume(parms.volId, parms.slot);
	break;
    case CMD_REMOVE:
	RemoveVolume(parms.volId);
	break;
    case CMD_LOAD:
	LoadVolume(parms.volId, parms.dev);
	break;
    case CMD_UNLOAD:
	UnloadVolume(parms.volId, parms.dev);
	break;
    case CMD_OPENDOOR:
	OpenDoor();
	break;
    case CMD_MOVE:
	MoveVolume(parms.slot, parms.slot2);
	break;
    case CMD_READLABEL:
	ReadVolLabel(parms.slot);
	break;
    case CMD_LISTVOLS:
	BuildVolList();
	break;
    case CMD_DISPMSG:
	Dev_DisplayMsg(robotStream, parms.msg, 0);
	break;
    default:
	fprintf(stderr,"!! unexpected command: %d\n", cmd);
    }

    if (robotStream != -1) {
	close(robotStream);
    }

    MEM_REPORT("jctrl", ALLROUTINES, SORTBYOWNER);

    return 0;
}



/*
 *----------------------------------------------------------------------
 *
 * InsertVolume --
 *
 *	Add a volume to the physical archive.
 *
 * Results:
 *	none.
 *
 * Side effects:
 *	none.
 *
 *----------------------------------------------------------------------
 */
static void
InsertVolume(volId, slot)
    int volId;                /* volume id */
    int slot;                 /* home location */
{
    VolOwner *volOwnerPtr;

    if ((volCnt > 0) &&
	((slot=FindSlotInList(volId)) != -1)) {
	sprintf(printBuf,"Slot already in use by volume %d. Insert [n/y] ",
		volList[slot].volId);
	if (!Utils_GetOk(printBuf)) {
	    return;
	}
	
    }

    volOwnerPtr = Admin_FindVolOwner(volId, parms.root, "*.arch");
    if (volOwnerPtr->owner != NULL) {
	sprintf(printBuf, "Volume %d already owned by %s. Continue? [y/n] ",
	       volId, volOwnerPtr->owner);
	if (!Utils_GetOk(printBuf)) {
	    return;
	}
    }

    if (Dev_InsertVolume(robotStream, slot) != T_SUCCESS) {
	sprintf(printBuf,"Insertion failed: %s\n", sys_errlist[syserr]);
	Utils_Bailout(printBuf, BAIL_PRINT);
    }

    if (Utils_GetOk("Add volume to free list? ")) {
	if (Admin_PutFreeVol(parms.root,volId) != T_SUCCESS) {
	    fprintf(stderr,"Couldn't add volume to free list: %s\n",
		    sys_errlist[syserr]);
	} else if (parms.verbose) {
	    fprintf(stdout,"Volume added to free list.\n");
	}
    }

    if (parms.verbose) {
	fprintf(stdout,"Volume %d inserted.\n",	volId);
    }
}


/*
 *----------------------------------------------------------------------
 *
 * RemoveVolume --
 *
 *	Remove a volume from the physical archive.
 *
 * Results:
 *	none.
 *
 * Side effects:
 *	none.
 *
 *----------------------------------------------------------------------
 */
static void
RemoveVolume(volId)
    int volId;                /* volume Id */
{
    VolOwner *volOwnerPtr;
    int slot;
    int pruneCnt;
    int len;
    char *dirPath;

    if ((slot=FindVolumeInList(volId)) == -1) {
	sprintf(printBuf,"What slot is volume %d in? (return to abort) ", volId);
	if ((slot=Utils_GetInteger(printBuf, 0, INT_MAX)) < 0) {
	    exit(-1);
	}
    }

    if ((volOwnerPtr=Admin_FindVolOwner(volId, parms.root,"*.arch")) == NULL) {
	sprintf(printBuf,"Couldn't read owner information for volume %d.\n",
		volId);
    } else if (volOwnerPtr->owner != NULL) {
	sprintf(printBuf, "Volume %d in use by %s. Remove? [y/n] ",
	       volId, volOwnerPtr->owner);
	if (!Utils_GetOk(printBuf)) {
	    return;
	}
    } else if (Utils_GetOk("Remove volume from free list? ")) {
	if (Admin_GetFreeVol(parms.root, &volId) != T_SUCCESS) {
	    sprintf(printBuf,"Couldn't remove volume from free list. Continue? [y/n] ");
	    if (!Utils_GetOk(printBuf)) {
		return;
	    }
	} else if (parms.verbose) {
	    fprintf(stdout, "Removed volume from free list.\n");
	    fflush(stdout);
	}
    }

    if 	((volOwnerPtr != NULL) &&
	 (volOwnerPtr->owner != NULL) &&
 	 (Utils_GetOk("Remove all references to volume from index? "))) {
	dirPath = Str_Cat(3, parms.root, "/", volOwnerPtr->owner);
	if ((pruneCnt=PruneDirectory(volOwnerPtr->minTBuf,
				     volOwnerPtr->maxTBuf,
				     dirPath, "")) == -1) {
	    sprintf(printBuf,"Couldn't prune index. errno %d", syserr);
	} else if (parms.verbose) {
	    fprintf(stdout, "Pruned %d items from index.\n", pruneCnt);
	    fflush(stdout);
	}
    }

    if (Dev_RemoveVolume(robotStream, slot) != T_SUCCESS) {
	sprintf(printBuf,"Removal failed: %s\n", sys_errlist[syserr]);
	Utils_Bailout(printBuf, BAIL_PRINT);
    }

    if (parms.verbose) {
	fprintf(stdout,"Removed volume %d.\n", volId);
    }
    
}


/*
 *----------------------------------------------------------------------
 *
 * LoadVolume --
 *
 *	Load a volume into a reader
 *
 * Results:
 *	none.
 *
 * Side effects:
 *	none.
 *
 *----------------------------------------------------------------------
 */
static void
LoadVolume(volId, dev)
    int volId;                /* source volume id */
    char *dev;                /* destination device name */
{
    int volSlot;
    int devSlot;

    if ((volSlot=FindVolumeInList(volId)) == -1) {
	sprintf(printBuf,"Don't know location of volume %d.\n", volId);
	Utils_Bailout(printBuf, BAIL_PRINT);
    }

    if ((devSlot=FindDeviceInList(dev)) == -1) {
	sprintf(printBuf,"Don't know location of device %s.\n", dev);
	Utils_Bailout(printBuf, BAIL_PRINT);
    }

    if (Dev_MoveVolume(robotStream, volSlot, devSlot) != T_SUCCESS) {
	sprintf(printBuf,"Couldn't load volume %d into %s: %s\n",
		volId, dev, sys_errlist[syserr]);
	Utils_Bailout(printBuf, BAIL_PRINT);
    }

    if (parms.verbose) {
	fprintf(stdout,"Volume %d loaded from slot %d into %s.\n",
		volSlot, volId, dev);
    }

}


/*
 *----------------------------------------------------------------------
 *
 * UnloadVolume --
 *
 *	Unload a volume from a reader
 *
 * Results:
 *	none.
 *
 * Side effects:
 *	none.
 *
 *----------------------------------------------------------------------
 */
static void
UnloadVolume(volId, dev)
    int volId;                /* source volume */
    char *dev;                /* source device name */
{
    int volSlot;
    int devSlot;

    if ((volSlot=FindVolumeInList(volId)) == -1) {
	sprintf(printBuf,"Don't know location of volume %d.\n", volId);
	Utils_Bailout(printBuf, BAIL_PRINT);
    }

    if ((devSlot=FindDeviceInList(dev)) == -1) {
	sprintf(printBuf,"Don't know location of device %s.\n", dev);
	Utils_Bailout(printBuf, BAIL_PRINT);
    }

    if (parms.verbose) {
	fprintf(stdout,"Ejecting volume from device %s...", dev);
	fflush(stdout);
    }

    if (Dev_UnloadVolume(dev) != T_SUCCESS) {
	sprintf(printBuf,"Couldn't eject device %s: %s\n",
		dev, sys_errlist[syserr]);
	Utils_Bailout(printBuf, BAIL_PRINT);
    }

    if (parms.verbose) {
	fprintf(stdout,"ejected.\n");
	fflush(stdout);
    }

    if (Dev_MoveVolume(robotStream, devSlot, volSlot) != T_SUCCESS) {
	sprintf(printBuf,"Couldn't unload volume %d from %s: %s\n",
		volId, dev, sys_errlist[syserr]);
	Utils_Bailout(printBuf, BAIL_PRINT);
    }

    if (parms.verbose) {
	fprintf(stdout,"Volume %d unloaded from %s to slot %d.\n",
		volId, dev, volSlot);
    }

}


/*
 *----------------------------------------------------------------------
 *
 * MoveVolume --
 *
 *	Move volume from 1 slot to another
 *
 * Results:
 *	none.
 *
 * Side effects:
 *	none.
 *
 *----------------------------------------------------------------------
 */
static void
MoveVolume(srcSlot, destSlot)
    int srcSlot;              /* source location */
    int destSlot;             /* destination volume */
{

    if (Dev_MoveVolume(robotStream, srcSlot, destSlot) != T_SUCCESS) {
	sprintf(printBuf,"Couldn't move volume from %d to %d: %s\n",
		srcSlot, destSlot, sys_errlist[syserr]);
	Utils_Bailout(printBuf, BAIL_PRINT);
    }

    if (parms.verbose) {
	fprintf(stdout,"Volume moved from slot %d to slot %d.\n",
		srcSlot, destSlot);
    }

}


/*
 *----------------------------------------------------------------------
 *
 * MoveVolume --
 *
 *	Move volume from 1 slot to another
 *
 * Results:
 *	none.
 *
 * Side effects:
 *	none.
 *
 *----------------------------------------------------------------------
 */
static void
ReadVolLabel(srcSlot)
    int srcSlot;              /* source location */
{
    char volLabel[T_MAXSTRINGLEN];
    int volId;

    if (Dev_ReadVolLabel(robotStream, srcSlot, volLabel, &volId)!= T_SUCCESS) {
	fprintf(stderr,"Couldn't read volume label in slot %d: %s\n",
		srcSlot, sys_errlist[syserr]);
    } else {
	fprintf(stdout,"Volume in slot %d has label: %s.\n",
		srcSlot, volLabel);
    }

}



/*
 *----------------------------------------------------------------------
 *
 * OpenDoor --
 *
 *	Open door to jukebox.
 *
 * Results:
 *	none.
 *
 * Side effects:
 *	Takes jukebox offline.
 *
 *----------------------------------------------------------------------
 */
static void
OpenDoor()
{
    if (Dev_OpenDoor(robotStream) != T_SUCCESS) {
	fprintf(stderr,"Couldn't open door: %s\n", sys_errlist[syserr]);
    } else if (parms.verbose) {
	fprintf(stdout,"Door released.\n");
    }
}



/*
 *----------------------------------------------------------------------
 *
 * BuildVolList --
 *
 *	Create list of 
 *
 * Results:
 *	none.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
static void
BuildVolList()
{
    int cnt = 0;
    VolConfig *listPtr;
    VolConfig *workPtr;

    Dev_BuildVolList(robotStream, NULL, &cnt);
    listPtr = (VolConfig *)MEM_ALLOC("BuildVolList",cnt*sizeof(VolConfig));
    if (Dev_BuildVolList(robotStream, listPtr, &cnt) != T_SUCCESS) {
	fprintf(stderr,"Couldn't build volume list: %s\n",
		sys_errlist[syserr]);
	MEM_FREE("BuildVolList",listPtr);
    } else {
	workPtr = listPtr;
	while (cnt--) {
	    fprintf(stdout, "%d %d %s\n",
		    workPtr->volId, workPtr->location, workPtr->volLabel);
	    workPtr++;
	}
	MEM_FREE("BuildVolList",listPtr);
    }
}



/*
 *----------------------------------------------------------------------
 *
 * FindVolumeInList --
 *
 *	Locate volumeId in list.
 *
 * Results:
 *	none.
 *
 * Side effects:
 *	none.
 *
 *----------------------------------------------------------------------
 */

static int
FindVolumeInList(volId)
    int volId;                /* volume Id */
{
    int i;

    /* Must inquire, if we don't have a volconfig file */
    if (volCnt == 0) {
	sprintf(stderr, "What slot is volume %d in? [<return> to exit] ",
		volId);
	return Utils_GetInteger(printBuf, 0, INT_MAX);
    }

    for (i=0; i<volCnt; i++) {
	if (volId == volList[i].volId) {
	    return volList[i].location;
	}
    }
    return -1;
}


/*
 *----------------------------------------------------------------------
 *
 * FindSlotInList --
 *
 *	Locate slot in list.
 *
 * Results:
 *	none.
 *
 * Side effects:
 *	none.
 *
 *----------------------------------------------------------------------
 */

static int
FindSlotInList(slot)
    int slot;                 /* slot number */
{
    int i;
    char volLabel[T_MAXLABELLEN];

    /* Must inquire, if we don't have a volconfig file */
    if (volCnt == 0) {
	if (Dev_ReadVolLabel(robotStream, slot, volLabel, &i) == T_SUCCESS) {
	    return i;
	} else {
	    sprintf(printBuf, "What volume is in slot %d? [<return> to exit] ",
		    slot);
	    return Utils_GetInteger(printBuf, 0, INT_MAX);
	}
    }

    for (i=0; i<volCnt; i++) {
	if (slot == volList[i].location) {
	    return volList[i].volId;
	}
    }
    return -1;
}



/*
 *----------------------------------------------------------------------
 *
 * FindDeviceInList --
 *
 *	Locate device name in list.
 *
 * Results:
 *	none.
 *
 * Side effects:
 *	none.
 *
 *----------------------------------------------------------------------
 */

static int
FindDeviceInList(reader)
    char *reader;             /* name of device */
{
    int i;

    /* Must inquire, if we don't have a devconfig file */
    if (devCnt == 0) {
	sprintf(printBuf, "What slot does device % use? [<return> to exit] ",
		reader);
	return Utils_GetInteger(printBuf, 0, INT_MAX);
    }

    for (i=0; i<devCnt; i++) {
	if (strcmp(reader,devList[i].name) == 0) {
	    return devList[i].location;
	}
    }
    return -1;
}



/*
 *----------------------------------------------------------------------
 *
 * CheckOptions -- 
 *
 *	Make sure command line options look reasonable
 *
 * Results:
 *	none.
 *
 * Side effects:
 *	none.
 *
 *----------------------------------------------------------------------
 */

static void
CheckOptions(parmsPtr, cmdPtr)
    Parms *parmsPtr;          /* parameter block */
    int *cmdPtr;              /* converted command */
{
    Cmd *localCmdPtr;

    if ((localCmdPtr=FindCmdInList(parmsPtr->cmd)) == NULL) {
	Utils_Bailout("Need argument for -cmd: insert, remove, load, unload,\nmove, readlabel, listvols, msg, or opendoor.\n", BAIL_PRINT);
    }
    if ((localCmdPtr->flags & NEEDVOL) &&
	((parmsPtr->volId == DEF_VOL) || (parmsPtr->volId < 0))) {
	Utils_Bailout("Need non-negative argument on -vol option.\n",
		      BAIL_PRINT);
    }
    if ((localCmdPtr->flags & NEEDSLOT) &&
	((parmsPtr->slot == DEF_SLOT) || (parmsPtr->slot < 0))) {
	Utils_Bailout("Need non-negative argument on -slot option.\n",
		      BAIL_PRINT);
    }
    if ((localCmdPtr->flags & NEEDSLOT2) &&
	((parmsPtr->slot == DEF_SLOT2) || (parmsPtr->slot2 < 0))) {
	Utils_Bailout("Need non-negative argument on -slot2 option.\n",
		      BAIL_PRINT);
    }
    if ((localCmdPtr->flags & NEEDDEV) &&
	(strcmp(parmsPtr->dev, DEF_DEV) == 0)) {
	Utils_Bailout("Need non-null device name.\n", BAIL_PRINT);
    }
    *cmdPtr = localCmdPtr->id;
}



/*
 *----------------------------------------------------------------------
 *
 * FindCmdInList --
 *
 *	Locate user command and return entry.
 *
 * Results:
 *	none.
 *
 * Side effects:
 *	none.
 *
 *----------------------------------------------------------------------
 */

static Cmd *
FindCmdInList(string)
    char *string;             /* Command string */
{
    int i;
    static Cmd cmds[NUMCMDS] = {
	{"insert",   CMD_INSERT,     NEEDVOL+NEEDSLOT},
	{"remove",   CMD_REMOVE,     NEEDVOL},
	{"load",     CMD_LOAD,       NEEDVOL+NEEDDEV},
	{"unload",   CMD_UNLOAD,     NEEDVOL+NEEDDEV},
	{"opendoor", CMD_OPENDOOR,   0},
	{"msg",      CMD_DISPMSG,    0},
	{"move",     CMD_MOVE,       NEEDSLOT+NEEDSLOT2},
	{"readlabel", CMD_READLABEL, NEEDSLOT},
	{"listvols", CMD_LISTVOLS,   0}
    };

    for (i=0; i<NUMCMDS; i++) {
	if (strcmp(string,cmds[i].name) == 0) {
	    return &cmds[i];
	}
    }

    return NULL;
}


/*
 *----------------------------------------------------------------------
 *
 * PruneDirectory --
 *
 *	Locate and remove all references to specified volume
 *
 * Results:
 *	none.
 *
 * Side effects:
 *	none.
 *
 *----------------------------------------------------------------------
 */

static int
PruneDirectory(minTBuf, maxTBuf, dirPath, curPath)
    int minTBuf;              /* lowest tbuf on volume */
    int maxTBuf;              /* highest tbuf on volume */
    char *dirPath;            /* directory to be pruned */
    char *curPath;            /* current user path */
{
    T_FileStat statInfo;
    char indxPath[2*T_MAXPATHLEN];
    char newPath[2*T_MAXPATHLEN];
    char filePath[2*T_MAXPATHLEN];
    DIR *dirPtr;
    DirObject *entryPtr;
    struct stat unixStatBuf;
    FILE *indxStream;
    FILE *newStream;
    int pruneCnt = 0;
    char *simpleName;

    /*
     * First prune the file in this directory
     */
    strcpy(indxPath, dirPath);
    strcat(indxPath, "/_jaquith.files");
    strcpy(newPath, dirPath);
    strcat(newPath, "/_jaquith.files.new");
    if ((indxStream=fopen(indxPath, "r")) == (FILE *)NULL) {
	sprintf(printBuf, "open %s", indxPath);
	perror(printBuf);
    } else if ((newStream=fopen(newPath, "w")) == (FILE *)NULL) {
	sprintf(printBuf, "open %s", newPath);
	perror(printBuf);
    } else {
	while (Indx_ReadIndxEntry(indxStream, &statInfo) == T_SUCCESS) {
	    if ((statInfo.tBufId >= minTBuf) &&
		(statInfo.tBufId <= maxTBuf)) {
		pruneCnt++;
	    } else {
		simpleName = statInfo.fileName;
		if (*curPath == '\0') {
		    statInfo.fileName = Str_Dup("/");
		} else {
		    statInfo.fileName = Str_Cat(3, curPath, "/", simpleName);
		}
		Indx_WriteIndxEntry(&statInfo, -1, newStream);
		MEM_FREE("PruneDirectory", simpleName);
		Utils_FreeFileStat(&statInfo, 0);
	    }
	}	
	fclose(indxStream);
	fclose(newStream);
	if (unlink(indxPath) == -1) {
	    sprintf(printBuf, "unlink %s", indxPath);
	    perror(printBuf);
	} else if (rename(newPath, indxPath) == -1) {
	    sprintf(printBuf, "rename %s to %s", newPath, indxPath);
	    perror(printBuf);
	}
    }

    /*
     * Now recurse through child directories
     */
    if ((dirPtr=(DIR *)opendir(dirPath)) == (DIR *) NULL) {
	sprintf(printBuf, "open dir %s", dirPath);
	perror(printBuf);
    } else {
	while ((entryPtr=readdir(dirPtr)) != (DirObject *)NULL) {
	    if ((strcmp(entryPtr->d_name, ".") != 0) &&
		(strcmp(entryPtr->d_name, "..") != 0)) {
		strcpy(indxPath, dirPath);
		strcat(indxPath, "/");
		strcat(indxPath, entryPtr->d_name);
		if (stat(indxPath, &unixStatBuf) == -1) {
		    sprintf(printBuf, "stat %s", indxPath);
		    perror(printBuf);
		} else if (S_ISADIR(unixStatBuf.st_mode)) {
		    strcpy(newPath, curPath);
		    strcat(newPath, "/");
		    if (*curPath != '\0') {
			strcat(newPath, entryPtr->d_name);
		    }
		    pruneCnt += PruneDirectory(minTBuf, maxTBuf,
					       indxPath, newPath);
		}
	    }
	}
	closedir(dirPtr);
    }

    return pruneCnt;
}
