/* 
 * getidprom.c --
 *
 *	This module provides idprom support.
 *
 * Copyright 1989 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 */

#ifndef lint
static char rcsid[] = "$Header: /sprite/src/boot/netBoot.OpenProm/sun4c.md/RCS/getidprom.c,v 1.1 91/01/13 02:52:18 dlong Exp $ SPRITE (Berkeley)";
#endif /* not lint */

#include "boot.h"


/*
 *----------------------------------------------------------------------
 *
 * SearchProm --
 *
 *	Search through the prom devices to find out system attributes.
 *	Modified version of function of same name in devGraphics.c,
 * 	also allows displaying attributes of the whole tree if
 *	name and attr are given as "*".  If attr is given, and name
 *	is "*", the info will be retrieved from the first node with
 *	that attribute.
 *
 * Results:
 *	length of info retrieved.
 *
 *----------------------------------------------------------------------
 */
#ifdef sun4c
static int
SearchProm(node, name, attr, buf, buflen)
    unsigned	int		node;
    char	*name;
    char	*attr;
    char	*buf;
    int		buflen;
{
    unsigned	int		newNode;
    char 	searchBuffer[64];
    int		length = 0;
    struct	config_ops	*configPtr;
    ReturnStatus		ret;
    char	*prop;

    configPtr = romVectorPtr->v_config_ops;
    while (node != 0) {
	length = configPtr->devr_getproplen(node, "name");
	if (length > 0) {
	    if (length > sizeof (searchBuffer)) {
		panic("SearchProm: buffer too small.\n");
	    }
	    configPtr->devr_getprop(node, "name", searchBuffer);
	    if (strcmp("*", name) == 0 && strcmp("*", attr) == 0) {
		prop = 0;
		while (1) {
		    prop = (char *)configPtr->devr_nextprop(node, prop);
		    if (prop && *prop) {
			if (strcmp(prop, attr) == 0
				|| strcmp(prop, "*") == 0) {
			    printf("%s: %s\n", searchBuffer, prop);
			} 
			printf("%s: %s\n", searchBuffer, prop);
		    } else {
			break;
		    }
		}
	    }
	    else if (strcmp(searchBuffer, name) == 0
		    || strcmp(name, "*") == 0) {
		length = configPtr->devr_getproplen(node, attr);
		if (length <= 0) {
		    printf("No %s found for %s in prom.\n", attr, name);
		} else if (length > buflen) {
		    printf("Data size (%d) is greater than buffer size (%d)\n",
			length, buflen);
		} else {
		    configPtr->devr_getprop(node, attr, buf);
		}
		return length;
	    }
	}
	newNode = configPtr->devr_child(node);
	ret = SearchProm(newNode, name, attr, buf, buflen);
	if (ret > 0) {
	    return ret;
	}
	node = configPtr->devr_next(node);
    }
    return 0;
}
#endif /* sun4c */


/*
 *----------------------------------------------------------------------
 *
 * GetIDProm --
 *
 *	Get IDPROM information in a machine independent way.
 *
 * Results:
 *	IDPROM info.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
int
GetIDProm(buf, buflen)
    char	*buf;
    int		buflen;
{
#ifdef sun4c
    struct	config_ops	*configPtr;
    unsigned	int		node;

    configPtr = romVectorPtr->v_config_ops;
    /*
     * Find the idprom:  First get the root node id of the tree of
     * devices in the prom.  Then traverse it depth-first to find
     * the idprom.  Use "*" because we aren't sure what type of
     * machine it is, and the "idprom" is a property of the root
     * node, which is the machine type.  I guess we don't need to
     * traverse the tree, since it is at the root node, but
     * Sun might put it somewhere else in the next PROM version!
     */
    node = configPtr->devr_next(0);

#ifdef traverse_tree_debug
    (void) SearchProm(node, "*", "*", NULL, 0);
#endif
    return SearchProm(node, "*", "idprom", buf, buflen);
#else
    return -1;
#endif /* sun4c */
}

#ifdef sun4c
ShowProm()
{
    struct	config_ops	*configPtr;
    unsigned	int		node;
    configPtr = romVectorPtr->v_config_ops;

    node = configPtr->devr_next(0);
    ShowNode(node);
    ShowMemList();
}

ShowNode(node)
    unsigned	int		node;
{
    unsigned	int		newNode;
    char 	nameBuffer[128], valueBuffer[128];
    int		length = 0;
    int		vlength;
    struct	config_ops	*configPtr;
    ReturnStatus		ret;
    char	*prop;

    configPtr = romVectorPtr->v_config_ops;
    while (node != 0) {
	length = configPtr->devr_getproplen(node, "name");
	if (length > 0) {
	    configPtr->devr_getprop(node, "name", nameBuffer);
	    {
		prop = 0;
		while (1) {
		    prop = (char *)configPtr->devr_nextprop(node, prop);
		    if (prop && prop[0]) {
			unsigned int intVal;
			configPtr->devr_getprop(node, prop, valueBuffer);
			vlength = configPtr->devr_getproplen(node, prop);
			bcopy(valueBuffer, (char *) &intVal, sizeof(int));
			valueBuffer[vlength] = 0;
			    printf("%s: %s = 0x%x \"%s\"", nameBuffer, prop,
					    intVal, valueBuffer);
		    } else {
			break;
		    }
		}
	    }
	}
	newNode = configPtr->devr_child(node);
	ret = ShowNode(newNode);
	node = configPtr->devr_next(node);
    }
    return 0;
}

ShowMemList()
{
	int  buflen;
	struct memlist mlist[16];

    struct	config_ops	*configPtr;
    unsigned	int		node;
    int		length, i;
    configPtr = romVectorPtr->v_config_ops;

    node = configPtr->devr_next(0);

    length = SearchProm(node, "memory", "available", (char *) mlist, 
				sizeof(mlist));
    for (i = 0; i < length / sizeof(struct memlist); i++) {
	printf("memlist[%d], next = 0x%x addr = 0x%x, size = %d\n",
		i, mlist[i].next, mlist[i].address, mlist[i].size);
    }

}
#endif
