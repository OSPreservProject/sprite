#include "stdio.h"
#include "usergraphics.h"

#define iters 400

main () {
    int t0, t1, w, h;
    register i;
    if (wm_NewWindow(0) == 0) exit(0);
    wm_ClearWindow();
    wm_SetTitle ("Timing test");
    fflush(winout);
    t0 = time(0);
    for (i = iters; --i>=0; )
    wm_GetDimensions(&w,&h);
    t1 = time(0);
    printf ("%d calls in %d seconds = %d usec/call\n", iters, t1-t0,
	(t1-t0)*1000000/iters);
}


FlagRedraw() {
}
