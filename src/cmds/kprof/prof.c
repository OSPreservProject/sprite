#include "sprite.h"
#include "stdio.h"

main(argc, argv)
    int argc;
    char *argv[];
{
    ReturnStatus status;
    char *fileName;
    char *option;

    if (argc < 2) {
	fprintf(stderr,"Usage: %s <start | end | dump filename>\n",argv[0]);
	exit(1);
    } else {
	if (argc > 3) {
	    printf("Extra arguments ignored: %s...\n", argv[3]);
	}
	option = argv[1];
	fileName = argv[2];
    }
    if (strcmp(option, "start") == 0) {
	status = Prof_Start();
    } else if (strcmp(option, "end") == 0) {
	status = Prof_End();
    } else if (strcmp(option, "dump") == 0) {
	status = Prof_Dump(fileName);
    } else {
	printf(stderr,"%s: Unknown option: %s\n",argv[0], option);
	status = 1;
    }

    exit(status);
}
