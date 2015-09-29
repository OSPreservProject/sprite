#include "stdio.h"
#include "usergraphics.h"

int Redraw;

program(Test)
main () {
    wm_NewWindow (0);
    wm_AddMenu ("Exit:q");
    wm_SelectRegion (0);
    wm_SetStandardCursor ('B');
    wm_AddMenu ("Focus flip test:p");
    wm_AddMenu ("Region save test:s");
    wm_AddMenu ("Hide test:h");
    wm_AddMenu ("Clip test:c");
    wm_AddMenu ("Trapezoid test:t");
    wm_AddMenu ("Zoom animation test:z");
    wm_AddMenu ("Menu prefix test:m");
    wm_AddMenu ("Region link test:l");
    while (1) {
	wm_ClearWindow ();
	wm_DefineRegion (0, 10, 10, 10, 100);
	wm_SetFunction (f_black);
	wm_MoveTo (10, 20);
	fflush (winout);
	fprintf (winout, "Ahem ...  redraw=%d", Redraw);
	Redraw = 0;
	fflush (winout);
	switch (getc (winin)) {
            case 'l':
		wm_LinkRegion(1,0);
		wm_NameRegion(2,"TestRegion");
		wm_DefineRegion(2, 50, 50, 50, 50);
	    	wm_DefineRegion(1, 100, 100, 50, 50);
		wm_SelectRegion(2);
		wm_AddMenu("This is region 2:l");
		wm_AddMenu("Test a:a");
		wm_AddMenu("TEST b:a");
		wm_SelectRegion(-1);
		wm_AddMenu("Test c:a");
		wm_AddMenu("TEST d:a");
		break;
	    case 'z':
	    wm_ZoomFrom (10,10,10,10);
	    break;
	    case 'c': 
		wm_SetClipRectangle (10, 10, 30, 20);
		wm_ClearWindow ();
		wm_MoveTo (-10, -10);
		wm_DrawTo (100, 100);
		wm_MoveTo (15, 100);
		wm_DrawTo (10, -20);
		fflush (winout);
		sleep (2);
		break;
	    case 'g': 
		wm_GiveupInputFocus ();
		break;
	    case 'h': 
		wm_HideMe ();
		fflush (winout);
		sleep (3);
		wm_ExposeMe ();
		break;
	    case 'm':
		wm_SetMenuPrefix ("M");
		break;
		case 'M':
		wm_DrawString (10,100,0,"Prefix seen");
		fflush(winout);
		sleep (2);
		break;
	    case 'p': 
		wm_GiveupInputFocus ();
		fflush (winout);
		sleep (3);
		wm_AcquireInputFocus ();
		break;
	    case 'r': 
		wm_HideMe ();
		fflush (winout);
		wm_ExposeMe ();
		{
		    register    i;
		    for (i = 0; i < 600; i++)
			wm_MoveTo (0, 600 - i), wm_DrawTo (i, 0);
		}
		break;
	    case 's': 
		wm_SaveRegion (1, 10, 10, 100, 100);
		wm_SetFunction (f_WhiteOnBlack);
		wm_DrawString (10, 10, wm_AtTop, "Region test");
		fflush (winout);
		sleep (2);
		wm_SetFunction (f_copy);
		wm_RestoreRegion (1, 10, 10);
		wm_SetFunction (f_black);
		wm_DrawString (10, 100, wm_AtTop, "restored?");
		fflush (winout);
		sleep (2);
		break;
	    case 't': 
		wm_FillTrapezoid (40, 10, 100, 10, 100, 160, -1, 'G');
		fflush (winout);
		sleep (2);
		wm_FillTrapezoid (100, 90, 10, 9, 220, 47, -1, 'H');
		fflush (winout);
		sleep (7);
		break;
		case 'u':
		wm_FillTrapezoid (16,10,1,33,100,33,-1,'G');
		fflush (winout);
		getc(winin);
		break;
	    case 'w': 
		wm_WriteToCutBuffer ();
		fprintf (winout, "This is very silly%cOK.", 0);
		fflush (winout);
		sleep (2);
		break;
	    case 'y': 
		wm_ReadFromCutBuffer (0);
		GR_WAITFOR (GR_HEREISCUTBUFFER, 1);
		fprintf (winout, "n=%d\n", uarg[0]);
		{
		    register    c;
		    while (c = getc (winin))
			putc (c, winout);
		    fflush (winout);
		    sleep (5);
		}
		break;
	    case 'q': 
		exit (0);
	}
    }
}

FlagRedraw () {
    Redraw++;
}
