/**************************************************************************\
* 									   *
* 	File: display.h							   *
* 	Copyright (c) 1984 IBM						   *
* 	Date: Mon Mar 19 14:54:41 1984					   *
* 	Author: James Gosling						   *
* 									   *
* Defines the external characteristics of a display device.  All displays  *
* are expected to be accessible entirely through display structures.	   *
* 									   *
* HISTORY								   *
* 									   *
\**************************************************************************/


struct display {
    struct ViewPort screen;
    char *DeviceFileName;
    int d_CurrentColor;
    int base;
    int (*d_RasterOp) ();
    int (*d_RasterSmash) ();
    int (*d_vector) ();
    int (*d_DrawString) ();
    int (*d_DrawIcon) ();
    int (*d_SizeofRaster) ();
    int (*d_CopyScreenToMemory) ();
    int (*d_CopyMemoryToScreen) ();
    int (*d_InitializeDisplay) ();
    int (*d_DefineColor) ();
    int (*d_SelectColor) ();
    int (*d_SnapShot) ();
    int (*d_FillTrapezoid) ();
};

#define HW_RasterOp (*display.d_RasterOp)
#define HW_RasterSmash (*display.d_RasterSmash)
#define HW_vector (*display.d_vector)
#define HW_DrawString (*display.d_DrawString)
#define HW_DrawIcon (*display.d_DrawIcon)
#define HW_CopyScreenToMemory (*display.d_CopyScreenToMemory)
#define HW_CopyMemoryToScreen (*display.d_CopyMemoryToScreen)
#define HW_InitializeDisplay (*display.d_InitializeDisplay)
#define HW_DefineColor (*display.d_DefineColor)
#define HW_SelectColor(c) ((c) != display.d_CurrentColor ? (*display.d_SelectColor) (c) : 0)
#define HW_SizeofRaster (*display.d_SizeofRaster)
#define HW_FillTrapezoid (*display.d_FillTrapezoid)

#define MaxDisplays 5		/* Maximum number of displays */

int NDisplays;
struct display *CurrentDisplay;
struct display 	displays[MaxDisplays];
struct display display;
struct display *CursorDisplay;
int CursorDisplayNumber;


extern NOP ();

#ifndef f_black
#define	f_black		0
#define	f_white		1
#define	f_invert	513
#define	f_copy		514
#define	f_BlackOnWhite	515
#define	f_WhiteOnBlack	516
#endif
#define f_CharacterContext 1024

struct hw_color {
    short index;
    short range;
};
