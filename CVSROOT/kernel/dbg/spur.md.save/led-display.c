#include "sprite.h"
#include "mach.h"
    
/* This displays its first argument in the seven segment displays, its
   second in the led lights, and if its third is non-zero, waits for about
   .5 seconds to give a human time to see it.  The wait may be over-ridden
   by on board switches.  Switch 0 set to 1 will remove all waits.  Switch 
   1 set to 1 will always wait (regardless of the software argument).  If 
   it matters, switch 0 (don't wait) overrides switch 1 (wait).*/

static void wait();

#define LAMP_OFF 0x1000            /* Turn off lamp test */
				/*Some physical addresses  */
#define LIGHTS   0x20000  /* The status lights on the board */
#define SWITCHES 0x40000  /* The status switches on the board */

void 
led_display(seven_seg,led,wait_flag)
     int seven_seg;
     int led;
     int wait_flag;
{

  extern int machLEDValues[16];
  machLEDValues[Mach_GetProcessorNumber()] = seven_seg;
  write_physical_word(LIGHTS,(seven_seg&0xff)|LAMP_OFF|((led<<8) & 0x0f00));

  wait(1,wait_flag);

}


static void 
wait (time,wait_flag)
     int time;			/* How long to wait in .1 second units */
     int wait_flag;
{
  int switches = read_physical_word(SWITCHES);
  int i,k;
  if (wait_flag || (switches & 0x40)) {
      if (switches & 0) {
	return;
      } else {
	for (i = 0; i < time; i++) {
                        	  /* assumes 10 mhz and 3 instructions */
	  for (k = 0; k < (1000000/3); k++) {
	  }
	}	 
      }
    }
}
