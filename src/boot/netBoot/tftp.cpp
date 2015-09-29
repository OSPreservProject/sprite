# 1 "tftp.c"

 




 


# 1 "boot.h"
 



















# 1 "sun3.md/sunromvec.h"

 

 




 








































struct sunromvec {
	char		*v_initsp;		 
	void		(*v_startmon)();	 
	int		*v_diagberr;		 

 
	struct bootparam	**v_bootparam;	 
 	unsigned	*v_memorysize;		 

 
	unsigned char	(*v_getchar)();		 
	void		(*v_putchar)();		 
	int		(*v_mayget)();		 
	int		(*v_mayput)();		 
	unsigned char	*v_echo;		 
	unsigned char	*v_insource;		 
	unsigned char	*v_outsink;		 

 
	int		(*v_getkey)();		 
	void		(*v_initgetkey)();	 
	unsigned int	*v_translation;		 
	unsigned char	*v_keybid;		 
	int		*v_screen_x;		 
	int		*v_screen_y;		 
	struct keybuf	*v_keybuf;		 

	char		*v_mon_id;		 

 
	void		(*v_fwritechar)();	 
	int		*v_fbaddr;		 
	char		**v_font;		 
	void		(*v_fwritestr)();	 

 
	void		(*v_boot_me)();		 

 
	unsigned char	*v_linebuf;		 
	unsigned char	**v_lineptr;		 
	int		*v_linesize;		 
	void		(*v_getline)();		 
	unsigned char	(*v_getone)();		 
	unsigned char	(*v_peekchar)();	 
	int		*v_fbthere;		 
	int		(*v_getnum)();		 

 
	int		(*v_printf)();		 
	void		(*v_printhex)();	 

	unsigned char	*v_leds;		 
	void		(*v_set_leds)();	 

 
	void		(*v_nmi)();		 
	void		(*v_abortent)();	 
	int		*v_nmiclock;		 

	int		*v_fbtype;		 

 
	unsigned	v_romvec_version;	  
	struct globram	*v_gp;			 
	struct zscc_device 	*v_keybzscc;	 

	int		*v_keyrinit;		 
	unsigned char	*v_keyrtick; 		 
	unsigned	*v_memoryavail;		 
	long		*v_resetaddr;		 
	long		*v_resetmap;		 
						 
	void		(*v_exit_to_mon)();	 

	unsigned char	**v_memorybitmap;	 
	void		(*v_setcxsegmap)();	 
	void		(**v_vector_cmd)();	 
	int		dummy1z;
	int		dummy2z;
	int		dummy3z;
	int		dummy4z;
};

 





 








 













 


struct bootparam {
	char		*bp_argv[8];	 
	char		bp_strings[100]; 
	char		bp_dev[2];	 
	int		bp_ctlr;	 
	int		bp_unit;	 
	int		bp_part;	 
	char		*bp_name;	 
	struct boottab	*bp_boottab;	 
};


 






struct boottab {
	char	b_dev[2];		 
	int	(*b_probe)();		 
	int	(*b_boot)();		 
	int	(*b_open)();		 
	int	(*b_close)();		 
	int	(*b_strategy)();	 
	char	*b_desc;		 
	struct devinfo *b_devinfo;	 
};

 
enum MAPTYPES {
	MAP_MAINMEM, MAP_OBIO, MAP_MBMEM, MAP_MBIO,
	MAP_VME16A16D, MAP_VME16A32D,
	MAP_VME24A16D, MAP_VME24A32D,
	MAP_VME32A16D, MAP_VME32A32D,
};

 





struct devinfo {
	unsigned	d_devbytes;	 
	unsigned	d_dmabytes;	 
	unsigned	d_localbytes;	 
	unsigned	d_stdcount;	 
	unsigned long	*d_stdaddrs;	 
	enum MAPTYPES 	d_devtype;	 
	unsigned	d_maxiobytes;	 
};

 




 




























 











 




























 





























# 21 "boot.h"









# 10 "tftp.c"


# 1 "sun3.md/saio.h"

 

 



 



# 1 "./types.h"

 

 



 


# 1 "./systypes.h"
 


 





 


 


 


typedef	unsigned char	u_char;
typedef	unsigned short	u_short;
typedef	unsigned int	u_int;
typedef	unsigned long	u_long;
typedef	unsigned short	ushort;		 













typedef	struct	_quad { long val[2]; } quad;
typedef	long	daddr_t;
typedef	char *	caddr_t;
typedef	u_long	ino_t;
typedef	long	swblk_t;
typedef	int	size_t;
typedef	int	time_t;
typedef	short	dev_t;
typedef	int	off_t;

typedef	struct	fd_set { int fds_bits[1]; } fd_set;


# 11 "./types.h"


















# 12 "sun3.md/saio.h"

# 1 "sun3.md/sunromvec.h"

 

 




 








































struct sunromvec {
	char		*v_initsp;		 
	void		(*v_startmon)();	 
	int		*v_diagberr;		 

 
	struct bootparam	**v_bootparam;	 
 	unsigned	*v_memorysize;		 

 
	unsigned char	(*v_getchar)();		 
	void		(*v_putchar)();		 
	int		(*v_mayget)();		 
	int		(*v_mayput)();		 
	unsigned char	*v_echo;		 
	unsigned char	*v_insource;		 
	unsigned char	*v_outsink;		 

 
	int		(*v_getkey)();		 
	void		(*v_initgetkey)();	 
	unsigned int	*v_translation;		 
	unsigned char	*v_keybid;		 
	int		*v_screen_x;		 
	int		*v_screen_y;		 
	struct keybuf	*v_keybuf;		 

	char		*v_mon_id;		 

 
	void		(*v_fwritechar)();	 
	int		*v_fbaddr;		 
	char		**v_font;		 
	void		(*v_fwritestr)();	 

 
	void		(*v_boot_me)();		 

 
	unsigned char	*v_linebuf;		 
	unsigned char	**v_lineptr;		 
	int		*v_linesize;		 
	void		(*v_getline)();		 
	unsigned char	(*v_getone)();		 
	unsigned char	(*v_peekchar)();	 
	int		*v_fbthere;		 
	int		(*v_getnum)();		 

 
	int		(*v_printf)();		 
	void		(*v_printhex)();	 

	unsigned char	*v_leds;		 
	void		(*v_set_leds)();	 

 
	void		(*v_nmi)();		 
	void		(*v_abortent)();	 
	int		*v_nmiclock;		 

	int		*v_fbtype;		 

 
	unsigned	v_romvec_version;	  
	struct globram	*v_gp;			 
	struct zscc_device 	*v_keybzscc;	 

	int		*v_keyrinit;		 
	unsigned char	*v_keyrtick; 		 
	unsigned	*v_memoryavail;		 
	long		*v_resetaddr;		 
	long		*v_resetmap;		 
						 
	void		(*v_exit_to_mon)();	 

	unsigned char	**v_memorybitmap;	 
	void		(*v_setcxsegmap)();	 
	void		(**v_vector_cmd)();	 
	int		dummy1z;
	int		dummy2z;
	int		dummy3z;
	int		dummy4z;
};

 





 








 













 


struct bootparam {
	char		*bp_argv[8];	 
	char		bp_strings[100]; 
	char		bp_dev[2];	 
	int		bp_ctlr;	 
	int		bp_unit;	 
	int		bp_part;	 
	char		*bp_name;	 
	struct boottab	*bp_boottab;	 
};


 






struct boottab {
	char	b_dev[2];		 
	int	(*b_probe)();		 
	int	(*b_boot)();		 
	int	(*b_open)();		 
	int	(*b_close)();		 
	int	(*b_strategy)();	 
	char	*b_desc;		 
	struct devinfo *b_devinfo;	 
};

 
enum MAPTYPES {
	MAP_MAINMEM, MAP_OBIO, MAP_MBMEM, MAP_MBIO,
	MAP_VME16A16D, MAP_VME16A32D,
	MAP_VME24A16D, MAP_VME24A32D,
	MAP_VME32A16D, MAP_VME32A32D,
};

 





struct devinfo {
	unsigned	d_devbytes;	 
	unsigned	d_dmabytes;	 
	unsigned	d_localbytes;	 
	unsigned	d_stdcount;	 
	unsigned long	*d_stdaddrs;	 
	enum MAPTYPES 	d_devtype;	 
	unsigned	d_maxiobytes;	 
};

 




 




























 











 




























 





























# 13 "sun3.md/saio.h"


 







struct saioreq {
	char	si_flgs;
	struct boottab *si_boottab;	 
	char	*si_devdata;		 
	int	si_ctlr;		 
	int	si_unit;		 
	daddr_t	si_boff;		 
	daddr_t	si_cyloff;
	off_t	si_offset;
	daddr_t	si_bn;			 
	char	*si_ma;			 
	int	si_cc;			 
	struct	saif *si_sif;		 
	char 	*si_devaddr;		 
	char	*si_dmaaddr;		 
};







 





 





 


struct saif {
	int	(*sif_xmit)();		 
	int	(*sif_poll)();		 
	int	(*sif_reset)();		 
};

 


enum RESOURCES { 
	RES_MAINMEM,		 
	RES_RAWVIRT,		 
	RES_DMAMEM,		 
	RES_DMAVIRT,		 
};


 











# 98 "sun3.md/saio.h"

 









# 12 "tftp.c"

# 1 "socket.h"
 


 



 








 







				 




 

















 



struct sockaddr {
	u_short	sa_family;		 
	char	sa_data[14];		 
};

 



struct sockproto {
	u_short	sp_family;		 
	u_short	sp_protocol;		 
};

 

















 




 




 


struct msghdr {
	caddr_t	msg_name;		 
	int	msg_namelen;		 
	struct	iovec *msg_iov;		 
	int	msg_iovlen;		 
	caddr_t	msg_accrights;		 
	int	msg_accrightslen;
};






# 13 "tftp.c"

# 1 "if.h"
 
 


 
























 






struct ifnet {
	char	*if_name;		 
	short	if_unit;		 
	short	if_mtu;			 
	int	if_net;			 
	short	if_flags;		 
	short	if_timer;		 
	int	if_host[2];		 
	struct	sockaddr if_addr;	 
	union {
		struct	sockaddr ifu_broadaddr;
		struct	sockaddr ifu_dstaddr;
	} if_ifu;


	struct	ifqueue {
		struct	mbuf *ifq_head;
		struct	mbuf *ifq_tail;
		int	ifq_len;
		int	ifq_maxlen;
		int	ifq_drops;
	} if_snd;			 
 
	int	(*if_init)();		 
	int	(*if_output)();		 
	int	(*if_ioctl)();		 
	int	(*if_reset)();		 
	int	(*if_watchdog)();	 
 
	int	if_ipackets;		 
	int	if_ierrors;		 
	int	if_opackets;		 
	int	if_oerrors;		 
	int	if_collisions;		 
 
	struct	ifnet *if_next;
	struct	ifnet *if_upper;	 
	struct	ifnet *if_lower;	 
	int	(*if_input)();		 
	int	(*if_ctlin)();		 
	int	(*if_ctlout)();		 



};










 








# 109 "if.h"








# 125 "if.h"




 





struct	ifreq {

	char	ifr_name[16 ];		 
	union {
		struct	sockaddr ifru_addr;
		struct	sockaddr ifru_dstaddr;
		char	ifru_oname[16 ];	 
		short	ifru_flags;
		char	ifru_data[1];		 
	} ifr_ifru;





};

 





struct	ifconf {
	int	ifc_len;		 
	union {
		caddr_t	ifcu_buf;
		struct	ifreq *ifcu_req;
	} ifc_ifcu;


};

 


struct arpreq {
	struct sockaddr	arp_pa;		 
	struct sockaddr	arp_ha;		 
	int	arp_flags;		 
};
 





# 14 "tftp.c"

# 1 "in.h"
 


 




 











 















 








 







 






 





 






 


struct in_addr {
	union {
		struct { u_char s_b1,s_b2,s_b3,s_b4; } S_un_b;
		struct { u_short s_w1,s_w2; } S_un_w;
		u_long S_addr;
	} S_un;






};

 




















 


struct sockaddr_in {
	short	sin_family;
	u_short	sin_port;
	struct	in_addr sin_addr;
	char	sin_zero[8];
};


 








# 15 "tftp.c"

# 1 "if_ether.h"
 


 


struct ether_addr {
	u_char	ether_addr_octet[6];
};

 


struct	ether_header {
	struct	ether_addr ether_dhost;
	struct	ether_addr ether_shost;
	u_short	ether_type;
};






 










 






struct	ether_arp {
	u_short	arp_hrd;	 

	u_short	arp_pro;	 
	u_char	arp_hln;	 
	u_char	arp_pln;	 
	u_short	arp_op;




	u_char	arp_xsha[6];	 
	u_char	arp_xspa[4];	 
	u_char	arp_xtha[6];	 
	u_char	arp_xtpa[4];	 
};





 




struct	arpcom {
	struct 	ifnet ac_if;	 
	struct ether_addr ac_enaddr;	 
	struct	arpcom *ac_ac;	 
};

 


struct	arptab {
	struct	in_addr at_iaddr;	 
	struct	ether_addr at_enaddr;	 
	struct	mbuf *at_hold;		 
	u_char	at_timer;		 
	u_char	at_flags;		 
};

# 16 "tftp.c"

# 1 "in_systm.h"
 


 





 







typedef u_short n_short;		 
typedef u_long	n_long;			 
typedef	u_long	n_time;			 


# 17 "tftp.c"

# 1 "ip.h"
 
















 





 






struct ip {

	u_char	ip_hl:4,		 
		ip_v:4;			 


	u_char	ip_v:4,			 
		ip_hl:4;		 

	u_char	ip_tos;			 
	short	ip_len;			 
	u_short	ip_id;			 
	short	ip_off;			 


	u_char	ip_ttl;			 
	u_char	ip_p;			 
	u_short	ip_sum;			 
	struct	in_addr ip_src,ip_dst;	 
};



 





















 







 


struct	ip_timestamp {
	u_char	ipt_code;		 
	u_char	ipt_len;		 
	u_char	ipt_ptr;		 

	u_char	ipt_flg:4,		 
		ipt_oflw:4;		 


	u_char	ipt_oflw:4,		 
		ipt_flg:4;		 

	union ipt_timestamp {
		n_long	ipt_time[1];
		struct	ipt_ta {
			struct in_addr ipt_addr;
			n_long ipt_time;
		} ipt_ta[1];
	} ipt_timestamp;
};

 




 








 









# 18 "tftp.c"

# 1 "udp.h"
 


 



struct udphdr {
	u_short	uh_sport;		 
	u_short	uh_dport;		 
	short	uh_ulen;		 
	u_short	uh_sum;			 
};
# 19 "tftp.c"

# 1 "sainet.h"

 

 



 


struct sainet {
	struct in_addr    sain_myaddr;	 
	struct ether_addr sain_myether;	 
	struct in_addr    sain_hisaddr;	 
	struct ether_addr sain_hisether; 
};
# 20 "tftp.c"

# 1 "sun3.md/sunromvec.h"

 

 




 








































struct sunromvec {
	char		*v_initsp;		 
	void		(*v_startmon)();	 
	int		*v_diagberr;		 

 
	struct bootparam	**v_bootparam;	 
 	unsigned	*v_memorysize;		 

 
	unsigned char	(*v_getchar)();		 
	void		(*v_putchar)();		 
	int		(*v_mayget)();		 
	int		(*v_mayput)();		 
	unsigned char	*v_echo;		 
	unsigned char	*v_insource;		 
	unsigned char	*v_outsink;		 

 
	int		(*v_getkey)();		 
	void		(*v_initgetkey)();	 
	unsigned int	*v_translation;		 
	unsigned char	*v_keybid;		 
	int		*v_screen_x;		 
	int		*v_screen_y;		 
	struct keybuf	*v_keybuf;		 

	char		*v_mon_id;		 

 
	void		(*v_fwritechar)();	 
	int		*v_fbaddr;		 
	char		**v_font;		 
	void		(*v_fwritestr)();	 

 
	void		(*v_boot_me)();		 

 
	unsigned char	*v_linebuf;		 
	unsigned char	**v_lineptr;		 
	int		*v_linesize;		 
	void		(*v_getline)();		 
	unsigned char	(*v_getone)();		 
	unsigned char	(*v_peekchar)();	 
	int		*v_fbthere;		 
	int		(*v_getnum)();		 

 
	int		(*v_printf)();		 
	void		(*v_printhex)();	 

	unsigned char	*v_leds;		 
	void		(*v_set_leds)();	 

 
	void		(*v_nmi)();		 
	void		(*v_abortent)();	 
	int		*v_nmiclock;		 

	int		*v_fbtype;		 

 
	unsigned	v_romvec_version;	  
	struct globram	*v_gp;			 
	struct zscc_device 	*v_keybzscc;	 

	int		*v_keyrinit;		 
	unsigned char	*v_keyrtick; 		 
	unsigned	*v_memoryavail;		 
	long		*v_resetaddr;		 
	long		*v_resetmap;		 
						 
	void		(*v_exit_to_mon)();	 

	unsigned char	**v_memorybitmap;	 
	void		(*v_setcxsegmap)();	 
	void		(**v_vector_cmd)();	 
	int		dummy1z;
	int		dummy2z;
	int		dummy3z;
	int		dummy4z;
};

 





 








 













 


struct bootparam {
	char		*bp_argv[8];	 
	char		bp_strings[100]; 
	char		bp_dev[2];	 
	int		bp_ctlr;	 
	int		bp_unit;	 
	int		bp_part;	 
	char		*bp_name;	 
	struct boottab	*bp_boottab;	 
};


 






struct boottab {
	char	b_dev[2];		 
	int	(*b_probe)();		 
	int	(*b_boot)();		 
	int	(*b_open)();		 
	int	(*b_close)();		 
	int	(*b_strategy)();	 
	char	*b_desc;		 
	struct devinfo *b_devinfo;	 
};

 
enum MAPTYPES {
	MAP_MAINMEM, MAP_OBIO, MAP_MBMEM, MAP_MBIO,
	MAP_VME16A16D, MAP_VME16A32D,
	MAP_VME24A16D, MAP_VME24A32D,
	MAP_VME32A16D, MAP_VME32A32D,
};

 





struct devinfo {
	unsigned	d_devbytes;	 
	unsigned	d_dmabytes;	 
	unsigned	d_localbytes;	 
	unsigned	d_stdcount;	 
	unsigned long	*d_stdaddrs;	 
	enum MAPTYPES 	d_devtype;	 
	unsigned	d_maxiobytes;	 
};

 




 




























 











 




























 





























# 21 "tftp.c"

# 1 "sun3.md/cpu.addrs.h"

 

 



 
















 






 













 


 






 



 








 





 





 






 






 







 






























# 22 "tftp.c"



# 1 "tftp.h"

 

 



 




 








struct	tftphdr {
	short	th_opcode;		 
	union {
		short	tu_block;	 
		short	tu_code;	 
		char	tu_stuff[1];	 
	} th_u;
	char	th_data[1];		 
};






 










# 25 "tftp.c"


# 1 "/sprite/lib/include/sun3.md/sys/exec.h"
 










 


struct exec {
    unsigned short	a_machtype;	 
    unsigned short	a_magic;	 
    unsigned long	a_text;		 
    unsigned long	a_data;		 
    unsigned long	a_bss;		 
    unsigned long	a_syms;		 
    unsigned long	a_entry;	 
    unsigned long	a_trsize;	 
    unsigned long	a_drsize;	 
};

 







 










# 27 "tftp.c"


char	*tftp_errs[] = {
	"not defined",
	"file not found",
	"access violation",
	"disk full or allocation exceeded",
	"illegal TFTP operation",
	"unknown transfer ID",
	"file already exists",
	"no such user"
};



struct tftp_pack {	 
	struct ether_header tf_ether;	 
	struct ip tf_ip;		 
	struct udphdr tf_udp;		 
	struct tftphdr tf_tftp;		 
	char tftp_data[	512		];	 
};

 





struct tftpglob {
	struct tftp_pack tf_out;	 
	struct sainet tf_inet;		 
	char	tf_tmpbuf[1600];	 
	int	tf_block;		 
	char	*tf_data;		 
};





tftpload(sip, bp)
    register struct saioreq *sip;
    struct bootparam *bp;
{
    register struct tftpglob *tf = ((struct tftpglob *)(    (0xb4000 -0x4000 )  + 0x3000)) ;
    register struct tftp_pack *out = &tf->tf_out;
    register struct tftp_pack *in = (struct tftp_pack *)tf->tf_tmpbuf;
    register char *p, *q, *x;
    register short i, len;
    int autoboot = 0;
    int firsttry = 0;
    int feedback = 0;
    int finished = 0;
    int time, xcount, locked, retry;
    struct exec *header;
    char	  *ind = "-=";
    





     


    out->tf_ip.ip_v = 4 ;
    out->tf_ip.ip_hl = sizeof (struct ip) / 4;
    out->tf_ip.ip_ttl = 	255		;
    out->tf_ip.ip_p = 	17		;
    out->tf_udp.uh_sport =  ((*((struct sunromvec *)0x0FEF0000) ->v_nmiclock)  & 1023) + 1024;
    out->tf_udp.uh_dport =  	69 ;
    out->tf_udp.uh_sum =  0;		 
    
     



    out->tf_ip.ip_src = tf->tf_inet.sain_myaddr;
    
# 120 "tftp.c"

      (*((struct sunromvec *)0x0FEF0000) ->v_printf) ("Downloading \"%s\" from tftp server at ", bp->bp_name); 
    inet_print(out->tf_ip.ip_dst);

    
    ++firsttry;
    locked = 0;
    retry = 0;
    tf->tf_block = 1;
    tf->tf_data = (char *)0x4000  - sizeof(struct exec);;
    
     


    out->tf_tftp.th_opcode = 01			;
    p = out->tf_tftp.th_u.tu_stuff ;
    q = bp->bp_name;
    while (*p++ = *q++) {
	  ;
    }
    q = "octet";
    while (*p++ = *q++)
	;
    out->tf_udp.uh_ulen = sizeof (struct udphdr) + 2 +
	(p - out->tf_tftp.th_u.tu_stuff );
    out->tf_ip.ip_len = sizeof (struct ip) +
	out->tf_udp.uh_ulen;
    
    time = 0;
    for (xcount = 0; xcount < 5;) {
	if ((*((struct sunromvec *)0x0FEF0000) ->v_nmiclock)  - time >= 4000	) {
	    time = (*((struct sunromvec *)0x0FEF0000) ->v_nmiclock) ;
	     


	      (*((struct sunromvec *)0x0FEF0000) ->v_printf) ("X\b?\b");
	    if (ip_output(sip, (caddr_t)out, out->tf_ip.ip_len +
			  sizeof (struct ether_header), &tf->tf_inet,
			  tf->tf_tmpbuf))
		  (*((struct sunromvec *)0x0FEF0000) ->v_printf) ("X\b");
	    if (locked == 0 || retry > 15)
		xcount++;
	    else 
		retry++;
	}
	len = ip_input(sip, (caddr_t)in, &tf->tf_inet);
	if (len < (sizeof (struct ether_header) + sizeof (struct ip) + sizeof (struct udphdr) + 4) )
	    continue;
	if (in->tf_ip.ip_p != 	17		 ||
	    in->tf_udp.uh_dport != out->tf_udp.uh_sport) 
	    continue;
	if (locked &&
	    out->tf_ip.ip_dst.S_un.S_addr	 != in->tf_ip.ip_src.S_un.S_addr	)
	{
	      (*((struct sunromvec *)0x0FEF0000) ->v_printf) ("bogus packet from ");
	    inet_print(in->tf_ip.ip_src);
	    continue;
	}
	if (in->tf_tftp.th_opcode == 05			) {
	    if (autoboot && tf->tf_block == 1)
		continue;
	    if (in->tf_tftp.	th_u.tu_code  < 0 ||
		in->tf_tftp.	th_u.tu_code  > sizeof(tftp_errs)/sizeof(char *)){
		      (*((struct sunromvec *)0x0FEF0000) ->v_printf) ("tftp: Unknown error 0x%x\n",
			   in->tf_tftp.	th_u.tu_code );
	    } else {
		  (*((struct sunromvec *)0x0FEF0000) ->v_printf) ("tftp: %s @ block %d\n",
		       tftp_errs[in->tf_tftp.	th_u.tu_code ], tf->tf_block);
	    }




	    return (-1);
	}
	if (in->tf_tftp.th_opcode != 03			) {
	      (*((struct sunromvec *)0x0FEF0000) ->v_printf) ("unhandled opcode %d\n", in->tf_tftp.th_opcode);
	    continue;
	} else if (in->tf_tftp.th_u.tu_block  != tf->tf_block) {
	      (*((struct sunromvec *)0x0FEF0000) ->v_printf) ("expected block %d, got %d\n", tf->tf_block,
		   in->tf_tftp.th_u.tu_block );
	     



	    if (in->tf_tftp.th_u.tu_block  == tf->tf_block - 1) {
		xcount = retry = 0;
		out->tf_tftp.th_opcode = 04			;
		out->tf_tftp.th_u.tu_block  = in->tf_tftp.th_u.tu_block ;
		out->tf_udp.uh_ulen = sizeof(struct udphdr) + 4;
		out->tf_ip.ip_len = sizeof(struct ip) + out->tf_udp.uh_ulen;
		time = (*((struct sunromvec *)0x0FEF0000) ->v_nmiclock) ;
		if (ip_output(sip, (caddr_t)out,
			      out->tf_ip.ip_len + sizeof(struct ether_header),
			      &tf->tf_inet, tf->tf_tmpbuf))
		{
		      (*((struct sunromvec *)0x0FEF0000) ->v_printf) ("X\b");
		}
	    }
	    continue;
	}
	
	 


	if (tf->tf_block == 1) {	 
	    out->tf_udp.uh_dport = in->tf_udp.uh_sport;




	    locked = 1;
	}
	 


	len = in->tf_udp.uh_ulen - (sizeof(struct udphdr) + 4);
	if (len) {
	    bcopy(in->tf_tftp.th_data, tf->tf_data, len);
	    if (tf->tf_block == 1) {
		header = (struct exec *)tf->tf_data;
		  (*((struct sunromvec *)0x0FEF0000) ->v_printf) ("Size: %d", header->a_text);
	    }
	    tf->tf_data += len;

	    if (header->a_text) {
		header->a_text -= len;
		if (tf->tf_block == 1) {
		     



		    header->a_text += sizeof(struct exec);
		}
		if ((int)header->a_text <= 0) {
		      (*((struct sunromvec *)0x0FEF0000) ->v_printf) ("+%d", header->a_data);
		    header->a_data += header->a_text;
		    header->a_text = 0;
		}
	    } else {
		header->a_data -= len;
		if ((int)header->a_data <= 0) {
		      (*((struct sunromvec *)0x0FEF0000) ->v_printf) ("+%d\n", header->a_bss);
		    finished = 1;
		}
	    }
	}
	if ((tf->tf_block & 0xf) == 1) {
	      (*((struct sunromvec *)0x0FEF0000) ->v_printf) ("%c\b", ind[feedback++ & 1]);	 
	}
	 


	xcount = 0;
	retry = 0;
	out->tf_tftp.th_opcode = (!finished ? 04			 : 05			);
	out->tf_tftp.th_u.tu_block  = tf->tf_block++;
	out->tf_udp.uh_ulen = sizeof (struct udphdr) + 4;
	out->tf_ip.ip_len = sizeof (struct ip) + out->tf_udp.uh_ulen;
	time = (*((struct sunromvec *)0x0FEF0000) ->v_nmiclock) ;
	if (ip_output(sip, (caddr_t)out, out->tf_ip.ip_len +
		      sizeof (struct ether_header), &tf->tf_inet,
		      tf->tf_tmpbuf))
	      (*((struct sunromvec *)0x0FEF0000) ->v_printf) ("X\b");
	if ((len < 	512		) || finished) {	 
	     



	    tf->tf_data += (int)header->a_data;
	    bzero(tf->tf_data, header->a_bss);
	    
	      (*((struct sunromvec *)0x0FEF0000) ->v_printf) ("Downloaded %d bytes from tftp server.\n\n",
		   tf->tf_data - 0x4000 );
	    return (0x4000 );
	}
    }
      (*((struct sunromvec *)0x0FEF0000) ->v_printf) ("tftp: time-out.\n");




    return (-1);
}

 





struct ether_addr etherbroadcastaddr = { 
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
};

struct arp_packet {
	struct ether_header	arp_eh;
	struct ether_arp	arp_ea;

	char	filler[(60-14)  - sizeof(struct ether_arp)];
};



 




arp(sip, sain, tmpbuf)
	register struct saioreq *sip;
	register struct sainet *sain;
	char *tmpbuf;
{
	struct arp_packet out;

	if (in_lnaof(sain->sain_hisaddr) == 0x00000000  ||
	    (in_lnaof(sain->sain_hisaddr) &     0x000000ff ) ==     0x000000ff ) {
		sain->sain_hisether = etherbroadcastaddr;
		return;
	}
	out.arp_eh.ether_type = 0x0806		;
	out.arp_ea.arp_op = 1	;
	(*(struct ether_addr *)(&out.arp_ea)->arp_xtha)  = etherbroadcastaddr;	 
	(*(struct in_addr *)(&out.arp_ea)->arp_xtpa) .S_un.S_addr	 = sain->sain_hisaddr.S_un.S_addr	;
	comarp(sip, sain, &out, tmpbuf);
}

 




comarp(sip, sain, out, tmpbuf)
	register struct saioreq *sip;
	register struct sainet *sain;
	register struct arp_packet *out;
	char *tmpbuf;
{
	register struct arp_packet *in = (struct arp_packet *)tmpbuf;
	register int e, count, time, feedback,len, delay = 2;
	char    *ind = "-\\|/";

	out->arp_eh.ether_dhost = etherbroadcastaddr;
	out->arp_eh.ether_shost = sain->sain_myether;
	out->arp_ea.arp_hrd =  	1	;
	out->arp_ea.arp_pro = 	0x0800		;
	out->arp_ea.arp_hln = sizeof(struct ether_addr);
	out->arp_ea.arp_pln = sizeof(struct in_addr);
	(*(struct ether_addr *)(&out->arp_ea)->arp_xsha)  = sain->sain_myether;
	(*(struct in_addr *)(&out->arp_ea)->arp_xspa) .S_un.S_addr	 = sain->sain_myaddr.S_un.S_addr	;
	feedback = 0;

	for (count=0; ; count++) {
		if (count == 2	) {
			if (out->arp_ea.arp_op == 1	) {
				  (*((struct sunromvec *)0x0FEF0000) ->v_printf) ("Requesting Ethernet address for ");
				inet_print((*(struct in_addr *)(&out->arp_ea)->arp_xtpa) );
			} else {
				  (*((struct sunromvec *)0x0FEF0000) ->v_printf) ("Requesting Internet address for ");
				ether_print(&(*(struct ether_addr *)(&out->arp_ea)->arp_xtha) );
			}
		}
		e = (*sip->si_sif->sif_xmit)(sip->si_devdata, (caddr_t)out,
			sizeof *out);
		
		time = (*((struct sunromvec *)0x0FEF0000) ->v_nmiclock)  + (delay * 1000);	 
		  (*((struct sunromvec *)0x0FEF0000) ->v_printf) ("%c\b", ind[feedback++ % 4]);     

		while ((*((struct sunromvec *)0x0FEF0000) ->v_nmiclock)  <= time) {
			len = (*sip->si_sif->sif_poll)(sip->si_devdata, tmpbuf);
			if (len < (sizeof (struct ether_header)+sizeof(struct ether_arp)) )
				continue;
			if (in->arp_ea.arp_pro != 	0x0800		)
				continue;
			if (out->arp_ea.arp_op == 1	) {
				if (in->arp_eh.ether_type != 0x0806		)
					continue;
				if (in->arp_ea.arp_op != 2	)
					continue;
				if ((*(struct in_addr *)(&in->arp_ea)->arp_xspa) .S_un.S_addr	 !=
				    (*(struct in_addr *)(&out->arp_ea)->arp_xtpa) .S_un.S_addr	)
					continue;
				if (count >= 2	) {
					  (*((struct sunromvec *)0x0FEF0000) ->v_printf) ("Found at ");
					ether_print(&(*(struct ether_addr *)(&in->arp_ea)->arp_xsha) );
				}
				sain->sain_hisether = (*(struct ether_addr *)(&in->arp_ea)->arp_xsha) ;
				return;
			} else {		 
				if (in->arp_eh.ether_type !=0x8035		)
					continue;
				if (in->arp_ea.arp_op != 4	)
					continue;
				if (bcmp((caddr_t)&(*(struct ether_addr *)(&in->arp_ea)->arp_xtha) ,
				    (caddr_t)&(*(struct ether_addr *)(&out->arp_ea)->arp_xtha) , 
				    sizeof (struct ether_addr)) != 0)
					continue;

				  (*((struct sunromvec *)0x0FEF0000) ->v_printf) ("Using IP Address ");
				inet_print((*(struct in_addr *)(&in->arp_ea)->arp_xtpa) );

				sain->sain_myaddr = (*(struct in_addr *)(&in->arp_ea)->arp_xtpa) ;
				 
				sain->sain_hisaddr = (*(struct in_addr *)(&in->arp_ea)->arp_xspa) ;
				sain->sain_hisether = (*(struct ether_addr *)(&in->arp_ea)->arp_xsha) ;
				return;
			}
		}
		delay = delay * 2;	 

		if (delay > 64)		 
			delay = 64;

		(*sip->si_sif->sif_reset)(sip->si_devdata);
	}
	 
}

 



ip_output(sip, buf, len, sain, tmpbuf)
	register struct saioreq *sip;
	caddr_t buf, tmpbuf;
	short len;
	register struct sainet *sain;
{
	register struct ether_header *eh;
	register struct ip *ip;

	eh = (struct ether_header *)buf;
	ip = (struct ip *)(buf + sizeof(struct ether_header));
	if (ip->ip_dst.S_un.S_addr	 != sain->sain_hisaddr.S_un.S_addr	) {
		sain->sain_hisaddr.S_un.S_addr	 = ip->ip_dst.S_un.S_addr	;
		arp(sip, sain, tmpbuf);
	}
	eh->ether_type = 	0x0800		;
	eh->ether_shost = sain->sain_myether;
	eh->ether_dhost = sain->sain_hisether;
	 
	ip->ip_sum = 0;
	ip->ip_sum = ipcksum((caddr_t)ip, sizeof (struct ip));
	if (len < (60-14) +sizeof(struct ether_header))
		len = (60-14) +sizeof(struct ether_header);
	return (*sip->si_sif->sif_xmit)(sip->si_devdata, buf, len);
}

 





ip_input(sip, buf, sain)
	register struct saioreq *sip;
	caddr_t buf;
	register struct sainet *sain;
{
	register short len;
	register struct ether_header *eh;
	register struct ip *ip;
	register struct ether_arp *ea;

	len = (*sip->si_sif->sif_poll)(sip->si_devdata, buf);
	eh = (struct ether_header *)buf;
	if (eh->ether_type == 	0x0800		 &&
	    len >= sizeof(struct ether_header)+sizeof(struct ip)) {
		ip = (struct ip *)(buf + sizeof(struct ether_header));
		if (ip->ip_dst.S_un.S_addr	 != sain->sain_myaddr.S_un.S_addr	) 
			return (0);
		return (len);
	}
	if (eh->ether_type == 0x0806		 &&
	    len >= sizeof(struct ether_header)+sizeof(struct ether_arp)) {
		ea = (struct ether_arp *)(buf + sizeof(struct ether_header));
		if (ea->arp_pro != 	0x0800		) 
			return (0);
		if ((*(struct in_addr *)(ea)->arp_xspa) .S_un.S_addr	 == sain->sain_hisaddr.S_un.S_addr	)
			sain->sain_hisether = (*(struct ether_addr *)(ea)->arp_xsha) ;
		if (ea->arp_op == 1	 &&
		    (*(struct in_addr *)(ea)->arp_xtpa) .S_un.S_addr	 == sain->sain_myaddr.S_un.S_addr	) {
			ea->arp_op = 2	;
			eh->ether_dhost = (*(struct ether_addr *)(ea)->arp_xsha) ;
			eh->ether_shost = sain->sain_myether;
			(*(struct ether_addr *)(ea)->arp_xtha)  = (*(struct ether_addr *)(ea)->arp_xsha) ;
			(*(struct in_addr *)(ea)->arp_xtpa)  = (*(struct in_addr *)(ea)->arp_xspa) ;
			(*(struct ether_addr *)(ea)->arp_xsha)  = sain->sain_myether;
			(*(struct in_addr *)(ea)->arp_xspa)  = sain->sain_myaddr;
			(*sip->si_sif->sif_xmit)(sip->si_devdata, buf, 
						sizeof(struct arp_packet));
		}
		return (0);
	}
	return (0);
}

 


in_lnaof(in)
	struct in_addr in;
{
	register u_long i = (in.S_un.S_addr	) ;

	if (	((((long)(i))&0x80000000)==0) )
		return ((i)&	0x00ffffff );
	else if (	((((long)(i))&0xc0000000)==0x80000000) )
		return ((i)&	0x0000ffff );
	else
		return ((i)&	0x000000ff );
}

 



ipcksum(cp, count)
	caddr_t	cp;
	register unsigned short	count;
{
	register unsigned short	*sp = (unsigned short *)cp;
	register unsigned long	sum = 0;
	register unsigned long	oneword = 0x00010000;

	count >>= 1;
	while (count--) {
		sum += *sp++;
		if (sum >= oneword) {		 
			sum -= oneword;
			sum++;
		}
	}
	return (~sum);
}

inet_print(s)
	struct in_addr s;
{
	int	len = 2;

	  (*((struct sunromvec *)0x0FEF0000) ->v_printf) ("%d.%d.%d.%d = ", s.S_un.S_un_b.s_b1, s.S_un.S_un_b.s_b2,
		s.S_un.S_un_b.s_b3, s.S_un.S_un_b.s_b4);

	 (*((struct sunromvec *)0x0FEF0000) ->v_printhex) (s.S_un.S_un_b.s_b1, len);
	 (*((struct sunromvec *)0x0FEF0000) ->v_printhex) (s.S_un.S_un_b.s_b2, len);
	 (*((struct sunromvec *)0x0FEF0000) ->v_printhex) (s.S_un.S_un_b.s_b3, len);
	 (*((struct sunromvec *)0x0FEF0000) ->v_printhex) (s.S_un.S_un_b.s_b4, len);
	  (*((struct sunromvec *)0x0FEF0000) ->v_printf) ("\n");
}

ether_print(ea)
	struct ether_addr *ea;
{
	register u_char *p = &ea->ether_addr_octet[0];

	  (*((struct sunromvec *)0x0FEF0000) ->v_printf) ("%x:%x:%x:%x:%x:%x\n", p[0], p[1], p[2], p[3], p[4], p[5]);
}
