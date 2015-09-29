/* 
 * hash.c --
 *
 *	This file is a benchmark program to measure the cost of a
 *	hash table probe.  It should be invoked as follows:
 *	
 *	hash count [tableSize]
 *
 *	Where count is the number of probes to make and tableSize is
 *	the initial hash table size.  It puts 100 randome entries into
 *	the hash table, makes "count" probes, then prints out the
 *	average time per probe.
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

#include <stdio.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <hash.h>
#include <stdlib.h>

Hash_Table table;

#define STRING_LENGTH 10

main(argc, argv)
    int argc;
    char **argv;
{
    Hash_Entry *hPtr;
    int count, i, j;
    char names[100][STRING_LENGTH+1];
    struct timeval start, stop;
    struct timezone tz;
    int micros, tableSize;
    double timePer;

    if ((argc != 2) && (argc != 3)){
	fprintf(stderr, "Usage: getpid count [tableSize]\n");
	exit(1);
    }
    count = atoi(argv[1]);
    if (argc == 3) {
	tableSize = atoi(argv[2]);
    } else {
	tableSize = 0;
    }

    Hash_InitTable(&table, tableSize, HASH_STRING_KEYS);
    for (i = 0; i < 100; i++) {
	for (j = 0; j < STRING_LENGTH; j++) {
	    names[i][j] = 'A' + ((rand() >> 10) & 077);
	}
	names[i][STRING_LENGTH] = 0;
	Hash_CreateEntry(&table, names[i], (Boolean *) NULL);
    }

    gettimeofday(&start, (struct timezone *) NULL);

    j = 0;
    for (i = 0; i < count; i++) {
	hPtr = Hash_FindEntry(&table, names[j]);
	j++;
	if (j >= 100) {
	    j = 0;
	}
    }

    gettimeofday(&stop, (struct timezone *) NULL);
    micros = 1000000*(stop.tv_sec - start.tv_sec)
	    + stop.tv_usec - start.tv_usec;
    timePer = micros;
    printf("Time per probe: %.2f microseconds\n", timePer/count);
}
