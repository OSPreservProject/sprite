/*
 * zebra.h --
 *
 *	General definitions for the Zebra file system.
 *
 * Copyright 1992 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that this copyright
 * notice appears in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *
 * $Header: /sprite/lib/forms/RCS/proto.h,v 1.8 92/03/02 15:32:57 bmiller Exp $ SPRITE (Berkeley)
 */

#ifndef _ZEBRA
#define _ZEBRA

#include <list.h>

/*
 * Stripe Identifier.  This is used to uniquely identify stripes.
 * Format of an SID:
 *
 * -------------------------------------------------------------------
 * |  31 - 24     |                    23 - 4               | 3 - 0  |
 * |  Host #	  |                  Sequence #             | Unused |
 * -------------------------------------------------------------------
 */

typedef unsigned int Zebra_Sid; 

#define ZEBRA_SID_HOST_BITS 8
#define ZEBRA_SID_HOST_SHIFT 24

#define ZEBRA_SID_SEQ_BITS 20
#define ZEBRA_SID_SEQ_SHIFT 4

/* 
 * Macro to create an SID given a host # and a sequence #.
 */

#define Zebra_SidCreate(host, sequence) 				    \
    ((((host) & ((1 << ZEBRA_SID_HOST_BITS) - 1)) << ZEBRA_SID_HOST_SHIFT) | \
    (((sequence) & ((1 << ZEBRA_SID_SEQ_BITS) - 1)) << ZEBRA_SID_SEQ_SHIFT))

/* 
 * Macro to compare two SIDs. Returns 0 if they are equal, 1 otherwise.
 */

#define Zebra_SidCmp(sid1, sid2) ((sid1) != (sid2))

/* 
 * Stripe Fragment Identifier. Uniquely identifies a stripe fragment. The
 * format is the same as an SID, except that the low bits encode the
 * ID of the storage server that stores the fragment.
 */

typedef Zebra_Sid Zebra_Sfid;

#define ZEBRA_SFID_SERVER_BITS 4

/*
 * Maximum number of storage servers that can store a stripe. 
 */

#define ZEBRA_MAX_STORAGE_SERVERS (1 << ZEBRA_SFID_SERVER_BITS)

/* 
 * Macro to create an SFID from an SID and a storage server ID.
 */

#define Zebra_SfidCreate(sid, server) 				    \
    ((server) & ((1 << ZEBRA_SFID_SERVER_BITS) - 1) | (sid))


/*
 * Zebra Checksum. This is used to validate various data structures such
 * as the fragments.
 */

typedef unsigned int Zebra_Checksum;

/*
 * Zebra Timestamp. 
 */

typedef unsigned int Zebra_TimeStamp;

/*
 * A unique id.
 */

typedef unsigned int Zebra_Uid;

/*
 * This structure is used to store information about each Zebra domain.
 * The metaInfo is used to find the correct domainInfo based upon
 * a metadata file handle. The volumes is an array of information
 * on the storage servers for the domain. Each element in the array
 * contains a storage server ID and a volume ID. 
 */
typedef struct {
    List_Links	links;				/* Used to make lists. */
    int 	serverID;			/* Server for metadata. */
    int		domainID;			/* ID of the domain. */
    int		numServers;			/* Number of storage servers
						 * for the domain. */
    int		fragSize;			/* Stripe fragment size
						 * (in KBytes). */
    struct {
	int		serverID;		/* Storage server ID. */
	Zebra_Uid	vid;			/* Volume ID. */
    } volumes[ZEBRA_MAX_STORAGE_SERVERS];
} Zebra_DomainInfo;

/*
 * Header for a Zebra metadata file.
 */

typedef struct {
    int		magic;		/* Magic number. See below. */
    int		blocks;		/* Number of blocks in the file. */
    char	pad[8];		/* Pad this out to 16 bytes so metadata
				 * elements don't span file blocks. */
} Zebra_MetadataHdr;

#define ZEBRA_METADATA_HDR_MAGIC		0xdeadcafe

/*
 * These structures follow the header in the metadata file. There is
 * one structure per block in the Zebra file.
 */

typedef struct {
    int		serverID;	/* Storage server ID. */
    Zebra_Uid	vid;		/* Volume ID. */
    Zebra_Sfid	sfid;		/* Stripe fragment ID. */
    int		offset;		/* Offset within stripe fragment. */
} Zebra_Metadata;

#endif /* _ZEBRA */

