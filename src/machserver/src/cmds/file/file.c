#ifndef lint
static	char sccsid[] = "@(#)file.c	4.12 (Berkeley) 11/17/85";
#endif
/*
 * file - determine type of file
 */

#include <sys/param.h>
#include <sys/stat.h>
#include <stdio.h>
#include <ctype.h>
/* #include <a.out.h> */

#include "file.h"

int	errno;
int	sys_nerr;
extern char	*sys_errlist[];
int in;
int i  = 0;
char buf[BUFSIZ+1];

char *troff[] = {	/* new troff intermediate lang */
	"x","T","res","init","font","202","V0","p1",0};
char *fort[] = {
	"function","subroutine","common","dimension","block","integer",
	"real","data","double",0};
char *asc[] = {
	"chmk","mov","tst","clr","jmp", ".stabs" ,0};
char *c[] = {
	"int","char","float","double","struct","extern",0};
char *as[] = {
	"globl","byte","align","text","data","comm",0};
char *sh[] = {
	"fi", "elif", "esac", "done", "export",
	"readonly", "trap", "PATH", "HOME", 0 };
char *csh[] = {
	"alias", "breaksw", "endsw", "foreach", "limit",  "onintr",
	"repeat", "setenv", "source", "path", "home", "echo", "set", 0 };

char *spriteDevices[] = {
	"term", "syslog", "scsi worm", "placeholder_2", "scsi disk",
	"scsi tape", "memory", "xylogics", "net", "scsi hba", "raid",
	"debug", "mouse", "graphics", "placeholder_3", "audio",
};

int	ifile;


/*
 * Array of routines that determine if a file is an object file
 * for a specific machine type.
 * 
 */
#ifdef __STDC__
const char *(*(machType[])) (int bsize, const char *buf, int *m, int *sym,
    char **other) = {
#else
char *(*(machType[])) () = {
#endif
    machType68k,
    machTypeSparc,
    machTypeMips,
    machTypeSymm,
};

#define MACHINECOUNT (sizeof (machType) / sizeof(*machType))

int hostFmt = HOST_FMT;

void
main(argc, argv)
int argc;
char **argv;
{
	FILE *fl;
	register char *p;
	char ap[MAXPATHLEN + 1];
	extern char _sobuf[];

	if (argc < 2) {
		fprintf(stderr, "usage: %s file ...\n", argv[0]);
		exit(3);
	}
		
	if (argc>1 && argv[1][0]=='-' && argv[1][1]=='f') {
		if ((fl = fopen(argv[2], "r")) == NULL) {
			perror(argv[2]);
			exit(2);
		}
		while ((p = fgets(ap, sizeof ap, fl)) != NULL) {
			int l = strlen(p);
			if (l>0)
				p[l-1] = '\0';
			printf("%s:	", p);
			type(p);
			if (ifile>=0)
				close(ifile);
		}
		exit(1);
	}
	while(argc > 1) {
		printf("%s:	", argv[1]);
		type(argv[1]);
		fflush(stdout);
		argc--;
		argv++;
		if (ifile >= 0)
			close(ifile);
	}
	exit(0);
}

type(file)
char *file;
{
	int j,nl;
	char ch;
	struct stat mbuf;
	char slink[MAXPATHLEN + 1];
/*	struct exec *execPtr;   */

	ifile = -1;
	if (lstat(file, &mbuf) < 0) {
		printf("%s\n",
		(unsigned)errno < sys_nerr? sys_errlist[errno]: "Cannot stat");
		return;
	}
	switch (mbuf.st_mode & S_IFMT) {

	case S_IFPDEV:
		printf("pseudo-device\n");
		return;

	case S_IFRLNK:
		printf("remote link");
		j = readlink(file, slink, sizeof slink - 1);
		if (j >= 0) {
			slink[j] = '\0';
			printf(" to %s", slink);
		}
		printf("\n");
		return;

	case S_IFLNK:
		printf("symbolic link");
		j = readlink(file, slink, sizeof slink - 1);
		if (j >= 0) {
			slink[j] = '\0';
			printf(" to %s", slink);
		}
		printf("\n");
		return;

	case S_IFDIR:
		if (mbuf.st_mode & S_ISVTX)
			printf("append-only ");
		printf("directory\n");
		return;

	case S_IFCHR:
	case S_IFBLK:
		printf("%s special (%d/%d)",
		    (mbuf.st_mode&S_IFMT) == S_IFCHR ? "character" : "block",
		     major(mbuf.st_rdev), minor(mbuf.st_rdev));
#ifdef sprite
		if (major(mbuf.st_rdev)<sizeof(spriteDevices)/sizeof(char *)) {
		    printf(" %s", spriteDevices[major(mbuf.st_rdev)]);
		}
#endif
		printf("\n");
		return;

	case S_IFSOCK:
		printf("socket\n");
		return;
	}

	ifile = open(file, 0);
	if(ifile < 0) {
		printf("%s\n",
		(unsigned)errno < sys_nerr? sys_errlist[errno]: "Cannot read");
		return;
	}
	in = read(ifile, buf, BUFSIZ);
	if (in < 0) {
	    printf("%s\n", strerror(errno));
	    return;
	}
	if(in == 0){
		printf("empty\n");
		return;
	}
	for (j = 0; j < MACHINECOUNT; j++) {
	    const char *machineName;
	    int magic;
	    int syms;
	    char *other;

	    machineName = machType[j](in, buf, &magic, &syms, &other);
	    if (machineName != NULL) {
		printf("%s ", machineName);
		switch(magic) {

		case 0413:
		    printf("demand paged ");
		    /* FALLTHROUGH */

		case 0410:
		    printf("pure ");
		    goto exec;

		case 0411:
		    printf("jfr or pdp-11 unix 411 executable\n");
		    return;

		case 0407:
exec:
		    if (*other != NULL) {
			printf("%s ", other);
		    }
		    if (mbuf.st_mode & S_ISUID) {
			printf("set-uid ");
		    }
		    if (mbuf.st_mode & S_ISGID) {
			printf("set-gid ");
		    }
		    if (mbuf.st_mode & S_ISVTX) {
			printf("sticky ");
		    }
		    printf("executable");

		    if(syms != 0) {
			printf(" not stripped");
#if 0
			if(oldo(buf)) {
			    printf(" (old format symbol table)");
			}
#endif
		    }
		    printf("\n");
		    return;

		case 0177555:
		    printf("very old archive\n");
		    return;

		case 0177545:
		    printf("old archive\n");
		    return;

		case 070707:
		    printf("cpio data\n");
		    return;

		default:
		    continue;
		}
	    }
	}

	if (buf[0] == '#' && buf[1] == '!' && shellscript(buf+2, &mbuf))
		return;
	if (buf[0] == '\037' && buf[1] == '\235') {
		if (buf[2]&0x80)
			printf("block ");
		printf("compressed %d bit code data\n", buf[2]&0x1f);
		return;
	}
	if (strncmp(buf+6, "JFIF",4) ==0 && buf[10]=='\0') {
		printf("JPEG image\n");
		return;
	}
	if (strncmp(buf, "GIF", 3) == 0 && isdigit(buf[3]) &&
		    isdigit(buf[4]) && islower(buf[5])) {
		printf("GIF\n");
		return;
	}
	if (strncmp(buf,"begin ",6) == 0 && isdigit(buf[6]) && isdigit(buf[7])
		    && buf[9]==' ' && checkbeg("M",2)) {
		printf("uuencoded file");
		goto outa;

        }
	if(strncmp(buf, "!<arch>\n__.SYMDEF", 17) == 0 ) {
		printf("archive random library\n");
		return;
	}
	if (strncmp(buf,"gremlinfile\n",12) == 0  ||
		strncmp(buf,"sungremlinfile\n",15) == 0) {
	    printf("gremlin input");
	    goto outa;
	}
	if (strncmp(buf, "%I Idraw",8)==0) {
	    printf("Idraw");
	    goto outa;
	}
	if (strncmp(buf, "This is Common TeX, ", 20)==0) {
		printf("TeX log");
		goto outa;
	}
	if ((unsigned char)buf[0]==247 && buf[1]==2) {
	    printf("TeX dvi file\n");
	    return;
	}
	if (buf[0]=='P'&&buf[1]>='1'&&buf[1]<='6'&&buf[2]=='\n') {
	    int n1,n2,n3=0;
	    char c1,c2;
	    if (sscanf(buf+3,"%d%c%d%c%d",&n1,&c1,&n2,&c2,&n3)>=4 && n1>0 &&
		    n1<10000 && n2>0 && n2<10000 && c1==' ' && c2=='\n') {
		switch(buf[1]) {
		    case '1':
			printf("%dx%d PGM image\n",n1,n2);
			return;
		    case '2':
			printf("%dx%d PGM image with %d levels\n",n1,n2,n3);
			return;
		    case '3':
			printf("%dx%d PPM image with %d levels\n",n1,n2,n3);
			return;
		    case '4':
			printf("%dx%d RawBits PGM image\n",n1,n2);
			return;
		    case '5':
			printf("%dx%d Raw PGM image with %d levels\n",n1,n2,
				n3);
			return;
		    case '6':
			printf("%dx%d Raw PPM image with %d levels\n",n1,n2,
				n3);
			return;
		}
	    }
	}
	if ((unsigned char)buf[0]==247 && buf[1]=='Y') {
	    char buf2[40];
	    int len;
	    len = (unsigned)buf[2];
	    if (len>=40) len=39;
	    bcopy(buf+3,buf2,len);
	    buf2[len]='\0';
	    printf("TeX font file: %s\n", buf2);
	    return;
	}
	if ((unsigned char)buf[0]==247) {
	    printf("TeX file of some sort\n");
	}
	if (strncmp(buf, "\\relax\n\\", 8)==0 ||
		strncmp(buf, "\\relax \n\\", 9)==0) {
	    printf("TeX aux file\n");
	    return;
	}
	if (strncmp(buf, "%!PS", 4)==0 || strncmp(buf, "%!\n", 3)==0) {
		printf("PostScript");
		goto outa;
	}
	if (strncmp(buf, "!<arch>\n", 8)==0) {
		printf("archive\n");
		return;
	}
	if (strncmp(buf,"PK\001\002",4)==0 || strncmp(buf,"PK\003\004",4)==0 ||
		strncmp(buf,"PK\005\006",4)==0) {
	    printf("zip compressed data\n");
	    return;
	}
	if (buf[6]==0 && buf[2]*256+buf[3]>2 && buf[0]*256+buf[1] == 7 +
		(buf[2]+buf[6]-buf[4]+buf[8]+
		buf[10]+buf[12]+buf[14]+buf[16]+buf[18]+buf[20]+buf[22])*256 +
		(unsigned char)buf[3]+(unsigned char)buf[7]-
		(unsigned char)buf[5]+(unsigned char)buf[9]+
		(unsigned char)buf[11]+(unsigned char)buf[13]+
		(unsigned char)buf[15]+ (unsigned char)buf[17]+
		(unsigned char)buf[19]+(unsigned char)buf[21]+
		(unsigned char)buf[23]) {
		    
	    printf("TFM font file\n");
	    return;
	}
	if (checkbeg("head ",1) && checkbeg("access", 3) &&
		checkbeg("symbols", 4) && checkbeg("locks", 5)) {
	    printf("RCS control file");
	    goto outa;
	}
	if ((checkbeg("From",4)||checkbeg("Sender: ",15)) &&
		checkbeg("Newsgroups: ",6)&& checkbeg("Subject: ",15)) {
	    printf("News file");
	    goto outa;
	}
	if (checkbeg("From: ",2)||checkbeg("From ",2)) {
	    int offset = 10;
	    if (checkbeg("Received: ", 4)) {
		offset = 40;
	    }
	    if (checkbeg("Date: ",offset)+ checkbeg("Message-Id: ",offset)+
		    checkbeg("To: ",offset)+ checkbeg("Subject: ",offset)+
		    checkbeg("Status: ",offset)>3) {
		printf("Mail file");
		goto outa;
	    }
	}
	if (checkbeg("\\documentstyle",20) || checkbeg("\\title",20) ||
		checkbeg("\\begin",20) || checkbeg("\\section",20)) {
		printf("TeX input file");
		goto outa;
	}
#if 0
	if (mbuf.st_size % 512 == 0) {	/* it may be a PRESS file */
		lseek(ifile, -512L, 2);	/* last block */
		if (read(ifile, buf, BUFSIZ) > 0 && *(short *)buf == 12138) {
			printf("PRESS file\n");
			return;
		}
	}
#endif
	if (checktar(buf)) {
	    printf("tar archive\n");
	    return;
	}

	i = 0;
	if(ccom() == 0)goto notc;
	while(buf[i] == '#'){
		j = i;
		while(buf[i++] != '\n'){
			if(i - j > 255){
				printf("data\n"); 
				return;
			}
			if(i >= in)goto notc;
		}
		if(ccom() == 0)goto notc;
	}
check:
	if(lookup(c) == 1){
		while((ch = buf[i++]) != ';' && ch != '{')if(i >= in)goto notc;
		printf("c program text");
		goto outa;
	}
	nl = 0;
	while(buf[i] != '('){
		if(buf[i] <= 0)
			goto notas;
		if(buf[i] == ';'){
			i++; 
			goto check; 
		}
		if(buf[i++] == '\n')
			if(nl++ > 6)goto notc;
		if(i >= in)goto notc;
	}
	while(buf[i] != ')'){
		if(buf[i++] == '\n')
			if(nl++ > 6)goto notc;
		if(i >= in)goto notc;
	}
	while(buf[i] != '{'){
		if(buf[i++] == '\n')
			if(nl++ > 6)goto notc;
		if(i >= in)goto notc;
	}
	printf("c program text");
	goto outa;
notc:
	i = 0;
	while(buf[i] == 'c' || buf[i] == '#'){
		while(buf[i++] != '\n')if(i >= in)goto notfort;
	}
	if(lookup(fort) == 1){
		printf("fortran program text");
		goto outa;
	}
notfort:
	i=0;
	if(ascom() == 0)goto notas;
	j = i-1;
	if(buf[i] == '.'){
		i++;
		if(lookup(as) == 1){
			printf("assembler program text"); 
			goto outa;
		}
		else if(buf[j] == '\n' && isalpha(buf[j+2]) &&
			    (buf[j+4]=='\n' || buf[j+4]==' ')){
			printf("roff, nroff, or eqn input text");
			goto outa;
		}
	}
	while(lookup(asc) == 0){
		if(ascom() == 0)goto notas;
		while(buf[i] != '\n' && buf[i++] != ':')
			if(i >= in)goto notas;
		while(buf[i] == '\n' || buf[i] == ' ' || buf[i] == '\t')if(i++ >= in)goto notas;
		j = i-1;
		if(buf[i] == '.'){
			i++;
			if(lookup(as) == 1){
				printf("assembler program text"); 
				goto outa; 
			}
			else if(buf[j] == '\n' && isalpha(buf[j+2]) &&
				(buf[j+4]=='\n' || buf[j+4]==' ')){
				printf("roff, nroff, or eqn input text");
				goto outa;
			}
		}
	}
	printf("assembler program text");
	goto outa;
notas:
	for(i=0; i < in; i++)if(buf[i]&0200){
		if (buf[0]=='\100' && buf[1]=='\357')
			printf("troff (CAT) output\n");
		else
			printf("data\n"); 
		return;
	}
	if (checkbeg("%{",30)||checkbeg("%token",30)||checkbeg("%term",30)) {
		printf("lex/yacc program text");
		goto outa;
	}
	if (checkbeg("#include",30)||checkbeg("#define",30)) {
	    if (checkbeg("/*",40)) {
		printf("c program text");
	    } else {
		 printf("program text");
	    }
	     goto outa;
	}
	if (mbuf.st_mode&((S_IEXEC)|(S_IEXEC>>3)|(S_IEXEC>>6))) {
		if (mbuf.st_mode & S_ISUID)
			printf("set-uid ");
		if (mbuf.st_mode & S_ISGID)
			printf("set-gid ");
		if (mbuf.st_mode & S_ISVTX)
			printf("sticky ");
		if (shell(buf, in, sh))
			printf("shell script");
		else if (shell(buf, in, csh))
			printf("c-shell script");
		else
			printf("commands text");
	} else if (troffint(buf, in))
		printf("troff intermediate output text");
	else if (shell(buf, in, sh))
		printf("shell commands");
	else if (shell(buf, in, csh))
		printf("c-shell commands");
	else if (buf[0]!='#' && english(buf,in)) {
		/* message printed by english() */
	} else {
	    printf("ascii text");
	}
outa:
	while(i < in)
		if((buf[i++]&0377) > 127){
			printf(" with garbage\n");
			return;
		}
	/* if next few lines in then read whole file looking for nulls ...
		while((in = read(ifile,buf,BUFSIZ)) > 0)
			for(i = 0; i < in; i++)
				if((buf[i]&0377) > 127){
					printf(" with garbage\n");
					return;
				}
		/*.... */
	printf("\n");
}

#if 0
oldo(ex)
	struct exec *ex;
{
	struct stat stb;

	if (fstat(ifile, &stb) < 0)
		return(0);
	if (N_STROFF(*ex)+sizeof(off_t) > stb.st_size)
		return (1);
	return (0);
}
#endif

troffint(bp, n)
char *bp;
int n;
{
	int k;

	i = 0;
	for (k = 0; k < 6; k++) {
		if (lookup(troff) == 0)
			return(0);
		if (lookup(troff) == 0)
			return(0);
		while (i < n && buf[i] != '\n')
			i++;
		if (i++ >= n)
			return(0);
	}
	return(1);
}
lookup(tab)
char *tab[];
{
	char r;
	int k,j,l;
	while(buf[i] == ' ' || buf[i] == '\t' || buf[i] == '\n')i++;
	for(j=0; tab[j] != 0; j++){
		l=0;
		for(k=i; ((r=tab[j][l++]) == buf[k] && r != '\0');k++);
		if(r == '\0')
			if(buf[k] == ' ' || buf[k] == '\n' || buf[k] == '\t'
			    || buf[k] == '{' || buf[k] == '/'){
				i=k;
				return(1);
			}
	}
	return(0);
}
/*
 * Check if a line begins with the specified word in the first num lines.
 */
checkbeg(word,num)
char *word;
int num;
{
	char *ptr = buf;
	int l = strlen(word);
	buf[in] = 0;

	while (ptr<buf+in) {
	    if (!strncmp(word,ptr,l)) {
		return 1;
	    }
	    while (ptr<buf+in && *ptr !='\n') {
		ptr++;
	    }
	    if (num-- <= 0) {
		return 0;
	    }
	    ptr++;
	}
	return 0;
}

ccom(){
	char cc;
	while((cc = buf[i]) == ' ' || cc == '\t' || cc == '\n')if(i++ >= in)return(0);
	if(buf[i] == '/' && buf[i+1] == '*'){
		i += 2;
		while(buf[i] != '*' || buf[i+1] != '/'){
			if(buf[i] == '\\')i += 2;
			else i++;
			if(i >= in)return(0);
		}
		if((i += 2) >= in)return(0);
	}
	if(buf[i] == '\n')if(ccom() == 0)return(0);
	return(1);
}
ascom(){
	while(buf[i] == '/'){
		i++;
		while(buf[i++] != '\n')if(i >= in)return(0);
		while(buf[i] == '\n')if(i++ >= in)return(0);
	}
	return(1);
}

#define BS 010
/*
 * Return 0 for not English text.
 * Return 1 for English.
 */
english (bp, n)
unsigned char *bp;
{
# define NASC 128
	int ct[NASC], j, vow, freq, rare;
	int badpun = 0, punct = 0;
	int numalpha = 0, numspace=0, badchar=0, numnum=0, numsign=0;
	int numdot=0;
	int overstrike=0;
	if (n<50) return(0); /* no point in statistics on squibs */
	for(j=0; j<NASC; j++)
		ct[j]=0;
	for(j=0; j<n; j++)
	{
		if (bp[j]<NASC)
			ct[bp[j]|040]++;
		if (isalpha(bp[j])) {
		    numalpha++;
		} else if (isspace(bp[j])) {
		    numspace++;
		} else if (isdigit(bp[j])) {
		    numnum++;
		} else if (bp[j]==BS) {
		    overstrike++;
		} else if (bp[j]=='+' || bp[j]=='-') {
		    numsign++;
		} else {
		    switch (bp[j]) {
		    case '.': 
			numdot++;
			/* fallthrough */
		    case ',': 
		    case ')': 
		    case '%':
		    case ';': 
		    case ':': 
		    case '?':
			    punct++;
	/*
	 * Don't count errors with '.', since there are too many things
	 * like 5.3 or foo.bar mentioned in computer documentation.
	 */
			    if ( bp[j] != '.' && j < n-1 && bp[j+1] != ' ' &&
				    bp[j+1] != '\n') {
				badpun++;
			}
			break;
		    default :
			badchar++;
		    }
		}
	}
	if (numnum+numspace+numsign+numdot==n && numnum>0) {
	    printf("ascii ");
	    if (numsign>0) {
		printf("signed ");
	    }
	    if (numdot==0) {
		printf("integer ");
	    } else {
		printf("float ");
	    }
	    printf("data");
	    return 1;
	}
	if (numnum+numspace+ct['a']+ct['b']+ct['c']+ct['d']+ct['e']+ct['f'] ==
		n && numnum>0) {
	    printf("ascii hexadecimal data");
	    return 1;
	}
	if (badpun*5 > punct) {
		return(0);
	}
	vow = ct['a'] + ct['e'] + ct['i'] + ct['o'] + ct['u'];
	freq = ct['e'] + ct['t'] + ct['a'] + ct['i'] + ct['o'] + ct['n'];
	rare = ct['v'] + ct['j'] + ct['k'] + ct['q'] + ct['x'] + ct['z'];
	if (2*ct[';'] > ct['e']) {
		return(0);
	}
	if ( (ct['>']+ct['<']+ct['/'])>ct['e']) return(0); /* shell file test */
	if (3*numalpha<2*(n-numspace) || punct*20>n || 10*numspace<n ||
		8*badchar>n) {
	    return 0;
	}
	if (vow*5 < n-ct[' '] || freq < 10*rare) return 0;
	printf("English text");
	if (overstrike) {
	    printf(" with overstrikes");
	}
	return 1;
}

shellscript(buf, sb)
	char buf[];
	struct stat *sb;
{
	register char *tp;
	char *cp, *xp, *index();

	cp = index(buf, '\n');
	if (cp == 0 || cp - buf > in)
		return (0);
	for (tp = buf; tp != cp && isspace(*tp); tp++)
		if (!isascii(*tp))
			return (0);
	for (xp = tp; tp != cp && !isspace(*tp); tp++)
		if (!isascii(*tp))
			return (0);
	if (tp == xp)
		return (0);
	if (sb->st_mode & S_ISUID)
		printf("set-uid ");
	if (sb->st_mode & S_ISGID)
		printf("set-gid ");
	if (strncmp(xp, "/bin/sh", tp-xp) == 0)
		xp = "shell";
	else if (strncmp(xp, "/bin/csh", tp-xp) == 0)
		xp = "c-shell";
	else
		*tp = '\0';
	printf("executable %s script\n", xp);
	return (1);
}

shell(bp, n, tab)
	char *bp;
	int n;
	char *tab[];
{

	int evidence = 0;
	i = 0;
	do {
		if (buf[i] == '#') evidence = 1;
		if (buf[i] == '#' || buf[i] == ':')
			while (i < n && buf[i] != '\n')
				i++;
		if (++i >= n)
			break;
		if (lookup(tab) == 1) {
			if (evidence) {
				return 1;
			} else {
			    evidence++;
			}
		}
		while (i < n && buf[i] != '\n')
			i++;

	} while (i < n);
	return (0);
}

#define RECORDSIZE      512
#define NAMSIZ  100
#define TUNMLEN 32
#define TGNMLEN 32

typedef struct tarheader {
                char    name[NAMSIZ];
                char    mode[8];
                char    uid[8];
                char    gid[8];
                char    size[12];
                char    mtime[12];
                char    chksum[8];
                char    linkflag;
                char    linkname[NAMSIZ];
                char    magic[8];
                char    uname[TUNMLEN];
                char    gname[TGNMLEN];
                char    devmajor[8];
                char    devminor[8];
} tarheader;
#define numfield(x) ((x)[0]==' '&& isdigit((x)[5])&&(x)[6]==' '&&(x)[7]=='\0')

/*
 * Return 1 if this is a tar archive.
 */
checktar(bp)
tarheader *bp;
{
    int val;
    char *cp = (char *)bp;
    int j;
    if (bp->name[0] != '/' && bp->name[0] != '.' && !isalpha(bp->name[0])) {
	return 0;
    }
    for (j=1;j<32;j++) {
	if (!isprint(bp->name[j])) break;
    }
    for (;j<NAMSIZ;j++) {
	if (bp->name[j] != 0) return 0;
    }
    if (!numfield(bp->mode) || !numfield(bp->uid) || !numfield(bp->gid)) {
	return 0;
    }
    return 1;
}
