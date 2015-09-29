/* 
 * host.c --
 *
 *	This file contains a program that exercises the Host_
 *	library procedures.  Invoke it with no parameters;  it
 *	will print messages on stderr for any problems it detects
 *	with the string procedures.
 *
 * Copyright 1988 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 */

#ifndef lint
static char rcsid[] = "$Header: /sprite/src/tests/host/RCS/host.c,v 1.2 88/12/15 09:27:17 ouster Exp $ SPRITE (Berkeley)";
#endif not lint

#include <stdio.h>
#include <host.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define error(string) \
    fprintf(stderr, string); \
    exit(1);

main()
{
    Host_Entry *entryPtr;
    struct in_addr addr;
    static unsigned char etherAddr[6] = {8, 0, 0x20, 1, 0x5c, 0xce};
    int i;

    if (Host_SetFile("gorp") != -1) {
	error("host error 1\n");
    }
    if (Host_SetFile("file1") != 0) {
	error("host error 2\n");
    }
    entryPtr = Host_Next();
    if (entryPtr == NULL) {
	error("host error 3\n");
    }
    if (strcmp(entryPtr->name, "lust.Berkeley.EDU") != 0) {
	error("host error 4\n");
    }
    if (strcmp(entryPtr->aliases[0], "lust") != 0) {
	error("host error 5\n");
    }
    if (entryPtr->aliases[1] != 0) {
	error("host error 6\n");
    }
    if (entryPtr->id != 1) {
	error("host error 7\n");
    }
    if (entryPtr->netType != HOST_ETHER) {
	error("host error 8\n");
    }
    if ((entryPtr->netAddr.etherAddr[0] != 8)
	    || (entryPtr->netAddr.etherAddr[1] != 0)
	    || (entryPtr->netAddr.etherAddr[2] != 0x20)
	    || (entryPtr->netAddr.etherAddr[3] != 1)
	    || (entryPtr->netAddr.etherAddr[4] != 2)
	    || (entryPtr->netAddr.etherAddr[5] != 0xc6)) {
	error("host error 9\n");
    }
    if (entryPtr->inetAddr.s_addr != inet_addr("128.32.150.11")) {
	error("host error 10\n");
    }
    Host_End();
    entryPtr = Host_ByID(5);
    if (entryPtr == NULL) {
	error("host error 11\n");
    }
    if (strcmp(entryPtr->name, "basil.Berkeley.EDU") != 0) {
	error("host error 12\n");
    }
    entryPtr = Host_ByID(108);
    if (entryPtr != NULL) {
	error("host error 13\n");
    }
    addr.s_addr = inet_addr("128.32.150.8");
    entryPtr = Host_ByInetAddr(addr);
    if (entryPtr == NULL) {
	error("host error 14\n");
    }
    if (strcmp(entryPtr->name, "paprika.Berkeley.EDU") != 0) {
	error("host error 15\n");
    }
    entryPtr = Host_ByInetAddr(46);
    if (entryPtr != NULL) {
	error("host error 16\n");
    }
    entryPtr = Host_ByName("mace.Berkeley.EDU");
    if (entryPtr == NULL) {
	error("host error 17\n");
    }
    if (entryPtr->id != 27) {
	error("host error 18\n");
    }
    entryPtr = Host_ByName("oregano");
    if (entryPtr == NULL) {
	error("host error 19\n");
    }
    if (entryPtr->id != 38) {
	error("host error 20\n");
    }
    entryPtr = Host_ByName("foobar");
    if (entryPtr != NULL) {
	error("host error 21\n");
    }
    entryPtr = Host_ByNetAddr(HOST_ETHER, (char *) etherAddr);
    if (entryPtr == NULL) {
	error("host error 22\n");
    }
    if (strcmp(entryPtr->name, "mint.Berkeley.EDU") != 0) {
	error("host error 23\n");
    }
    etherAddr[0] = 0xff;
    entryPtr = Host_ByNetAddr(HOST_ETHER, (char *) etherAddr);
    if (entryPtr != NULL) {
	error("host error 24\n");
    }
    if (Host_Start() != 0) {
	error("host error 25\n");
    }
    for (i = 0; Host_Next() != NULL; i++) {
	/* Null body. */
    }
    if (i != 33) {
	error("host error 26\n");
    }
    Host_SetFile("file2");
    if (Host_Next() != NULL) {
	error("host error 27\n");
    }
    Host_SetFile("file3");
    if (Host_Next() != NULL) {
	error("host error 28\n");
    }
    Host_SetFile("file4");
    if (Host_Next() != NULL) {
	error("host error 29\n");
    }
    Host_SetFile("file5");
    if (Host_Next() != NULL) {
	error("host error 30\n");
    }
    Host_SetFile("file6");
    if (Host_Next() != NULL) {
	error("host error 31\n");
    }
    Host_SetFile("file7");
    if (Host_Next() != NULL) {
	error("host error 31\n");
    }
    Host_SetFile("file8");
    entryPtr = Host_ByName("10");
    if (entryPtr == NULL) {
	error("host error 32\n");
    }
    entryPtr = Host_ByName("savory");
    if (entryPtr == NULL) {
	error("host error 33\n");
    }
}
