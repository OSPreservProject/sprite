/* 
 * ioctl.c --
 *
 *	Procedure to map from Unix ioctl system call to Sprite.
 *
 * Copyright 1986 Regents of the University of California
 * All rights reserved.
 */

#ifndef lint
static char rcsid[] = "$Header$ SPRITE (Berkeley)";
#endif not lint

#include "user/sys/ioctl.h"
#include "user/sys/termios.h"
#include "user/netEther.h"
#include "sprite.h"
#include "compatInt.h"
#include "user/dev/tty.h"
#include "user/dev/net.h"
#include "user/dev/graphics.h"
#include "fs.h"
#include "errno.h"

#include "user/sys/types.h"
#include "fcntl.h"
#include "user/sys/socket.h"
#include "user/sys/ttychars.h"
#include "user/sys/ttydev.h"
#include "user/net/route.h"
#include "user/net/if.h"
#include "user/sys/time.h"
#include "machInt.h"
#include "mach.h"

#ifdef __STDC__
static void DecodeRequest(int request);
#else
#define const
static void DecodeRequest();
#endif

#ifdef notdef
int compatTapeIOCMap[] = {
    IOC_TAPE_WEOF, 		/* 0, MTWEOF */
    IOC_TAPE_SKIP_FILES,	/* 1, MTFSF */
    IOC_TAPE_BACKUP_FILES,	/* 2, MTBSF */
    IOC_TAPE_SKIP_BLOCKS,	/* 3, MTFSR */
    IOC_TAPE_BACKUP_BLOCKS,	/* 4, MTBSR */
    IOC_TAPE_REWIND, 		/* 5, MTREW */
    IOC_TAPE_OFFLINE, 		/* 6, MTOFFL */
    IOC_TAPE_NO_OP,		/* 7, MTNOP */
    IOC_TAPE_RETENSION,		/* 8, MTRETEN */
    IOC_TAPE_ERASE,		/* 9, MTERASE */
};
#endif

unsigned ifcBuf[16] = {
0x306573, 0x8011a4e0, 0x1, 0x0,
0x2, 0x21100110, 0x0, 0x0,
0x306f6c, 0x0, 0x0, 0xf5c0,
0x2, 0x0100007f, 0x0, 0x0,
};


/*
 *----------------------------------------------------------------------
 *
 * ioctl --
 *
 *	Compat procedure for Unix ioctl call. This procedure usually does
 *	nothing.
 *
 * Results:
 *	Error code returned, SUCCESS if no error.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
int
MachUNIXIoctl(fd, request, buf)
    int		fd;
    int		request;
    char *	buf;
{
    ReturnStatus	status;
    Address		usp;
    extern Mach_State	*machCurStatePtr;

    usp = (Address)machCurStatePtr->userState.regState.regs[SP];
    switch (request) {

	/*
	 * Generic calls:
	 */

	case FIOCLEX: {
		/*
		 * Set the close-on-exec bit for the file.
		 * Buf is not used.
		 */

		int flag = IOC_CLOSE_ON_EXEC;
		usp -= sizeof(flag);
		status = Vm_CopyOut(sizeof(flag), (Address)&flag, usp);
		if (status != SUCCESS) {
		    break;
		}
		status = Fs_IOControlStub(fd, IOC_SET_BITS, 
					  sizeof(flag), usp, 0, (Address) NULL);
	    }
	    break;

	case FIONCLEX: {
		/*
		 * Clear the close-on-exec bit for the file.
		 * Buf is not used.
		 */

		int flag = IOC_CLOSE_ON_EXEC;
		usp -= sizeof(flag);
		status = Vm_CopyOut(sizeof(flag), (Address)&flag, usp);
		if (status != SUCCESS) {
		    break;
		}
		status = Fs_IOControlStub(fd, IOC_CLEAR_BITS, 
				      sizeof(flag), usp, 0, (Address) NULL);
	    }
	    break;

	case FIONREAD:
	    status = Fs_IOControlStub(fd, IOC_NUM_READABLE, 0, (Address) NULL,
				      sizeof(int), buf);
	    break;

        case FIONBIO: {
		/*
		 * Set or clear the non-blocking I/O bit for the file.
		 */

		int flag = IOC_NON_BLOCKING;
		int setBit;

		if (Vm_CopyIn(4, buf, &setBit) != SUCCESS) {
		    status = SYS_ARG_NOACCESS;
		    break;
		}
		usp -= sizeof(flag);
		status = Vm_CopyOut(sizeof(flag), (Address)&flag, usp);
		if (status != SUCCESS) {
		    break;
		}
		if (setBit) {
		    status = Fs_IOControlStub(fd, IOC_SET_BITS, 
			    sizeof(flag), usp, 0, (Address) NULL);
		} else {
		    status = Fs_IOControlStub(fd, IOC_CLEAR_BITS, 
			    sizeof(flag), usp, 0, (Address) NULL);
		}
	    }
	    break;

	case FIOASYNC: {
		/*
		 * Set or clear the asynchronous I/O bit for the file.
		 */

		int flag = IOC_ASYNCHRONOUS;
		int setBit;

		if (Vm_CopyIn(4, buf, &setBit) != SUCCESS) {
		    status = SYS_ARG_NOACCESS;
		    break;
		}
		usp -= sizeof(flag);
		status = Vm_CopyOut(sizeof(flag), (Address)&flag, usp);
		if (status != SUCCESS) {
		    break;
		}
		if (setBit) {
		    status = Fs_IOControlStub(fd, IOC_SET_BITS, 
			    sizeof(flag), usp, 0, (Address) NULL);
		} else {
		    status = Fs_IOControlStub(fd, IOC_CLEAR_BITS, 
			    sizeof(flag), usp, 0, (Address) NULL);
		}
	    }
	    break;


	case FIOGETOWN:
	case SIOCGPGRP:
	case TIOCGPGRP: {
		Ioc_Owner owner;

		usp -= sizeof(owner);
		status = Fs_IOControlStub(fd, IOC_GET_OWNER, 0,
				    (Address)NULL, sizeof(Ioc_Owner), usp);
		if (status == SUCCESS) {
		    (void)Vm_CopyIn(sizeof(owner), usp, (Address)&owner);
		    status = Vm_CopyOut(sizeof(int), &owner.id, buf);
		}
	    }
	    break;

	case FIOSETOWN:
	case SIOCSPGRP:
	case TIOCSPGRP: {
	    Ioc_Owner owner;

	    status = Vm_CopyIn(sizeof(int), buf, &owner.id);
	    if (status != SUCCESS) {
		break;
	    }
	    owner.procOrFamily = IOC_OWNER_FAMILY;
	    usp -= sizeof(owner);
	    status = Vm_CopyOut(sizeof(owner), (Address)&owner, usp);
	    if (status != SUCCESS) {
		break;
	    }
	    status = Fs_IOControlStub(fd, IOC_SET_OWNER, sizeof(Ioc_Owner),
				      usp, 0, (Address)NULL);
	    break;
	}

	/* 
	 * Tty-related calls:
	 */

	case TIOCGETP:
	    status = Fs_IOControlStub(fd, IOC_TTY_GET_PARAMS, 0, (Address) NULL,
		    sizeof(struct sgttyb), (Address) buf);
	    break;
	case TIOCSETP:
	    status = Fs_IOControlStub(fd, IOC_TTY_SET_PARAMS, sizeof(struct sgttyb),
		    (Address) buf, 0, (Address) NULL);
	    break;
	case TIOCSETN:
	    status = Fs_IOControlStub(fd, IOC_TTY_SETN, sizeof(struct sgttyb),
		    (Address) buf, 0, (Address) NULL);
	    break;
	case TIOCEXCL:
	    status = Fs_IOControlStub(fd, IOC_TTY_EXCL, 0, (Address) NULL,
		    0, (Address) NULL);
	    break;
	case TIOCNXCL:
	    status = Fs_IOControlStub(fd, IOC_TTY_NXCL, 0, (Address) NULL,
		    0, (Address) NULL);
	    break;
	case TIOCHPCL:
	    status = Fs_IOControlStub(fd, IOC_TTY_HUP_ON_CLOSE, 0,
		    (Address) NULL, 0, (Address) NULL);
	    break;
	case TIOCFLUSH:
	    status = Fs_IOControlStub(fd, IOC_TTY_FLUSH, sizeof(int),
		    (Address) buf, 0, (Address) NULL);
	    break;
	case TIOCSTI:
	    status = Fs_IOControlStub(fd, IOC_TTY_INSERT_CHAR, sizeof(char),
		    (Address) buf, 0, (Address) NULL);
	    break;
	case TIOCSBRK:
	    status = Fs_IOControlStub(fd, IOC_TTY_SET_BREAK, 0,
		    (Address) NULL, 0, (Address) NULL);
	    break;
	case TIOCCBRK:
	    status = Fs_IOControlStub(fd, IOC_TTY_CLEAR_BREAK, 0,
		    (Address) NULL, 0, (Address) NULL);
	    break;
	case TIOCSDTR:
	    status = Fs_IOControlStub(fd, IOC_TTY_SET_DTR, 0, 
		    (Address) NULL, 0, (Address) NULL);
	    break;
	case TIOCCDTR:
	    status = Fs_IOControlStub(fd, IOC_TTY_CLEAR_DTR, 0,
		    (Address) NULL, 0, (Address) NULL);
	    break;
	case TIOCGETC:
	    status = Fs_IOControlStub(fd, IOC_TTY_GET_TCHARS, 0, (Address) NULL,
		    sizeof(struct tchars), (Address) buf);
	    break;
	case TIOCSETC:
	    status = Fs_IOControlStub(fd, IOC_TTY_SET_TCHARS,
		    sizeof(struct tchars), (Address) buf, 0, (Address) NULL);
	    break;
	case TIOCGLTC:
	    status = Fs_IOControlStub(fd, IOC_TTY_GET_LTCHARS, 0, (Address) NULL,
		    sizeof(struct ltchars), (Address) buf);
	    break;
	case TIOCSLTC:
	    status = Fs_IOControlStub(fd, IOC_TTY_SET_LTCHARS,
		    sizeof(struct ltchars), (Address) buf, 0, (Address) NULL);
	    break;
	case TIOCLBIS:
	    status = Fs_IOControlStub(fd, IOC_TTY_BIS_LM,
		    sizeof(int), (Address) buf, 0, (Address) NULL);
	    break;
	case TIOCLBIC:
	    status = Fs_IOControlStub(fd, IOC_TTY_BIC_LM,
		    sizeof(int), (Address) buf, 0, (Address) NULL);
	    break;
	case TIOCLSET:
	    status = Fs_IOControlStub(fd, IOC_TTY_SET_LM,
		    sizeof(int), (Address) buf, 0, (Address) NULL);
	    break;
	case TIOCLGET:
	    status = Fs_IOControlStub(fd, IOC_TTY_GET_LM, 0, (Address) NULL,
		    sizeof(int), (Address) buf);
	    break;
	case TIOCGETD:
	    status = Fs_IOControlStub(fd, IOC_TTY_GET_DISCIPLINE, 0,
		    (Address) NULL, sizeof(int), (Address) buf);
	    break;
	case TIOCSETD:
	    status = Fs_IOControlStub(fd, IOC_TTY_SET_DISCIPLINE,
		sizeof(int), (Address) buf, 0, (Address) NULL);
	    break;

	case SIOCATMARK:
	    status = Fs_IOControlStub(fd, IOC_NET_IS_OOB_DATA_NEXT,
			    0, (Address) NULL, sizeof(int), (Address) buf);
	    break;

        case TCGETS:
	    status = Fs_IOControlStub(fd, IOC_TTY_GET_TERMIO,
		0, (Address) NULL, sizeof(struct termios), (Address) buf);
	    break;

        case TCSETS:
	    status = Fs_IOControlStub(fd, IOC_TTY_SET_TERMIO,
		sizeof(struct termios), (Address) buf, 0, (Address) NULL);
	    break;

	case TIOCGWINSZ:
	    status = Fs_IOControlStub(fd, IOC_TTY_GET_WINDOW_SIZE,
		0, (Address) NULL, sizeof(struct winsize), (Address) buf);
	    break;

        case TIOCSWINSZ:
	    status = Fs_IOControlStub(fd, IOC_TTY_SET_WINDOW_SIZE,
		sizeof(struct winsize), (Address) buf, 0, (Address) NULL);
	    break;
	/*
	 * Graphics requests.
	 */
	case QIOCGINFO:
	    status = Fs_IOControlStub(fd, IOC_GRAPHICS_GET_INFO,
		0, (Address) NULL, sizeof(DevScreenInfo *), (Address)buf);
	    break;
	case QIOCPMSTATE:
	    status = Fs_IOControlStub(fd, IOC_GRAPHICS_MOUSE_POS,
		sizeof(DevCursor), (Address) buf, 0, (Address)NULL);
	    break;
	case QIOWCURSORCOLOR:
	    status = Fs_IOControlStub(fd, IOC_GRAPHICS_CURSOR_COLOR,
		sizeof(unsigned int [6]), (Address) buf, 0, (Address)NULL);
	    break;
	case QIOCINIT:
	    status = Fs_IOControlStub(fd, IOC_GRAPHICS_INIT_SCREEN,
		0, (Address) NULL, 0, (Address)NULL);
	    break;
	case QIOCKPCMD:
	    status = Fs_IOControlStub(fd, IOC_GRAPHICS_KBD_CMD,
		sizeof(DevKpCmd), (Address) buf, 0, (Address)NULL);
	    break;
	case QIOCADDR:
	    status = Fs_IOControlStub(fd, IOC_GRAPHICS_GET_INFO_ADDR,
		0, (Address) NULL, sizeof(DevScreenInfo *), (Address)buf);
	    break;
	case QIOWCURSOR:
	    status = Fs_IOControlStub(fd, IOC_GRAPHICS_CURSOR_BIT_MAP,
		sizeof(short[32]), (Address) buf, 0, (Address)NULL);
	    break;
	case QIOKERNLOOP:
	    status = Fs_IOControlStub(fd, IOC_GRAPHICS_KERN_LOOP,
		0, (Address) NULL, 0, (Address)NULL);
	    break;
	case QIOKERNUNLOOP:
	    status = Fs_IOControlStub(fd, IOC_GRAPHICS_KERN_UNLOOP,
		0, (Address) NULL, 0, (Address)NULL);
	    break;
	case QIOVIDEOON:
	    status = Fs_IOControlStub(fd, IOC_GRAPHICS_VIDEO_ON,
		0, (Address) NULL, 0, (Address)NULL);
	    break;
	case QIOVIDEOOFF:
	    status = Fs_IOControlStub(fd, IOC_GRAPHICS_VIDEO_OFF,
		0, (Address) NULL, 0, (Address)NULL);
	    break;
	case QIOSETCMAP:
	    status = Fs_IOControlStub(fd, IOC_GRAPHICS_COLOR_MAP,
		sizeof(DevColorMap), (Address) buf, 0, (Address)NULL);
	    break;
	case SIOCGIFCONF: {
	    /*
	     * Fake the ifconfig ioctl.  This is a hack done for X.  
	     * A more general implementation is probably needed.
	     */
	    struct ifconf ifc;
	    struct ifreq   ifreq;
	    extern int	  machHostID;
	    int		  *intPtr;

	    status = Vm_CopyIn(sizeof(struct ifconf), buf, (Address)&ifc);
	    if (status != SUCCESS) {
		return(status);
	    }

	    if (ifc.ifc_len < 32) {
		status = SYS_INVALID_ARG;
		break;
	    }
	    /*
	     * We give a length of 32 and put in the request buffer 
	     * the name ("se0"), followed by the family (AF_INET), 
	     * and finally our internet address.
	     */
	    ifc.ifc_len = 32;
	    strcpy(ifreq.ifr_name, "se0");
	    intPtr = (int *)&ifreq.ifr_ifru;
	    *intPtr = AF_INET;
	    *(intPtr + 1) = machHostID;

	    status = Vm_CopyOut(sizeof(struct ifconf), (Address)&ifc, buf);
	    status = Vm_CopyOut(32, (Address)&ifreq,
				(Address)ifc.ifc_ifcu.ifcu_req);
	    break;
	}


	case SIOCRPHYSADDR: {
		/* Get the ethernet address. */

		struct ifdevea *p;
#if 1
		Net_EtherAddress etherAddress;

		Mach_GetEtherAddress(&etherAddress);
		p = (struct ifdevea *) buf;
		p->default_pa[0] = p->current_pa[0] = etherAddress.byte1;
		p->default_pa[1] = p->current_pa[1] = etherAddress.byte2;
		p->default_pa[2] = p->current_pa[2] = etherAddress.byte3;
		p->default_pa[3] = p->current_pa[3] = etherAddress.byte4;
		p->default_pa[4] = p->current_pa[4] = etherAddress.byte5;
		p->default_pa[5] = p->current_pa[5] = etherAddress.byte6;
#else
		/*
		 * Stuff with forgery's ether address for testing
		 * verilog.
		 */
		p = (struct ifdevea *) buf;
		p->default_pa[0] = p->current_pa[0] = 0x08;
		p->default_pa[1] = p->current_pa[1] = 0x00;
		p->default_pa[2] = p->current_pa[2] = 0x2b;
		p->default_pa[3] = p->current_pa[3] = 0x10;
		p->default_pa[4] = p->current_pa[4] = 0x75;
		p->default_pa[5] = p->current_pa[5] = 0x24;
		status = SUCCESS;
#endif

	    }
	    break;

	/*
	 * Unknown requests:
	 */

	default:
	    DecodeRequest(request);
	    status = SYS_INVALID_ARG;
    }
    return(status);
}



/*
 *----------------------------------------------------------------------
 *
 * DecodeRequest --
 *
 *	Takes a UNIX ioctl request and prints the name of the request
 *	on the standard error file.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

typedef struct {
    char *name;
    int	 request;
} RequestName;

static void
DecodeRequest(request)
    int request;
{
    register int i;
    /*
     * The following list contains all the ioctls in 
     * /usr/include/sys/ioctl.h (SunOS 3.2), and
     * /usr/include/sys/termios.h (SunOS 4.0).
     */
    const static RequestName mapping[] = {
	{"TIOCGETD",	TIOCGETD},
	{"TIOCSETD",	TIOCSETD},
	{"TIOCHPCL",	TIOCHPCL},
	{"TIOCMODG",	TIOCMODG},
	{"TIOCMODS",	TIOCMODS},
	{"TIOCGETP",	TIOCGETP},
	{"TIOCSETP",	TIOCSETP},
	{"TIOCSETN",	TIOCSETN},
	{"TIOCEXCL",	TIOCEXCL},
	{"TIOCNXCL",	TIOCNXCL},
	{"TIOCFLUSH",	TIOCFLUSH},
	{"TIOCSETC",	TIOCSETC},
	{"TIOCGETC",	TIOCGETC},
	{"TIOCLBIS",	TIOCLBIS},
	{"TIOCLBIC",	TIOCLBIC},
	{"TIOCLSET",	TIOCLSET},
	{"TIOCLGET",	TIOCLGET},
	{"TIOCSBRK",	TIOCSBRK},
	{"TIOCCBRK",	TIOCCBRK},
	{"TIOCSDTR",	TIOCSDTR},
	{"TIOCCDTR",	TIOCCDTR},
	{"TIOCSLTC",	TIOCSLTC},
	{"TIOCGLTC",	TIOCGLTC},
	{"TIOCOUTQ",	TIOCOUTQ},
	{"TIOCSTI",	TIOCSTI},
	{"TIOCNOTTY",	TIOCNOTTY},
	{"TIOCPKT",	TIOCPKT},
	{"TIOCSTOP",	TIOCSTOP},
	{"TIOCSTART",	TIOCSTART},
	{"TIOCMSET",	TIOCMSET},
	{"TIOCMBIS",	TIOCMBIS},
	{"TIOCMBIC",	TIOCMBIC},
	{"TIOCMGET",	TIOCMGET},
	{"TIOCREMOTE",	TIOCREMOTE},
	{"TIOCGWINSZ",	TIOCGWINSZ},
	{"TIOCSWINSZ",	TIOCSWINSZ},
	{"TIOCUCNTL",	TIOCUCNTL},
	{"TIOCCONS",	TIOCCONS},
	{"TIOCSSIZE",	TIOCSSIZE},
	{"TIOCGSIZE",	TIOCGSIZE},
	{"SIOCSHIWAT",	SIOCSHIWAT},
	{"SIOCGHIWAT",	SIOCGHIWAT},
	{"SIOCSLOWAT",	SIOCSLOWAT},
	{"SIOCGLOWAT",	SIOCGLOWAT},
	{"SIOCADDRT",	SIOCADDRT},
	{"SIOCDELRT",	SIOCDELRT},
	{"SIOCSIFADDR",	SIOCSIFADDR},
	{"SIOCGIFADDR",	SIOCGIFADDR},
	{"SIOCSIFDSTADDR",	SIOCSIFDSTADDR},
	{"SIOCGIFDSTADDR",	SIOCGIFDSTADDR},
	{"SIOCSIFFLAGS",	SIOCSIFFLAGS},
	{"SIOCGIFFLAGS",	SIOCGIFFLAGS},
	{"SIOCGIFCONF",	SIOCGIFCONF},
	{"SIOCGIFBRDADDR",	SIOCGIFBRDADDR},
	{"SIOCSIFBRDADDR",	SIOCSIFBRDADDR},
	{"SIOCGIFNETMASK",	SIOCGIFNETMASK},
	{"SIOCSIFNETMASK",	SIOCSIFNETMASK},
	{"SIOCGIFMETRIC",	SIOCGIFMETRIC},
	{"SIOCSIFMETRIC",	SIOCSIFMETRIC},
	{"SIOCSARP",	SIOCSARP},
	{"SIOCGARP",	SIOCGARP},
	{"SIOCDARP",	SIOCDARP},
	{"TCXONC",      TCXONC},
	{"TCFLSH",      TCFLSH},
	{"TCGETS",      TCGETS},
	{"TCSETS",      TCSETS},
    };

    if ((request == TIOCGSIZE) || (request == TIOCGWINSZ)) {
	/*
	 * Special-case TIOCGSIZE since every visually-oriented program
	 * uses it and it's annoying to constantly get the messages.
	 */
	return;
    }

    /*
     * For now search the list linearly. It's slow but simple...
     */
    for (i = sizeof(mapping)/sizeof(*mapping) - 1; i >= 0; --i) {
	if (request == mapping[i].request) {
	    printf("ioctl: bad command %s\n", mapping[i].name);
	    return;
	}
    }
    printf("ioctl: bad command 0x%x\n", request);
    return;
}
