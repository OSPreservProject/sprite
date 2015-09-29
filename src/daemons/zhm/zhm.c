/* This file is part of the Project Athena Zephyr Notification System.
 * It contains the hostmanager client program.
 *
 *      Created by:     David C. Jedlinsky
 *
 *      $Source: /usr/sww/share/src/Zephyr/src/zhm/RCS/zhm.c,v $
 *      $Author: smoot $
 *
 *      Copyright (c) 1987,1991 by the Massachusetts Institute of Technology.
 *      For copying and distribution information, see the file
 *      "mit-copyright.h". 
 */

#include "zhm.h"

static char rcsid_hm_c[] = "$Id: zhm.c,v 1.49.1.6 1992/09/21 23:33:06 smoot Exp $";

#include <ctype.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <sys/file.h>
#include <string.h>
#include <errno.h>
/* 
 * warning: sys/param.h may include sys/types.h which may not be protected from
 * multiple inclusions on your system
 */
#include <sys/param.h>

#ifdef Z_HaveHesiod
#include <hesiod.h>
#endif

#ifdef macII
#define srandom srand48
#endif

int hmdebug, rebootflag, errflg, dieflag, inetd, oldpid;
int no_server = 1, nservchang, nserv, nclt;
int booting = 1, timeout_type, deactivated = 1;
long starttime;
u_short cli_port;
struct sockaddr_in cli_sin, serv_sin, from;
char **serv_list, **cur_serv_list;
char prim_serv[MAXHOSTNAMELEN], cur_serv[MAXHOSTNAMELEN];
char *zcluster;
int sig_type;
struct hostent *hp;
char **clust_info;
char hostname[MAXHOSTNAMELEN], loopback[4];
char PidFile[MAXPATHLEN];

extern char *index(), *sbrk();

void init_hm(), detach(), handle_timeout(), resend_notices(), die_gracefully();

#if defined(ultrix) || defined(_POSIX_SOURCE)
void
#endif
  set_sig_type(sig)
     int sig;
{
     sig_type = sig;
}

char *strsave();

main(argc, argv)
char *argv[];
{
     ZNotice_t notice;
     ZPacket_t packet;
     Code_t ret;
     int opt, pak_len;
     extern int optind;
     register int i, j = 0;

     if (gethostname(hostname, MAXHOSTNAMELEN) < 0) {
	  printf("Can't find my hostname?!\n");
	  exit(-1);
     }
     prim_serv[0] = '\0';
     while ((opt = getopt(argc, argv, "drhi")) != EOF)
	  switch(opt) {
	  case 'd':
	       hmdebug++;
	       break;
	  case 'h':
	       /* Die on SIGHUP */
	       dieflag++;
	       break;
	  case 'r':
	       /* Reboot host -- send boot notice -- and exit */
	       rebootflag++;
	       break;
	  case 'i':
	       /* inetd operation: don't do bind ourselves, fd 0 is
		  already connected to a socket. Implies -h */
	       inetd++;
	       dieflag++;
	       break;
	  case '?':
	  default:
	       errflg++;
	       break;
	  }
     if (errflg) {
	  fprintf(stderr, "Usage: %s [-d] [-h] [-r] [server]\n", argv[0]);
	  exit(2);
     }

     /* Override server argument? */
     if (optind < argc) {
	 if ((hp = gethostbyname(argv[optind++])) == NULL) {
	     printf("Unknown server name: %s\n", argv[optind-1]);
	 } else
	     (void) strcpy(prim_serv, hp->h_name);
	 /* argc-optind is the # of other servers on the command line */
	 serv_list = (char **)malloc((argc-optind + 2) * sizeof(char *));
	 serv_list[0] = prim_serv;
	 for (i = 1; optind < argc; optind++) {
	     if ((hp = gethostbyname(argv[optind])) == NULL) {
		 printf("Unknown server name '%s', ignoring\n", argv[optind]);
		 continue;
	     }
	     serv_list[i] = strsave(hp->h_name);
	     i++;
	 }
	 serv_list[i] = NULL;
     }
#ifdef Z_HaveHesiod
     else {

	 if ((clust_info = hes_resolve(hostname, "CLUSTER")) == NULL) {
	     zcluster = NULL;
	 } else
	     for ( ; *clust_info; clust_info++) {
		 /* Remove the following check once we have changed over to
		  * new Hesiod format (i.e. ZCLUSTER.sloc lookup, no primary
		  * server
		  */
		 if (!strncasecmp("ZEPHYR", *clust_info, 6)) {
		     register char *c;
	
		     if ((c = index(*clust_info, ' ')) == 0) {
			 printf("Hesiod error getting primary server info.\n");
		     } else
			 (void)strcpy(prim_serv, c+1);
		     break;
		 }
		 if (!strncasecmp("ZCLUSTER", *clust_info, 9)) {
		     register char *c;

		     if ((c = index(*clust_info, ' ')) == 0) {
			 printf("Hesiod error getting zcluster info.\n");
		     } else {
			 if ((zcluster = malloc((unsigned)(strlen(c+1)+1)))
			     != NULL) {
			     (void)strcpy(zcluster, c+1);
			 } else {
			     printf("Out of memory.\n");
			     exit(-5);
			 }
		     }
		     break;
		 }
	     }
	  
	 if (zcluster == NULL) {
	     if ((zcluster = malloc((unsigned)(strlen("zephyr")+1))) != NULL)
		 (void)strcpy(zcluster, "zephyr");
	 }
	 while ((serv_list = hes_resolve(zcluster, "sloc")) == (char **)NULL) {
	     syslog(LOG_ERR, "No servers or no hesiod");
	     /* wait a bit, and try again */
	     sleep(30);
	 }
	 clust_info = (char **)malloc(2*sizeof(char *));
	 if (prim_serv[0]) {
	     clust_info[0] = prim_serv;
	     j = 1;
	 } else
	     j = 0;
	 for (i = 0; serv_list[i]; i++)
	     /* copy in non-duplicates */
	     /* assume the names returned in the sloc are full domain names */
	     if (!prim_serv[0] || strcasecmp(prim_serv, serv_list[i])) {
		 clust_info = (char **) realloc(clust_info,
						(j+2) * sizeof(char *));
		 clust_info[j++] = strsave(serv_list[i]);
	     }
	 clust_info[j] = NULL;
	 serv_list = clust_info;
     }
     if (!prim_serv[0] && j) {
	 srandom(time((long *) 0));
	 (void) strcpy(prim_serv, serv_list[random() % j]);
     }
#endif
     if (*prim_serv == '\0') {
       /* nothing found so far; try to use "zephyr-server" as a hostname */
       if (hp = gethostbyname("zephyr-server.cs.berkeley.edu")) {
	 (void) strcpy(prim_serv, hp->h_name);
	 /* argc-optind is the # of other servers on the command line */
	 serv_list = (char **)malloc(2 * sizeof(*serv_list));
	 serv_list[0] = prim_serv;
	 serv_list[1] = 0;
       } else {
	 printf("No valid primary server found, exiting.\n");
	 exit(ZERR_SERVNAK);
       }
     }
     init_hm();

     DPR2 ("zephyr server port: %u\n", ntohs(serv_sin.sin_port));
     DPR2 ("zephyr client port: %u\n", ntohs(cli_port));
  
     /* Main loop */
     for ever {
	  DPR ("Waiting for a packet...");
	  switch(sig_type) {
	  case 0:
	       break;
	  case SIGHUP:	/* A SIGHUP means we are deactivating the ws. */
	       sig_type = 0;
	       if (dieflag) {
		    die_gracefully();
	       } else {
		    send_flush_notice(HM_FLUSH);
		    deactivated = 1;
	       }
	       break;
	  case SIGTERM:
	       sig_type = 0;
	       die_gracefully();
	       break;
	  case SIGALRM:
	       sig_type = 0;
	       handle_timeout();
	       break;
	  default:
	       sig_type = 0;
	       syslog (LOG_WARNING, "Unknown system interrupt.");
	       break;
	  }
	  ret = ZReceivePacket(packet, &pak_len, &from);
	  if ((ret != ZERR_NONE) && (ret != EINTR)){
	       Zperr(ret);
	       com_err("hm", ret, "receiving notice");
	  } else if (ret != EINTR) {
	       /* Where did it come from? */
	       if ((ret = ZParseNotice(packet, pak_len, &notice))
		   != ZERR_NONE) {
		   Zperr(ret);
		   com_err("hm", ret, "parsing notice");
	      } else {
		   DPR ("Got a packet.\n");
		   DPR ("notice:\n");
		   DPR2("\tz_kind: %d\n", notice.z_kind);
		   DPR2("\tz_port: %u\n", ntohs(notice.z_port));
		   DPR2("\tz_class: %s\n", notice.z_class);
		   DPR2("\tz_class_inst: %s\n", notice.z_class_inst);
		   DPR2("\tz_opcode: %s\n", notice.z_opcode);
		   DPR2("\tz_sender: %s\n", notice.z_sender);
		   DPR2("\tz_recip: %s\n", notice.z_recipient);
		   DPR2("\tz_def_format: %s\n", notice.z_default_format);
		   DPR2("\tz_message: %s\n", notice.z_message);
		   if ((bcmp(loopback, (char *)&from.sin_addr, 4) != 0) &&
		       ((notice.z_kind == SERVACK) ||
			(notice.z_kind == SERVNAK) ||
			(notice.z_kind == HMCTL))) {
		       server_manager(&notice);
		  } else {
		       if ((bcmp(loopback, (char *)&from.sin_addr, 4) == 0) &&
			   ((notice.z_kind == UNSAFE) ||
			    (notice.z_kind == UNACKED) ||
			    (notice.z_kind == ACKED) ||
			    (notice.z_kind == HMCTL))) {
			   /* Client program... */
			   if (deactivated) {
				send_boot_notice(HM_BOOT);
				deactivated = 0;
			   }
			   transmission_tower(&notice, packet, pak_len);
			   DPR2 ("Pending = %d\n", ZPending());
		      } else {
			   if (notice.z_kind == STAT) {
				send_stats(&notice, &from);
			   } else {
				syslog(LOG_INFO,
				       "Unknown notice type: %d",
				       notice.z_kind);
			   }
		      }
		  }
	      }
	  }
     }
}

void init_hm()
{
     struct servent *sp;
     Code_t ret;
     FILE *fp;

     starttime = time((time_t *)0);
     OPENLOG("hm", LOG_PID, LOG_DAEMON);
  
     if ((ret = ZInitialize()) != ZERR_NONE) {
	  Zperr(ret);
	  com_err("hm", ret, "initializing");
	  closelog();
	  exit(-1);
     }
     (void)ZSetServerState(1);	/* Aargh!!! */
     init_queue();

     cur_serv_list = serv_list;
     if (*prim_serv == '\0')
	  (void)strcpy(prim_serv, *cur_serv_list);
  
#ifdef sprite
     {
         struct hostent *hentry;

         if ((hentry=gethostbyname(hostname)) == NULL) {
             herror("init_hm");
             exit(-1);
         }
         bcopy(hentry->h_addr, loopback, 4);
     }

     /* kill old hm if it exists */
     strcpy(PidFile, "/hosts/");
     strcat(PidFile, hostname);
     strcat(PidFile, "/");
     strcat(PidFile, PIDFILE);
#else
     loopback[0] = 127;
     loopback[1] = 0;
     loopback[2] = 0;
     loopback[3] = 1;

     /* kill old hm if it exists */
     strcpy(PidFile, PIDFILE);
#endif
     
     fp = fopen(PidFile, "r");
     if (fp != NULL) {
	  (void)fscanf(fp, "%d\n", &oldpid);
	  while (!kill(oldpid, SIGTERM))
	       sleep(1);
	  syslog(LOG_INFO, "Killed old image.");
	  (void) fclose(fp);
     }

     if (inetd) {
	     (void) ZSetFD(0);		/* fd 0 is on the socket,
					   thanks to inetd */
     } else {
	     /* Open client socket, for receiving client and server notices */
	     if ((sp = getservbyname(HM_SVCNAME, "udp")) == NULL) {
	       fprintf(stderr,"Warning: service '%s' not in /etc/services or equivalent;\n\tassuming port 2104\n", HM_SVCNAME);
		     cli_port = htons(2104);
	     } else
	     cli_port = sp->s_port;
      
	     if ((ret = ZOpenPort(&cli_port)) != ZERR_NONE) {
		     Zperr(ret);
		     com_err("hm", ret, "opening port");
		     exit(ret);
	     }
     }
     cli_sin = ZGetDestAddr();
  
     /* Open the server socket */

     bzero((char *)&serv_sin, sizeof(struct sockaddr_in));
     if ((sp = getservbyname(SERVER_SVCNAME, "udp")) == NULL) {
	       fprintf(stderr,"Warning: service '%s' not in /etc/services or equivalent;\n\tassuming port 2103\n", SERVER_SVCNAME);
	       serv_sin.sin_port = htons(2103);
     } else
     serv_sin.sin_port = sp->s_port;
  
#ifndef DEBUG
     if (!inetd)
	     detach();
  
     /* Write pid to file */
     fp = fopen(PidFile, "w");
     if (fp != NULL) {
	     fprintf(fp, "%d\n", getpid());
	     (void) fclose(fp);
     }
#endif /* DEBUG */

     if (hmdebug) {
	  syslog(LOG_INFO, "Debugging on.");
     }
      
     /* Set up communications with server */
     /* target is SERVER_SVCNAME port on server machine */

     serv_sin.sin_family = AF_INET;
  
     /* who to talk to */
     if ((hp = gethostbyname(prim_serv)) == NULL) {
	  DPR("gethostbyname failed\n");
	  find_next_server((char *)NULL);
     } else {
	  DPR2 ("Server = %s\n", prim_serv);
	  (void)strcpy(cur_serv, prim_serv);
	  bcopy(hp->h_addr, (char *)&serv_sin.sin_addr, hp->h_length);
     }

     send_boot_notice(HM_BOOT);
     deactivated = 0;
  
     (void)signal (SIGHUP,  set_sig_type);
     (void)signal (SIGALRM, set_sig_type);
     (void)signal (SIGTERM, set_sig_type);
}

void detach()
{
     /* detach from terminal and fork. */
     register int i, x = ZGetFD(), size = getdtablesize();
  
     if (i = fork()) {
	  if (i < 0)
	       perror("fork");
	  exit(0);
     }

     for (i = 0; i < size; i++)
	  if (i != x)
	       (void) close(i);
  
     if ((i = open("/dev/tty", O_RDWR, 666)) < 0)
	  ;		/* Can't open tty, but don't flame about it. */
     else {
	  (void) ioctl(i, TIOCNOTTY, (caddr_t) 0);
	  (void) close(i);
     }
}

static char version[BUFSIZ];

send_stats(notice, sin)
     ZNotice_t *notice;
     struct sockaddr_in *sin;
{
     ZNotice_t newnotice;
     Code_t ret;
     char *bfr;
     char *list[20];
     int len, i, nitems = 10;
     unsigned int size;

     newnotice = *notice;
     
     if ((ret = ZSetDestAddr(sin)) != ZERR_NONE) {
	  Zperr(ret);
	  com_err("hm", ret, "setting destination");
     }
     newnotice.z_kind = HMACK;

     list[0] = (char *)malloc(MAXHOSTNAMELEN);
     (void)strcpy(list[0], cur_serv);
     list[1] = (char *)malloc(64);
     (void)sprintf(list[1], "%d", queue_len());
     list[2] = (char *)malloc(64);
     (void)sprintf(list[2], "%d", nclt);
     list[3] = (char *)malloc(64);
     (void)sprintf(list[3], "%d", nserv);
     list[4] = (char *)malloc(64);
     (void)sprintf(list[4], "%d", nservchang);
     list[5] = (char *)malloc(64);
     (void)strcpy(list[5], rcsid_hm_c);
     list[6] = (char *)malloc(64);
     if (no_server)
	  (void)sprintf(list[6], "yes");
     else
	  (void)sprintf(list[6], "no");
     list[7] = (char *)malloc(64);
     (void)sprintf(list[7], "%ld", time((time_t *)0) - starttime);
#ifdef adjust_size
     size = (unsigned int)sbrk(0);
     adjust_size (size);
#else
     size = -1;
#endif
     list[8] = (char *)malloc(64);
     (void)sprintf(list[8], "%ld", size);
     list[9] = (char *)malloc(32);
     (void)strcpy(list[9], MACHINE);

     /* Since ZFormatRaw* won't change the version number on notices,
	we need to set the version number explicitly.  This code is taken
	from Zinternal.c, function Z_FormatHeader */
     if (!*version)
	     (void) sprintf(version, "%s%d.%d", ZVERSIONHDR, ZVERSIONMAJOR,
			    ZVERSIONMINOR);
     newnotice.z_version = version;

     if ((ret = ZFormatRawNoticeList(&newnotice, list, nitems, &bfr,
				     &len)) != ZERR_NONE) {
	  syslog(LOG_INFO, "Couldn't format stats packet");
     } else
	  if ((ret = ZSendPacket(bfr, len, 0)) != ZERR_NONE) {
	       Zperr(ret);
	       com_err("hm", ret, "sending stats");
	  }
     free(bfr);
     for(i=0;i<nitems;i++)
	  free(list[i]);
}

void handle_timeout()
{
     switch(timeout_type) {
     case BOOTING:
	  new_server((char *)NULL);
	  break;
     case NOTICES:
	  DPR ("Notice timeout\n");
	  resend_notices(&serv_sin);
	  break;
     default:
	  syslog (LOG_ERR, "Unknown timeout type: %d\n", timeout_type);
	  break;
     }
}

void die_gracefully()
{
     syslog(LOG_INFO, "Terminate signal caught...");
     send_flush_notice(HM_FLUSH);
     (void)unlink(PidFile);
     closelog();
     exit(0);
}

char *
strsave(sp)
char *sp;
{
    register char *ret;

    if((ret = malloc((unsigned) strlen(sp)+1)) == NULL) {
	    abort();
    }
    (void) strcpy(ret,sp);
    return(ret);
}
