typedef struct file29Hdr {
    unsigned short	id;
    unsigned short	sectionHdrCtr;
    long		timestamp;
    long		symbolFp;
    long		symbolCtr;
    unsigned short	optionalFhdrLength;
    unsigned short	fhdrFlags;
} file29Hdr;

typedef struct optionalFile29Hdr {
    unsigned short 	id;
    unsigned short	version;
    long		executibleSize;
    long		dataSize;
    long		bssSize;
    long		entryAddr;
    long		codeBaseAddr;
    long		dataBaseAddr;
} optionalFile29Hdr;

typedef struct section29Hdr {
    char		sectionName[8];
    long		loadAddr;
    long		virtualAddr;
    long		sectionSize;
    long		rawdFp;
    long		relocFp;
    long		lineFp;
    unsigned short	relocCtr;
    unsigned short	lineCtr;
    long		sectionFlag;
} section29Hdr;

#define STYPE_REG	0x0000
#define STYPE_TEXT	0x0020
#define STYPE_DATA	0x0040
#define STYPE_LIT	0x8020		/* same as STYPE_TEXT */

#define MAX_PACKET_SIZE	0x0300		/* max amt. to load in each packet */
