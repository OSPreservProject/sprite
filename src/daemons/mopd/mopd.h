/*
 * MOP include file.
 */

#ifdef ds3100
#define SWAP2(x) ((((x)&0xff00)>>8)|(((x)&0x00ff)<<8))
#define SWAP4(x) ((((x)>>24)&0xff)|(((x)&0x00ff0000)>>8)|(((x)&0x0000ff00)<<8)|(((x)&0x000000ff)<<24))
#define UNSWAP2(x) (x)
#define UNSWAP4(x) (x)
#else
#define SWAP2(x) (x)
#define SWAP4(x) (x)
#define UNSWAP2(x) ((((x)&0xff00)>>8)|(((x)&0x00ff)<<8))
#define UNSWAP4(x) ((((x)>>24)&0xff)|(((x)&0x00ff0000)>>8)|(((x)&0x0000ff00)<<8)|(((x)&0x000000ff)<<24))
#endif

#define PACKET_SIZE 1024
#define BUF_LEN (PACKET_SIZE+64)

#define MOP_DEVICE "/dev/etherMOP"
#define BOOT_DIR "/sprite/boot"

#define MOP_BROADCAST {0xab,0x00,0x00,0x01,0x00,0x00}

#define HDR_LEN 17
typedef struct mopHdr {
    struct ether_header header;
    short		len;
    unsigned char	code;
} mopHdr;

#define MOP_LOAD_XFER	0
#define MOP_LOAD	2
#define MOP_REQ_DUMP	4
#define MOP_ENTER	6
#define MOP_REQ		8
#define MOP_REQ_LOAD	10
#define MOP_RUNNING	12
#define MOP_DUMP	14
#define MOP_REMOTE_11	16
#define MOP_REMOTE_11A	18
#define MOP_PARAM_LOAD_XFER	20
#define MOP_LOOPBACK	24
#define MOP_LOOP_DATA	26

#define MOPLOAD_LEN 5
typedef struct mopLoad {
    unsigned char	loadNum;	/* Load number for multiple images.  */
    char		loadAddr[4];	/* Memory load address. */
} mopLoad;

typedef struct mopReqDump {
    char		memAddr[4];	/* Memory dump address. */
    char		numLocs[2];	/* # locations to dump. */
} mopReqDump;

typedef struct mopEnter {
    char		passwd[4];	/* The mop password. */
} mopEnter;

typedef struct mopReq {
    unsigned char	devType;	/* Requesting system type. */
    unsigned char	mopVer;		/* Version of MOP format. */
    unsigned char	pgmType;	/* Type of program requested. */
    unsigned char	softIDLen;	/* Request identifier. */
    char     		softID;
} mopReq;

typedef struct mopReqLoad {
    unsigned char	loadNum;	/* Load being requested. */
    unsigned char	error;		/* Error status. */
} mopReqLoad;

typedef struct mopRunning {
    unsigned char	devType;	/* Requesting system type. */
    unsigned char	mopVer;		/* Version of MOP format. */
    char 		memSize[4];	/* Size of phys mem. */
    unsigned char	features;	/* System features. */
} mopRunning;

typedef struct mopDump {
    char		memAddr[4];	/* Starting address. */
    char		data;		/* The data. */
} mopDump;

typedef struct mopParamLoad {
    unsigned char	loadNum;	/* Load being requested. */
    char		parameters;	/* A bunch of parameters. */
} mopParamLoad;

#define fprintEth(file,x) fprintf(file,"%2.2x.%2.2x.%2.2x.%2.2x.%2.2x.%2.2x",\
	x[0], x[1], x[2], x[3], x[4], x[5]);
