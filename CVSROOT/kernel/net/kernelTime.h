#ifdef notdef

#include <devTMR.h>

#define	TIMEIT_SIZE 1024
struct timeIt {
    int	   place;
    unsigned int time;
};
extern struct timeIt timeItArray[TIMEIT_SIZE];
extern int	timeItPoint;
#define	AddTime(Zplace)  { \
		if (!dbg_UsingNetwork) { \
		timeItArray[timeItPoint].place = (Zplace); \
		Dev_TimerReadRegInline(&timeItArray[timeItPoint].time); \
		timeItPoint++; \
		if (timeItPoint >= TIMEIT_SIZE) \
		    timeItPoint = 0; \
		} \
		}
#else

#define	TIMEIT_SIZE 1024
struct timeIt {
    int	   place;
    unsigned int time;
};
#define AddTime(Zplace)
#endif
