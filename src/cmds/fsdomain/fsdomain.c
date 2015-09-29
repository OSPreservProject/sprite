/*
 * fsdomain.c --
 *
 *     Read and possibly change the domain prefix of a file system.
 *  
 */

#ifndef lint
static char rcsid[] = "$Header: /sprite/src/cmds/fsdomain/RCS/fsdomain.c,v 1.5 92/03/12 16:36:36 voelker Exp $";
#endif not lint

#include "sprite.h"
#include "sys/file.h"
#include "stdio.h"
#include "errno.h"

#include "disk.h"
#include "option.h"

/*
 * The size is declared in <kernel/lfsSuperBlock.h>, and is 64
 * bytes.
 */
#define MAX_LFS_DOMAIN_PREFIX 63  
/*
 * OFS_SUMMARY_PREFIX_LENGTH is defined in <kernel/ofs.h>
 */
#define MAX_OFS_DOMAIN_PREFIX OFS_SUMMARY_PREFIX_LENGTH - 1

#define INV_DOMAIN -9999
int     newDomainNum = INV_DOMAIN;
int     newServerID = -1;
Boolean previousCheckPoint = FALSE;
Boolean printInfo = FALSE;
char    *newDomain = NULL;

Option optionArray[] = {
    {OPT_DOC, "", (Address)NIL,
	 "fsdomain device"},
    {OPT_STRING, "name", (char *)&newDomain,
         "New domain name"},
    {OPT_INT, "d", (Address)&newDomainNum,
	 "New domain number"},
    {OPT_INT, "id", (Address)&newServerID,
	 "New server ID"},
    {OPT_TRUE, "print", (Address)&printInfo,
	 "Print out values of file system structures."},
    {OPT_TRUE, "prev", (Address)&previousCheckPoint,
	 "Use the previous (older) checkpoint.  Useful for when the kernel\n\tand the disk get out of sync on LFS checkpoints."}
};

static int numOptions = sizeof(optionArray) / sizeof(Option);

/*
 *----------------------------------------------------------------------
 *
 * ChangeLfsDomainPrefix
 *
 *      Change the domain prefix of an LFS file system.  If
 *      newDomain is NULL, the old domain prefix is printed to stdout.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *	Writes the new domain prefix to the LFS file system on stream;
 *      Prints to stdout.
 *
 *----------------------------------------------------------------------
 */
void
ChangeLfsDomainPrefix(stream, labelPtr, deviceName, newDomain)
    int stream;
    Disk_Label *labelPtr;
    char *deviceName;
    char *newDomain;
{
    LfsCheckPointHdr *headerPtr;
    int              len;
    int              status;
    int              area = -1;
    Boolean          writeCheckPoint = FALSE;

    headerPtr = Disk_ReadLfsCheckPointHdr(stream, labelPtr, &area);
    printf("Latest checkpoint is #%d", area);
    if (previousCheckPoint == TRUE) {
	area = (area == 0) ? 1 : 0;
	printf("...using #%d instead.\n", area);
	headerPtr = Disk_ReadLfsCheckPointHdr(stream, labelPtr, &area);
    } else {
	putchar('\n');
    }
    if (headerPtr == NULL || area < 0 || area > 1) {
	return;
    }
    if (printInfo) {
	LfsCheckPointTrailer *trailerPtr;

	printf("LFS Checkpoint Header on %s:\n\n", deviceName);
	Disk_PrintLfsCheckPointHdr(headerPtr);
	printf("\nLFS Checkpoint Regions on %s:\n\n", deviceName);
	Disk_ForEachCheckPointRegion(headerPtr, 
				     Disk_PrintLfsCheckPointRegion);
	printf("\nLFS Checkpoint Trailer on %s:\n\n", deviceName);
	trailerPtr = Disk_LfsCheckPointTrailer(headerPtr);
	Disk_PrintLfsCheckPointTrailer(trailerPtr);
	return;
    }
    if (newDomain != NULL) {
	len = strlen(newDomain);
	if (len > MAX_LFS_DOMAIN_PREFIX) {
	    printf("The LFS limits domain prefixes to %d characters.\n",
		   MAX_LFS_DOMAIN_PREFIX);
	    printf("%s is %d characters too long.\n", newDomain,
		   len - MAX_LFS_DOMAIN_PREFIX);
	    return;
	}
	sprintf(headerPtr->domainPrefix, "%s", newDomain); 
	writeCheckPoint = TRUE;
    }
    if (newDomainNum != INV_DOMAIN) {
	headerPtr->domainNumber = newDomainNum;
	writeCheckPoint = TRUE;
    }
    if (newServerID > 0) {
	headerPtr->serverID = newServerID;
	writeCheckPoint = TRUE;
    }
    if (writeCheckPoint) {
	status = Disk_WriteLfsCheckPointHdr(stream, headerPtr, area, 
					    labelPtr); 
	if (status != 0) {
	    fprintf(stderr, "fsdomain: could not write LFS checkpoint ");
	    fprintf(stderr, "header: %s\n", deviceName);
	    free((Address)headerPtr);
	    return;
	}
    }	
    printf("%s:\t%s\n", deviceName, headerPtr->domainPrefix);
    printf("Server ID:\t%d\n", headerPtr->serverID);
    printf("Domain number:\t%d\n", headerPtr->domainNumber);
    free((Address)headerPtr);
    return;
}

/*
 *----------------------------------------------------------------------
 *
 * ChangeOfsDomainPrefix
 *
 *      Change the domain prefix of an OFS file system.  If
 *      newDomain is NULL, the old domain prefix is printed to stdout.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *	Writes the new domain prefix to the OFS file system on stream,
 *      modifying the Ofs_SummaryInfo;
 *      Prints to stdout.
 *
 *----------------------------------------------------------------------
 */
ChangeOfsDomainPrefix(stream, labelPtr, deviceName, newDomain)
    int stream;
    Disk_Label *labelPtr;
    char *deviceName;
    char *newDomain;
{
    Ofs_SummaryInfo  *summaryPtr;
    Ofs_DomainHeader *domainPtr;
    int              len;
    int              status;
    Boolean          writeSummary = FALSE;
    Boolean          writeDomain = FALSE;

    summaryPtr = Disk_ReadSummaryInfo(stream, labelPtr);
    if (summaryPtr == NULL) {
	printf(stderr, "fsdomain: couldn't read Ofs_SummaryInfo\n");
	return;
    }
    domainPtr = Disk_ReadDomainHeader(stream, labelPtr);
    if (domainPtr == NULL) {
	printf(stderr, "fsdomain: couldn't read Ofs_DomainHeader\n");
	return;
    }
    if (printInfo) {
	printf("OFS Domain Header on %s:\n\n", deviceName);
	Disk_PrintDomainHeader(domainPtr);
	printf("\nOFS Summary Info on %s:\n\n", deviceName);
	Disk_PrintSummaryInfo(summaryPtr);
	return;
    }
    if (newDomain != NULL) {
	len = strlen(newDomain);
	if (len > MAX_OFS_DOMAIN_PREFIX) {
	    printf("The OFS limits domain prefixes to %d characters.\n",
		   MAX_OFS_DOMAIN_PREFIX);
	    printf("%s is %d characters too long.\n", newDomain,
		   len - MAX_OFS_DOMAIN_PREFIX);
	    free((Address)summaryPtr);
	    return;
	}
	sprintf(summaryPtr->domainPrefix, "%s", newDomain);
	writeSummary = TRUE;
    }
    if (newDomainNum != INV_DOMAIN) {
	summaryPtr->domainNumber = newDomainNum;
	writeSummary = TRUE;
    }
    if (newServerID > 0) {
	domainPtr->device.serverID = newServerID;
	writeDomain = TRUE;
    }
    if (writeSummary) {
	status = Disk_WriteSummaryInfo(stream, labelPtr, summaryPtr);
	if (status != 0) {
	    fprintf(stderr, "fsdomain: could not write OFS summary ");
	    fprintf(stderr, "sector: %s\n", deviceName);
	    free((Address)summaryPtr);
	    return;
	}
    }
    if (writeDomain) {
	status = Disk_WriteDomainHeader(stream, labelPtr, domainPtr);
	if (status != 0) {
	    fprintf(stderr, "fsdomain: could not write OFS domain ");
	    fprintf(stderr, "header: %s\n", deviceName);
	    free((Address)summaryPtr);
	    return;
	}
    }
    printf("%s:\t%s\n", deviceName, summaryPtr->domainPrefix);
    printf("Server ID:\t%d\n", domainPtr->device.serverID);
    printf("Domain number:\t%d\n", summaryPtr->domainNumber);
    free((Address)summaryPtr);
    return;
}

/*
 *----------------------------------------------------------------------
 *
 * main --
 *
 *      Open the device stream, read and possibly change the
 *      domain prefix of the file system found on the stream.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *	Prints to stdout.
 *
 *----------------------------------------------------------------------
 */
void
main(argc, argv)
    int argc;
    char *argv[];
{
    int        stream;
    int        fstype;
    Disk_Label *labelPtr;
    char       *deviceName;

    argc = Opt_Parse(argc, argv, optionArray, numOptions);
    if (argc != 2) {
	Opt_PrintUsage(argv[0], optionArray, numOptions);
	exit(FAILURE);
    }
    deviceName = argv[1];
    /*
     * If there are no new values, then just report the current ones.
     */
    if (newDomain == NULL && newDomainNum == INV_DOMAIN && newServerID < 0) {
	stream = open(deviceName, O_RDONLY, 0);
    } else {
	stream = open(deviceName, O_RDWR, 0);
    }
    if (stream < 0) {
	perror("opening device");
	exit(FAILURE);
    }

    labelPtr = Disk_ReadLabel(stream);
    if (labelPtr == NULL) {
	printf("fsdomain: cannot find label on device %s.  ", deviceName);
	printf("Is the label\non the disk of the correct type for");
	printf("the machine being used?\n");
    }

    fstype = Disk_HasFilesystem(stream, labelPtr);
    switch (fstype) {
    case DISK_HAS_OFS:
	ChangeOfsDomainPrefix(stream, labelPtr, deviceName, newDomain);
	break;
    case DISK_HAS_LFS:
	ChangeLfsDomainPrefix(stream, labelPtr, deviceName, newDomain);
	break;
    default:
	printf("%s: no file system found.\n", deviceName);
	break;
    }
    free((Address)labelPtr);
}
