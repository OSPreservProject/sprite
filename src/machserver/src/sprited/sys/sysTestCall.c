/* 
 * sysTestCall.c --
 *
 *	Test system calls for the Sprite server.
 *
 * Copyright 1991 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that this copyright
 * notice appears in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 */

#ifndef lint
static char rcsid[] = "$Header: /user5/kupfer/spriteserver/src/sprited/sys/RCS/sysTestCall.c,v 1.16 92/07/17 16:34:24 kupfer Exp $ SPRITE (Berkeley)";
#endif /* not lint */

#include <mach.h>
#include <bstring.h>
#include <ckalloc.h>
#include <status.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include <spriteSrvServer.h>
#include <sys.h>
#include <sysInt.h>
#include <utils.h>
#include <vm.h>


/*
 *----------------------------------------------------------------------
 *
 * Test_PutDecimalStub --
 *
 *	Write an integer to stdout in decimal.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

/* ARGSUSED */
kern_return_t
Test_PutDecimalStub(port, value)
    mach_port_t	port;		/* request port */
    int		value;		/* value to print */
{
    printf("%d", value);
    return KERN_SUCCESS;
}


/*
 *----------------------------------------------------------------------
 *
 * Test_PutOctalStub --
 *
 *	Write an integer to stdout in octal.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

/* ARGSUSED */
kern_return_t
Test_PutOctalStub(port, value)
    mach_port_t	port;		/* request port */
    int		value;		/* value to print */
{
    printf("0%o", value);
    return KERN_SUCCESS;
}


/*
 *----------------------------------------------------------------------
 *
 * Test_PutHexStub --
 *
 *	Write an integer to stdout in hex.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

/* ARGSUSED */
kern_return_t
Test_PutHexStub(port, value)
    mach_port_t	port;		/* request port */
    int		value;		/* value to print */
{
    printf("0x%x", value);
    return KERN_SUCCESS;
}


/*
 *----------------------------------------------------------------------
 *
 * Test_PutMessageStub --
 *
 *	Print a string to stdout.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

/* ARGSUSED */
kern_return_t
Test_PutMessageStub(port, message)
    mach_port_t	port;
    Test_MessageBuffer message;
{
    /* XXX verify that string ends with a null? */
    printf("%s", message);
    return KERN_SUCCESS;
}


/*
 *----------------------------------------------------------------------
 *
 * Test_PutTimeStub --
 *
 *	Format and print a time value.
 *
 * Results:
 *	Returns KERN_SUCCESS.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

kern_return_t
Test_PutTimeStub(port, time, overwrite)
    mach_port_t port;
    int time;			/* XXX should be time_t */
    Boolean overwrite;		/* hack: if true, pretend the cursor is at 
				 * the end of a formatted time and
				 * overwrite it */
{
    char buffer[1024];		/* what we actually print */
    int i;

#ifdef lint
    port = port;
#endif

    if (!overwrite) {
	buffer[0] = '\0';
    } else {
	for (i = 0; i < 24; ++i) {
	    buffer[i] = '\b';
	}
	buffer[i] = '\0';
    }
    strcat(buffer, ctime((time_t*)&time));
    /* 
     * Null out the final newline.
     */
    buffer[strlen(buffer)-1] = '\0';
    printf(buffer);

    return KERN_SUCCESS;
}


#if 0
/*
 *----------------------------------------------------------------------
 *
 * Test_PutStringStub --
 *
 *	Print a string to stdout, accessing it via Vm_MakeAccessible.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

kern_return_t
Test_PutStringStub(port, userString, numBytes)
    mach_port_t port;
    vm_address_t userString;
    int numBytes;
{
    Address string;		/* server address for the user's string */
    int mappedBytes;
    char *buffer;

#ifdef lint
    port = port;
#endif

    Vm_MakeAccessible(VM_READONLY_ACCESS, numBytes, (Address)userString,
		      &mappedBytes, &string);
    if (mappedBytes == 0) {
	return KERN_SUCCESS;
    }

    /* 
     * If the string doesn't end with a null, copy it to a buffer and tack 
     * one on.
     */
    if (string[mappedBytes-1] == '\0') {
	printf("%s", string);
    } else {
	buffer = ckalloc(mappedBytes + 1);
	bcopy(string, buffer, mappedBytes);
	buffer[mappedBytes] = '\0';
	printf("%s", buffer);
	ckfree(buffer);
    }

    Vm_MakeUnaccessible(string, mappedBytes);
    return KERN_SUCCESS;
}
#endif /* 0 */


/*
 *----------------------------------------------------------------------
 *
 * Test_PutStringStub --
 *
 *	Print a string to stdout, accessing it via Vm_StringNCopy.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

kern_return_t
Test_PutStringStub(port, userString, numBytes)
    mach_port_t port;
    vm_address_t userString;
    int numBytes;
{
    Address buffer;
    ReturnStatus status;
    int stringLength;		/* actual length of the string */
    
#ifdef lint
    port = port;
#endif
    
    if (numBytes <= 0) {
	return KERN_SUCCESS;
    }

    buffer = ckalloc(numBytes * 2);
    status = Vm_StringNCopy(numBytes, (Address)userString, buffer,
			    &stringLength);
    if (status != SUCCESS) {
	printf("Test_PutString: couldn't copy in string: %s\n",
	       Stat_GetMsg(status));
	return KERN_SUCCESS;
    }
			    
    if (stringLength > 0) {
	/* 
	 * Make sure the string has a trailing null.
	 */
	buffer[stringLength] = '\0';
	printf("%s", buffer);
    }

    ckfree(buffer);
    return KERN_SUCCESS;
}


/*
 *----------------------------------------------------------------------
 *
 * Test_GetStringStub --
 *
 *	Read a string from stdin and return it via Vm_MakeAccessible.
 *
 * Results:
 *	If we hit EOF on stdin, the first character in the buffer is made 
 *	to be null.
 *
 * Side effects:
 *	The string is copied into the user's address space.  EOF on stdin 
 *	is reset after returning an empty string.
 *
 *----------------------------------------------------------------------
 */

kern_return_t
Test_GetStringStub(port, userBuffer, bufferLength)
    mach_port_t port;
    vm_address_t userBuffer;
    int bufferLength;
{
#if 1
#ifdef lint
    port = port;
    userBuffer = userBuffer;
    bufferLength = bufferLength;
#endif /* lint */
    return KERN_FAILURE;	/* not supported with /dev/console */
#else
    Address ourBuffer;
    int mappedLength;

#ifdef lint
    port = port;
#endif

    Vm_MakeAccessible(VM_READWRITE_ACCESS, bufferLength, (Address)userBuffer,
		      &mappedLength, &ourBuffer);
    if (mappedLength == 0) {
	printf("Test_GetString: Couldn't map user buffer\n");
	return KERN_SUCCESS;
    }

    if (feof(stdin)) {
	ourBuffer[0] = '\0';
	clearerr(stdin);
    } else {
	fgets(ourBuffer, mappedLength, stdin);
	if (feof(stdin)) {
	    ourBuffer[0] = '\0';
	    clearerr(stdin);
	}
    }

    Vm_MakeUnaccessible(ourBuffer, mappedLength);
    return KERN_SUCCESS;
#endif
}


/*
 *----------------------------------------------------------------------
 *
 * Test_MemCheckStub --
 *
 *	Dump out information about server memory usage.
 *
 * Results:
 *	Returns KERN_SUCCESS.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

kern_return_t
Test_MemCheckStub(serverPort)
    mach_port_t serverPort;	/* request port */
{
#ifdef MEM_DEBUG
    /* 
     * Call the package the print the results.  The filename argument is 
     * ignored when the package is built for the server.
     */
    Mem_DumpActiveMemory("dummyFile");
#else
    printf("Warning: malloc debugging unavailable.\n");
#endif

#ifdef lint
    serverPort = serverPort;
#endif

    SysBufferStats();
    return KERN_SUCCESS;
}


/*
 *----------------------------------------------------------------------
 *
 * Test_Return1Stub --
 *
 *	Return immediately.  For testing "syscall" overhead.
 *
 * Results:
 *	Returns KERN_SUCCESS.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

kern_return_t
Test_Return1Stub(serverPort)
    mach_port_t serverPort;	/* request port */
{
#ifdef lint
    serverPort = serverPort;
#endif

    return KERN_SUCCESS;
}


/*
 *----------------------------------------------------------------------
 *
 * Test_Return2Stub --
 *
 *	Like Test_Return1Stub.
 *
 * Results:
 *	Returns KERN_SUCCESS.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

kern_return_t
Test_Return2Stub(serverPort)
    mach_port_t serverPort;	/* request port */
{
#ifdef lint
    serverPort = serverPort;
#endif

    return KERN_SUCCESS;
}
