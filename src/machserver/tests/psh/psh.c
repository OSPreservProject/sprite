/* 
 * psh.c --
 *
 *	Really primitive shell.  Reads and executes commands in a loop 
 *	until it gets end-of-file.
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
static char rcsid[] = "$Header: /user5/kupfer/spriteserver/tests/psh/RCS/psh.c,v 1.8 92/06/10 14:10:28 kupfer Exp $ SPRITE (Berkeley)";
#endif /* not lint */

#include <sprite.h>
#include <bstring.h>
#include <ctype.h>
#include <errno.h>
#include <fs.h>
#include <rpc.h>
#include <spriteTime.h>
#include <status.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sysStats.h>
#include <test.h>
#include <unistd.h>

#include <testRedef.h>

#define CMD_BUF_SIZE	4096
#define MAX_ARGS	110

#define BIN_DIR	"/users/kupfer/cmds.sprited/" /* standard directory for 
					       * commands */

/* forward declarations: */

static void Cd _ARGS_((char *command));
static void DoCmd _ARGS_((char *command));
static void DoExec _ARGS_((char *command));
static ReturnStatus DoTest _ARGS_((int command, int serverID,
				   int numEchoes, int size, 
				   Address inDataBuffer,
				   Address outDataBuffer, Time *timePtr));
static void MakeArgArray _ARGS_((char *command, char **argArray));
static void Ping _ARGS_((char *command));
static void RpcCmd _ARGS_((char *command));


/*
 *----------------------------------------------------------------------
 *
 * main --
 *
 *	Command loop.
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
main()
{
    char command[CMD_BUF_SIZE];

    for (;;) {
	Test_PutMessage("> ");
	Test_GetString(command, CMD_BUF_SIZE);
	if (command[0] == '\0') {
	    break;
	}
	DoCmd(command);
    }

    exit(0);
}


/*
 *----------------------------------------------------------------------
 *
 * DoCmd --
 *
 *	Execute a command.  For most commands, this means: fork off a 
 *	child, exec the named program, and wait for it to finish.  A couple 
 *	commands are built-ins.
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
DoCmd(command)
    char *command;		/* the command to execute */
{
    Boolean waitForChild = TRUE; /* wait for child to exit */
    pid_t child;

    /* 
     * Get rid of the trailing newline.
     */
    if (command[strlen(command)-1] == '\n') {
	command[strlen(command)-1] = '\0';
    }

    if (strcmp(command, "memcheck") == 0) {
	Test_MemCheck("dummyFile");
	return;
    } else if (strncmp(command, "RpcCmd", strlen("RpcCmd")) == 0) {
	RpcCmd(command);
	return;
    } else if (strncmp(command, "Ping", strlen("Ping")) == 0) {
	Ping(command);
	return;
    } else if (strncmp(command, "cd", strlen("cd")) == 0) {
	Cd(command);
	return;
    } else if (strncmp(command, "shutdown", strlen("shutdown")) == 0) {
	(void)Sys_Shutdown(SYS_HALT|SYS_KILL_PROCESSES|SYS_WRITE_BACK,
			   NULL);
	return;
    } else if (strncmp(command, "bg", strlen("bg")) == 0) {
	/* "background" command */
	command += strlen("bg");
	waitForChild = FALSE;
    }

    child = fork();
    switch (child) {
    case -1:
	perror("Fork failed");
	break;
    case 0:
	/* child */
	DoExec(command);
	exit(1);
	break;
    default:
	/* 
	 * Parent.  If it's a synchronous command, wait for the current 
	 * child and reap any previous, exited children.  If it's a
	 * background command, just give the process ID of the child.
	 */
	if (waitForChild) {
	    while (wait((union wait *)0) != child) {
		;
	    }
	    while (wait3((union wait *)0, WNOHANG, (struct rusage *)0) > 0) {
		;
	    }
	} else {
	    Test_PutMessage("[");
	    Test_PutHex(child);
	    Test_PutMessage("]\n");
	}
	break;
    }
}


/*
 *----------------------------------------------------------------------
 *
 * DoExec --
 *
 *	Get the program name and arguments from the command string and exec 
 *	the right program.
 *
 * Results:
 *	None.  Returns only if there's an error.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static void
DoExec(command)
    char *command;		/* the user's command string */
{
    char *args[MAX_ARGS];	/* argument pointers */
    char altCommand[FS_MAX_PATH_NAME_LENGTH];

    bzero((_VoidPtr)args, MAX_ARGS);

    /* Skip over initial whitespace */
    while (isspace(*command)) {
	command++;
    }

    MakeArgArray(command, args);
    if (args[0] == '\0') {
	exit(0);
    }

    (void)execv(command, args);

    /* 
     * If the exec failed because the file doesn't exist, maybe it's in the 
     * "bin" directory.
     */
    if (errno == ENOENT) {
	(void)strcpy(altCommand, BIN_DIR);
	(void)strcat(altCommand, command);
	(void)execv(altCommand, args);
    }

    Test_PutMessage("exec of `");
    Test_PutMessage(command);
    perror("'failed");
}


/*
 *----------------------------------------------------------------------
 *
 * MakeArgArray --
 *
 *	Break the command line up into argument strings.  
 *	
 * Results:
 *	Fills in the arguments array with pointers into the given command 
 *	string.  The last argument pointer is made nil.
 *
 * Side effects:
 *	Puts null bytes in the command string to end the argument strings.
 *
 *----------------------------------------------------------------------
 */

static void
MakeArgArray(command, argArray)
    char *command;		/* command string */
    char **argArray;		/* OUT: argument pointers */
{
    int argNum;
    char *chPtr;

    /* 
     *	The command string is a bunch of words separated by whitespace.  
     *	Each of these words is turned into a separate argument string by 
     *	sticking a null at the first whitespace character.
     */

    /* Skip over initial whitespace */
    chPtr = command;
    while (isspace(*chPtr)) {
	chPtr++;
    }
    if (*chPtr == '\0') {
	/* null command */
	argArray[0] = '\0';
	return;
    }
    argArray[0] = chPtr;
    argNum = 1;

    for (; *chPtr != '\0'; chPtr++) {
	if (isspace(*chPtr)) {
	    *chPtr = '\0';
	    ++chPtr;
	    /* skip over additional whitespace */
	    while (isspace(*chPtr)) {
		chPtr++;
	    }
	    /* if there's another argument, remember where it starts. */
	    if (*chPtr != '\0') {
		argArray[argNum] = chPtr;
		argNum++;
	    }
	    if (argNum >= MAX_ARGS-1) {
		Test_PutMessage("Too many arguments.\n");
		break;
	    }
	}
    }

    argArray[argNum] = '\0';
}


/*
 *----------------------------------------------------------------------
 *
 * RpcCmd --
 *
 *	Built-in to replace rpccmd, at least partially.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Issues a Sys_Stats call to the server, depending on arguments 
 *	to the command.
 *
 *----------------------------------------------------------------------
 */

static void
RpcCmd(command)
    char *command;		/* the actual user command */
{
    int commandLength;		/* characters in user command */
    char *usage;		/* "usage" string in case of user error */
    ReturnStatus status = SUCCESS;

    usage = "usage: RpcCmd [on|off]\n";
    commandLength = strlen(command);
    if (commandLength <= strlen("RpcCmd ")) {
	Test_PutMessage(usage);
	return;
    }
    command += strlen("RpcCmd ");
    if (strcmp(command, "on") == 0) {
	status = Sys_Stats(SYS_RPC_ENABLE_SERVICE, TRUE, (Address)NULL);
    } else if (strcmp(command, "off") == 0) {
	status = Sys_Stats(SYS_RPC_ENABLE_SERVICE, FALSE, (Address)NULL);
    } else {
	Test_PutMessage(usage);
	return;
    }
    if (status != SUCCESS) {
	Test_PutMessage("Couldn't turn RPC's ");
	Test_PutMessage(command);
	Test_PutMessage(": ");
	Test_PutMessage(Stat_GetMsg(status));
	Test_PutMessage("\n");
    }
}


/*
 *----------------------------------------------------------------------
 *
 * Ping --
 *
 *	Do the RPC ping test to the named host.
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
Ping(command)
    char *command;		/* the user's command */
{
    int serverID;		/* server to ping */
    int commandLength;		/* characters in user command */
    char *usage;		/* "usage" string in case of user error */
    ReturnStatus status;
    char buffer[16 * 1024];
    Time deltaTime;
    int	numEchoes = 100;
    int	blockSize = 32;

    usage = "usage: Ping host_ID\n";
    commandLength = strlen(command);
    if (commandLength <= strlen("Ping ")) {
	Test_PutMessage(usage);
	return;
    }
    command += strlen("Ping ");
    serverID = atoi(command);
    Test_PutMessage("Pinging server ");
    Test_PutDecimal(serverID);
    Test_PutMessage("\n");

    status = DoTest(TEST_RPC_SEND, serverID, numEchoes, blockSize, buffer,
		    buffer, &deltaTime);
    if (status != SUCCESS) {
	Test_PutMessage("RPC failed: ");
	Test_PutMessage(Stat_GetMsg(status));
	Test_PutMessage("\n");
    } else {
	Test_PutMessage("Send ");
	Test_PutDecimal(blockSize);
	Test_PutMessage(" bytes ");
	Test_PutDecimal(deltaTime.seconds);
	Test_PutMessage(" seconds, ");
	Test_PutDecimal(deltaTime.microseconds);
	Test_PutMessage(" microseconds.\n");
    }
}


/*
 *----------------------------------------------------------------------
 *
 * DoTest --
 *
 *	Actually do the RPC ping test.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static ReturnStatus
DoTest(command, serverID, numEchoes, size, inDataBuffer, outDataBuffer, 
       timePtr)
    int command;
    int serverID;
    int numEchoes;
    int size;
    Address inDataBuffer;
    Address outDataBuffer;
    Time *timePtr;
{
    Rpc_EchoArgs echoArgs;
    ReturnStatus error;

    echoArgs.serverID = serverID;
    echoArgs.n = numEchoes;
    echoArgs.size = size;
    echoArgs.inDataPtr = inDataBuffer;
    echoArgs.outDataPtr = outDataBuffer;
    echoArgs.deltaTimePtr = timePtr;
    error = Test_Rpc(command, (Address)&echoArgs);

    return(error);
}


/*
 *----------------------------------------------------------------------
 *
 * Cd --
 *
 *	Change the current working directory.
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
Cd(command)
    char *command;		/* user's command string */
{
    int commandLength = strlen(command);

    if (commandLength <= strlen("cd ")) {
	Test_PutMessage("Usage: cd directory\n");
	return;
    }

    command += strlen("cd ");
    if (chdir(command) < 0) {
	perror(command);
    }
}
