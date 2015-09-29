/*
 * MOP server daemon.
 */

#include <stdio.h>
#include <sys/file.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <net/if_arp.h>
#include <netinet/if_ether.h>
#include <netEther.h>
#ifndef mip
#define mips 1
#define notreallymips
#endif
#ifndef LANGUAGE_C
#define LANGUAGE_C
#endif
#ifndef MIPSEL
#define MIPSEL
#endif
#include <ds3100.md/sys/exec.h>
#ifdef notreallymips
#undef mips
#endif
#include <host.h>
#include <mopd.h>
#include <signal.h>
#include <varargs.h>
#include <time.h>
#include <string.h>

Net_EtherAddress myAddr;
Net_EtherAddress multiAddr;

unsigned char broadcast[] = MOP_BROADCAST;

int debug = 0;		/* Flag for debug mode. */

extern int errno;

#define TIMEOUTSEC 20

#define WAITING 0
#define SENDING 1
#define DONE	2

int mode=WAITING;	/* The state we're in. */
int seq, seqHi;		/* The current sequence number. */
int bootFd;		/* The boot program fd. */
int mopFd;		/* The mop device fd. */
u_char client[6];	/* The client we're dealing with. */
int loadAddr, length, startAddr, fileOffset; /* Boot file parameters. */
char filename[100];

static void log();
int timer();

#if 0
static char	*mopdlog = "/sprite/admin/mopdlog";
static char	*openFlags = "a+";
#else
static char	*mopdlog = "/dev/syslog";
static char	*openFlags = "w";
#endif

void
main(argc,argv)
    int argc;
    char **argv;
{
    unsigned char buf[BUF_LEN];
    int len;
    u_char us[6], them[6];
    char myName[50];
    mopHdr *mopHdrPtr = (mopHdr *)buf;
    Host_Entry *myEntry, *theirEntry;
    Net_Address theirNetAddress;
    int		i;

    multiAddr = * (Net_EtherAddress *) broadcast;
    if (gethostname(myName,50)<0) {
	perror("gethostname");
	exit(-1);
    }
    myEntry = Host_ByName(myName);
    if (myEntry == NULL) {
	perror("Host_ByName");
	exit(-1);
    }
    for (i = 0; i < myEntry->numNets; i++) {
	if (myEntry->nets[i].netAddr.type == NET_ADDRESS_ETHER) {
	    (void) Net_GetAddress(&myEntry->nets[i].netAddr,
			(Address) &myAddr);
	    break;
	}
    }
    Host_End();
    for (argc--, argv++ ; argc > 0 ; argc--, argv++) {
	if (argv[0][0] == '-') {
	    switch (argv[0][1]) {
		case 'd':
		    debug++;
		    break;
	    }
        }
    }

    if (debug==0) {
	if (fork()) exit(0);
    }

    signal(SIGALRM, timer);

    mopFd = open(MOP_DEVICE, O_RDWR, 0);
    if (mopFd < 0) {
	perror(MOP_DEVICE);
	exit(-1);
    }

    log("Mop server %s starting", myName);

#define US mopHdrPtr->header.ether_dhost
#define THEM mopHdrPtr->header.ether_shost
    while (1) {
	len = read(mopFd, buf, 1000);

	if (debug>1) {
	    log("Packet from %2.2x:%2.2x:%2.2x:%2.2x:%2.2x:%2.2x to %2.2x:%2.2x:%2.2x:%2.2x.", THEM[0],THEM[1],THEM[2],THEM[3],THEM[4],THEM[5], US[0],
		    US[1], US[2],US[3],US[4],US[5]);
	}

	if (!bcmp(&multiAddr, US, sizeof(Net_EtherAddress))) {
	    if (debug) {
		log("Got a broadcast packet");
	    }
	    Host_Start();
	    theirNetAddress.type = NET_ADDRESS_ETHER;
	    bcopy(THEM,&theirNetAddress.address.ether,
		    sizeof(Net_EtherAddress));
	    theirEntry = Host_ByNetAddr(&theirNetAddress);
	    Host_End();
	    if (theirEntry == NULL) {
		log("Packet from unknown %2.2x:%2.2x:%2.2x:%2.2x:%2.2x:%2.2x",
			THEM[0],THEM[1],THEM[2],THEM[3],THEM[4],THEM[5]);
		continue;
	    }
	} else if (bcmp(&myAddr, US, sizeof(Net_EtherAddress))) {
	    log("Mystery packet to %2.2x:%2.2x:%2.2x:%2.2x:%2.2x:%2.2x",
		    US[0],US[1],US[2],US[3],US[4],US[5]);
	    continue;
	}

	if (debug>2) {
	    log("Received: %d", mopHdrPtr->code);
	}

	if (mode == WAITING) {
	    bcopy(THEM,client,6);
	} else if (bcmp(client, THEM,6)!=0) {
	    /*
	     * We can't handle two clients at once, so this one must wait.
	     */
	    if (debug) {
		log("Mop request collision");
	    }
	    continue;
	}
	 
	if (mopHdrPtr->code == MOP_REQ) {
	    if (initRequest(buf)<0) continue;
	}
	if (mode==SENDING && (mopHdrPtr->code == MOP_REQ_LOAD ||
		mopHdrPtr->code == MOP_REQ)) {
	    alarm(TIMEOUTSEC);
	    sendData(buf);
	} else if (mode==DONE && mopHdrPtr->code == MOP_REQ_LOAD) {
	    if (debug) {
		mopReqLoad *mopReqLoadPtr = (mopReqLoad *)(buf+HDR_LEN);
		log("Load completed: status %d", mopReqLoadPtr->error);
	    }
	    alarm(0);
	    close(bootFd);
	    mode = WAITING;
	} else {
	    log("Out of order function: %d", mopHdrPtr->code);
	    if (mode==1) {
		close(bootFd);
	    }
	    mode = WAITING;
	}
    }
}

/*
 * Transmit a packet.
 */
mopXmit(fd, src, dst, len, code, buf)
    int fd;
    u_char *src;
    u_char *dst;
    int	len;
    int code;
    char *buf;
{
    short shortVal;
    mopHdr *mopHdrPtr = (mopHdr *)buf;
    if (debug>2) {
	log("Transmitting: code %d, len: %d", code, len);
    }
    bcopy(src, mopHdrPtr->header.ether_shost, 6);
    bcopy(dst, mopHdrPtr->header.ether_dhost, 6);
    shortVal = SWAP2(NET_ETHER_MOP);
    bcopy(&shortVal, &mopHdrPtr->header.ether_type, sizeof(short));
    shortVal = len+1; /* One byte for the code */
    bcopy(&shortVal, &mopHdrPtr->len, sizeof(short));
    mopHdrPtr->code = code;
    if (write(fd, buf, HDR_LEN+len)<0) {
	log("MOP packet write error: %d", errno);
    }
}

/*
 * Find how much code we should give the client.
 */
int
findStart(inFd, fileOffset, loadAddr, progLen, startAddr)
int inFd;			/* Fd for us to use */
int *fileOffset;		/* Return: offset of program in file. */
int *loadAddr;			/* Return: address to load program. */
int *progLen;			/* Return: the length of the program. */
int *startAddr;			/* Return: start of execution. */
{
    struct filehdr fileHdr;
    struct aouthdr aoutHdr;

    read(inFd, &fileHdr, sizeof(struct filehdr));
    read(inFd, &aoutHdr, sizeof(struct aouthdr));
    aoutHdr.magic = UNSWAP2(aoutHdr.magic);
    aoutHdr.vstamp = UNSWAP2(aoutHdr.vstamp);
    aoutHdr.text_start = UNSWAP4(aoutHdr.text_start);
    aoutHdr.tsize = UNSWAP4(aoutHdr.tsize);
    aoutHdr.dsize = UNSWAP4(aoutHdr.dsize);
    aoutHdr.entry = UNSWAP4(aoutHdr.entry);
    fileHdr.f_magic = UNSWAP2(fileHdr.f_magic);
    fileHdr.f_nscns = UNSWAP2(fileHdr.f_nscns);
    if (fileHdr.f_magic != MIPSELMAGIC || aoutHdr.magic != OMAGIC) {
	log("Bad magic number on boot file %s", filename);
	log("f_magic = %x, want %x.  a_magic = %x, want %x",
	    fileHdr.f_magic, MIPSELMAGIC, aoutHdr.magic, OMAGIC);
    }
    *fileOffset = N_TXTOFF(fileHdr, aoutHdr);
    *loadAddr = aoutHdr.text_start;
    *progLen = aoutHdr.tsize+aoutHdr.dsize;
    *startAddr = aoutHdr.entry;
    if (debug) {
	log("File: offset: %x, addr: %x, len: %x, start: %x\n", *fileOffset,
		*loadAddr, *progLen, *startAddr);
    }
}

/*
 * Open our input file.
 */
int
openFile(mopPtr)
mopReq *mopPtr;
{
    int bootFd;
    char *machineType;
    char *name;
    char *strPtr;

    if (mopPtr->devType == 0x43) {
	machineType = "ds3100.md";
    } else if (mopPtr->devType == 0x5e) {
	machineType = "ds5000.md";
    } else {
	log("Unknown machine type %d", mopPtr->devType);
	return -1;
    }
    name = (mopPtr->softID != '\0') ? &mopPtr->softID : "sprite";
    /*
     * Drop any boot flags.
     */
    strPtr = strchr(name,' ');
    if (strPtr != NULL) {
	*strPtr = '\0';
    }
    sprintf(filename,"%s/%s/%s", BOOT_DIR, machineType, name);
    bootFd = open(filename, O_RDONLY, 0);
    if (debug) {
	log("Booting %s", filename);
    }
    if (bootFd < 0) {
	log("Couldn't open %s", filename);
	/*
	 * See if we have an error file we can return.
	 */
	sprintf(filename,"%s/%s/%s", BOOT_DIR, machineType, "err");
	bootFd = open(filename, O_RDONLY, 0);
	if (bootFd < 0) {
	    return -1;
	}
    }
    return bootFd;
}

/*
 * log an error message
 *
 */
static void
log(va_alist)
    va_dcl
{
        FILE *fp;
        char *format;
        va_list args;
        time_t now;
        char *t;

        va_start(args);
        time(&now);
        t = asctime(localtime(&now));
        /* remove the newline */
        t[24] = '\0';
        if ((fp = fopen(mopdlog, openFlags)) == NULL) {
	    perror(mopdlog);
	    return;
	}
        fprintf(fp, "[mopd %s]: ", t);
        format = va_arg(args, char *);
        vfprintf(fp, format, args);
        putc('\n', fp);
        fclose(fp);
        return;
}

/*
 * Handle an initial request.
 */
initRequest(buf)
    char *buf;
{
    mopReq *mopPtr = (mopReq *)(buf+HDR_LEN);

    if (mode!=WAITING) {
	if (debug) {
	    log("Wasn't expecting request");
	}
    }
    seq = 0;
    seqHi = 0;
    bootFd = openFile(mopPtr);
    if (bootFd<0) {
	if (debug) {
	    log("openFile failed");
	}
	mode = WAITING;
	return -1;
    }
    mode = SENDING;
    if (findStart(bootFd, &fileOffset, &loadAddr, &length,
	    &startAddr)<0) {
	if (debug) {
	    log("findStart failed");
	}
	mode = WAITING;
	close(bootFd);
	return -1;
    }
    return 0;
}

/*
 * Send the next data packet.
 */
sendData(buf)
{
    mopHdr *mopHdrPtr = (mopHdr *)buf;
    mopLoad *mopLoadPtr = (mopLoad *)(buf+HDR_LEN);
    mopReqLoad *mopReqLoadPtr = (mopReqLoad *)(buf+HDR_LEN);
    mopParamLoad *mopParamLoadPtr = (mopParamLoad *)(buf+HDR_LEN);
    int intVal;
    int seqReq;
    int curPos, toRead;
    /*
     * Check the sequence numbers.
     */
    if (mopHdrPtr->code == MOP_REQ) {
	seqReq = 0;
    } else {
	seqReq = mopReqLoadPtr->loadNum;
	if (debug) {
	    log("Requested sequence number %d\n", seqReq);
	}
	if (mopReqLoadPtr->error != 0) {
	    if (debug) {
		log("Load error status %d", mopReqLoadPtr->error);
	    }
	    mode = WAITING;
	    close(bootFd);
	    return;
	}
    }
    if (seqReq != seq) {
	log("Out of sequence request %d vs %d from %2.2x:%2.2x:%2.2x:%2.2x:%2.2x:%2.2x\n", seqReq, seq, THEM[0], THEM[1], THEM[2], THEM[3], THEM[4], THEM[5]);
	/* Somehow we need to get back into sync. */
	seq = seqReq;
	return;
    }
    /*
     * Transmit the data.
     */
    curPos = PACKET_SIZE*(seq+seqHi*256);
    toRead = length-curPos;
    if (toRead > PACKET_SIZE) {
	toRead = PACKET_SIZE;
    }
    if (toRead > 0) {
	lseek(bootFd, fileOffset+curPos, 0);
	if (read(bootFd, buf+HDR_LEN+MOPLOAD_LEN, toRead) != toRead) {
	    log("Short boot file %s",filename);
	    mode = WAITING;
	    close(bootFd);
	    return;
	}
	mopLoadPtr->loadNum = seq;
	intVal = UNSWAP4((loadAddr + curPos));
	bcopy(&intVal, mopLoadPtr->loadAddr, sizeof(int));
	toRead = PACKET_SIZE; /* HACK */
	mopXmit(mopFd, &myAddr, client, MOPLOAD_LEN+toRead, MOP_LOAD, buf);
    } else {
	mopParamLoadPtr->loadNum = seq;
	mopParamLoadPtr->parameters = 0;
	intVal = UNSWAP4(startAddr);
	if (debug) {
	    log("Transferring to %x", startAddr);
	}
	bcopy(&intVal, &mopParamLoadPtr->parameters+1, sizeof(int));
	mopXmit(mopFd, &myAddr, client, sizeof(mopParamLoad)+sizeof(int),
		MOP_PARAM_LOAD_XFER, buf);
	mode = DONE;
    }
    seq++;
    if (seq==256) {
	seq=0;
	seqHi++;
    }
}

/*
 * Handle a timeout.
 */
timer()
{
    if (mode != WAITING) {
	log("Timeout");
	close(bootFd);
	mode = WAITING;
    }
}
