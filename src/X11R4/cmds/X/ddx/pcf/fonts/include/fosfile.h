#ifndef	FOSFILE_H
#define	FOSFILE_H 1

typedef struct	_fosFile *fosFilePtr;

extern	fosFilePtr	 fosStdin,fosStdout;

extern	fosFilePtr	 fosOpenFile();
extern	void		 fosCloseFile();

extern	Bool		 fosFilterFile();

extern	void		 fosSkip();
extern	void		 fosPad();

extern	char		*fosGetLine();

extern	INT8		 fosReadInt8();
extern	INT16		 fosReadMSB16(),fosReadLSB16();
extern	INT32		 fosReadMSB32(),fosReadLSB32();
extern	int		 fosReadBlock();

extern	Bool		 fosWriteInt8();
extern	Bool		 fosWriteMSB16(),fosWriteLSB16();
extern	Bool		 fosWriteMSB32(),fosWriteLSB32();
extern	int		 fosWriteBlock();

#endif /* FOSFILE_H */
