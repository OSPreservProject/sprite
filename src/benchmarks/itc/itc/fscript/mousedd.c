/*
 * Device dependent core graphics driver for the mouse
 * 22-Oct-82 by Mike Shantz.
 *
 */
#include	"coretypes.h"
#include	"corevars.h"
/*
#include "stdio.h"
#include "sgtty.h"
#include "sys/mouse.h"

static int fd;
static int butt[3];
static int mousex, mousey;
static struct mouseinfo mi;
static lastx, lasty;
*/

/*------------------------------------*/
mousedd(ddstruct) register ddargtype *ddstruct;
{
/*    char c;			/* V7 RELEASE HAS NO MOUSE  *%
    int n, dx, dy, accel;

    switch(ddstruct->opcode) {
    case INITIAL:
	fd = open("/dev/mouse", 0);
	ioctl(fd, MIOCGPOS, &mi);
	lastx = mi.mi_x;  lasty = mi.mi_y;
	mousex = 0; mousey = 0;
	butt[0] = FALSE;  butt[1] = FALSE;  butt[2] = FALSE;
	break;
    case TERMINATE:
	break;
    case BUTREAD:
	ioctl(fd, MIOCGPOS, &mi);
	    while (mi.mi_buttons--) {
		n = read(fd, &c, 1);
		if (n <= 0) exit (0);
		if (c == 'L') butt[0] = TRUE;
		if (c == 'M') butt[1] = TRUE;
		if (c == 'R') butt[2] = TRUE;
	    }
	if (butt[ddstruct->int1]) {		/* if button has been hit *%
	    ddstruct->logical = TRUE;		/* then return true *%
	    butt[ddstruct->int1] = FALSE;	/* and turn button off *%
	    }
	break;
    case XYREAD:
	ioctl(fd, MIOCGPOS, &mi);		/* read mouse location +-32k *%
	dx = (mi.mi_x - lastx);
	dy = (mi.mi_y - lasty);
	accel =  ((abs(dx) + abs(dy)) < 4) ? 4 : 5;
	mousex += dx << accel;
	mousey += dy << accel;
	if (mousex < -8192) mousex = -8192;	/* clamp to +-8k for NDC *%
	else if (mousex > 8192) mousex = 8192;
	if (mousey < -8192) mousey = -8192;
	else if (mousey > 8192) mousey = 8192;
	lastx = mi.mi_x;  lasty = mi.mi_y;
	ddstruct->int1 = mousex;
	ddstruct->int2 = mousey;
	break;
    case XYWRITE:
	mousex = ddstruct->int1;
	mousey = ddstruct->int2;
	break;
    default:
	    break;
    }
*/
}
