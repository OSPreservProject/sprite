/* This code is the SPUR host debug stub.  It interfaces with the
   GDB remote debug interface via the SPUR on-board RS-232 or the
   Nubus Ethernet driver.  Supported commands are documented in
   remote.c in the GDB system.  This code always sends and receives
   with the MSB first.  
   Copyright (C), 1988 Douglas Johnson.
*/




#include "sprite.h"
#include "mach.h"


/*      Define some progress codes for the on-board status lights */

#define LED_ERROR  0x00		/* Turn on the leds for an error*/
#define LED_OK     0x0f		/* Turn off the leds for a progress report*/

#define TRUE   1
#define FALSE  0

#define GET_REG_COMMAND 1
#define PUT_REG_COMMAND 2
#define GET_MEM_COMMAND 3
#define PUT_MEM_COMMAND 4
#define SINGLE_STEP_COMMAND 5
#define CONTINUE_COMMAND 6
#define DOWN_LOAD_COMMAND 7
#define ATTACH_COMMAND 8
#define DETACH_COMMAND 9
#define ZERO_MEMORY_COMMAND 0x10

#define RUNNING         0x20
#define GET_PKT         0x22
#define PUT_PKT         0x23

#define TRAPPED         0x40    /* or'ed with signal number */


#define ERROR_BAD_ADDRESS  0x80
#define ERROR_BAD_LENGTH  0x81
#define ERROR_GET_PKT  0x82
#define ERROR_PUT_PKT  0x83
				 /* Single step trap instruction */
#define SINGLE_STEP_TRAP (0xa2000000 | MACH_SINGLE_STEP_TRAP)




				/* Stuff the byte in the buffer */
static char *stuff_byte();
#define STUFF_BYTE(byte,b)\
b=stuff_byte(byte,b);

				/* Stuff word into buffer MSB first */
static char *stuff_word();
#define STUFF_WORD(word,b) \
b=stuff_word(word,b);

static int get_data();
				/* Get a byte from the buffer */
#define GET_BYTE(b) \
get_data(&b,2)

			/* Get a word from the buffer MSB first */
#define GET_WORD(b) \
get_data(&b,8)

static int fromhex();




/*  Note: don't initialize any static variables here, because that 
**  won't work with the stuff in ROM.  -- daj
*/

static char buffer[600];		/* Buffer for sending and receiving */


static void led_display();
static void putpkt();
static char tohex();
static void wait();
static void getpkt();
static char readchar();
static void writechar();
void kdb();

static int old_step_contents;  
static int step_addr;
static char nextChar = 0;

extern int debugger_active_address;
  
  
/*  This is the debugging stub that is entered by the kernel when a debug 
**  trap or error occurs.  "sig" is a trap code from machConst.h 
**  "state" is the registers at the time of the trap.  The trapping
**  routine may be resumed by returning from this routine.  The registers 
**  may have been modified by the debugger and thus must be restored.  
**
**  This stub has two copies:  one is in EPROM in the physical address space
**  and the other is in the kernel in the virtual space.  For the most
**  part, it doesn't matter which one is running.  However, the stub
**  does maintain some shared state in the physical space.  
*/





void
kdb (sig,state)
     int sig;
     Mach_RegState *state;
{
  int i;


  if(sig == MACH_POWERUP)	/* Initialize some stuff */
    {
                                
      write_physical_word
              (read_physical_word(&debugger_active_address),FALSE);

      old_step_contents = 0;
      step_addr = 0;
    }


  else
                                /* Remove the single step if one */
    {
      if(sig == MACH_SINGLE_STEP
	 && ((int)state->curPC) == step_addr)
	{				
	  *(int *)step_addr = old_step_contents;
	  step_addr = 0;
	  old_step_contents = 0;
	}
    }
  
  led_display(TRAPPED | sig,LED_OK,TRUE);

				/* Report status if there is anyone
				   out there to see it. */
  if(read_physical_word
     (read_physical_word(&debugger_active_address)))
    {
      char *b = buffer;

      *b++ = 'S';
      STUFF_BYTE(sig,b);
      *b++ = 0;
      putpkt(buffer);
    }

/* Now get a command from the host and do it. */

  while (TRUE)
    {
      getpkt(buffer);

      switch (buffer[0])
	{
	case 'a':		/* Attach debugger */
	  {
	    char *b = buffer;
	    led_display(ATTACH_COMMAND,LED_OK,FALSE);
	    
                               /* Remember we have someone to talk to. */
	    write_physical_word
              (read_physical_word(&debugger_active_address),TRUE);

	    *b++ = 'S';		/* Report status */
	    STUFF_BYTE(sig,b);
	    *b++ = 0;
	    putpkt(buffer);
	    break;
	  }

	case 'A':		/* Detach debugger, no status
				   change.  Should it continue
				   the program?*/
	  {
	    led_display(DETACH_COMMAND,LED_OK,FALSE);

                                   /* We no longer have someone to talk to. */
	    write_physical_word
              (read_physical_word(&debugger_active_address),FALSE);

	    putpkt("OK");
	    break;
	  }
	    

	case 'g':			/* Send the registers to the host */
	  {
	    char *b = buffer;
	    led_display(GET_REG_COMMAND,LED_OK,FALSE);

				/* First the general registers */
	    for (i = 0; i < 32; i ++)
	      {
		STUFF_WORD(state->regs[i][1],b); /* tag */
		STUFF_WORD(state->regs[i][0],b); /* data */
	      }
				/* Now the specials */
	  STUFF_WORD(state->kpsw,b);	
	  STUFF_WORD(state->upsw,b);	
	  STUFF_WORD(state->curPC,b);	
	  STUFF_WORD(state->nextPC,b);
	  STUFF_WORD(state->insert,b);	
	  STUFF_WORD(state->swp,b);	
	  STUFF_WORD(state->cwp,b);	
	  STUFF_WORD(state->swp,b);	

	  *b++ = 0;		/* Terminate the string */
	  
	  putpkt(buffer);		/* Send it off */
	  
	  break;
	}
	
      case 'G':		        /* Write the registers */
	{
	  char *b = &buffer[1];
	  Address	usp;

	  led_display(PUT_REG_COMMAND,LED_OK,FALSE);

	  
	  /* First the general registers */
	  for (i = 0; i < 32; i ++)
	    {
	      state->regs[i][1] =  GET_WORD(b);  /* tag */
	      state->regs[i][0] =  GET_WORD(b);  /* data */
	    }
	  /* Now the specials */
	  state->kpsw = GET_WORD(b);	
	  state->upsw = GET_WORD(b);	
	  state->curPC = (Address)GET_WORD(b);	
	  state->nextPC = (Address)GET_WORD(b);	
	  state->insert = GET_WORD(b);	
	  state->swp = (Address)GET_WORD(b);
	  state->cwp = GET_WORD(b);	
	  usp = (Address)GET_WORD(b);	
	  
	  putpkt("OK");			     /* Everything is all right */
	  
	  break;
	}
	
      case 'm':		        /* Read a chunk of memory and return it */
	{
	  int addr = 0;
	  int count = 0;
	  int length = 0;
	  char *b = &buffer[1];
	  
	  led_display(GET_MEM_COMMAND,LED_OK,FALSE);
	  /* Get the memory address */
	  for(;*b != ',' && *b != 0;)
	    addr = (addr << 4) + fromhex(*b++);
	  
	  
	  
	  /* Get data count (bytes) */
	  *b++;
	  for(length = 0; (*b != 0 && length < 8); length++)
	    count = (count << 4) + fromhex(*b++);
	  
	  
	  b = buffer;
	  
	  /* If bad address, return error */
	  if(!Vm_ValidateRange(addr,count))
	    {
	      *b++ = 'E';
	      STUFF_BYTE(ERROR_BAD_ADDRESS,b);
	    }
	  else 
	    /* Because of the byte swap problem, we
	       only handle lengths we know how to
	       swap. Ask for chars one at a time.*/
	    
	    switch (count) {
	      
	    case 1:	
	      STUFF_BYTE(*(char *) addr,b);
	      break;
	      
	    case 2:
	      {short data = *(short *) addr;
	       STUFF_BYTE((data >> 8),b);
	       STUFF_BYTE((data & 0xff),b);
	       break;
	     }
	      
	    case 4:
	      STUFF_WORD(*(int *) addr,b);
	      break;
	      
	    case 8:
	      STUFF_WORD(*(int *) addr + 4,b);
	      STUFF_WORD(*(int *) addr,b);
	      break;
	      
	    default:
	      *b++ = 'E';
	      STUFF_BYTE(ERROR_BAD_LENGTH,b);
	      break;
	    }
	  
	  *b = 0;			/* Terminate the buffer and send it */
	  putpkt(buffer);
	  break;
	}
	/* Modify memory command */
      case 'M':	  
	{
	  int addr = 0;
	  int count = 0;
	  int length = 0;
	  int error = 0;
	  char *b = &buffer[1];
	  led_display(PUT_MEM_COMMAND,LED_OK,FALSE);
	  /* Get the memory address */
	  for(;*b != ',' && *b != 0;)
	    addr = (addr << 4) + fromhex(*b++);
	  
	  
	  
	  /* Get data count (bytes) */
	  *b++;
	  for(length = 0;(*b != ':' && *b != 0); length++)
	    count = (count << 4) + fromhex(*b++);
	  
	  

	  /* If bad address, return error */
	  if(!Vm_ValidateRange(addr,count))
	    {
	      error = ERROR_BAD_ADDRESS;
	    }
	  else 
	    /* Because of the byte swap problem,
	       we only handle lengths we know how
	       to swap.  Change chars one at a time.*/

	    *b++;	    
	    switch (count) {
	      
	    case 1:	
	      *(char *) addr = GET_BYTE(b);
	      break;
	      
	    case 2:
	      *(short *) addr = (GET_BYTE(b) << 8) | GET_BYTE(b);
	      break;
	      
	    case 4:
	      *(int *) addr = GET_WORD(b);
	      break;
	      
	    case 8:
	      *(int *) (addr + 4) = GET_WORD(b);
	      *(int *) addr = GET_WORD(b);
	      break;
	      
	    default:
	      error = ERROR_BAD_LENGTH;
	      break;
	    }
	  
	  if (error)
	    {
	      b = buffer;
	      *b++ = 'E';
	      STUFF_BYTE(error,b);
	      *b = 0;			
	      putpkt(buffer);
	    }
	  else
	    putpkt("OK");
	  break;
	}
	
	/* Down load memory command */
      case 'D':	  
	{
	  int addr = 0;
	  int count = 0;
	  int error = 0;
	  int length = 0;
	  char *b = &buffer[1];
	  led_display(DOWN_LOAD_COMMAND,LED_OK,FALSE);
	  /* Get the memory address */
	  for(;*b != ',' && *b != 0;)
	    addr = (addr << 4) + fromhex(*b++);
	  
	  
	  
	  /* Get data count (bytes) */
	  *b++;
	  for(length = 0;(*b != ':' && *b != 0); length++)
	    count = (count << 4) + fromhex(*b++);
	  
	  /* If bad address, return error */
	  if(!Vm_ValidateRange (addr,count))
	    {
	      error = ERROR_BAD_ADDRESS;
	      b = buffer;
	      *b++ = 'E';
	      STUFF_BYTE(error,b);
	      *b = 0;			
	      putpkt(buffer);
	    }
	  else			/* Down load the data */
	    {
	      *b++;
	      for(length = 0; length < count; length += 4)
		*(int *) (addr + length) = GET_WORD(b);
	      putpkt("OK");
	    }
	  break;
	}

	/* Zero memory command */
      case 'Z':	  
	{
	  int addr = 0;
	  int count = 0;
	  int error = 0;
	  int length = 0;
	  char *b = &buffer[1];
	  led_display(ZERO_MEMORY_COMMAND,LED_OK,FALSE);
	  /* Get the memory address */
	  for(;*b != ',' && *b != 0;)
	    addr = (addr << 4) + fromhex(*b++);
	  
	  /* Get data count (bytes) */
	  *b++;
	  for(length = 0;(*b != ':' && *b != 0); length++)
	    count = (count << 4) + fromhex(*b++);
	  
	  /* If bad address, return error */
	  if(!Vm_ValidateRange (addr,count))
	    error = ERROR_BAD_ADDRESS;
	  else			/* Down load the data */
	    for(length = 0; length < count; length += 4)
	      *(int *) (addr + length) = 0;
	  
	  if (error)
	    {
	      b = buffer;
	      *b++ = 'E';
	      STUFF_BYTE(error,b);
	      *b = 0;			
	      putpkt(buffer);
	    }
	  else
	    putpkt("OK");
	  break;
	}
	  
	  
	  /* Single step */
	case 's':
	  {
	    int addr = 0;
	    char *b = &buffer[1];
	    
	    led_display(SINGLE_STEP_COMMAND,LED_OK,FALSE);    
	    if (*b != 0)
	      /* Get the memory address */
	      {
		for(;*b != ',' && *b != 0;)
		  addr = (addr << 4) + fromhex(*b++);    
		
		if(Vm_ValidateRange(addr,4))
		  {  
		    state->curPC = (Address)addr;
		    state->nextPC = 0;
		  }
		else		/* Bad address, display and report errors */
		  {
		    led_display(ERROR_BAD_ADDRESS,LED_ERROR,TRUE);
		    b = buffer;
		    *b++ = 'E';
		    STUFF_BYTE(ERROR_BAD_ADDRESS,b);
		    *b = 0;
		    putpkt(buffer);
		    break;		
		  }
	      }
	    
	    /* Insert a breakpoint trap */
	    /* at either the next pc or just after */
	    /* current pc. */
	    addr = (!(state->kpsw & MACH_KPSW_USE_CUR_PC)
		    ? (int)state->nextPC : (int)state->curPC + 4);
	    old_step_contents = *(int *) addr;  /* Remember old instruction */
	    *(int *) addr = SINGLE_STEP_TRAP;   /* Put step trap in instead */
	    step_addr = addr;		        /* Remember where we put it. */
	    state->kpsw |=  MACH_KPSW_USE_CUR_PC;
	    led_display(RUNNING,LED_OK,TRUE);
	    return;			/* Resume the executing process */
	    
	    
	    /* Continue execution */
	  }
      case 'c':
	{
	  int addr = 0;
	  char *b = &buffer[1];
	  led_display(CONTINUE_COMMAND,LED_OK,FALSE);
	  if (*b != 0)
	    /* Get the memory address */
	    {
	      for(;*b != ',' && *b != 0;)
		addr = (addr << 4) + fromhex(*b++);    
	      
	      if(Vm_ValidateRange(addr,4))
		{  
		  state->curPC = (Address)addr;
		  state->nextPC = 0;
		}
	      else		/* Bad address, display and report errors */
		{
		  led_display(ERROR_BAD_ADDRESS,LED_ERROR,TRUE);
		  b = buffer;
		  *b++ = 'E';
		  STUFF_BYTE(ERROR_BAD_ADDRESS,b);
		  *b = 0;
		  putpkt(buffer);
		  break;		
		}
	    }
	  state->kpsw |=  MACH_KPSW_USE_CUR_PC;
	  led_display(RUNNING,LED_OK,FALSE);
	  return;			/* Resume the executing process */
	}	  
	    
      }				/* End the command switch */
  }				/* End the infinite command loop */
}

static void
putpkt (buf)
     char *buf;
{

  led_display(PUT_PKT,LED_OK,FALSE);
  Dbg_putpkt(buf);
}
static void
getpkt (buf)
     char *buf;
{
  led_display(GET_PKT,LED_OK,FALSE);
  Dbg_getpkt(buf);
}


/*
A debug packet whose contents are <data>
is encapsulated for transmission in the form:

	$ <data> # CSUM1 CSUM2

	<data> must be ASCII alphanumeric and cannot include characters
	'$' or '#'

	CSUM1 and CSUM2 are ascii hex representation of an 8-bit 
	checksum of <data>, the most significant nibble is sent first.
	the hex digits 0-9,a-f are used.

Receiver responds with:

	+	- if CSUM is correct and ready for next packet
	-	- if CSUM is incorrect

*/

/* Send a packet to the remote machine, with error checking.
   The data of the packet is in BUF.  */

void
Dbg_Rs232putpkt (buf)
     char *buf;
{
  int i;
  char csum;
  char *p;

  led_display(PUT_PKT,LED_OK,FALSE);

  /* Send it over and over until we get a positive ack.  */
  
    while (1) {
	csum  = 0;
	writechar('$');		/* Header flag */
	for (i = 0; buf[i] != 0; i++) {
	    csum += buf[i];
	    writechar(buf[i]);
	}
	writechar('#');		/* trailer and checksum */
	writechar(tohex ((csum >> 4) & 0xf));
	writechar(tohex (csum & 0xf));

	nextChar = readchar();
	if (nextChar == '+') {	/* If all OK, exit */
	    nextChar = 0;
	    return;
	} else if (nextChar == '-') {
	    led_display(ERROR_PUT_PKT,LED_ERROR,TRUE);
	} else {			/* Report error on leds */
	    led_display(ERROR_PUT_PKT,LED_ERROR,TRUE);
	    return;
	} 
    }
}


/* Read a packet from the remote machine, with error checking,
   and store it in BUF.  */

void
Dbg_Rs232getpkt (buf)
     char *buf;
{
  char *bp;
  char csum;
  int c, c1, c2;


  while (1)
    {
      led_display(GET_PKT,LED_OK,FALSE);
      if (nextChar != 0) {
	c = nextChar;
	nextChar = 0;
	if (c != '$') {
	  while ((c = readchar()) != '$');
	}
      } else {
	  while ((c = readchar()) != '$');
      }
      csum = 0;
      bp = buf;
      while (1)
	{
	  c = readchar ();
	  if (c == '#')
	    break;
	  *bp++ = c;
	  csum += c;
	}
      *bp = 0;

      c1 = fromhex (readchar ());
      c2 = fromhex (readchar ());
      if (csum == (c1 << 4) + c2)
	break;

      led_display(ERROR_PUT_PKT,LED_ERROR,TRUE);
      writechar('-');
    }

  writechar('+');

}


/* Convert number NIB to a hex digit.  */

static char
tohex (nib)
     int nib;
{
  if (nib < 10)
    return '0'+nib;
  else
    return 'a'+nib-10;
}

/* Convert hex digit A to a number.  */

static int
fromhex (a)
     int a;
{
  if (a >= '0' && a <= '9')
    return a - '0';
  else if (a >= 'a' && a <= 'f')
    return a - 'a' + 10;
  else
    return 0xff;		/* Invalid digit, cause checksum to fail */
}

static char			/* Helper routine to STUFF_BYTE */
*stuff_byte(byte,b)
     char byte;
     char *b;
{
  *b++ = tohex((byte & 0xf0) >> 4); 
  *b++ = tohex((byte & 0xf));
  return b;
}

static char			/* Helper routine to STUFF_WORD */
*stuff_word(word,b)
     int word;
     char *b;
{
  STUFF_BYTE(word >> 24,b); 
  STUFF_BYTE(word >> 16,b); 
  STUFF_BYTE(word >> 8,b); 
  STUFF_BYTE(word,b) ;
  return b;
}

static int
get_data(b,length)			/* Helper to GET_WORD, GET_BYTE */
  char **b;
  int length;
{
  int data = 0;
  int i;
  
  for(i=0 ; i < length; i++)
    data = (data << 4) | fromhex(*(*b)++);    
  return data;
  }




