#include <stdio.h>
#include <X11/X.h>

#define _FONT_OS_VISIBLE
#include "fontos.h"
#include "fosfilestr.h"

/***====================================================================***/

static	fosFileRec	fosStdinRec	= {
	stdin,	0,	0,	NULL,	NULL
};

static	fosFileRec	fosStdoutRec	= {
	stdout,	0,	0,	NULL,	NULL
};

fosFilePtr	fosStdin=	&fosStdinRec;
fosFilePtr	fosStdout=	&fosStdoutRec;

/***====================================================================***/

fosFilePtr
fosOpenFile(name,mode)
char	*name;
char	*mode;
{
fosFilePtr	 pFile;
FILE		*file;

    file= fopen(name,mode);
    if (!file)
	return(NULL);
    pFile= (fosFilePtr)fosAlloc(sizeof(fosFileRec));
    if (!pFile) {
	fclose(file);
	return(NULL);
    }
    pFile->file=	file;
    pFile->position=	0;
    /* 8/20/89 (ef) -- write "mode compare" */
    if (!strcmp(mode,"r"))	pFile->pid=	0;
    else			pFile->pid=	-1;
    pFile->fmtPrivate=	NULL;
    pFile->osPrivate=	NULL;
    return(pFile);
}

/***====================================================================***/

void
fosCloseFile(pFile,free)
fosFilePtr	pFile;
Bool		free;
{
int	child;

    if ((pFile)&&(pFile->file)) {
	fclose(pFile->file);
	pFile->file=	NULL;
	fosFree(pFile);
    }
    if (pFile->pid>0) {
	do { 
	    child=	wait(0); 
	} while ( (child != pFile->pid) && (child != -1));
	pFile->pid=	0;
    }
    if (free) {
	pFile->fmtPrivate=	NULL;
	pFile->osPrivate=	NULL;
	if ((pFile!=fosStdin)&&(pFile!=fosStdout)) {
	    fosFree(pFile);
	}
    }
    return;
}

/***====================================================================***/

#if defined(macII) || (defined(SYSV) && !defined(hpux))
#define vfork() fork()
#endif

Bool
fosFilterFile(pFile, filter)
fosFilePtr	  pFile;
char		**filter;
{
FILE	*fp;
int	 pfd[2];
int	 pid;

    if ((pFile==NULL)||(pFile->file==NULL)||(pFile->pid!=0))
	return(FALSE);

    if (pipe(pfd) < 0) {
	return(FALSE);
    }

    fp=	pFile->file;

    switch(pid = vfork()) {
    case 0:
	dup2(fileno(fp), 0);
	close(fileno(fp));
	dup2(pfd[1], 1);
	close(pfd[0]);
	close(pfd[1]);
	execvp(filter[0], filter);
	_exit(127);
    case -1:
	close(pfd[0]);
	close(pfd[1]);
	return(FALSE);
    default:
	dup2(pfd[0], fileno(fp));
	close(pfd[0]);
	close(pfd[1]);
	pFile->pid=	pid;
	return(TRUE);
    }    
}

/***====================================================================***/

char *
fosGetLine(pFile, buf, max)
fosFilePtr	 pFile;
char		*buf;
int		 max;
{
char	*rtrn;

    if ((pFile==NULL)||(pFile->file==NULL))
	return(NULL);
    rtrn= fgets(buf, max, pFile->file);
    if (rtrn!=NULL)	pFile->position++;
    return(rtrn);
}

/***====================================================================***/

void
fosPad(pFile,nBytes)
fosFilePtr	pFile;
int		nBytes;
{
static	 char	 buf[100];
register FILE	*file;

    if ((pFile==NULL)||(pFile->file==NULL))
	return;

/* 8/3/89 (ef) -- two problems.  magic numbers and doesn't check return */
    file= pFile->file;
    pFile->position+=	nBytes;
    while (nBytes>100) {
   	fwrite(buf,1,100,file);
	nBytes-= 100;
    }
    if (nBytes>0)
	fwrite(buf,1,nBytes,file);
    return;
}

/***====================================================================***/

void
fosSkip(pFile,nBytes)
fosFilePtr	pFile;
int	 	nBytes;
{
	 char	 buf[BUFSIZ];
register FILE	*file;


    if ((pFile==NULL)||(pFile->file==NULL))
	return;
/* 8/3/89 (ef) -- doesn't make sure reads succeed. */
    file=	pFile->file;
    pFile->position+=	nBytes;
    if (fseek(file,nBytes,1)==-1) {
        while (nBytes>BUFSIZ) {
            fread(buf,1,BUFSIZ,file);
            nBytes-= BUFSIZ;
        }
        if (nBytes>0) {
            fread(buf,1,nBytes,file);
        }
    }
    return;
}

/***====================================================================***/

INT8
fosReadInt8(pFile)
fosFilePtr	pFile;
{
register int	val;

    if ((pFile==NULL)||(pFile->file==NULL))
	return(EOF);

    if ((val=getc(pFile->file))!=EOF)
	pFile->position++;
    return(val);
}

/***====================================================================***/

int
fosReadBlock(pFile,buf,size)
fosFilePtr	 pFile;
char		*buf;
int		 size;
{
int	nRead;

    if ((pFile==NULL)||(pFile->file==NULL))
	return(0);

    nRead=	fread(buf,1,size,pFile->file);
    if (nRead>0)
	pFile->position+= nRead;
    return(nRead);
}

/***====================================================================***/

Bool
fosWriteInt8(pFile,val)
fosFilePtr	pFile;
CARD8		val;
{
    if ((pFile==NULL)||(pFile->file==NULL))
	return(FALSE);

    putc(val,pFile->file);
    pFile->position++;
    return(TRUE);
}

/***====================================================================***/

int
fosWriteBlock(pFile,buf,size)
fosFilePtr	 pFile;
char		*buf;
int		 size;
{
int	nWritten;

    if ((pFile==NULL)||(pFile->file==NULL))
	return(FALSE);

    nWritten= fwrite(buf,1,size,pFile->file);
    if (nWritten>0)
	pFile->position+=	nWritten;
    return(nWritten);
}

/***====================================================================***/

INT16
fosReadLSB16(pFile)
fosFilePtr	pFile;
{
CARD8	val0;
int	val1;

    if ((pFile==NULL)||(pFile->file==NULL))
	return(EOF);

    val0=	getc(pFile->file);
    val1=	getc(pFile->file);
    if (val1!=EOF) {
	pFile->position+= 2;
    }
    return(val0|((val1&0xff)<<8));
}

/***====================================================================***/

INT32
fosReadLSB32(pFile)
fosFilePtr	pFile;
{
CARD8	val0,val1,val2;
int	val3;

    if ((pFile==NULL)||(pFile->file==NULL))
	return(EOF);
    val0=	getc(pFile->file);
    val1=	getc(pFile->file);
    val2=	getc(pFile->file);
    val3=	getc(pFile->file);
    if (val3!=EOF)	pFile->position+= 4;
    return((val0)|(val1<<8)|(val2<<16)|((val3&0xff)<<24));
}

/***====================================================================***/

INT16
fosReadMSB16(pFile)
fosFilePtr	pFile;
{
CARD8	val0;
int	val1;

    if ((pFile==NULL)||(pFile->file==NULL))
	return(EOF);
    val0=	getc(pFile->file);
    val1=	getc(pFile->file);
    if (val1!=EOF)	pFile->position+= 2;
    return((val1&0xff)|(val0<<8));
}

/***====================================================================***/

INT32
fosReadMSB32(pFile)
fosFilePtr	pFile;
{
CARD8	val0,val1,val2;
int	val3;

    if ((pFile==NULL)||(pFile->file==NULL))
	return(EOF);
    val0=	getc(pFile->file);
    val1=	getc(pFile->file);
    val2=	getc(pFile->file);
    val3=	getc(pFile->file);
    if (val3!=EOF)	pFile->position+= 4;
    return((val3&0xff)|(val2<<8)|(val1<<16)|(val0<<24));
}

/***====================================================================***/

Bool
fosWriteLSB16(pFile,val)
fosFilePtr	pFile;
CARD16		val;
{

    if ((pFile==NULL)||(pFile->file==NULL))
	return(FALSE);
    putc(val&0xff,pFile->file);
    putc((val>>8)&0xff,pFile->file);
    pFile->position+= 2;
    return(TRUE);
}

/***====================================================================***/

Bool
fosWriteLSB32(pFile,val)
fosFilePtr	pFile;
CARD32		val;
{
    if ((pFile==NULL)||(pFile->file==NULL))
	return(FALSE);
    putc(val&0xff,pFile->file);
    putc((val>>8)&0xff,pFile->file);
    putc((val>>16)&0xff,pFile->file);
    putc((val>>24)&0xff,pFile->file);
    pFile->position+= 4;
    return(TRUE);
}

/***====================================================================***/

Bool
fosWriteMSB16(pFile,val)
fosFilePtr	pFile;
CARD16		val;
{

    if ((pFile==NULL)||(pFile->file==NULL))
	return(FALSE);
    putc((val>>8)&0xff,pFile->file);
    putc(val&0xff,pFile->file);
    pFile->position+= 2;
    return(TRUE);
}

/***====================================================================***/

Bool
fosWriteMSB32(pFile,val)
fosFilePtr	pFile;
CARD32		val;
{
    if ((pFile==NULL)||(pFile->file==NULL))
	return(FALSE);
    putc((val>>24)&0xff,pFile->file);
    putc((val>>16)&0xff,pFile->file);
    putc((val>>8)&0xff,pFile->file);
    putc(val&0xff,pFile->file);
    pFile->position+= 4;
    return(TRUE);
}
