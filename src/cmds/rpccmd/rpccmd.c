/* 
 * rpccmd.c --
 *
 *	User interface to RPC related commands of the Fs_Command system call.
 *
 * Copyright (C) 1986 Regents of the University of California
 * All rights reserved.
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 */

#ifndef lint
static char rcsid[] = "$Header: /sprite/src/cmds/rpccmd/RCS/rpccmd.c,v 1.7 92/07/10 14:57:28 kupfer Exp $ SPRITE (Berkeley)";
#endif not lint

#include <sprite.h>
#include <status.h>
#include <option.h>
#include <stdio.h>
#include <fs.h>
#include <fsCmd.h>
#include <sysStats.h>
#include <rpc.h>
#include <host.h>

/*
 * Command line options.
 */

int	rpcEnable = -1;
int	rpcDebug = -1;
int	rpcTracing = -1;
int	rpc_NoTimeouts = -1;
int	rpcClientHist = -1;
int	rpcServerHist = -1;
Boolean	resetClientHist = FALSE;
Boolean	resetServerHist = FALSE;
char	*rpcEchoHost = (char *)0;
int	blockSize = 32;
int	rpcMaxServers = -1;
int	rpcNumServers = -1;
int	rpcNegAcks = -1;
int	rpcChannelNegAcks = -1;
int	rpcNumNackBufs = -1;
int	numEchoes = 100;
int	sanityChecks = -1;

Option optionArray[] = {
    {OPT_TRUE, "on", (Address) &rpcEnable, 
	"\tAllow servicing of RPC requests."},
    {OPT_FALSE, "off", (Address) &rpcEnable, 
	"\tTurn off servicing of RPC requests."},
    {OPT_STRING, "ping", (Address) &rpcEchoHost, 
	"\tRPC test against the specified host."},
    {OPT_INT, "b", (Address) &blockSize, 
	"\tBlock size to send in RPC test."},
    {OPT_INT, "t", (Address) &rpcTracing, 
	"\tSet rpc tracing flag."},
    {OPT_INT, "T", (Address) &rpc_NoTimeouts, 
	"\tSet rpc no timeouts flag."},
    {OPT_INT, "D", (Address) &rpcDebug, 
	"\tSet rpc debug flag."},
    {OPT_INT, "C", (Address) &rpcClientHist, 
	"\tSet client histogram flag."},
    {OPT_INT, "S", (Address) &rpcServerHist, 
	"\tSet server histogram flag."},
    {OPT_TRUE, "Creset", (Address) &resetClientHist, 
	"\tReset client RPC histograms."},
    {OPT_TRUE, "Sreset", (Address) &resetServerHist, 
	"\tReset server RPC histograms."},
    {OPT_INT, "maxServers", (Address) &rpcMaxServers,
	"\tSet the maximum number of allowed rpc server processes."},
    {OPT_INT, "numServers", (Address) &rpcNumServers,
	"\tCreate more rpc servers until this number exists."},
    {OPT_TRUE, "negAcksOn", (Address) &rpcNegAcks,
	"\tTurn on negative acknowledgements on a server."},
    {OPT_FALSE, "negAcksOff", (Address) &rpcNegAcks,
	"\tTurn off negative acknowledgements on a server (default)."},
    {OPT_INT, "numNackBufs", (Address) &rpcNumNackBufs,
	"\tMake sure this number of negative ack buffers exists."},
    {OPT_TRUE, "channelNegAcksOn", (Address) &rpcChannelNegAcks,
	"\tSet the client policy for handling negative acks to ramp down number of channels."},
    {OPT_FALSE, "channelNegAcksOff", (Address) &rpcChannelNegAcks,
	"\tSet the client policy for handling negative acks to the default backoff policy."},
    {OPT_INT, "numPings", (Address) &numEchoes,
	"\tNumber of pings to send."},
    {OPT_INT, "sanity", (Address)&sanityChecks,
	"\tTurn off/on rpc sanity checks."},

};
int numOptions = sizeof(optionArray) / sizeof(Option);

/* forward references: */

static void ResetClientHist();
static void ResetServerHist();
ReturnStatus SetFlag();


/*
 *----------------------------------------------------------------------
 *
 * main --
 *
 *	Collects arguments and branch to the code for the fs command.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Calls Fs_Command...
 *
 *----------------------------------------------------------------------
 */
main(argc, argv)
    int argc;
    char *argv[];
{
    register ReturnStatus status = SUCCESS;	/* status of system calls */

    argc = Opt_Parse(argc, argv, optionArray, numOptions, 0);

    /*
     * Set various rpc system flags.  The Fs_Command system call returns
     * the old value of the flag in place of the value passed in.
     */
    if (rpcEnable != -1) {
	status = Sys_Stats(SYS_RPC_ENABLE_SERVICE, rpcEnable, NIL);
    }
    if (rpcTracing != -1) {
	status = SetFlag(FS_SET_RPC_TRACING, rpcTracing, "RPC tracing");
    }
    if (rpc_NoTimeouts != -1) {
	status = SetFlag(FS_SET_RPC_NO_TIMEOUTS, rpc_NoTimeouts,
					    "No RPC timeouts");
    }
    if (rpcDebug != -1) {
	status = SetFlag(FS_SET_RPC_DEBUG, rpcDebug, "RPC debug prints");
    }
    if (rpcClientHist != -1) {
	status = SetFlag(FS_SET_RPC_CLIENT_HIST, rpcClientHist,
					     "Client RPC timing histograms");
    }
    if (rpcServerHist != -1) {
	status = SetFlag(FS_SET_RPC_SERVER_HIST, rpcServerHist,
					    "Server RPC timing histograms");
    }
    if (resetClientHist) {
	ResetClientHist();
    }
    if (resetServerHist) {
	ResetServerHist();
    }
    if (rpcMaxServers != -1) {
	status = Sys_Stats(SYS_RPC_SET_MAX, rpcMaxServers, NIL);
    }
    if (rpcNumServers != -1) {
	status = Sys_Stats(SYS_RPC_SET_NUM, rpcNumServers, NIL);
    }
    if (rpcNegAcks != -1) {
	status = Sys_Stats(SYS_RPC_NEG_ACKS, rpcNegAcks, NIL);
    }
    if (rpcNumNackBufs != -1) {
	status = Sys_Stats(SYS_RPC_NUM_NACK_BUFS, rpcNumNackBufs, NIL);
    }
    if (rpcChannelNegAcks != -1) {
	status = Sys_Stats(SYS_RPC_CHANNEL_NEG_ACKS, rpcChannelNegAcks, NIL);
    }
    if (rpcEchoHost != (char *)0) {
	status = RpcEcho(rpcEchoHost);
    }
    if (sanityChecks != -1) {
	status = Sys_Stats(SYS_RPC_SANITY_CHECK, sanityChecks, NIL);
    }
    exit(status);
}

RpcEcho(server)
    char *server;
{
    Host_Entry *entryPtr;
    int serverID;
    int myID;
    int error;
    char buffer[16 * 1024];
    Time deltaTime;

    entryPtr = Host_ByName(server);
    if (entryPtr == (Host_Entry *) NULL) {
	fprintf(stderr, "Unable to get host number for host '%s'\n",
		       server);
	exit(1);
    }
    serverID = entryPtr->id;

    error = Proc_GetHostIDs((int *) NULL, &myID);
    if (error != SUCCESS) {
	perror( "Proc_GetHostIDs");
	exit(error);
    }
    if (myID == serverID) {
	fprintf(stderr, "Unable to send RPC to yourself.\n");
	exit(1);
    }
    error = DoTest(TEST_RPC_SEND, serverID, numEchoes, blockSize, buffer,
	    buffer, &deltaTime);
    if (error != SUCCESS) {
	Stat_PrintMsg(error, "RPC failed:");
    } else {
	printf("Send %d bytes %d.%06d sec\n", blockSize, deltaTime.seconds,
		deltaTime.microseconds);
    }
    return(error);
}
ReturnStatus
DoTest(command, serverID, numEchoes, size, inDataBuffer, outDataBuffer, timePtr)
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
    error = Test_Rpc(command, &echoArgs);

    return(error);
}

ReturnStatus
SetFlag(command, value, comment)
    int command;		/* Argument to Fs_Command */
    int value;			/* Value for flag */
    char *comment;		/* For Io_Print */
{
    register int newValue;
    register ReturnStatus status;

    newValue = value;
    status = Fs_Command(command, sizeof(int), (Address) &value);
    printf("%s %s, was %s\n", comment,
		     newValue ? "on" : "off",
		     value ? "on" : "off");
    return(status);
}


/*
 *----------------------------------------------------------------------
 *
 * ResetClientHist --
 *
 *	Reset the client-side histograms.
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
ResetClientHist()
{
    ReturnStatus status;

    status = Sys_Stats(SYS_RPC_CLIENT_HIST, 0, NULL);
    if (status != SUCCESS) {
	fprintf(stderr, "Can't reset client RPC histograms: %s\n",
		Stat_GetMsg(status));
    }
}


/*
 *----------------------------------------------------------------------
 *
 * ResetServerHist --
 *
 *	Reset the server-side histograms.
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
ResetServerHist()
{
    ReturnStatus status;

    status = Sys_Stats(SYS_RPC_SERVER_HIST, 0, NULL);
    if (status != SUCCESS) {
	fprintf(stderr, "Can't reset server RPC histograms: %s\n",
		Stat_GetMsg(status));
    }
}
