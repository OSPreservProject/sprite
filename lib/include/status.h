/*
 * status.h --
 *
 * 	Define the statuses that are returned from modules.
 *
 *	The fields of a status value are defined in the following way:
 *
 *	MSB			      LSB
 *	+--------+----------+----------+
 *	|Pub/Priv| Module # | Message #|
 *	+--------+----------+----------+
 *	  1 bit    15 bits     16 bits
 *
 *	The Pub/Priv bit if 0 is used to specify that the status is
 *		public and can be used by other modules.
 *	The Pub/Priv bit if 1 is used to specify that the status is
 *		private and can not be used by other modules.
 *	The module and message numbers are used to index the message array.
 *
 * Copyright 1986, 1988 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *
 * $Header: /sprite/src/lib/include/RCS/status.h,v 1.15 92/09/21 16:40:57 mgbaker Exp $ SPRITE (Berkeley)
 */

#ifndef _STATUS
#define _STATUS

#ifndef _ASM

#ifndef _SPRITE
#include <sprite.h>
#endif

extern char *Stat_GetMsg _ARGS_((ReturnStatus status));
extern char *Stat_GetPrivateMsg();
extern void Stat_PrintMsg _ARGS_((ReturnStatus status, char *string));

/*
 * STAT_IS_PRIVATE is TRUE if the status value is private and
 * 	is FALSE if the status is public.
 */

#define STAT_IS_PRIVATE(status)	((status) & 0x80000000)
#define STAT_MODULE(status)	(((status)& 0x7fffffff) >> 16)
#define STAT_MSGNUM(status)	((status) & 0x0000FFFF)

/*
 * stat_LastError contains the result of the last failed system call (some
 * library functions may also alter its value).
 */
extern ReturnStatus	stat_LastError;
extern void 	  	Stat_SetErrorHandler();
extern void 	  	Stat_SetErrorData();
extern void		Stat_GetErrorHandler();
extern ReturnStatus	Stat_Error();

/*
 * The procedures below are used to map between Sprite and UNIX
 * error numbers, for the interim period before Sprite converts
 * over to using UNIX error numbers.
 */

extern int		Compat_MapCode _ARGS_((ReturnStatus status));
extern ReturnStatus	Compat_MapToSprite _ARGS_((int unixErrno));
#endif /* _ASM */

/*---------------------------------------------*/

/*	gen.stat	*/
#define GEN_SUCCESS                        	0x00000000
#define GEN_FAILURE                        	0x00000001
#define GEN_ABORTED_BY_SIGNAL              	0x00000002
#define GEN_NO_PERMISSION                  	0x00000003
#define GEN_NOT_IMPLEMENTED                	0x00000004
#define GEN_INVALID_ARG                    	0x00000005
#define GEN_TIMEOUT                        	0x00000006
#define GEN_EPERM                        	0x00000007
#define GEN_ENOENT                        	0x00000008
#define GEN_EINTR                        	0x00000009
#define GEN_E2BIG                        	0x0000000a
#define GEN_EAGAIN                        	0x0000000b
#define GEN_EACCES                        	0x0000000c
#define GEN_EFAULT                        	0x0000000d
#define GEN_EEXIST                        	0x0000000e
#define GEN_EINVAL                        	0x0000000f
#define GEN_EFBIG                        	0x00000010
#define GEN_ENOSPC                        	0x00000011
#define GEN_ERANGE                        	0x00000012
#define GEN_EIDRM                        	0x00000013

/*	proc.stat	*/
#define PROC_BAD_FILE_NAME                  	0x00010000
#define PROC_BAD_AOUT_FORMAT                	0x00010001
#define PROC_NO_SEGMENTS                    	0x00010002
#define PROC_CHILD_PROC                     	0x00010003
#define PROC_NO_EXITS                       	0x00010004
#define PROC_INVALID_PID                    	0x00010005
#define PROC_UID_MISMATCH                   	0x00010006
#define PROC_NO_CHILDREN                    	0x00010007
#define PROC_INVALID_FAMILY_ID              	0x00010008
#define PROC_NOT_SET_ENVIRON_VAR            	0x00010009
#define PROC_BAD_ENVIRON_STRING             	0x0001000a
#define PROC_ENVIRON_FULL                   	0x0001000b
#define PROC_INVALID_NODE_ID                	0x0001000c
#define PROC_MIGRATION_REFUSED              	0x0001000d
#define PROC_INVALID_STRING                 	0x0001000e
#define PROC_NO_STACKS                      	0x0001000f
#define PROC_NO_PEER                        	0x00010010

/*	sys.stat	*/
#define SYS_ARG_NOACCESS                   	0x00020000
#define SYS_INVALID_ARG                    	0x00020001
#define SYS_INVALID_SYSTEM_CALL            	0x00020002

/*	rpc.stat	*/
#define RPC_INVALID_ARG                    	0x00030000
#define RPC_NO_CHANNELS                    	0x00030001
#define RPC_TIMEOUT                        	0x00030002
#define RPC_TOO_MANY_ACKS                  	0x00030003
#define RPC_INTERNAL_ERROR                 	0x00030004
#define RPC_INVALID_RPC                    	0x00030005
#define RPC_NULL_ERROR                     	0x00030006
#define RPC_PARAMS_TOOBIG                  	0x00030007
#define RPC_DATA_TOOBIG                    	0x00030008
#define RPC_NO_REPLY                       	0x00030009
#define RPC_SERVICE_DISABLED               	0x0003000a
#define	RPC_NACK_ERROR				0x0003000b
#define	RPC_FS_NO_PREFIX			0x0003000c

/*	fs.stat	*/
#define FS_NO_ACCESS                      	0x00040000
#define FS_INVALID_ARG                    	0x00040001
#define FS_REMOTE_OP_INVALID              	0x00040002
#define FS_LOCAL_OP_INVALID               	0x00040003
#define FS_DEVICE_OP_INVALID              	0x00040004
#define FS_NEW_ID_TOO_BIG                 	0x00040005
#define FS_MASTER_CLOSED                  	0x00040006
#define FS_BROKEN_PIPE                    	0x00040007
#define FS_NO_DISK_SPACE                  	0x00040008
#define FS_LOOKUP_REDIRECT                	0x00040009
#define FS_NO_HANDLE                      	0x0004000a
#define FS_NEW_PREFIX                     	0x0004000b
#define FS_FILE_NOT_FOUND                 	0x0004000c
#define FS_WOULD_BLOCK                    	0x0004000d
#define FS_BUFFER_TOO_BIG                 	0x0004000e
#define FS_IS_DIRECTORY                   	0x0004000f
#define FS_NOT_DIRECTORY                  	0x00040010
#define FS_NOT_OWNER                      	0x00040011
#define FS_STALE_HANDLE                   	0x00040012
#define FS_FILE_EXISTS                    	0x00040013
#define FS_DIR_NOT_EMPTY                  	0x00040014
#define FS_NAME_LOOP                      	0x00040015
#define FS_CROSS_DOMAIN_OPERATION         	0x00040016
#define FS_TIMEOUT                        	0x00040017
#define FS_NO_SHARED_LOCK                 	0x00040018
#define FS_NO_EXCLUSIVE_LOCK              	0x00040019
#define FS_WRONG_TYPE                     	0x0004001a
#define FS_FILE_REMOVED                   	0x0004001b
#define FS_FILE_BUSY                      	0x0004001c
#define FS_BAD_SEEK                       	0x0004001d
#define FS_DOMAIN_UNAVAILABLE             	0x0004001e
#define FS_VERSION_MISMATCH               	0x0004001f
#define FS_NOT_CACHEABLE                  	0x00040020
#define FS_NO_REFERENCE                  	0x00040021
#define FS_RECOV_SKIP                  		0x00040022

/*	vm.stat	*/
#define VM_WRONG_SEG_TYPE                 	0x00050000
#define VM_SEG_TOO_LARGE                  	0x00050001
#define VM_SHORT_READ                     	0x00050002
#define VM_SHORT_WRITE                    	0x00050003
#define VM_SWAP_ERROR                     	0x00050004
#define VM_NO_SEGMENTS                    	0x00050005

/*	sig.stat	*/
#define SIG_INVALID_SIGNAL                 	0x00060000
#define SIG_INVALID_ACTION                 	0x00060001

/*	dev.stat	*/
#define DEV_DMA_FAULT                      	0x00070000
#define DEV_INVALID_UNIT                   	0x00070001
#define DEV_TIMEOUT                        	0x00070002
#define DEV_OFFLINE                        	0x00070003
#define DEV_HANDSHAKE_ERROR                	0x00070004
#define DEV_RETRY_ERROR                    	0x00070005
#define DEV_NO_DEVICE                      	0x00070006
#define DEV_INVALID_ARG                    	0x00070007
#define DEV_HARD_ERROR                     	0x00070008
#define DEV_END_OF_TAPE                    	0x00070009
#define DEV_NO_MEDIA                       	0x0007000a
#define DEV_EARLY_CMD_COMPLETION           	0x0007000b
#define DEV_NO_SENSE                       	0x0007000c
#define DEV_BLANK_CHECK                    	0x0007000d
#define DEV_BUSY				0x0007000e
#define DEV_RESET				0x0007000f

/*	net.stat	*/
#define NET_UNREACHABLE_NET                	0x00080000
#define NET_UNREACHABLE_HOST               	0x00080001
#define NET_CONNECT_REFUSED                	0x00080002
#define NET_CONNECTION_RESET               	0x00080003
#define NET_NO_CONNECTS                    	0x00080004
#define NET_ALREADY_CONNECTED              	0x00080005
#define NET_NOT_CONNECTED                  	0x00080006
#define NET_ADDRESS_IN_USE                 	0x00080007
#define NET_ADDRESS_NOT_AVAIL              	0x00080008
#define NET_BAD_PROTOCOL                   	0x00080009
#define NET_BAD_OPERATION                  	0x0008000a
#define NET_BAD_OPTION                     	0x0008000b


/*---------------------------------------------*/

#endif /* _STATUS */
