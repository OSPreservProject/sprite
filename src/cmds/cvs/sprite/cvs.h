/*	$Id: cvs.h,v 1.3 91/08/20 12:58:34 jhh Exp $	*/

#include <strings.h>
#include <string.h>
#include <stdio.h>
#ifdef sprite
#include <sys/dir.h>
#endif
/*
 *    Copyright (c) 1989, Brian Berliner
 *
 *    You may distribute under the terms of the GNU General Public License
 *    as specified in the README file that comes with the CVS 1.0 kit.
 *
 * Definitions for the CVS Administrative directory and
 * the files it contains.  Here as #define's to make changing
 * the names a simple task.
 */
#define	CVSADM		"CVS.adm"
#define	CVSADM_ENT	"CVS.adm/Entries"
#define	CVSADM_ENTBAK	"CVS.adm/Entries.Backup"
#define	CVSADM_ENTSTAT	"CVS.adm/Entries.Static"
#define	CVSADM_FILE	"CVS.adm/Files"
#define	CVSADM_MOD	"CVS.adm/Mod"
#define	CVSADM_REP	"CVS.adm/Repository"
#define	CVSADM_CIPROG	"CVS.adm/Checkin.prog"

/*
 * Definitions for the CVSROOT Administrative directory and
 * the files it contains.  This directory is created as a
 * sub-directory of the $CVSROOT environment variable, and holds
 * global administration information for the entire source
 * repository beginning at $CVSROOT.
 */
#define	CVSROOTADM		"CVSROOT.adm"
#define	CVSROOTADM_MODULES	"CVSROOT.adm/modules"
#define	CVSROOTADM_LOGINFO	"CVSROOT.adm/loginfo"

/* support for the CVSROOTADM files */
#define	CVSMODULE_FILE	"modules" /* last component of CVSROOTADM_MODULES */
#define	CVSMODULE_TMP	".#modules.XXXXXX"
#define	CVSMODULE_OPTS	"ai:o:t:"
#define	CVSLOGINFO_FILE	"loginfo" /* last component of CVSROOTADM_LOGINFO */
#define	CVSLOGINFO_TMP	".#loginfo.XXXXXX"

/* Other CVS file names */
#define	CVSATTIC	"Attic"
#define	CVSLCK		"#cvs.lock"
#define	CVSTFL		"#cvs.tfl"
#define	CVSRFL		"#cvs.rfl"
#define	CVSWFL		"#cvs.wfl"
#define	CVSEXT_OPT	",p"
#define	CVSEXT_LOG	",t"
#define	CVSPREFIX	",,"
#define	CVSTEMP		"/tmp/cvslog.XXXXXX"

/* miscellaneous CVS defines */
#define	CVSEDITPREFIX	"CVS: "
#define	CVSLCKAGE	600		/* 10-min old lock files cleaned up */
#define	CVSLCKSLEEP	15		/* wait 15 seconds before retrying */
#define	DFLT_RECORD	"/dev/null"
#define	BAKPREFIX	".#"		/* when rcsmerge'ing */
#define	DEVNULL		"/dev/null"

#define	FALSE		0
#define	TRUE		1

/*
 * Definitions for the RCS file names.
 */
#define	RCS		"rcs"
#define	RCS_CI		"ci"
#define	RCS_CO		"co"
#define	RCS_RLOG	"rlog"
#define	RCS_DIFF	"rcsdiff"
#define	RCS_MERGE	"rcsmerge"
#define	RCS_MERGE_PAT	"^>>>>>>> "	/* runs "grep" with this pattern */
#define	RCSID_PAT	"\"\\(\\$Id.*\\)\\|\\(\\$Header.*\\)\"" /* when committing files */
#define	RCSEXT		",v"
#define	RCSHEAD		"head"
#define	RCSBRANCH	"branch"
#define	RCSSYMBOL	"symbols"
#define	RCSDATE		"date"
#define	RCSDESC		"desc"		/* ends the search for branches */
#define	DATEFORM	"%02d.%02d.%02d.%02d.%02d.%02d"

/* Programs that cvs runs */
#define	DIFF		"/bin/diff"
#define	GREP		"/bin/grep"
#define	RM		"/bin/rm"
#define	SORT		"/usr/bin/sort"

/*
 * Environment variable used by CVS
 */
#define	CVSREAD_ENV	"CVSREAD"	/* make files read-only */
#define	CVSREAD_DFLT	FALSE		/* writable files by default */

#define	RCSBIN_ENV	"RCSBIN"	/* RCS binary directory */
#define	RCSBIN_DFLT	"/sprite/cmds" /* directory to find RCS progs */

#define	EDITOR_ENV	"EDITOR"	/* which editor to use */
#define	EDITOR_DFLT	"/sprite/cmds/vi"	/* somewhat standard */

#define	CVSROOT_ENV	"CVSROOT"	/* source directory root */
#define	CVSROOT_DFLT	NULL		/* No dflt; must set for checkout */

/*
 * If the beginning of the Repository matches the following string,
 * strip it so that the output to the logfile does not contain a full pathname.
 *
 * If the CVSROOT environment variable is set, it overrides this define.
 */
#define	REPOS_STRIP	"/src/master/"

/*
 * The maximum number of files per each CVS directory.
 * This is mainly for sizing arrays statically rather than
 * dynamically.  3000 seems plenty for now.
 */
#define	MAXFILEPERDIR	3000
#define	MAXLINELEN	1000		/* max input line from a file */
#define	MAXPROGLEN	30000		/* max program length to system() */
#define	MAXLISTLEN	20000		/* For [A-Z]list holders */
#define	MAXMESGLEN	1000		/* max RCS log message size */

/*
 * The type of request that is being done in do_module() &&
 * the type of request that is being done in Find_Names().
 */
enum mtype { CHECKOUT, TAG, PATCH };
enum ftype { ALL, ALLPLUSATTIC, MOD };

extern char *progname, *command;
extern char Clist[], Glist[], Mlist[], Olist[], Dlist[];
extern char Alist[], Rlist[], Wlist[], Llist[], Blist[];
extern char User[], Repository[], SRepository[], Rcs[];
extern char VN_User[], VN_Rcs[], TS_User[], TS_Rcs[];
extern char Options[], Tag[], Date[], prog[];
extern char *Rcsbin, *Editor, *CVSroot;
extern int really_quiet, quiet;
extern int use_editor;
extern int cvswrite;
extern int force_tag_match;

extern int fileargc;			/* for Find_Names() */
extern char *fileargv[];

/*
 * Externs that are included directly in the CVS sources
 */
extern FILE *open_file();
extern char *xmalloc();
extern int ppstrcmp();
extern int ppstrcmp_files();
extern void Lock_Cleanup();

/*
 * Externs that are included in libc, but are used frequently
 * enough to warrant defining here.
 */
extern char *sprintf();
extern char *optarg;			/* for getopt() support */
extern char *getwd();
extern char *re_comp();
extern int optind;

#ifdef sprite
#define dirent direct
#endif
