#include <stdio.h>
#include <stdlib.h>
#include <sprite.h>
#include <option.h>
#include <sys/time.h>


int	pageSize = 4096;
int	numPageFaults = 1000;
int	repeats = 3;
Boolean	pause = FALSE;
Boolean	trace = FALSE;
Boolean dirty = FALSE;
Boolean longcheck = FALSE;
Boolean staticArray = FALSE;
static  char sArray[4*1024*1024];

Option optionArray[] = {
    {OPT_INT, "p", (Address)&pageSize,
	"The page size to fault on (default 4096)."},
    {OPT_INT, "n", (Address)&numPageFaults,
	"Number of page faults (default 1000)"},
    {OPT_INT, "r", (Address)&repeats,
	"Number of repeats of faulting sequence (default 3)"},
    {OPT_TRUE, "P", (Address)&pause,
	"Wait for input before starting"},
    {OPT_TRUE, "t", (Address)&trace,
	"Should print out information as page faults happen"},
    {OPT_TRUE, "d", (Address)&dirty,
	"Dirty the page on each pass."},
    {OPT_TRUE, "c", (Address)&longcheck,
	"Do a more complete check of the page."},
    {OPT_TRUE, "s", (Address)&staticArray,
	"Page from the static 4meg array."},
};
int numOptions = sizeof(optionArray) / sizeof(Option);

static char	*big;

main(argc, argv)
    int  argc;
    char *argv[];
{
    register	char	*bigPtr;
    register	struct timeval	*beforeArr; 
    register	struct timeval	*afterArr;
    register	int	*intPtr;
    register	int	i;
    register	int	j;
    struct	timeval		before, after;
    char		*big;
    char		c;
    Boolean		once = FALSE;
    double		rate;

    (void)Opt_Parse(argc, argv, optionArray, numOptions,0);
    if (pause) {
	scanf("%c", &c);
    }
    if (staticArray) {
	big = sArray;
	numPageFaults = 4*1024*1024/pageSize;
    } else { 
	big = (char *)malloc(pageSize * numPageFaults);
    }
    if (big == NULL) {
	fprintf(stderr,"Can't allocated %d bytes of memory.\n",
			pageSize * numPageFaults);
	exit(1);
    }
    beforeArr = (struct timeval *)malloc(sizeof(struct timeval) * repeats);
    afterArr = (struct timeval *)malloc(sizeof(struct timeval) * repeats);
    if (beforeArr == NULL || afterArr == NULL) {
	fprintf(stderr,"Can't allocated memory for timing arrays.\n");
	exit(1);
    }
    gettimeofday(&before, NULL);
    for (j = 0; j < repeats; j++) {
	gettimeofday(&beforeArr[j], NULL);
	for (i = 1, bigPtr = big; i <= numPageFaults; i++, bigPtr += pageSize) {
	    intPtr = (int *)bigPtr;
	    if (dirty) {
		*intPtr = *intPtr;
	    }
	    if (!longcheck) {
		if (once && *intPtr != i) {
		    printf("Error on page %d address 0x%x ", i, intPtr);
		    printf("Found 0x%x should be 0x%x\n",*intPtr,i);
		    fflush(stdout);
		    abort();
		}
	    } else {
		int	*end = intPtr + pageSize/sizeof(int);
		int	*c;
		if (once) {
		   for (c = intPtr; c < end; c += 4) {
		       if (*c != i) {
			    printf("Error on page %d address 0x%x ", i,intPtr);
			    printf("Found 0x%x should be 0x%x\n",*c,i);
			    fflush(stdout);
			}
		    }
		}
	    }
	    if (trace && (i % 100 == 0)) {
		printf("%d\n", i);
		fflush(stdout);
	    }
	    if (!once || dirty) {
		if (!longcheck) {
		    *intPtr = i;
		} else {
		    int	*end = intPtr + pageSize/sizeof(int);
		    for (; intPtr < end; intPtr += 4) {
			*intPtr = i;
		    }
	       }
	    }
	}
	gettimeofday(&afterArr[j], NULL);
	once = TRUE;
	if (trace) {
	    printf("Pass %d\n\n", j + 1);
	    fflush(stdout);
	}
    }
    for (j = 0; j < repeats; j++) {
	rate = (afterArr[j].tv_sec - beforeArr[j].tv_sec) * 1000;
	rate += (afterArr[j].tv_usec - beforeArr[j].tv_usec) / 1000;
	rate = rate / numPageFaults;
	printf("Pass %d: %0.3f ms per page fault\n", j + 1, rate);
    }

    gettimeofday(&after,NULL);
    rate = (after.tv_sec - before.tv_sec) * 1000;
    rate += (after.tv_usec - before.tv_usec)*.001;
    rate = rate / (numPageFaults * repeats);
    printf("Totals: time=%0.3f sec, faults=%d rate=%0.3f ms per fault\n", 
		(after.tv_sec - before.tv_sec) + 
		(after.tv_usec - before.tv_usec) / 1000000.0,
		numPageFaults * repeats, rate);
    exit(0);
}
