#ifndef _HOSTD
#define _HOSTD

typedef struct Dev_ClientInfo {
    int		hostNumber;
    int		hostState;
} Dev_ClientInfo;

#define	DEV_CLIENT_STATE_NEW_HOST	0x1
#define	DEV_CLIENT_STATE_DEAD_HOST	0x2
#define	DEV_CLIENT_START_LIST		0xF004	/* Unlikely to be ioctl # */
#define	DEV_CLIENT_END_LIST		0xF008	/* Unlikely to be ioctl # */

#endif /* _HOSTD */
