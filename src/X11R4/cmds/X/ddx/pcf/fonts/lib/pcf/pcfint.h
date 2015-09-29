#ifndef	PCFINT_H
#define	PCFINT_H

/***====================================================================***/

      /*\
      |* Low level file operations
      \*/
typedef struct _OrderFuncs {
      INT16   (*readInt16)();
      INT32   (*readInt32)();
      Bool    (*writeInt16)();
      Bool    (*writeInt32)();
} pcfOrderFuncs;

extern        void    pcfSetByteOrder();
#define       pcfPrivate(f)           ((pcfOrderFuncs *)(f)->fmtPrivate)
#define       pcfReadInt16(f)         (*pcfPrivate(f)->readInt16)(f)
#define       pcfReadInt32(f)         (*pcfPrivate(f)->readInt32)(f)
#define       pcfWriteInt16(f,v)      (*pcfPrivate(f)->writeInt16)(f,v)
#define       pcfWriteInt32(f,v)      (*pcfPrivate(f)->writeInt32)(f,v)

/***====================================================================***/

#define       pcfAlignPad(n)  (((((n)+3)>>2)<<2)-(n))

      /*\
      |* Functions to write PCF files
      \*/

#ifdef PCF_NEED_WRITERS
typedef struct _pcfWriteFuncs {
	int	(*sizeFunc)();
	int	(*writeFunc)();
	int	(*formatFunc)();
} pcfWriteFuncsRec, *pcfWriteFuncsPtr;

extern	int	pcfSizeMetrics(),pcfSizeBitmaps(),pcfSizeSWidths();
extern	int	pcfSizeGlyphNames(),pcfSizeEncoding(),pcfSizeProperties();
extern	int	pcfSizeAccelerators();

extern	int	pcfWriteMetrics(),pcfWriteBitmaps(),pcfWriteSWidths();
extern	int	pcfWriteGlyphNames(),pcfWriteEncoding(),pcfWriteProperties();
extern	int	pcfWriteAccelerators();

extern	int	pcfSelectMetricFormat();

extern	pcfWriteFuncsRec	pcfWriteFuncs[];

#define       PCF_ADD_PADDING(f,tmp) \
    if (((tmp)=pcfAlignPad(fosFilePosition(f)))>0) { \
	fosPad((f),tmp);\
    }

#endif /* PCF_NEED_WRITERS */

/***====================================================================***/

	/*\
	 *  Functions to read PCF files 
	\*/
#ifdef PCF_NEED_READERS

typedef int	(*pcfReadFunc)();

extern	pcfReadFunc	pcfReadFuncs[];

extern	int	pcfReadMetrics(),pcfReadBitmaps(),pcfReadSWidths();
extern	int	pcfReadGlyphNames(),pcfReadEncoding(),pcfReadProperties();
extern	int	pcfReadAccelerators();

#define	PCF_SKIP_PADDING(f,tmp) \
    if (((tmp)=pcfAlignPad(fosFilePosition(f)))>0) { \
	fosSkip((f),(tmp)); \
    }
#endif /* PCF_NEED_READERS */

#endif /* PCFINT_H */
