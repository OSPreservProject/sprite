--- ds3100.md/nfsAttr.o ---
rm -f ds3100.md/nfsAttr.o
cc  -g3 -O -Dds3100 -Dsprite -Uultrix -E -I.   -Ids3100.md -I/sprite/lib/include -I/sprite/lib/include/ds3100.md -c nfsAttr.c -o ds3100.md/nfsAttr.o
# 1 "nfsAttr.c"















static char rcsid[] = "$Header: /a/newcmds/nfsmount/RCS/nfsAttr.c,v 1.6 89/10/10 13:14:57 brent Exp $ SPRITE (Berkeley)";



# 1 "/sprite/lib/include/stdio.h"































typedef int *ClientData;





















































typedef struct _file {
    unsigned char *lastAccess;	


    int readCount;		


    int writeCount;		





    unsigned char *buffer;	

    int bufSize;		


    void (*readProc)();		
    void (*writeProc)();	
    int (*closeProc)();		

    ClientData clientData;	


    int status;			


    int flags;			

    struct _file *nextPtr;	




} FILE;


































































# 190 "/sprite/lib/include/stdio.h"












































extern FILE stdioInFile, stdioOutFile, stdioErrFile;


































extern void	clearerr();
extern int	fclose();
extern FILE *	fdopen();
extern int	fflush();
extern int	fgetc();
extern char *	fgets();
extern int	fileno();
extern FILE *	fopen();
extern int	fprintf();
extern int	fputc();
extern int	fputs();
extern int	fread();
extern FILE *	freopen();
extern int	fscanf();
extern long	fseek();
extern long	ftell();
extern int	fwrite();
extern char *	gets();
extern int	getw();
extern void	perror();



# 295 "/sprite/lib/include/stdio.h"

extern int	printf();

extern int	puts();
extern int	putw();
extern void	rewind();
extern int	scanf();
extern void	setbuf();
extern void	setbuffer();
extern void	setlinebuf();
extern int	setvbuf();
extern char *	sprintf();
extern int	sscanf();
extern FILE *	tmpfile();
extern char *	tmpnam();
extern char *	tempnam();
extern int	ungetc();
extern int	vfprintf();
extern int	vfscanf();
extern int	vprintf();
extern char *	vsprintf();
extern void	_cleanup();

extern void	Stdio_Setup();


# 20 "nfsAttr.c"


# 1 "./nfs.h"





















# 1 "/sprite/lib/include/sys/time.h"















struct timeval {
	long	tv_sec;		
	long	tv_usec;	
};

struct timezone {
	int	tz_minuteswest;	
	int	tz_dsttime;	
};



























struct	itimerval {
	struct	timeval it_interval;	
	struct	timeval it_value;	
};



# 1 "/sprite/lib/include/time.h"














struct tm {
	int	tm_sec;
	int	tm_min;
	int	tm_hour;
	int	tm_mday;
	int	tm_mon;
	int	tm_year;
	int	tm_wday;
	int	tm_yday;
	int	tm_isdst;
	long	tm_gmtoff;
	char	*tm_zone;
};

extern	struct tm *gmtime(), *localtime();
extern	char *asctime(), *ctime();


# 59 "/sprite/lib/include/sys/time.h"



# 22 "./nfs.h"

# 1 "./rpc/rpc.h"









































# 1 "./rpc/types.h"










































# 45 "./rpc/types.h"


extern char *malloc();





# 1 "/sprite/lib/include/sys/types.h"























typedef	unsigned char	u_char;
typedef	unsigned short	u_short;
typedef	unsigned int	u_int;
typedef	unsigned long	u_long;
typedef	unsigned short	ushort;		

# 35 "/sprite/lib/include/sys/types.h"

# 41 "/sprite/lib/include/sys/types.h"

typedef	struct	_quad { long val[2]; } quad;
typedef	long	daddr_t;
typedef	char *	caddr_t;
typedef	long *	qaddr_t;	
typedef	u_long	ino_t;
typedef	long	swblk_t;


typedef	int	size_t;

typedef	long	time_t;
typedef	short	dev_t;
typedef	long	off_t;
typedef	short	uid_t;
typedef	short	gid_t;












typedef long	fd_mask;





typedef	struct fd_set {
	fd_mask	fds_bits[	(((256)+(( (sizeof(fd_mask) * 8		)	)-1))/( (sizeof(fd_mask) * 8		)	))];
} fd_set;







# 53 "./rpc/types.h"




# 42 "./rpc/rpc.h"

# 1 "/sprite/lib/include/netinet/in.h"



















# 1 "/sprite/lib/include/ds3100.md/machparam.h"






































# 1 "/sprite/lib/include/ds3100.md/limits.h"
























































































# 39 "/sprite/lib/include/ds3100.md/machparam.h"







































# 20 "/sprite/lib/include/netinet/in.h"











































struct in_addr {
	u_long s_addr;
};








































struct sockaddr_in {
	short	sin_family;
	u_short	sin_port;
	struct	in_addr sin_addr;
	char	sin_zero[8];
};










# 127 "/sprite/lib/include/netinet/in.h"

unsigned short  ntohs(), htons();
unsigned long   ntohl(), htonl();



# 43 "./rpc/rpc.h"



# 1 "./rpc/xdr.h"











































































enum xdr_op {
	XDR_ENCODE=0,
	XDR_DECODE=1,
	XDR_FREE=2
};

















typedef	int (*xdrproc_t)();







typedef struct {
	enum xdr_op	x_op;		
	struct xdr_ops {
		int	(*x_getlong)();	
		int	(*x_putlong)();	
		int	(*x_getbytes)();
		int	(*x_putbytes)();
		u_int	(*x_getpostn)();
		int  (*x_setpostn)();
		long *	(*x_inline)();	
		void	(*x_destroy)();	
	} *x_ops;
	caddr_t 	x_public;	
	caddr_t		x_private;	
	caddr_t 	x_base;		
	int		x_handy;	
} XDR;































































struct xdr_discrim {
	int	value;
	xdrproc_t proc;
};


































extern int	xdr_void();
extern int	xdr_int();
extern int	xdr_u_int();
extern int	xdr_long();
extern int	xdr_u_long();
extern int	xdr_short();
extern int	xdr_u_short();
extern int	xdr_bool();
extern int	xdr_enum();
extern int	xdr_array();
extern int	xdr_bytes();
extern int	xdr_opaque();
extern int	xdr_string();
extern int	xdr_union();
extern int	xdr_char();
extern int	xdr_u_char();
extern int	xdr_vector();
extern int	xdr_float();
extern int	xdr_double();
extern int	xdr_reference();
extern int	xdr_pointer();
extern int	xdr_wrapstring();






struct netobj {
	u_int	n_len;
	char	*n_bytes;
};
typedef struct netobj netobj;
extern int   xdr_netobj();





extern void   xdrmem_create();		
extern void   xdrstdio_create();	
extern void   xdrrec_create();		
extern int xdrrec_endofrecord();	
extern int xdrrec_skiprecord();	
extern int xdrrec_eof();		


# 46 "./rpc/rpc.h"



# 1 "./rpc/auth.h"
















































enum auth_stat {
	AUTH_OK=0,
	


	AUTH_BADCRED=1,			
	AUTH_REJECTEDCRED=2,		
	AUTH_BADVERF=3,			
	AUTH_REJECTEDVERF=4,		
	AUTH_TOOWEAK=5,			
	


	AUTH_INVALIDRESP=6,		
	AUTH_FAILED=7			
};

# 68 "./rpc/auth.h"





struct opaque_auth {
	int	oa_flavor;		
	caddr_t	oa_base;		
	u_int	oa_length;		
};





typedef struct {
	struct	opaque_auth	ah_cred;
	struct	opaque_auth	ah_verf;
	struct auth_ops {
		void	(*ah_nextverf)();
		int	(*ah_marshal)();	
		int	(*ah_validate)();	
		int	(*ah_refresh)();	
		void	(*ah_destroy)();	
	} *ah_ops;
	caddr_t ah_private;
} AUTH;




































extern struct opaque_auth _null_auth;















extern AUTH *authunix_create();
extern AUTH *authunix_create_default();	
extern AUTH *authnone_create();		






# 49 "./rpc/rpc.h"



# 1 "./rpc/clnt.h"













































enum clnt_stat {
	RPC_SUCCESS=0,			
	


	RPC_CANTENCODEARGS=1,		
	RPC_CANTDECODERES=2,		
	RPC_CANTSEND=3,			
	RPC_CANTRECV=4,			
	RPC_TIMEDOUT=5,			
	


	RPC_VERSMISMATCH=6,		
	RPC_AUTHERROR=7,		
	RPC_PROGUNAVAIL=8,		
	RPC_PROGVERSMISMATCH=9,		
	RPC_PROCUNAVAIL=10,		
	RPC_CANTDECODEARGS=11,		
	RPC_SYSTEMERROR=12,		

	


	RPC_UNKNOWNHOST=13,		
	RPC_UNKNOWNPROTO=17,		

	


	RPC_PMAPFAILURE=14,		
	RPC_PROGNOTREGISTERED=15,	
	


	RPC_FAILED=16
};





struct rpc_err {
	enum clnt_stat re_status;
	union {
		int RE_errno;		
		enum auth_stat RE_why;	
		struct {
			u_long low;	
			u_long high;	
		} RE_vers;
		struct {		
			long s1;
			long s2;
		} RE_lb;		
	} ru;




};







typedef struct {
	AUTH	*cl_auth;			
	struct clnt_ops {
		enum clnt_stat	(*cl_call)();	
		void		(*cl_abort)();	
		void		(*cl_geterr)();	
		int		(*cl_freeres)(); 
		void		(*cl_destroy)();
		int          (*cl_control)();
	} *cl_ops;
	caddr_t			cl_private;	
} CLIENT;

















































































































extern CLIENT *clntraw_create();





extern CLIENT *
clnt_create(); 






	
	












extern CLIENT *clnttcp_create();






















extern CLIENT *clntudp_create();
extern CLIENT *clntudp_bufcreate();




void clnt_pcreateerror();	
char *clnt_spcreateerror();	



 
void clnt_perrno();	




void clnt_perror(); 	
char *clnt_sperror();	




struct rpc_createerr {
	enum clnt_stat cf_stat;
	struct rpc_err cf_error; 
};

extern struct rpc_createerr rpc_createerr;






char *clnt_sperrno();	







# 52 "./rpc/rpc.h"



# 1 "./rpc/rpc_msg.h"















































enum msg_type {
	CALL=0,
	REPLY=1
};

enum reply_stat {
	MSG_ACCEPTED=0,
	MSG_DENIED=1
};

enum accept_stat {
	SUCCESS=0,
	PROG_UNAVAIL=1,
	PROG_MISMATCH=2,
	PROC_UNAVAIL=3,
	GARBAGE_ARGS=4,
	SYSTEM_ERR=5
};

enum reject_stat {
	RPC_MISMATCH=0,
	AUTH_ERROR=1
};










struct accepted_reply {
	struct opaque_auth	ar_verf;
	enum accept_stat	ar_stat;
	union {
		struct {
			u_long	low;
			u_long	high;
		} AR_versions;
		struct {
			caddr_t	where;
			xdrproc_t proc;
		} AR_results;
		
	} ru;


};




struct rejected_reply {
	enum reject_stat rj_stat;
	union {
		struct {
			u_long low;
			u_long high;
		} RJ_versions;
		enum auth_stat RJ_why;  
	} ru;


};




struct reply_body {
	enum reply_stat rp_stat;
	union {
		struct accepted_reply RP_ar;
		struct rejected_reply RP_dr;
	} ru;


};




struct call_body {
	u_long cb_rpcvers;	
	u_long cb_prog;
	u_long cb_vers;
	u_long cb_proc;
	struct opaque_auth cb_cred;
	struct opaque_auth cb_verf; 
};




struct rpc_msg {
	u_long			rm_xid;
	enum msg_type		rm_direction;
	union {
		struct call_body RM_cmb;
		struct reply_body RM_rmb;
	} ru;


};










extern int	xdr_callmsg();







extern int	xdr_callhdr();







extern int	xdr_replymsg();







extern void	_seterr_reply();
# 55 "./rpc/rpc.h"

# 1 "./rpc/auth_unix.h"





















































struct authunix_parms {
	u_long	 aup_time;
	char	*aup_machname;
	int	 aup_uid;
	int	 aup_gid;
	u_int	 aup_len;
	int	*aup_gids;
};

extern int xdr_authunix_parms();






struct short_hand_verf {
	struct opaque_auth new_cred;
};
# 56 "./rpc/rpc.h"



# 1 "./rpc/svc.h"






























































enum xprt_stat {
	XPRT_DIED,
	XPRT_MOREREQS,
	XPRT_IDLE
};




typedef struct {
	int		xp_sock;
	u_short		xp_port;	 
	struct xp_ops {
	    int	(*xp_recv)();	 
	    enum xprt_stat (*xp_stat)(); 
	    int	(*xp_getargs)(); 
	    int	(*xp_reply)();	 
	    int	(*xp_freeargs)();
	    void	(*xp_destroy)(); 
	} *xp_ops;
	int		xp_addrlen;	 
	struct sockaddr_in xp_raddr;	 
	struct opaque_auth xp_verf;	 
	caddr_t		xp_p1;		 
	caddr_t		xp_p2;		 
} SVCXPRT;
















































struct svc_req {
	u_long		rq_prog;	
	u_long		rq_vers;	
	u_long		rq_proc;	
	struct opaque_auth rq_cred;	
	caddr_t		rq_clntcred;	
	SVCXPRT	*rq_xprt;		
};












extern int	svc_register();








extern void	svc_unregister();







extern void	xprt_register();







extern void	xprt_unregister();






























extern int  svc_sendreply();
extern void	svcerr_decode();
extern void	svcerr_weakauth();
extern void	svcerr_noproc();

















extern fd_set svc_fdset;

# 238 "./rpc/svc.h"






extern void rpctest_service();

extern void	svc_getreq();
extern void	svc_getreqset();	
extern void	svc_run(); 	 













extern SVCXPRT *svcraw_create();




extern SVCXPRT *svcudp_create();
extern SVCXPRT *svcudp_bufcreate();




extern SVCXPRT *svctcp_create();




# 59 "./rpc/rpc.h"

# 1 "./rpc/svc_auth.h"









































extern enum auth_stat _authenticate();
# 60 "./rpc/rpc.h"




# 1 "./rpc/netdb.h"

































struct rpcent {
      char    *r_name;        
      char    **r_aliases;    
      int     r_number;       
};

struct rpcent *getrpcbyname(), *getrpcbynumber(), *getrpcent();
# 64 "./rpc/rpc.h"


# 23 "./nfs.h"

# 1 "./mount.h"

# 1 "./voiddef.h"





















# 24 "./voiddef.h"

typedef int *VoidPtr;






# 2 "./mount.h"





typedef char fhandle[32];
int xdr_fhandle();


struct fhstatus {
	u_int fhs_status;
	union {
		fhandle fhs_fhandle;
	} fhstatus_u;
};
typedef struct fhstatus fhstatus;
int xdr_fhstatus();


typedef char *dirpath;
int xdr_dirpath();


typedef char *arbname;
int xdr_arbname();


struct mountlist {
	arbname ml_hostname;
	dirpath ml_directory;
	struct mountlist *ml_next;
};
typedef struct mountlist mountlist;
int xdr_mountlist();


typedef struct groupnode *groups;
int xdr_groups();


struct groupnode {
	arbname gr_name;
	groups gr_next;
};
typedef struct groupnode groupnode;
int xdr_groupnode();


typedef struct exportnode *exports;
int xdr_exports();


struct exportnode {
	dirpath ex_dir;
	groups ex_groups;
	exports ex_next;
};
typedef struct exportnode exportnode;
int xdr_exportnode();





extern VoidPtr mountproc_null_1();

extern fhstatus *mountproc_mnt_1();

extern mountlist *mountproc_dump_1();

extern VoidPtr mountproc_umnt_1();

extern VoidPtr mountproc_umntall_1();

extern exports *mountproc_export_1();

extern exports *mountproc_exportall_1();

# 24 "./nfs.h"

# 1 "./nfs_prot.h"

# 1 "./voiddef.h"


















# 30 "./voiddef.h"


# 2 "./nfs_prot.h"
















enum nfsstat {
	NFS_OK = 0,
	NFSERR_PERM = 1,
	NFSERR_NOENT = 2,
	NFSERR_IO = 5,
	NFSERR_NXIO = 6,
	NFSERR_ACCES = 13,
	NFSERR_EXIST = 17,
	NFSERR_NODEV = 19,
	NFSERR_NOTDIR = 20,
	NFSERR_ISDIR = 21,
	NFSERR_FBIG = 27,
	NFSERR_NOSPC = 28,
	NFSERR_ROFS = 30,
	NFSERR_NAMETOOLONG = 63,
	NFSERR_NOTEMPTY = 66,
	NFSERR_DQUOT = 69,
	NFSERR_STALE = 70,
	NFSERR_WFLUSH = 99,
};
typedef enum nfsstat nfsstat;
int xdr_nfsstat();


enum ftype {
	NFNON = 0,
	NFREG = 1,
	NFDIR = 2,
	NFBLK = 3,
	NFCHR = 4,
	NFLNK = 5,
	NFSOCK = 6,
	NFBAD = 7,
	NFFIFO = 8,
};
typedef enum ftype ftype;
int xdr_ftype();


struct nfs_fh {
	char data[32];
};
typedef struct nfs_fh nfs_fh;
int xdr_nfs_fh();


struct nfstime {
	u_int seconds;
	u_int useconds;
};
typedef struct nfstime nfstime;
int xdr_nfstime();


struct fattr {
	ftype type;
	u_int mode;
	u_int nlink;
	u_int uid;
	u_int gid;
	u_int size;
	u_int blocksize;
	u_int rdev;
	u_int blocks;
	u_int fsid;
	u_int fileid;
	nfstime atime;
	nfstime mtime;
	nfstime ctime;
};
typedef struct fattr fattr;
int xdr_fattr();


struct sattr {
	u_int mode;
	u_int uid;
	u_int gid;
	u_int size;
	nfstime atime;
	nfstime mtime;
};
typedef struct sattr sattr;
int xdr_sattr();


typedef char *filename;
int xdr_filename();


typedef char *nfspath;
int xdr_nfspath();


struct attrstat {
	nfsstat status;
	union {
		fattr attributes;
	} attrstat_u;
};
typedef struct attrstat attrstat;
int xdr_attrstat();


struct sattrargs {
	nfs_fh file;
	sattr attributes;
};
typedef struct sattrargs sattrargs;
int xdr_sattrargs();


struct diropargs {
	nfs_fh dir;
	filename name;
};
typedef struct diropargs diropargs;
int xdr_diropargs();


struct diropokres {
	nfs_fh file;
	fattr attributes;
};
typedef struct diropokres diropokres;
int xdr_diropokres();


struct diropres {
	nfsstat status;
	union {
		diropokres diropres;
	} diropres_u;
};
typedef struct diropres diropres;
int xdr_diropres();


struct readlinkres {
	nfsstat status;
	union {
		nfspath data;
	} readlinkres_u;
};
typedef struct readlinkres readlinkres;
int xdr_readlinkres();


struct readargs {
	nfs_fh file;
	u_int offset;
	u_int count;
	u_int totalcount;
};
typedef struct readargs readargs;
int xdr_readargs();


struct readokres {
	fattr attributes;
	struct {
		u_int data_len;
		char *data_val;
	} data;
};
typedef struct readokres readokres;
int xdr_readokres();


struct readres {
	nfsstat status;
	union {
		readokres reply;
	} readres_u;
};
typedef struct readres readres;
int xdr_readres();


struct writeargs {
	nfs_fh file;
	u_int beginoffset;
	u_int offset;
	u_int totalcount;
	struct {
		u_int data_len;
		char *data_val;
	} data;
};
typedef struct writeargs writeargs;
int xdr_writeargs();


struct createargs {
	diropargs where;
	sattr attributes;
};
typedef struct createargs createargs;
int xdr_createargs();


struct renameargs {
	diropargs from;
	diropargs to;
};
typedef struct renameargs renameargs;
int xdr_renameargs();


struct linkargs {
	nfs_fh from;
	diropargs to;
};
typedef struct linkargs linkargs;
int xdr_linkargs();


struct symlinkargs {
	diropargs from;
	nfspath to;
	sattr attributes;
};
typedef struct symlinkargs symlinkargs;
int xdr_symlinkargs();


typedef char nfscookie[4];
int xdr_nfscookie();


struct readdirargs {
	nfs_fh dir;
	nfscookie cookie;
	u_int count;
};
typedef struct readdirargs readdirargs;
int xdr_readdirargs();


struct entry {
	u_int fileid;
	filename name;
	nfscookie cookie;
	struct entry *nextentry;
};
typedef struct entry entry;
int xdr_entry();


struct dirlist {
	entry *entries;
	int eof;
};
typedef struct dirlist dirlist;
int xdr_dirlist();


struct readdirres {
	nfsstat status;
	union {
		dirlist reply;
	} readdirres_u;
};
typedef struct readdirres readdirres;
int xdr_readdirres();


struct statfsokres {
	u_int tsize;
	u_int bsize;
	u_int blocks;
	u_int bfree;
	u_int bavail;
};
typedef struct statfsokres statfsokres;
int xdr_statfsokres();


struct statfsres {
	nfsstat status;
	union {
		statfsokres reply;
	} statfsres_u;
};
typedef struct statfsres statfsres;
int xdr_statfsres();





extern VoidPtr nfsproc_null_2();

extern attrstat *nfsproc_getattr_2();

extern attrstat *nfsproc_setattr_2();

extern VoidPtr nfsproc_root_2();

extern diropres *nfsproc_lookup_2();

extern readlinkres *nfsproc_readlink_2();

extern readres *nfsproc_read_2();

extern VoidPtr nfsproc_writecache_2();

extern attrstat *nfsproc_write_2();

extern diropres *nfsproc_create_2();

extern nfsstat *nfsproc_remove_2();

extern nfsstat *nfsproc_rename_2();

extern nfsstat *nfsproc_link_2();

extern nfsstat *nfsproc_symlink_2();

extern diropres *nfsproc_mkdir_2();

extern nfsstat *nfsproc_rmdir_2();

extern readdirres *nfsproc_readdir_2();

extern statfsres *nfsproc_statfs_2();

# 25 "./nfs.h"

# 1 "/sprite/lib/include/errno.h"





















extern int	errno;		

extern int	sys_nerr;	
extern char	*sys_errlist[];	















































































# 26 "./nfs.h"



# 1 "/sprite/lib/include/status.h"






































# 1 "/sprite/lib/include/sprite.h"

























# 28 "/sprite/lib/include/sprite.h"

# 31 "/sprite/lib/include/sprite.h"



typedef int Boolean;






typedef int  ReturnStatus;






















# 66 "/sprite/lib/include/sprite.h"








typedef char *Address;







# 85 "/sprite/lib/include/sprite.h"










# 39 "/sprite/lib/include/status.h"


extern char *Stat_GetMsg();
extern char *Stat_GetPrivateMsg();
extern void Stat_PrintMsg();














extern ReturnStatus	stat_LastError;
extern void 	  	Stat_SetErrorHandler();
extern void 	  	Stat_SetErrorData();
extern void		Stat_GetErrorHandler();
extern ReturnStatus	Stat_Error();







extern int		Compat_MapCode();
extern ReturnStatus	Compat_MapToSprite();




































































































































# 29 "./nfs.h"

# 1 "/sprite/lib/include/kernel/fs.h"































# 38 "/sprite/lib/include/kernel/fs.h"


# 1 "/sprite/lib/include/kernel/sys.h"

















# 1 "/sprite/lib/include/kernel/user/sys.h"





















# 24 "/sprite/lib/include/kernel/user/sys.h"


typedef enum {
    SYS_WARNING,
    SYS_FATAL
} Sys_PanicLevel;



























typedef struct {
    int architecture;		
    int type;			
    int	processors;		
} Sys_MachineInfo;

















extern ReturnStatus		Sys_GetMachineInfo();


# 18 "/sprite/lib/include/kernel/sys.h"

# 1 "/sprite/lib/include/sprite.h"








































# 94 "/sprite/lib/include/sprite.h"

# 19 "/sprite/lib/include/kernel/sys.h"

# 1 "/sprite/lib/include/status.h"









































# 203 "/sprite/lib/include/status.h"

# 20 "/sprite/lib/include/kernel/sys.h"
















# 51 "/sprite/lib/include/kernel/sys.h"







extern	ReturnStatus	Sys_GetTimeOfDay();
extern	ReturnStatus	Sys_SetTimeOfDay();
extern	ReturnStatus	Sys_DoNothing();
extern	ReturnStatus	Sys_Shutdown();
extern	ReturnStatus	Sys_GetMachineInfo();
extern	ReturnStatus	Sys_GetMachineInfoNew();



# 40 "/sprite/lib/include/kernel/fs.h"

# 1 "/sprite/lib/include/kernel/sync.h"



















































# 1 "/sprite/lib/include/sprite.h"








































# 94 "/sprite/lib/include/sprite.h"

# 52 "/sprite/lib/include/kernel/sync.h"

# 1 "/sprite/lib/include/list.h"




















# 23 "/sprite/lib/include/list.h"





















































typedef struct List_Links {
    struct List_Links *prevPtr;
    struct List_Links *nextPtr;
} List_Links;





void	List_Init();    
void    List_Insert();  
void    List_ListInsert();  
void 	List_Remove();  
void 	List_Move();    














    






































































































































































# 53 "/sprite/lib/include/kernel/sync.h"

# 60 "/sprite/lib/include/kernel/sync.h"


# 1 "/sprite/lib/include/sync.h"






























# 33 "/sprite/lib/include/sync.h"

















typedef struct Sync_UserLock {
    Boolean inUse;		
    Boolean waiting;		
} Sync_UserLock;


typedef Sync_UserLock Sync_Lock;	 











typedef struct Sync_Condition {
    Boolean waiting;		
} Sync_Condition;













# 86 "/sprite/lib/include/sync.h"






typedef struct {
    int		inUse;				
    int		class;				
    int		hit;				
    int		miss;				
    char	name[30];			
    int		priorCount;			
    int		priorTypes[1];	
    int		activeCount;			
    int		deadCount;			
    int		spinCount;			
} Sync_LockStat;




































































































extern ReturnStatus	Sync_GetLock();
extern ReturnStatus	Sync_Unlock();
extern ReturnStatus	Sync_SlowLock();
extern ReturnStatus	Sync_SlowWait();
extern ReturnStatus	Sync_SlowBroadcast();


# 62 "/sprite/lib/include/kernel/sync.h"

# 1 "/sprite/lib/include/kernel/proc.h"






















# 31 "/sprite/lib/include/kernel/proc.h"


# 1 "/sprite/lib/include/proc.h"























# 1 "/sprite/lib/include/spriteTime.h"




















# 23 "/sprite/lib/include/spriteTime.h"








typedef struct {
    int	seconds;
    int	microseconds;
} Time;

typedef struct {
    int year;
    int month;
    int dayOfYear;
    int dayOfMonth;
    int dayOfWeek;
    int	hours;
    int	minutes;
    int	seconds;
    int localOffset;
    Boolean dst;
} Time_Parts;






















extern Time time_ZeroSeconds;
extern Time time_OneMicrosecond;
extern Time time_OneMillisecond;
extern Time time_TenMilliseconds;
extern Time time_HundredMilliseconds;
extern Time time_HalfSecond;
extern Time time_OneSecond;
extern Time time_TwoSeconds;
extern Time time_TenSeconds;
extern Time time_OneMinute;
extern Time time_OneHour;
extern Time time_OneDay;
extern Time time_OneYear;
extern Time time_OneLeapYear;




extern void 	Time_Add();
extern void 	Time_Subtract();
extern void 	Time_Multiply();
extern void 	Time_Divide();
extern void 	Time_Normalize();
extern void	Time_ToAscii();
extern void	Time_ToParts();
















































# 24 "/sprite/lib/include/proc.h"

# 1 "/sprite/lib/include/sig.h"













































typedef	struct {
    int		action;
    int		(*handler)();
    int		sigHoldMask;
} Sig_Action;

































































































# 25 "/sprite/lib/include/proc.h"

# 1 "/sprite/lib/include/ds3100.md/kernel/mach.h"



















# 23 "/sprite/lib/include/ds3100.md/kernel/mach.h"


# 1 "/sprite/lib/include/ds3100.md/kernel/machConst.h"



















# 22 "/sprite/lib/include/ds3100.md/kernel/machConst.h"


# 1 "/sprite/lib/include/ds3100.md/kernel/vmPmaxConst.h"
































































































































































































# 24 "/sprite/lib/include/ds3100.md/kernel/machConst.h"




































































































































































































































































































































































































# 25 "/sprite/lib/include/ds3100.md/kernel/mach.h"

# 1 "/sprite/lib/include/fmt.h"
































typedef int Fmt_Format;














extern int	Fmt_Convert();
extern int	Fmt_Size();



# 26 "/sprite/lib/include/ds3100.md/kernel/mach.h"





typedef enum {
    MACH_USER,
    MACH_KERNEL
} Mach_ProcessorStates;














































extern ReturnStatus (*(mach_NormalHandlers[]))();
extern ReturnStatus (*(mach_MigratedHandlers[]))();












typedef struct {
    Address		pc;			
    unsigned		regs[32];	
    unsigned		fpRegs[32];	

    unsigned		fpStatusReg;		

    unsigned		mflo, mfhi;		

} Mach_RegState;




typedef struct {
    Mach_RegState	regState;		

    int			unixRetVal;		

} Mach_UserState;




typedef struct Mach_State {
    Mach_UserState	userState;		
    Mach_RegState	switchRegState;		

    Address		kernStackStart;		

    Address		kernStackEnd;		

    unsigned		sstepInst;		


    unsigned		tlbHighEntry;		

    unsigned		tlbLowEntries[4 - 1];
    						


} Mach_State;




typedef struct {
    int		  	break1Inst;	

    Mach_UserState	userState;	

    unsigned		fpRegs[32];	

    unsigned		fpStatusReg;		

} Mach_SigContext;




typedef struct {
    int		regs[32];
    int		fpRegs[32];
    unsigned	sig[32];
    unsigned	excPC;
    unsigned	causeReg;
    unsigned	multHi;
    unsigned	multLo;
    unsigned	fpCSR;
    unsigned	fpEIR;
    unsigned	trapCause;
    unsigned	trapInfo;
    unsigned	tlbIndex;
    unsigned	tlbRandom;
    unsigned	tlbLow;
    unsigned	tlbContext;
    unsigned	badVaddr;
    unsigned	tlbHi;
    unsigned	statusReg;
} Mach_DebugState;






extern	Boolean	mach_KernelMode;
extern	int	mach_NumProcessors;
extern	Boolean	mach_AtInterruptLevel;
extern	int	*mach_NumDisableIntrsPtr;



extern	char	*mach_MachineType;






extern	Fmt_Format	mach_Format;





extern void	Mach_Init();




extern	void		Mach_InitFirstProc();
extern	ReturnStatus	Mach_SetupNewState();
extern	void		Mach_SetReturnVal();
extern	void		Mach_StartUserProc();
extern	void		Mach_ExecUserProc();
extern	void		Mach_FreeState();
extern	void		Mach_CopyState();
extern	void		Mach_GetDebugState();
extern	void		Mach_SetDebugState();
extern	Address		Mach_GetUserStackPtr();




extern ReturnStatus		Mach_EncapState();
extern ReturnStatus		Mach_DeencapState();
extern ReturnStatus		Mach_GetEncapSize();
extern Boolean			Mach_CanMigrate();




extern void			Mach_InitSyscall();
extern Mach_ProcessorStates	Mach_ProcessorState();
extern int			Mach_GetNumProcessors();
extern Address			Mach_GetPC();



extern	void		Mach_GetEtherAddress();
extern	void		Mach_ContextSwitch();
extern	int		Mach_TestAndSet();
extern	int		Mach_GetMachineType();
extern	int		Mach_GetMachineArch();
extern	Address		Mach_GetStackPointer();
extern 	void		Mach_CheckSpecialHandling();
extern 	int		Mach_GetBootArgs();
extern  ReturnStatus	Mach_ProbeAddr();
extern	void		Mach_FlushCode();




extern	Address	mach_KernStart;
extern	Address	mach_CodeStart;
extern	Address	mach_StackBottom;
extern	int	mach_KernStackSize;
extern	Address	mach_KernEnd;
extern	Address	mach_FirstUserAddr;
extern	Address	mach_LastUserAddr;
extern	Address	mach_MaxUserStackAddr;
extern	int	mach_LastUserStackPage;


# 26 "/sprite/lib/include/proc.h"
# 28 "/sprite/lib/include/proc.h"


# 1 "/sprite/lib/include/vm.h"
















# 1 "/sprite/lib/include/sprite.h"








































# 94 "/sprite/lib/include/sprite.h"

# 17 "/sprite/lib/include/vm.h"

# 1 "/sprite/lib/include/ds3100.md/vmMach.h"

































# 1 "/sprite/lib/include/sprite.h"








































# 94 "/sprite/lib/include/sprite.h"

# 34 "/sprite/lib/include/ds3100.md/vmMach.h"







# 18 "/sprite/lib/include/vm.h"

# 1 "/sprite/lib/include/vmStat.h"















# 18 "/sprite/lib/include/vmStat.h"


# 1 "/sprite/lib/include/ds3100.md/kernel/vmMachStat.h"























typedef struct {
    int	stealTLB;	
    int stealPID;	
} VmMachDepStat;


# 20 "/sprite/lib/include/vmStat.h"









typedef struct {
    int	numPhysPages;		
    


    int	numFreePages;		
    int	numDirtyPages;		
    int	numReservePages;	

    int	numUserPages;		


    int	kernStackPages;		
    int kernMemPages;		

    


    int	totalFaults;		

    int	totalUserFaults;	

    int	zeroFilled;		

    int	fsFilled;		

    int	psFilled;		

    int	collFaults;		

    int	quickFaults;		

    int	codeFaults;		
    int	heapFaults;		
    int	stackFaults;		
    


    int	numAllocs;		
    int	gotFreePage;		
    int	pageAllocs;		
    int	gotPageFromFS;		

    int	numListSearches;	

    int	usedFreePage;		
    int	lockSearched;		

    int	refSearched;		

    int	dirtySearched;		

    int	reservePagesUsed;	

    


    int	pagesWritten;		

    int	cleanWait;		


    int	pageoutWakeup;		


    int	pageoutNoWork;		

    int pageoutWait;		


    


    int	mapPageWait;		


    int	accessWait;		


    


    VmMachDepStat	machDepStat;
    



    int	minVMPages;
    


    int	fsAsked;		

    int	haveFreePage;		

    int	fsMap;			

    int	fsUnmap;		

    int	maxFSPages;		

    int	minFSPages;		

    


    int	numCOWHeapPages;	
    int	numCOWStkPages;		
    int numCORHeapPages;	
    int numCORStkPages;		
    int	numCOWHeapFaults;	
    int	numCOWStkFaults;	
    int	quickCOWFaults;		
    int numCORHeapFaults;	
    int numCORStkFaults;	
    int	quickCORFaults;		
    int swapPagesCopied;	
    int	numCORCOWHeapFaults;	

    int	numCORCOWStkFaults;	

    


    int	potModPages;		


    int	notModPages;		

    int	notHardModPages;	

    


    int	codePrefetches;		
    int	heapSwapPrefetches;	
    int	heapFSPrefetches;	

    int	stackPrefetches;	

    int	codePrefetchHits;	
    int	heapSwapPrefetchHits;	

    int	heapFSPrefetchHits;	

    int	stackPrefetchHits;	

    int	prefetchAborts;		


} Vm_Stat;

extern	Vm_Stat	vmStat;

# 19 "/sprite/lib/include/vm.h"



































































































typedef int Vm_SegmentID;





















typedef struct Vm_SegmentInfo {
    int			segNum;		
    int 		refCount;	

				        

    char		objFileName[50];
    int           	type;		
    int			numPages;	
    int			ptSize;		
    int			resPages;	

    int			flags;		

    int			ptUserCount;	

    int			numCOWPages;	

    int			numCORPages;	

    Address		minAddr;	

    Address		maxAddr;	

    int			traceTime;	

} Vm_SegmentInfo;




extern	ReturnStatus	Vm_PageSize();
extern	ReturnStatus	Vm_CreateVA();
extern	ReturnStatus	Vm_DestroyVA();
extern	ReturnStatus	Vm_Cmd();
extern	ReturnStatus	Vm_GetSegInfo();


# 30 "/sprite/lib/include/proc.h"

















































typedef unsigned int 	Proc_PID;





























































typedef enum {
    PROC_UNUSED,	
    PROC_RUNNING,	
    PROC_READY,		
    PROC_WAITING,	

    PROC_EXITING,	

    PROC_DEAD,		
    PROC_MIGRATED,	
    PROC_NEW,		
    PROC_SUSPENDED	
} Proc_State;













































































typedef struct {
    Time kernelCpuUsage;	
    Time userCpuUsage;		

    Time childKernelCpuUsage;	

    Time childUserCpuUsage;	

    int	numQuantumEnds;		

    int numWaitEvents;		


} Proc_ResUsage;





typedef enum {
    PROC_GET_THIS_DEBUG,
    PROC_GET_NEXT_DEBUG,
    PROC_CONTINUE,
    PROC_SINGLE_STEP,
    PROC_GET_DBG_STATE,
    PROC_SET_DBG_STATE,
    PROC_READ,
    PROC_WRITE,
    PROC_DETACH_DEBUGGER
} Proc_DebugReq;
















typedef struct {
    Proc_PID	processID;		
    int	termReason;			

    int	termStatus;			

    int	termCode;			
    Mach_RegState regState;		
    int	sigHoldMask;			
    int	sigPendingMask;			
    int	sigActions[		32]; 	

    int	sigMasks[		32]; 	

    int	sigCodes[		32]; 	


} Proc_DebugState;





typedef struct {
    char *name;		
    char *value;	
} Proc_EnvironVar;




typedef struct  {
    int		processor;	



    Proc_State	state;		
 

    int		genFlags;	
 

    











    Proc_PID	processID;		



    Proc_PID	parentID;		

    int		familyID;		

    int		userID;			


    int		effectiveUserID;	


    









    int		 event;		 

    








    int 	 billingRate;	

    unsigned int recentUsage;	
    unsigned int weightedUsage;	

    unsigned int unweightedUsage; 


    







    Time kernelCpuUsage;	
    Time userCpuUsage;		

    Time childKernelCpuUsage;	

    Time childUserCpuUsage;	

    int 	numQuantumEnds;		


    int		numWaitEvents;		


    unsigned int schedQuantumTicks;	


    






    Vm_SegmentID		vmSegments[	4];


    







    int		sigHoldMask;		
    int		sigPendingMask;		
    					

    int		sigActions[		32];
    					

    






    int		peerHostID;		 


    Proc_PID	peerProcessID;		 

} Proc_PCBInfo;



































typedef struct {
    Time	interval;	
    Time	curValue;	
} Proc_TimerInterval;















typedef struct {
    char argString[256];
} Proc_PCBArgString;

















































extern ReturnStatus Proc_SetExitHandler();
extern void	    Proc_Exit();





extern ReturnStatus Proc_Fork();
extern void	    Proc_RawExit();
extern ReturnStatus Proc_Detach();
extern ReturnStatus Proc_Wait();
extern ReturnStatus Proc_RawWait();
extern ReturnStatus Proc_Exec();
extern ReturnStatus Proc_ExecEnv();

extern ReturnStatus Proc_GetIDs();
extern ReturnStatus Proc_SetIDs();
extern ReturnStatus Proc_GetGroupIDs();
extern ReturnStatus Proc_SetGroupIDs();
extern ReturnStatus Proc_GetFamilyID();
extern ReturnStatus Proc_SetFamilyID();

extern ReturnStatus Proc_GetPCBInfo();
extern ReturnStatus Proc_GetResUsage();
extern ReturnStatus Proc_GetPriority();
extern ReturnStatus Proc_SetPriority();

extern ReturnStatus Proc_Debug();
extern ReturnStatus Proc_Profile();

extern ReturnStatus Proc_SetIntervalTimer();
extern ReturnStatus Proc_GetIntervalTimer();

extern ReturnStatus Proc_SetEnviron();
extern ReturnStatus Proc_UnsetEnviron();
extern ReturnStatus Proc_GetEnvironVar();
extern ReturnStatus Proc_GetEnvironRange();
extern ReturnStatus Proc_InstallEnviron();
extern ReturnStatus Proc_CopyEnviron();

extern ReturnStatus Proc_Migrate();




# 33 "/sprite/lib/include/kernel/proc.h"

# 1 "/sprite/lib/include/sync.h"



































































































































# 210 "/sprite/lib/include/sync.h"

# 34 "/sprite/lib/include/kernel/proc.h"

# 1 "/sprite/lib/include/kernel/syncLock.h"























# 27 "/sprite/lib/include/kernel/syncLock.h"


# 1 "/sprite/lib/include/list.h"































































































































































































# 270 "/sprite/lib/include/list.h"

# 29 "/sprite/lib/include/kernel/syncLock.h"

# 1 "/sprite/lib/include/user/sync.h"



































































































































# 210 "/sprite/lib/include/user/sync.h"

# 30 "/sprite/lib/include/kernel/syncLock.h"






# 39 "/sprite/lib/include/kernel/syncLock.h"





# 46 "/sprite/lib/include/kernel/syncLock.h"

























typedef struct Sync_ListInfo {
    List_Links	links;		
    Address	lock;		

} Sync_ListInfo;








typedef enum Sync_LockClass {
    SYNC_SEMAPHORE,			
    SYNC_LOCK
} Sync_LockClass;




typedef struct Sync_Semaphore {
    


    int value;				



# 107 "/sprite/lib/include/kernel/syncLock.h"



    char *name;				
    Address holderPC;			
    Address holderPCBPtr;		



# 119 "/sprite/lib/include/kernel/syncLock.h"


} Sync_Semaphore;





typedef struct Sync_KernelLock{
    


    Boolean inUse;			
    Boolean waiting;	        	


# 142 "/sprite/lib/include/kernel/syncLock.h"



    char *name;				
    Address holderPC;			
    Address holderPCBPtr;		



# 155 "/sprite/lib/include/kernel/syncLock.h"

} Sync_KernelLock;


# 161 "/sprite/lib/include/kernel/syncLock.h"





# 35 "/sprite/lib/include/kernel/proc.h"

# 1 "/sprite/lib/include/list.h"































































































































































































# 270 "/sprite/lib/include/list.h"

# 36 "/sprite/lib/include/kernel/proc.h"

# 1 "/sprite/lib/include/kernel/timer.h"





















# 1 "/sprite/lib/include/list.h"































































































































































































# 270 "/sprite/lib/include/list.h"

# 22 "/sprite/lib/include/kernel/timer.h"

# 28 "/sprite/lib/include/kernel/timer.h"


# 1 "/sprite/lib/include/spriteTime.h"












































# 142 "/sprite/lib/include/spriteTime.h"

# 30 "/sprite/lib/include/kernel/timer.h"

# 1 "/sprite/lib/include/ds3100.md/kernel/timerTick.h"




















# 1 "/sprite/lib/include/spriteTime.h"












































# 142 "/sprite/lib/include/spriteTime.h"

# 21 "/sprite/lib/include/ds3100.md/kernel/timerTick.h"




typedef Time Timer_Ticks;





extern unsigned int 	timer_IntZeroSeconds; 
extern unsigned int 	timer_IntOneMillisecond;   
extern unsigned int 	timer_IntOneSecond;
extern unsigned int 	timer_IntOneMinute;
extern unsigned int 	timer_IntOneHour; 
extern Timer_Ticks	timer_TicksZeroSeconds;
extern Time 		timer_MaxIntervalTime; 






# 51 "/sprite/lib/include/ds3100.md/kernel/timerTick.h"











extern void Timer_AddIntervalToTicks();
extern void Timer_GetCurrentTicks();






















# 92 "/sprite/lib/include/ds3100.md/kernel/timerTick.h"









# 31 "/sprite/lib/include/kernel/timer.h"

# 1 "/sprite/lib/include/ds3100.md/kernel/timerMach.h"































# 32 "/sprite/lib/include/kernel/timer.h"

# 1 "/sprite/lib/include/kernel/syncLock.h"






























































# 164 "/sprite/lib/include/kernel/syncLock.h"


# 33 "/sprite/lib/include/kernel/timer.h"





































































































































 

































typedef struct {
    List_Links	links;		

    void	(*routine)();	
    Timer_Ticks	time;		
 
    ClientData	clientData;	


    Boolean	processed;	


    unsigned int interval;	


} Timer_QueueElement;





typedef struct {
    int		callback;	
    int		profile;	
    int		spurious;	
    int		schedule;	
    int		resched;	
    int		desched;	
} Timer_Statistics;

extern Timer_Statistics	timer_Statistics;
extern	Time		timer_UniversalApprox;
extern Sync_Semaphore 	timer_ClockMutex;












# 251 "/sprite/lib/include/kernel/timer.h"



extern void	Timer_ScheduleRoutine();
extern Boolean  Timer_DescheduleRoutine();

extern void Timer_GetTimeOfDay();
extern void Timer_GetRealTimeOfDay();
extern void Timer_SetTimeOfDay();

extern void Timer_LockRegister();







extern void 	Timer_TimerInit();
extern void 	Timer_TimerStart();
extern void	Timer_TimerInactivate();





extern	void	Timer_TimerServiceInterrupts();




extern void 	Timer_TimerGetInfo();
extern void	Timer_DumpQueue();
extern void	Timer_DumpStats();


# 37 "/sprite/lib/include/kernel/proc.h"

# 1 "/sprite/lib/include/kernel/sig.h"
















# 24 "/sprite/lib/include/kernel/sig.h"


# 1 "/sprite/lib/include/sig.h"


















































































# 147 "/sprite/lib/include/sig.h"

# 26 "/sprite/lib/include/kernel/sig.h"

# 1 "/sprite/lib/include/ds3100.md/kernel/mach.h"




















































































# 259 "/sprite/lib/include/ds3100.md/kernel/mach.h"

# 27 "/sprite/lib/include/kernel/sig.h"





typedef struct {
    int			oldHoldMask;	


    Mach_SigContext	machContext;	

} Sig_Context;




typedef struct {
    int		sigNum;		
    int		sigCode;    	
    Sig_Context	*contextPtr;	

} Sig_Stack;
























extern	ReturnStatus	Sig_Send();
extern	ReturnStatus 	Sig_SendProc();
extern	ReturnStatus	Sig_UserSend();
extern	ReturnStatus	Sig_SetHoldMask();
extern	ReturnStatus	Sig_SetAction();
extern	ReturnStatus	Sig_Pause();

extern	void		Sig_Init();
extern	void		Sig_ProcInit();
extern	void		Sig_Fork();
extern	void		Sig_Exec();
extern	void		Sig_ChangeState();
extern	Boolean		Sig_Handle();
extern	void		Sig_Return();





extern ReturnStatus	Sig_GetEncapSize();
extern ReturnStatus	Sig_EncapState();
extern ReturnStatus	Sig_DeencapState();
extern	void		Sig_AllowMigration();



# 38 "/sprite/lib/include/kernel/proc.h"

# 1 "/sprite/lib/include/ds3100.md/kernel/mach.h"




















































































# 259 "/sprite/lib/include/ds3100.md/kernel/mach.h"

# 39 "/sprite/lib/include/kernel/proc.h"












































typedef struct {
    unsigned int	interval;	

    ClientData		clientData;	
    ClientData		token;		
} Proc_CallInfo;





typedef struct {
    int			refCount;	

    int			size;		
    struct ProcEnvironVar *varArray;	
} Proc_EnvironInfo;










typedef struct {
    List_Links links;			
    struct Proc_ControlBlock *procPtr;	
} Proc_PCBLink;









typedef union {
    Timer_Ticks	ticks;	
    Time	time;	
} Proc_Time;

typedef struct {
    int		type;		
    Address	lockPtr;	
} Proc_LockStackElement;








typedef struct Proc_ControlBlock {
    List_Links	links;		

    int		processor;	



    Proc_State	state;		
 

    int		genFlags;	
 
    int		syncFlags;	
    int		schedFlags;	
    int		exitFlags;	


    List_Links		childListHdr;	
    List_Links		*childList;	
    Proc_PCBLink	siblingElement;	

    Proc_PCBLink	familyElement;	


    











    Proc_PID	processID;		



    Proc_PID	parentID;		

    int		familyID;		

    int		userID;			


    int		effectiveUserID;	


    










    int		 event;		 
    Proc_PCBLink eventHashChain; 

    



    Sync_Condition	waitCondition;
    Sync_Condition	lockedCondition;

    




    int			waitToken;

    








    int 	 billingRate;	

    unsigned int recentUsage;	
    unsigned int weightedUsage;	

    unsigned int unweightedUsage; 


    







    Proc_Time kernelCpuUsage;	
    Proc_Time userCpuUsage;	
    Proc_Time childKernelCpuUsage;	

    Proc_Time childUserCpuUsage;	

    int 	numQuantumEnds;		


    int		numWaitEvents;		


    unsigned int schedQuantumTicks;	



    







 
    struct	Mach_State	*machStatePtr;


    






    struct	Vm_ProcInfo	*vmPtr;

    







    struct Fs_ProcessState	*fsPtr;

    







    int	termReason;		

				
    int	termStatus;		

				
    int	termCode;		
				


    







    int		sigHoldMask;		
    int		sigPendingMask;		
    					

    int		sigActions[		32];
    					

    int		sigMasks[		32];
					

    int		sigCodes[		32];
    int		sigFlags;		

    int		oldSigHoldMask;		


    





    struct ProcIntTimerInfo	*timerArray;

    






    int			peerHostID;	


    Proc_PID		peerProcessID; 	

    struct Proc_ControlBlock
	             *rpcClientProcess;	



    







    


    Proc_EnvironInfo	*environPtr;

    


    char	*argString;



# 382 "/sprite/lib/include/kernel/proc.h"



    





     Sync_Semaphore		lockInfo;


    






    ReturnStatus (**kcallTable)();	





    int specialHandling;		





    







     short *Prof_Buffer;    

     int Prof_BufferSize;   
     int Prof_Offset;       
     int Prof_Scale;        

     int Prof_PC;           


    



    Address	remoteExecBuffer;	 


} Proc_ControlBlock;

































extern Proc_ControlBlock  **proc_RunningProcesses;








extern Proc_ControlBlock **proc_PCBTable;






extern int proc_MaxNumProcesses;




extern Boolean proc_RefuseMigrations;


































































extern void		  	Proc_Init();
extern void		  	Proc_InitMainProc();
extern ReturnStatus		Proc_NewProc();
extern void			ProcStartUserProc();
extern void			Proc_ExitInt();
extern void			Proc_Exit();
extern void			Proc_DetachInt();
extern ReturnStatus		Proc_Detach();
extern void			Proc_InformParent();	
extern void			Proc_Reaper();
extern void			Proc_NotifyMigratedWaiters();
extern void			Proc_PutOnDebugList();
extern void			Proc_SuspendProcess();
extern void			Proc_ResumeProcess();
extern int			Proc_ExecEnv();
extern int			Proc_RemoteExec();
extern ReturnStatus 		Proc_GetHostIDs();


extern ReturnStatus		Proc_EvictForeignProcs();
extern ReturnStatus		Proc_EvictProc();
extern Boolean			Proc_IsMigratedProc();
extern void			Proc_FlagMigration();
extern void			Proc_MigrateTrap();
extern void			Proc_OkayToMigrate();
extern ReturnStatus		Proc_MigSendUserInfo();
extern ReturnStatus		Proc_DoRemoteCall();
extern void			Proc_SetEffectiveProc();
extern Proc_ControlBlock *	Proc_GetEffectiveProc();
extern ReturnStatus		Proc_ByteCopy();
extern ReturnStatus		Proc_MakeStringAccessible();
extern void			Proc_MakeUnaccessible();
extern void			Proc_MigrateStartTracing();
extern void			Proc_DestroyMigratedProc();
extern void			Proc_NeverMigrate();
extern ReturnStatus		Proc_MigGetStats();
extern ReturnStatus		Proc_MigZeroStats();
extern  void	        Proc_MigAddToCounter();

extern void			ProcInitMainEnviron();
extern void			ProcSetupEnviron();
extern void			ProcDecEnvironRefCount();

extern void			Proc_SetServerPriority();

extern	int			Proc_KillAllProcesses();
extern	void			Proc_WakeupAllProcesses();

extern	void			Proc_Unlock();
extern	void			Proc_Lock();
extern	Proc_ControlBlock	*Proc_LockPID();
extern	ReturnStatus		Proc_LockFamily();
extern	void			Proc_UnlockFamily();
extern	void			Proc_TakeOffDebugList();
extern	Boolean			Proc_HasPermission();

extern	void			Proc_ServerInit();
extern	void			Proc_CallFunc();
extern	void			Proc_CancelCallFunc();
extern	ClientData		Proc_CallFuncAbsTime();
extern	void			Proc_ServerProc();
extern	int			proc_NumServers;

extern  ReturnStatus		Proc_Dump();
extern  void			Proc_DumpPCB();

extern  void			Proc_RemoveFromLockStack();
extern  void			Proc_PushLockStack();








extern ReturnStatus		Proc_SetEnvironStub();
extern ReturnStatus		Proc_UnsetEnvironStub();
extern ReturnStatus		Proc_GetEnvironVarStub();
extern ReturnStatus		Proc_GetEnvironRangeStub();
extern ReturnStatus		Proc_InstallEnvironStub();
extern ReturnStatus		Proc_CopyEnvironStub();
extern int                      Proc_KernExec();


# 63 "/sprite/lib/include/kernel/sync.h"

# 1 "/sprite/lib/include/kernel/syncLock.h"






























































# 164 "/sprite/lib/include/kernel/syncLock.h"


# 64 "/sprite/lib/include/kernel/sync.h"

# 1 "/sprite/lib/include/kernel/sys.h"






















# 66 "/sprite/lib/include/kernel/sys.h"

# 65 "/sprite/lib/include/kernel/sync.h"

# 1 "/sprite/lib/include/ds3100.md/kernel/mach.h"




















































































# 259 "/sprite/lib/include/ds3100.md/kernel/mach.h"

# 66 "/sprite/lib/include/kernel/sync.h"






# 75 "/sprite/lib/include/kernel/sync.h"





# 82 "/sprite/lib/include/kernel/sync.h"



















typedef struct Sync_Instrument {
    int numWakeups;		
    int numWakeupCalls;		
    int numSpuriousWakeups;	
    int numLocks;		
    int numUnlocks;		
    int spinCount[60+1]; 
    int sched_MutexMiss;	


    char pad[	32];

} Sync_Instrument;






typedef struct Sync_RegElement {
    List_Links 		links;			
    int			hit;			
    int			miss;			
    int			type;			
    char		*name;			
    Sync_LockClass	class;			
    int			priorCount;		
    int			priorTypes[1]; 
    int			activeLockCount;	
    List_Links		activeLocks;		
    int			deadLockCount;		
} Sync_RegElement;





typedef struct {
    List_Links	links;		
    int		hostID;		
    Proc_PID	pid;		
    int		waitToken;	
} Sync_RemoteWaiter;










    
extern Sync_Instrument 	sync_Instrument[	1];
extern Sync_Instrument	*sync_InstrumentPtr[	1];
extern int sync_BusyWaits;

extern 	void 		Sync_Init();

extern 	void 		Sync_WakeupProcess();
extern 	void 		Sync_EventWakeup();
extern 	void 		Sync_WakeWaitingProcess();
extern 	void 		Sync_UnlockAndSwitch();

extern 	Boolean 	Sync_SlowMasterWait();
extern 	Boolean 	Sync_SlowWait();
extern 	Boolean 	Sync_EventWait();
extern 	Boolean 	Sync_WaitTime();
extern 	Boolean 	Sync_WaitTimeInTicks();
extern 	Boolean 	Sync_WaitTimeInterval();

extern 	Boolean 	Sync_ProcWait();
extern 	void 		Sync_ProcWakeup();
extern 	void 		Sync_GetWaitToken();
extern 	void 		Sync_SetWaitToken();
extern 	ReturnStatus 	Sync_RemoteNotify();
extern 	ReturnStatus 	Sync_RemoteNotifyStub();
	
extern 	ReturnStatus 	Sync_SlowLockStub();
extern 	ReturnStatus 	Sync_SlowWaitStub();
extern 	ReturnStatus 	Sync_SlowBroadcastStub();

extern 	void 		Sync_PrintStat();

extern	void		Sync_LockStatInit();
extern	void		Sync_AddPriorInt();
extern	void		SyncDeleteCurrentInt();
extern 	void		SyncMergePriorInt();
extern	void		Sync_RegisterInt();
extern	void		Sync_CheckoutInt();
extern	void		Sync_PrintLockStats();

extern Sync_RegElement  *regQueuePtr;






























































# 309 "/sprite/lib/include/kernel/sync.h"

















































































































































































# 491 "/sprite/lib/include/kernel/sync.h"

# 497 "/sprite/lib/include/kernel/sync.h"

# 504 "/sprite/lib/include/kernel/sync.h"




























# 537 "/sprite/lib/include/kernel/sync.h"

# 547 "/sprite/lib/include/kernel/sync.h"

# 559 "/sprite/lib/include/kernel/sync.h"





























# 592 "/sprite/lib/include/kernel/sync.h"

# 599 "/sprite/lib/include/kernel/sync.h"

# 606 "/sprite/lib/include/kernel/sync.h"





























# 640 "/sprite/lib/include/kernel/sync.h"

# 650 "/sprite/lib/include/kernel/sync.h"

# 660 "/sprite/lib/include/kernel/sync.h"






























# 695 "/sprite/lib/include/kernel/sync.h"






















# 726 "/sprite/lib/include/kernel/sync.h"
























# 759 "/sprite/lib/include/kernel/sync.h"
























# 790 "/sprite/lib/include/kernel/sync.h"
























# 820 "/sprite/lib/include/kernel/sync.h"

























# 850 "/sprite/lib/include/kernel/sync.h"


























# 882 "/sprite/lib/include/kernel/sync.h"
































# 918 "/sprite/lib/include/kernel/sync.h"






















# 944 "/sprite/lib/include/kernel/sync.h"


















# 966 "/sprite/lib/include/kernel/sync.h"









# 41 "/sprite/lib/include/kernel/fs.h"

# 1 "/sprite/lib/include/kernel/proc.h"








































































































































































































































































# 643 "/sprite/lib/include/kernel/proc.h"

# 42 "/sprite/lib/include/kernel/fs.h"

# 1 "/sprite/lib/include/fs.h"






















# 1 "/sprite/lib/include/spriteTime.h"












































# 142 "/sprite/lib/include/spriteTime.h"

# 23 "/sprite/lib/include/fs.h"

# 1 "/sprite/lib/include/proc.h"













































































































































































































































































# 586 "/sprite/lib/include/proc.h"

# 24 "/sprite/lib/include/fs.h"

























































































typedef struct Fs_Attributes {
    int	serverID;		
    int domain;			
    int fileNumber;		
    int type;			
    int size;			
    int numLinks;		
    unsigned int permissions;	
    int uid;			
    int gid;			
    int devServerID;		
    int devType;		
    int devUnit;		
    Time createTime;		
    Time accessTime;		
    Time descModifyTime;	
    Time dataModifyTime;	
    int  blocks;		
    int  blockSize;		
    int	version;		
    int userType;		
    int pad[4];			
} Fs_Attributes;



































































































typedef struct Fs_FileID {
    int		type;		


    int		serverID;	


    int		major;		
    int		minor;		
} Fs_FileID;			







typedef struct Fs_UserIDs {
    int user;			
    int numGroupIDs;		
    int group[8];	
} Fs_UserIDs;			






























































typedef struct Ioc_RepositionArgs {
    int base;	
    int offset;	
} Ioc_RepositionArgs;










































typedef struct Ioc_LockArgs {
    int		flags;		





    




    int		hostID;		
    Proc_PID	pid;		
    int		token;		
} Ioc_LockArgs;









typedef struct Ioc_Owner {
    Proc_PID	id;		
    int		procOrFamily;	
} Ioc_Owner;







typedef struct Ioc_MapArgs {
    int		numBytes;
    Address	address;
} Ioc_MapArgs;




typedef struct Ioc_PrefixArgs {
    char	prefix[1024];  
} Ioc_PrefixArgs;








typedef struct Ioc_WriteBackArgs {
    int		firstByte;	
    int		lastByte;	
    Boolean	shouldBlock;	
} Ioc_WriteBackArgs;





typedef struct Ioc_MmapInfoArgs {
    int		isMapped;	
    int		clientID;	
} Ioc_MmapInfoArgs;





























































typedef struct Fs_TwoPaths {
    int		pathLen1;	
    int		pathLen2;	
    char 	*path1;		
    char 	*path2;		
} Fs_TwoPaths;




typedef struct {
    int	maxKbytes;		

    int	freeKbytes;		



    int	maxFileDesc;		

    int	freeFileDesc;		
    int blockSize;		
    int optSize;		
} Fs_DomainInfo;








typedef struct Fs_Prefix {
    int serverID;		

    int domain;			
    int fileNumber;		
    int version;		
    int flags;			
    char prefix[64];
    Fs_DomainInfo domainInfo;	
} Fs_Prefix;













typedef struct {
    int		bufSize;	
    Address	buffer;		


} Fs_IOVector;







typedef struct Fs_Device {
    int		serverID;	

    int		type;		

    int		unit;		

    ClientData	data;		


} Fs_Device;




typedef ClientData Fs_TimeoutHandler;

extern void		Fs_Dispatch();
extern void		Fs_EventHandlerCreate();
extern void 		Fs_EventHandlerDestroy();
extern ClientData 	Fs_EventHandlerData();
extern ClientData 	Fs_EventHandlerChangeData();
extern Fs_TimeoutHandler Fs_TimeoutHandlerCreate();
extern void 		Fs_TimeoutHandlerDestroy();

extern Boolean		Fs_IsATerm();
extern char		*Fs_GetTempName();
extern int		Fs_GetTempFile();

# 594 "/sprite/lib/include/fs.h"



# 43 "/sprite/lib/include/kernel/fs.h"

# 1 "/sprite/lib/include/fmt.h"
























# 51 "/sprite/lib/include/fmt.h"


# 44 "/sprite/lib/include/kernel/fs.h"

















typedef struct Fs_ProcessState {
    struct Fs_Stream	*cwdPtr;	
    unsigned int   	filePermissions;




    int		   	numStreams;	
    struct Fs_Stream   **streamList;	


    char		*streamFlags;	


    int			numGroupIDs;	
    int			*groupIDs;	

} Fs_ProcessState;




















typedef struct Fs_HandleHeader {
    Fs_FileID		fileID;		
    int			flags;		
    Sync_Condition	unlocked;	
    int			refCount;	
    char		*name;		
    List_Links		lruLinks;	

    int			lockProcID;	

} Fs_HandleHeader;















typedef struct Fs_NameInfo {
    Fs_FileID		fileID;		
    Fs_FileID		rootID;		


    int			domainType;	
    struct Fsprefix	*prefixPtr;	


} Fs_NameInfo;

























typedef struct Fs_Stream {
    Fs_HandleHeader	hdr;		


    int			offset;		
    int			flags;		
    Fs_HandleHeader	*ioHandlePtr;	


    Fs_NameInfo	 	*nameInfoPtr;	
    List_Links		clientList;	

} Fs_Stream;








































































































typedef struct Fs_IOParam {
    Address	buffer;			
    int		length;			
    int		offset;			
    int		flags;			
    Proc_PID	procID;			
    Proc_PID	familyID;		
    int		uid;			
    int		reserved;		
} Fs_IOParam;



























typedef struct Fs_IOReply {
    int		length;			
    int		flags;			
    int		signal;			
    int		code;			
} Fs_IOReply;
















typedef struct Fs_IOCParam {
    int		command;	
    Address	inBuffer;	
    int		inBufSize;	
    Address	outBuffer;	
    int		outBufSize;	
    Fmt_Format	format;		

    Proc_PID	procID;		
    Proc_PID	familyID;	
    int		uid;		
    int		flags;		


} Fs_IOCParam;











typedef enum {
    FS_CODE_PAGE,
    FS_HEAP_PAGE,
    FS_SWAP_PAGE,
    FS_SHARED_PAGE
} Fs_PageType;







typedef Address Fs_NotifyToken;
























extern  Boolean fsutil_Initialized;	




extern int fsMaxRpcDataSize;
extern int fsMaxRpcParamSize;




extern	int	fsutil_TmpDirNum;




extern	Boolean	fsutil_DelayTmpFiles;
extern	Boolean	fsutil_WriteThrough;
extern	Boolean	fsutil_WriteBackASAP;
extern	Boolean	fsutil_WriteBackOnClose;
extern	Boolean	fsutil_WBOnLastDirtyBlock;






extern	void	Fs_Init();
extern	void	Fs_ProcInit();
extern	void	Fs_InheritState();
extern	void	Fs_CloseState();





extern	ReturnStatus	Fs_AttachDiskStub();
extern	ReturnStatus	Fs_ChangeDirStub();
extern	ReturnStatus	Fs_CommandStub();
extern	ReturnStatus	Fs_CreatePipeStub();
extern	ReturnStatus	Fs_GetAttributesIDStub();
extern	ReturnStatus	Fs_GetAttributesStub();
extern	ReturnStatus	Fs_GetNewIDStub();
extern	ReturnStatus	Fs_HardLinkStub();
extern	ReturnStatus	Fs_IOControlStub();
extern	ReturnStatus	Fs_LockStub();
extern	ReturnStatus	Fs_MakeDeviceStub();
extern	ReturnStatus	Fs_MakeDirStub();
extern	ReturnStatus	Fs_OpenStub();
extern	ReturnStatus	Fs_ReadLinkStub();
extern	ReturnStatus	Fs_ReadStub();
extern	ReturnStatus	Fs_ReadVectorStub();
extern	ReturnStatus	Fs_RemoveDirStub();
extern	ReturnStatus	Fs_RemoveStub();
extern	ReturnStatus	Fs_RenameStub();
extern	ReturnStatus	Fs_SelectStub();
extern	ReturnStatus	Fs_SetAttributesIDStub();
extern	ReturnStatus	Fs_SetAttributesStub();
extern	ReturnStatus	Fs_SetAttrIDStub();
extern	ReturnStatus	Fs_SetAttrStub();
extern	ReturnStatus	Fs_SetDefPermStub();
extern	ReturnStatus	Fs_SymLinkStub();
extern	ReturnStatus	Fs_TruncateIDStub();
extern	ReturnStatus	Fs_TruncateStub();
extern	ReturnStatus	Fs_WriteStub();
extern	ReturnStatus	Fs_WriteVectorStub();
extern	ReturnStatus	Fs_FileWriteBackStub();




extern	ReturnStatus	Fs_UserClose();
extern	ReturnStatus	Fs_UserRead();
extern	ReturnStatus	Fs_UserReadVector();
extern	ReturnStatus	Fs_UserWrite();
extern	ReturnStatus	Fs_UserWriteVector();




extern	ReturnStatus	Fs_ChangeDir();
extern	ReturnStatus	Fs_Close();
extern	ReturnStatus	Fs_Command();
extern	ReturnStatus	Fs_CheckAccess();
extern	ReturnStatus	Fs_GetAttributes();
extern	ReturnStatus	Fs_GetAttributesID();
extern	ReturnStatus	Fs_GetNewID();
extern	ReturnStatus	Fs_HardLink();
extern	ReturnStatus	Fs_IOControl();
extern	ReturnStatus	Fs_MakeDevice();
extern	ReturnStatus	Fs_MakeDir();
extern	ReturnStatus	Fs_Open();
extern	ReturnStatus	Fs_Read();
extern	ReturnStatus	Fs_Remove();
extern	ReturnStatus	Fs_RemoveDir();
extern	ReturnStatus	Fs_Rename();
extern	ReturnStatus	Fs_SetAttributes();
extern	ReturnStatus	Fs_SetAttributesID();
extern	ReturnStatus	Fs_SetDefPerm();
extern	ReturnStatus	Fs_SymLink();
extern	ReturnStatus	Fs_Trunc();
extern	ReturnStatus	Fs_TruncStream();
extern	ReturnStatus	Fs_Write();




extern	int		Fs_Cat();
extern	int		Fs_Copy();
extern  void		Fs_CheckSetID();
extern  void		Fs_CloseOnExec();






extern	int		Fs_GetEncapSize();
extern	ReturnStatus	Fs_InitiateMigration();
extern  ReturnStatus    Fs_EncapFileState();
extern  ReturnStatus    Fs_DeencapFileState();





extern	ReturnStatus	Fs_PageRead();
extern	ReturnStatus	Fs_PageWrite();
extern	ReturnStatus	Fs_PageCopy();
extern	ReturnStatus	Fs_FileBeingMapped();




extern ReturnStatus	Fs_GetStreamID();
extern void		Fs_ClearStreamID();
extern ReturnStatus	Fs_GetStreamPtr();

extern  void		Fs_Bin();
extern ReturnStatus	Fs_GetAttrStream();
extern  void 		Fs_InstallDomainLookupOps();
extern  void		Fs_CheckSetID();

extern  ClientData      Fs_GetFileHandle();
extern struct Vm_Segment **Fs_GetSegPtr();


# 30 "./nfs.h"

# 1 "/sprite/lib/include/pfs.h"






















# 1 "/sprite/lib/include/fs.h"

















































































































































































































































































# 596 "/sprite/lib/include/fs.h"

# 23 "/sprite/lib/include/pfs.h"

# 1 "/sprite/lib/include/pdev.h"






















# 1 "/sprite/lib/include/fs.h"

















































































































































































































































































# 596 "/sprite/lib/include/fs.h"

# 23 "/sprite/lib/include/pdev.h"

# 1 "/sprite/lib/include/dev/pdev.h"
































































# 1 "/sprite/lib/include/proc.h"













































































































































































































































































# 586 "/sprite/lib/include/proc.h"

# 65 "/sprite/lib/include/dev/pdev.h"
# 67 "/sprite/lib/include/dev/pdev.h"


# 1 "/sprite/lib/include/kernel/fs.h"
























































































































































































































































# 546 "/sprite/lib/include/kernel/fs.h"

# 69 "/sprite/lib/include/dev/pdev.h"


















typedef int Pdev_Op;

















typedef struct Pdev_Notify {
    unsigned int	magic;			
    int			newStreamID;
    int			reserved;		
} Pdev_Notify;









typedef struct Pdev_OpenParam {
    int flags;			
    Proc_PID pid;		
    int hostID;			
    int uid;			
    int gid;			
    int byteOrder;		
    int reserved;		
} Pdev_OpenParam;





typedef Fs_IOParam Pdev_RWParam;





typedef Fs_IOCParam Pdev_IOCParam;







typedef struct {
    int		flags;		
    int		uid;		
    int		gid;		
} Pdev_SetAttrParam;









typedef struct {
    unsigned int magic;		
    Pdev_Op	operation;	
    int		messageSize;	

    int		requestSize;	
    int		replySize;	
    int		dataOffset;	
} Pdev_RequestHdr;

typedef struct {
    Pdev_RequestHdr	hdr;	
    union {			
	Pdev_OpenParam		open;
	Pdev_RWParam		read;
	Pdev_RWParam		write;
	Pdev_IOCParam		ioctl;
	Pdev_SetAttrParam	setAttr;
    } param;
} Pdev_Request;











typedef struct Pdev_Reply {
    unsigned int magic;		
    ReturnStatus status;	
    int		selectBits;	
    int		replySize;	
    Address	replyBuf;	
    int		signal;		
    int		code;		
} Pdev_Reply;











typedef struct Pdev_ReplyData {
    unsigned int magic;		
    ReturnStatus status;	
    int		selectBits;	
    int		replySize;	
    Address	replyBuf;	
    int		signal;		
    int		code;		
    char	data[16];	
} Pdev_ReplyData;








































































typedef struct Pdev_SetBufArgs {
    Address	requestBufAddr;		
    int		requestBufSize;		
    Address	readBufAddr;		

    int		readBufSize;		
} Pdev_SetBufArgs;





typedef struct Pdev_BufPtrs {
    int magic;			
    Address	requestAddr;	



    int requestFirstByte;	

    int requestLastByte;	
    int readFirstByte;		

    int readLastByte;		
} Pdev_BufPtrs;






typedef struct Pdev_Signal {
    unsigned int signal;
    unsigned int code;
} Pdev_Signal;


# 24 "/sprite/lib/include/pdev.h"





extern int pdev_Trace;








typedef struct {
    int (*open)();		
    int (*read)();		
    int (*write)();		
    int (*ioctl)();		
    int (*close)();		
    




    int (*getAttr)();		
    int (*setAttr)();		
} Pdev_CallBacks;






typedef struct Pdev_Stream {
    unsigned int magic;		
    int streamID;		


    ClientData clientData;	
} Pdev_Stream;

typedef char *Pdev_Token;	









extern char pdev_ErrorMsg[];

extern	Pdev_Token		Pdev_Open();
extern	void			Pdev_Close();
extern	int 		      (*Pdev_SetHandler())();
extern	int			Pdev_EnumStreams();

extern	Pdev_Stream	       *PdevSetup();


# 24 "/sprite/lib/include/pfs.h"

# 1 "/sprite/lib/include/dev/pfs.h"






































# 1 "/sprite/lib/include/dev/pdev.h"













































































































































































# 330 "/sprite/lib/include/dev/pdev.h"

# 39 "/sprite/lib/include/dev/pfs.h"

# 42 "/sprite/lib/include/dev/pfs.h"


# 1 "/sprite/lib/include/kernel/fsNameOps.h"







































































extern	ReturnStatus (*fs_DomainLookup[		4][		11])();








typedef struct Fs_AttrOps {
    ReturnStatus	(*getAttr)();
    ReturnStatus	(*setAttr)();
} Fs_AttrOps;

extern Fs_AttrOps fs_AttrOpTable[];



























typedef struct Fs_OpenArgs {
    Fs_FileID	prefixID;	
    Fs_FileID	rootID;		

    int		useFlags;	
    int		permissions;	

    int		type;		
    int		clientID;	
    int		migClientID;	
    Fs_UserIDs	id;		
} Fs_OpenArgs;

typedef struct Fs_OpenResults {
    Fs_FileID	ioFileID;	



    Fs_FileID	streamID;	
    Fs_FileID	nameID;		
    int		dataSize;	
    ClientData	streamData;	
} Fs_OpenResults;





typedef struct Fs_LookupArgs {
    Fs_FileID prefixID;	
    Fs_FileID rootID;	
    int useFlags;	
    Fs_UserIDs id;	
    int clientID;	
    int migClientID;	
} Fs_LookupArgs;




typedef struct Fs_GetAttrResults {
    Fs_FileID		*fileIDPtr;	
    Fs_Attributes	*attrPtr;	
} Fs_GetAttrResults;




typedef	union	Fs_GetAttrResultsParam {
    int	prefixLength;
    struct	AttrResults {
	Fs_FileID	fileID;
	Fs_Attributes	attrs;
    } attrResults;
} Fs_GetAttrResultsParam;




typedef struct Fs_SetAttrArgs {
    Fs_OpenArgs		openArgs;
    Fs_Attributes	attr;
    int			flags;	
} Fs_SetAttrArgs;




typedef struct Fs_MakeDeviceArgs {
    Fs_OpenArgs open;
    Fs_Device device;
} Fs_MakeDeviceArgs;




typedef struct Fs_2PathParams {
    Fs_LookupArgs	lookup;
    Fs_FileID		prefixID2;
} Fs_2PathParams;

typedef struct Fs_2PathData {
    char		path1[1024];
    char		path2[1024];
} Fs_2PathData;

typedef struct Fs_2PathReply {
    int		prefixLength;	
    Boolean	name1ErrorP;	



} Fs_2PathReply;





typedef struct Fs_RedirectInfo {
    int	prefixLength;		



    char fileName[1024];	


} Fs_RedirectInfo;

typedef struct Fs_2PathRedirectInfo {
    int name1ErrorP;		


    int	prefixLength;		



    char fileName[1024];	


} Fs_2PathRedirectInfo;





extern void Fs_SetIDs();


# 44 "/sprite/lib/include/dev/pfs.h"








































typedef struct {
    Pdev_RequestHdr	hdr;	
    union {			
	Fs_OpenArgs		open;
	Fs_OpenArgs		getAttr;
	Fs_OpenArgs		setAttr;
	Fs_MakeDeviceArgs	makeDevice;
	Fs_OpenArgs		makeDir;
	Fs_LookupArgs		remove;
	Fs_LookupArgs		removeDir;
	Fs_2PathParams		rename;
	Fs_2PathParams		hardLink;
	Fs_OpenArgs		symLink;
	Fs_FileID		domainInfo;
    } param;
} Pfs_Request;









typedef struct {
    Fs_Attributes	attr;		
    int			flags;		
    int			nameLength;	
    char		name[4];	
} Pfs_SetAttrData;






































# 25 "/sprite/lib/include/pfs.h"





extern int pfs_Trace;








typedef struct {
    int (*open)();		
    int (*getAttr)();		
    int (*setAttr)();		
    int (*makeDevice)();	
    int (*makeDir)();		
    int (*remove)();		
    int (*removeDir)();		
    int (*rename)();		
    int (*hardLink)();		
    int (*symLink)();		
    int (*domainInfo)();	
} Pfs_CallBacks;






typedef char *Pfs_Token;



extern char pfs_ErrorMsg[];

extern	Pfs_Token	        Pfs_Open();
extern	int		       (*Pfs_SetHandler())();
extern	Pdev_Stream	       *Pfs_OpenConnection();
extern	int			Pfs_PassFile();
extern	void			Pfs_Close();

# 31 "./nfs.h"

# 1 "/sprite/lib/include/pdev.h"




































# 85 "/sprite/lib/include/pdev.h"

# 32 "./nfs.h"

extern char myhostname[];






typedef struct {
    char	*host;			
    char	*nfsName;		
    char	*prefix;		
    Pfs_Token	pfsToken;		
    CLIENT	*mountClnt;		
    nfs_fh	*mountHandle;		
    CLIENT	*nfsClnt;		
} NfsState;




extern struct timeval nfsTimeout;




typedef struct {
    nfs_fh *handlePtr;		
    AUTH *authPtr;		
    int openFlags;		

} NfsOpenFile;

extern NfsOpenFile **nfsFileTable;
extern NfsOpenFile **nextFreeSlot;
extern int nfsFileTableSize;




extern int nfsToSpriteFileType[];
extern int spriteToNfsModeType[];



















extern int nfsStatusMap[];




extern CLIENT *Nfs_MountInitClient();
extern void Nfs_MountTest();
extern void Nfs_MountDump();
extern void Nfs_Unmount();

extern CLIENT *Nfs_InitClient();
extern Pfs_CallBacks nfsNameService;
extern Pdev_CallBacks nfsFileService;
extern int BadProc();

extern int NfsOpen();
extern int NfsClose();
extern int NfsRead();
extern int NfsWrite();
extern int NfsIoctl();
extern int NfsGetAttrStream();
extern int NfsSetAttrStream();
extern int NfsGetAttrPath();
extern int NfsSetAttrPath();
extern int NfsMakeDevice();
extern int NfsMakeDir();
extern int NfsRemove();
extern int NfsRemoveDir();
extern int NfsRename();
extern int NfsHardLink();
extern int NfsSymLink();
extern int NfsDomainInfo();

extern void Nfs_UnmountAll();
extern void Nfs_Exports();
extern nfs_fh *Nfs_Mount();




extern void NfsToSpriteAttr();
extern void SpriteToNfsAttr();
extern void NfsToSpriteDirectory();
extern void NfsFindCookie();

# 22 "nfsAttr.c"

# 1 "/sprite/lib/include/sys/stat.h"











struct	stat
{
	dev_t	st_dev;
	ino_t	st_ino;
	unsigned short st_mode;
	short	st_nlink;
	uid_t	st_uid;
	gid_t	st_gid;
	dev_t	st_rdev;
	off_t	st_size;
	time_t	st_atime;
	int	st_spare1;
	time_t	st_mtime;
	int	st_spare2;
	time_t	st_ctime;
	int	st_spare3;
	long	st_blksize;
	long	st_blocks;
	long	st_serverID;
	long	st_version;
	long	st_userType;
	long	st_devServerID;
};








































# 23 "nfsAttr.c"

# 1 "/sprite/lib/include/kernel/fslcl.h"
















# 1 "/sprite/lib/include/kernel/fscache.h"





















# 1 "/sprite/lib/include/kernel/sync.h"























































































































































































































































































































































































# 856 "/sprite/lib/include/kernel/sync.h"




















# 882 "/sprite/lib/include/kernel/sync.h"
































# 918 "/sprite/lib/include/kernel/sync.h"






















# 944 "/sprite/lib/include/kernel/sync.h"


















# 966 "/sprite/lib/include/kernel/sync.h"









# 22 "/sprite/lib/include/kernel/fscache.h"

# 1 "/sprite/lib/include/list.h"































































































































































































# 270 "/sprite/lib/include/list.h"

# 23 "/sprite/lib/include/kernel/fscache.h"

# 1 "/sprite/lib/include/kernel/fs.h"
























































































































































































































































# 546 "/sprite/lib/include/kernel/fs.h"

# 24 "/sprite/lib/include/kernel/fscache.h"

	






typedef struct Fscache_Attributes {
    int		firstByte;	
    int		lastByte;	
    int		accessTime;	
    int		modifyTime;	
    int		createTime;	


    int		userType;	

    


    int		permissions;	
    int		uid;		
    int		gid;		
} Fscache_Attributes;




typedef struct Fscache_IOProcs {
    

























    ReturnStatus (*allocate)();
    ReturnStatus (*blockRead)();
    ReturnStatus (*blockWrite)();
    ReturnStatus (*blockCopy)();
} Fscache_IOProcs;







typedef struct Fscache_FileInfo {
    List_Links	   links;	   

    List_Links	   dirtyList;	   


    List_Links	   blockList;      
    List_Links	   indList;	   
    Sync_Lock	   lock;	   
    int		   flags;	   

    int		   version;	   
    struct Fs_HandleHeader *hdrPtr; 
    int		   blocksInCache;  

    int		   blocksWritten;  


    int		   numDirtyBlocks; 
    Sync_Condition noDirtyBlocks;  
    int		   lastTimeTried;  

    Fscache_Attributes attr;	   
    Fscache_IOProcs   *ioProcsPtr;  
} Fscache_FileInfo;
















































typedef struct Fscache_Block {
    List_Links	cacheLinks;	


    List_Links	dirtyLinks;	

    List_Links	fileLinks;	


    unsigned int timeDirtied;	

    unsigned int timeReferenced;

    Address	blockAddr;	

    Fscache_FileInfo *cacheInfoPtr;	
    int		fileNum;	
    int		blockNum;	
    int		diskBlock;	

    int		blockSize;	
    int		refCount;	

    Sync_Condition ioDone;	
    int		flags;		
} Fscache_Block;


















































































































typedef struct Fscache_ReadAheadInfo {
    Sync_Lock		lock;		
    int			count;		
    Boolean		blocked;	
    Sync_Condition	done;		

    Sync_Condition	okToRead;	

} Fscache_ReadAheadInfo;			



















# 338 "/sprite/lib/include/kernel/fscache.h"



extern int	fscache_MaxBlockCleaners;
extern Boolean	fscache_RATracing;
extern int	fscache_NumReadAheadBlocks;

extern List_Links *fscacheFullWaitList;






extern  void            Fscache_WriteBack();
extern  ReturnStatus    Fscache_FileWriteBack();
extern  void            Fscache_FileInvalidate();
extern  void            Fscache_Empty();
extern  void            Fscache_CheckFragmentation();

extern  ReturnStatus	Fscache_CheckVersion();
extern  ReturnStatus    Fscache_Consist();

extern	void		Fscache_SetMaxSize();
extern	void		Fscache_SetMinSize();
extern	void		Fscache_BlocksUnneeded();
extern	void		Fscache_DumpStats();
extern  void		Fscache_GetPageFromFS();

extern  void            Fscache_InfoInit();
extern  void            Fscache_InfoSyncLockCleanup();
extern  void            Fscache_FetchBlock();
extern  void            Fscache_UnlockBlock();
extern  void            Fscache_BlockTrunc();
extern  void            Fscache_IODone();
extern  void            Fscache_CleanBlocks();
extern  void            Fscache_Init();
extern  int             Fscache_PreventWriteBacks();
extern  void            Fscache_AllowWriteBacks();






extern void             Fscache_Trunc();
extern ReturnStatus     Fscache_Read();
extern ReturnStatus     Fscache_Write();
extern ReturnStatus     Fscache_BlockRead();

extern Boolean          Fscache_UpdateFile();
extern void             Fscache_UpdateAttrFromClient();
extern void             Fscache_UpdateAttrFromCache();
extern void             Fscache_UpdateCachedAttr();
extern void             Fscache_UpdateDirSize();
extern void             Fscache_GetCachedAttr();

extern Boolean		Fscache_AllBlocksInCache();
extern Boolean		Fscache_OkToScavenge();




extern void		Fscache_ReadAheadInit();
extern void		Fscache_ReadAheadSyncLockCleanup();
extern void		Fscache_WaitForReadAhead();
extern void		Fscache_AllowReadAhead();

extern void		FscacheReadAhead();



# 17 "/sprite/lib/include/kernel/fslcl.h"

# 1 "/sprite/lib/include/kernel/fsio.h"





















# 1 "/sprite/lib/include/kernel/fs.h"
























































































































































































































































# 546 "/sprite/lib/include/kernel/fs.h"

# 22 "/sprite/lib/include/kernel/fsio.h"








































































extern int fsio_LclToRmtType[];
extern int fsio_RmtToLclType[];




























typedef struct Fsio_OpenOps {
    int		type;			
    













    ReturnStatus (*srvOpen)();
} Fsio_OpenOps;

extern Fsio_OpenOps fsio_OpenOpTable[];









typedef struct Fsio_StreamTypeOps {
    int		type;			
    














    ReturnStatus (*cltOpen)();
    


















    ReturnStatus (*read)();
    ReturnStatus (*write)();
    














    ReturnStatus (*ioControl)();
    













    ReturnStatus (*select)();
    















    ReturnStatus (*getIOAttr)();
    ReturnStatus (*setIOAttr)();
    














    Fs_HandleHeader *(*clientVerify)();
    





































    ReturnStatus (*release)();
    ReturnStatus (*migEnd)();
    ReturnStatus (*migrate)();
    














    ReturnStatus (*reopen)();
    

























    Boolean	 (*scavenge)();
    void	 (*clientKill)();
    ReturnStatus (*close)();
} Fsio_StreamTypeOps;

extern Fsio_StreamTypeOps fsio_StreamOpTable[];

typedef struct FsioStreamClient {
    List_Links links;		
    int		clientID;	
} FsioStreamClient;






typedef struct FsMigInfo {
    Fs_FileID	streamID;	
    Fs_FileID    ioFileID;     	
    Fs_FileID	nameID;		
    Fs_FileID	rootID;		
    int		srcClientID;	
    int         offset;     	
    int         flags;      	
} FsMigInfo;








typedef struct Fsutil_UseCounts {
    int		ref;		
    int		write;		
    int		exec;		
} Fsutil_UseCounts;





typedef struct Fsio_RecovTestInfo {
    int		(*refFunc)();		
    int		(*numBlocksFunc)();	
    int		(*numDirtyBlocksFunc)();
} Fsio_RecovTestInfo;

extern	Fsio_RecovTestInfo	fsio_StreamRecovTestFuncs[];




extern	int	Fsio_FileRecovTestUseCount();
extern	int	Fsio_FileRecovTestNumCacheBlocks();
extern	int	Fsio_FileRecovTestNumDirtyCacheBlocks();
extern	int	Fsio_DeviceRecovTestUseCount();
extern	int	Fsio_PipeRecovTestUseCount();




extern void Fsio_InstallStreamOps();
extern void Fsio_InstallSrvOpenOp();

extern ReturnStatus Fsio_CreatePipe();

extern void Fsio_Bin();
extern void Fsio_InitializeOps();




extern Boolean		Fsio_StreamClientOpen();
extern Boolean		Fsio_StreamClientClose();
extern Boolean		Fsio_StreamClientFind();




extern Fs_Stream	*Fsio_StreamCreate();
extern Fs_Stream	*Fsio_StreamAddClient();
extern void		Fsio_StreamMigClient();
extern Fs_Stream	*Fsio_StreamClientVerify();
extern void		Fsio_StreamCreateID();
extern void		Fsio_StreamCopy();
extern void		Fsio_StreamDestroy();
extern Boolean		Fsio_StreamScavenge();
extern ReturnStatus	Fsio_StreamReopen();




extern void		Fsio_LockInit();
extern ReturnStatus	Fsio_IocLock();
extern ReturnStatus	Fsio_Lock();
extern ReturnStatus	Fsio_Unlock();
extern void		Fsio_LockClose();
extern void		Fsio_LockClientKill();




extern ReturnStatus	Fsio_FileTrunc();





extern void Fsio_DevNotifyException();
extern void Fsio_DevNotifyReader();
extern void Fsio_DevNotifyWriter();

extern ReturnStatus Fsio_VanillaDevReopen();




extern ReturnStatus	Fsio_EncapStream();
extern ReturnStatus	Fsio_DeencapStream();
extern ReturnStatus	Fsio_MigrateUseCounts();
extern void		Fsio_MigrateClient();




extern void Fsio_NullClientKill();
extern ReturnStatus Fsio_NoProc();
extern ReturnStatus Fsio_NullProc();
extern Fs_HandleHeader *Fsio_NoHandle();



# 18 "/sprite/lib/include/kernel/fslcl.h"





typedef struct Fslcl_DirEntry {
    int fileNumber;		
    short recordLength;		
    short nameLength;		
    char fileName[255+1];	
} Fslcl_DirEntry;
























extern void		Fslcl_DomainInit();
extern ReturnStatus	Fslcl_DeleteFileDesc();
extern void 		Fslcl_NameInitializeOps();
extern void 		Fslcl_NameHashInit();


# 24 "nfsAttr.c"

typedef struct SavedCookie {
    int offset;
    nfscookie cookie;
    struct SavedCookie *next;
} SavedCookie;

void NfsToSpriteAttr();
void SpriteToNFsAttr();
















void
NfsToSpriteDirectory(dirListPtr, offset, countPtr, buffer, fileIDPtr)
    dirlist *dirListPtr;
    int offset;
    int *countPtr;
    char *buffer;
    Fs_FileID *fileIDPtr;
{
    register int count = 0;
    register entry *entryPtr;
    register Fslcl_DirEntry *spriteEntryPtr;
    register int nameLength;

    if (dirListPtr->eof) {
	*countPtr = 0;
	fileIDPtr->major = 0;
	return;
    }
    entryPtr = dirListPtr->entries;
    spriteEntryPtr = (Fslcl_DirEntry *)buffer;
    while (entryPtr != (entry *)0) {
	nameLength = strlen(entryPtr->name);
	spriteEntryPtr->fileNumber = entryPtr->fileid;
	spriteEntryPtr->nameLength = nameLength;
	strcpy(spriteEntryPtr->fileName, entryPtr->name);
	if (entryPtr->nextentry != (entry *)0) {
	    spriteEntryPtr->recordLength =      ((sizeof(int) + 2 * sizeof(short)) +     ((nameLength / 4) + 1) * 4);
	    count += spriteEntryPtr->recordLength;
	    spriteEntryPtr = (Fslcl_DirEntry *)((int)spriteEntryPtr +
					    spriteEntryPtr->recordLength);
	} else {
	    





	    register int extraRoom;
	    SavedCookie *savedCookie;

	    extraRoom = 512 - (count % 512);
	    spriteEntryPtr->recordLength = extraRoom;
	    count += extraRoom;
	    


	    savedCookie = (SavedCookie *)malloc(sizeof(SavedCookie));
	    savedCookie->offset = offset + count;
	    bcopy((char *)&entryPtr->cookie, (char *)&savedCookie->cookie,
		    sizeof(nfscookie));
	    savedCookie->next = (SavedCookie *)fileIDPtr->major;
	    fileIDPtr->major = (int)savedCookie;
	}
	entryPtr = entryPtr->nextentry;
    }
    *countPtr = count;
}


















void
NfsFindCookie(fileIDPtr, offset, cookiePtr)
    Fs_FileID *fileIDPtr;
    int offset;
    nfscookie *cookiePtr;
{
    register SavedCookie *savedCookiePtr;
    int badCookie = -1;

    savedCookiePtr = (SavedCookie *)fileIDPtr->major;
    while (savedCookiePtr != (SavedCookie *)0) {
	if (savedCookiePtr->offset == offset) {
	    bcopy((char *)&savedCookiePtr->cookie, (char *)cookiePtr,
		sizeof(nfscookie));
	    return;
	}
	savedCookiePtr = savedCookiePtr->next;
    }
    printf("NfsFindCookie: no directory cookie for offset %d\n");
    bcopy((char *)&badCookie, (char *)cookiePtr, sizeof(nfscookie));
}


















ReturnStatus
NfsGetAttrStream(streamPtr, spriteAttrPtr, selectBitsPtr)
    Pdev_Stream *streamPtr;
    Fs_Attributes *spriteAttrPtr;
    int *selectBitsPtr;
{
    register Fs_FileID *fileIDPtr = (Fs_FileID *)streamPtr->clientData;
    register nfs_fh *handlePtr;
    register int status;
    NfsState *nfsPtr;
    attrstat attrStat;

    if (fileIDPtr->minor >= 0 && fileIDPtr->minor < nfsFileTableSize) {
	handlePtr = nfsFileTable[fileIDPtr->minor]->handlePtr;
	nfsPtr = (NfsState *)fileIDPtr->serverID;
	nfsPtr->nfsClnt->cl_auth = nfsFileTable[fileIDPtr->minor]->authPtr;

	if (		((*(nfsPtr->nfsClnt)->cl_ops->cl_call)(nfsPtr->nfsClnt,  ((u_long)1),  xdr_nfs_fh,  handlePtr, 
		    xdr_attrstat,  &attrStat,  nfsTimeout)) != RPC_SUCCESS) {
	    clnt_perror(nfsPtr->nfsClnt, "NFSPROC_GETATTR");
	    status = 		0x00000001;
	} else {
	    status = attrStat.status;
	    if (status == NFS_OK) {
		NfsToSpriteAttr(&attrStat.attrstat_u.attributes, spriteAttrPtr);
	    } else {
		status =      ((status >= 0 && status < sys_nerr) ? nfsStatusMap[status] : status);
	    }
	}
    } else {
	printf("NfsGetAttrStream: bad fileID <%d,%d,%d,%d>\n", fileIDPtr->type,
		fileIDPtr->serverID, fileIDPtr->major, fileIDPtr->minor);
	status = 		0x00000001;
    }
    *selectBitsPtr = 	  		0x001 |   		0x002;
    return(status);
}















void
NfsToSpriteAttr(nfsAttrPtr, spriteAttrPtr)
    register fattr *nfsAttrPtr;
    register Fs_Attributes *spriteAttrPtr;
{
    spriteAttrPtr->serverID		= -1;
    spriteAttrPtr->domain		= nfsAttrPtr->fsid;
    spriteAttrPtr->fileNumber		= nfsAttrPtr->fileid;
    spriteAttrPtr->type			= nfsToSpriteFileType[(int) nfsAttrPtr->type];
    spriteAttrPtr->size			= nfsAttrPtr->size;
    spriteAttrPtr->numLinks		= nfsAttrPtr->nlink;
    spriteAttrPtr->permissions		= nfsAttrPtr->mode & 07777;
    spriteAttrPtr->uid			= nfsAttrPtr->uid;
    spriteAttrPtr->gid			= nfsAttrPtr->gid;
    spriteAttrPtr->devServerID		= -1;
    if (nfsAttrPtr->type == NFBLK || nfsAttrPtr->type == NFCHR) {
	spriteAttrPtr->devType		= major(nfsAttrPtr->rdev);
	spriteAttrPtr->devUnit		= minor(nfsAttrPtr->rdev);
    } else {
	spriteAttrPtr->devType		= -1;
	spriteAttrPtr->devUnit		= -1;
    }
    spriteAttrPtr->createTime.seconds		= nfsAttrPtr->mtime.seconds;
    spriteAttrPtr->createTime.microseconds	= nfsAttrPtr->mtime.useconds;
    spriteAttrPtr->accessTime.seconds		= nfsAttrPtr->atime.seconds;
    spriteAttrPtr->accessTime.microseconds	= nfsAttrPtr->atime.useconds;
    spriteAttrPtr->descModifyTime.seconds	= nfsAttrPtr->ctime.seconds;
    spriteAttrPtr->descModifyTime.microseconds	= nfsAttrPtr->ctime.useconds;
    spriteAttrPtr->dataModifyTime.seconds	= nfsAttrPtr->mtime.seconds;
    spriteAttrPtr->dataModifyTime.microseconds	= nfsAttrPtr->mtime.useconds;
    




    spriteAttrPtr->blocks		= nfsAttrPtr->blocks / 2;
    spriteAttrPtr->blockSize		= nfsAttrPtr->blocksize;
    spriteAttrPtr->version		= nfsAttrPtr->mtime.seconds;
    spriteAttrPtr->userType		= 0;
}

















ReturnStatus
NfsSetAttrStream(streamPtr, flags, uid, gid, spriteAttrPtr, selectBitsPtr)
    Pdev_Stream *streamPtr;
    int flags;			
    int uid;			
    int gid;			
    Fs_Attributes *spriteAttrPtr;
    int *selectBitsPtr;
{
    register Fs_FileID *fileIDPtr = (Fs_FileID *)streamPtr->clientData;
    register nfs_fh *handlePtr;
    register int status;
    NfsState *nfsPtr;
    sattrargs sattrArgs;
    attrstat attrStat;

    if (fileIDPtr->minor >= 0 && fileIDPtr->minor < nfsFileTableSize) {
	handlePtr = nfsFileTable[fileIDPtr->minor]->handlePtr;
	nfsPtr = (NfsState *)fileIDPtr->serverID;
	nfsPtr->nfsClnt->cl_auth = nfsFileTable[fileIDPtr->minor]->authPtr;

	bcopy((char *)handlePtr, (char *)&sattrArgs.file, sizeof(nfs_fh));
	SpriteToNfsAttr(flags, spriteAttrPtr, &sattrArgs.attributes);
	if (		((*(nfsPtr->nfsClnt)->cl_ops->cl_call)(nfsPtr->nfsClnt,  ((u_long)2),  xdr_sattrargs, 
		    &sattrArgs,  xdr_attrstat,  &attrStat,  nfsTimeout))
			!= RPC_SUCCESS) {
	    clnt_perror(nfsPtr->nfsClnt, "NFSPROC_SETATTR");
	    status = 		0x00000001;
	} else {
	    status =      ((((int)attrStat.status) >= 0 && ((int)attrStat.status) < sys_nerr) ? nfsStatusMap[((int)attrStat.status)] : ((int)attrStat.status));
	}
    } else {
	printf("NfsSetAttrStream: bad fileID <%d,%d,%d,%d>\n", fileIDPtr->type,
		fileIDPtr->serverID, fileIDPtr->major, fileIDPtr->minor);
	status = 		0x00000001;
    }
    *selectBitsPtr = 	  		0x001 |   		0x002;
    return(status);
}
















void
SpriteToNfsAttr(flags, spriteAttrPtr, nfsAttrPtr)
    register int flags;			
    register Fs_Attributes *spriteAttrPtr;
    register sattr *nfsAttrPtr;
{
    if (flags & 	0x02) {
	nfsAttrPtr->mode = spriteAttrPtr->permissions & 07777;
    } else {
	nfsAttrPtr->mode = -1;
    }
    if (flags & 	0x04) {
	nfsAttrPtr->uid		= spriteAttrPtr->uid;
	nfsAttrPtr->gid		= spriteAttrPtr->gid;
    } else {
	nfsAttrPtr->uid		= -1;
	nfsAttrPtr->gid		= -1;
    }
    nfsAttrPtr->size		= -1;		
    if (flags & 	0x01) {
	nfsAttrPtr->atime.seconds  = spriteAttrPtr->accessTime.seconds;
	nfsAttrPtr->atime.useconds = spriteAttrPtr->accessTime.microseconds;
	nfsAttrPtr->mtime.seconds  = spriteAttrPtr->dataModifyTime.seconds;
	nfsAttrPtr->mtime.useconds = spriteAttrPtr->dataModifyTime.microseconds;
    } else {
	nfsAttrPtr->atime.seconds  = -1;
	nfsAttrPtr->atime.useconds = -1;
	nfsAttrPtr->mtime.seconds  = -1;
	nfsAttrPtr->mtime.useconds = -1;
    }
}
















void
NfsCacheAttributes(fileIDPtr, nfsAttrPtr)
    register Fs_FileID *fileIDPtr;
    register fattr *nfsAttrPtr;
{
    return;
}

