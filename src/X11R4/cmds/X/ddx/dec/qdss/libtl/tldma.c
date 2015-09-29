/***********************************************************
Copyright 1987, 1988 by Digital Equipment Corporation, Maynard, Massachusetts,
and the Massachusetts Institute of Technology, Cambridge, Massachusetts.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its 
documentation for any purpose and without fee is hereby granted, 
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in 
supporting documentation, and that the names of Digital or MIT not be
used in advertising or publicity pertaining to distribution of the
software without specific, written prior permission.  

DIGITAL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
DIGITAL BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
SOFTWARE.

******************************************************************/

#ifndef lint
static char *sccsid = "@(#)tldma.c	1.4	8/11/87";
static char rcsid[] = "$XConsortium: tldma.c,v 1.3 89/11/26 18:07:49 keith Exp $";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1985, 1986 by			*
 *		Digital Equipment Corporation, Maynard, MA		*
 *			All rights reserved.				*
 *									*
 *   This software is furnished under a license and may be used and	*
 *   copied  only  in accordance with the terms of such license and	*
 *   with the  inclusion  of  the  above  copyright  notice.   This	*
 *   software  or  any  other copies thereof may not be provided or	*
 *   otherwise made available to any other person.  No title to and	*
 *   ownership of the software is hereby transferred.			*
 *									*
 *   The information in this software is subject to change  without	*
 *   notice  and should not be construed as a commitment by Digital	*
 *   Equipment Corporation.						*
 *									*
 *   Digital assumes no responsibility for the use  or  reliability	*
 *   of its software on equipment which is not supplied by Digital.	*
 *									*
 ************************************************************************/

#include <errno.h>
#include <sys/time.h>
#include        "Ultrix2.0inc.h"
#include        "tl.h"

/* These used by uVaxII */
#include	<vaxuba/qdioctl.h>
#include	<vaxuba/qduser.h>
#include        <vaxuba/qdreg.h>
/* end - uVaxII	*/

/* These used by Vaxstar */
#include "tlsg.h"
/* end - Vaxstar  */

extern int errno;

#ifdef DEBUG
int DebugDma = 0; /* used in tl.h */
long NextSaveDmaSlot = 0;
long SaveDmaSlots[16];
SaveDmaPC()
{
    register callersPC; /* r11 if PCC */
#ifdef __GNUC__
    asm(" movl 16(fp),%0" : "=r" (callersPC));
#else
    asm(" movl 16(fp),r11");
#endif
    SaveDmaSlots[NextSaveDmaSlot&15] = callersPC;
    NextSaveDmaSlot++;
}
#endif

/* These used by Vaxstar */
extern VOLATILE short  * sg_int_flag; /* Ugly hack? is set in the driver */
extern VOLATILE short  * change_section; /* so is change_section 	     */
short	sg_fifo_func = 0;
/* end - Vaxstar  */

/* These used by uVaxII */
struct DMAreq *DMArequest; /* pointer to per-packet header of current packet */
u_short *DMAbuf;	   /* either a pointer to the beginning of the current
/* end - uVaxII	*/

/* These used by Vaxstar */
struct FIFOreq *FIFOrequest;
u_short *SG_Fifo;   
u_short sg_fifo_type;
VOLATILE struct  fcc *fcc_cbcsr;
/* end - Vaxstar */

u_short *dma_word;	/* tail of packet under construction */
u_short *req_buf_max;   /* pointer to word following    */
u_short *dma_max;       /* expected end of dma list. touched only in tl.h */

int	last_DMAtype;
int     req_buf_size;      /* size (in words) of a single packet  */
int     total_buf_size;    /* sum of sizes (in bytes) of packets */

VOLATILE unsigned short *AdderPtr;	/* this is initialized in tlInit */

#define        NUM_DMAreq      3

Enable_dma()
{
    register struct dga *dga = Dga;

    int	i;		/* loop counter */

    /*
     * halt pending ops
     */

    dga->csr &= HALT_DMA_OPS;

    /********************************************************/
    /* see if the DMA iobuf needs to be mapped to the user	*/
    /********************************************************/

    if (DMAheader == 0) {
    	if (ioctl(fd_qdss, QD_MAPIOBUF, &DMAheader) == -1) {
    		printf("ioctl() error. errno = %d\n", errno);
    		exit (1);
    	}
    }

    /****************************************************************/
    /* initialize total number of request buffers in the request	*/
    /* queue							*/
    /****************************************************************/

    DMAheader->size = NUM_DMAreq;
    DMAheader->used = DMAheader->oldest = DMAheader->newest = 0;

    dga->csr |= DMA_IE;	/* flush any spurious interrupts */
    
    /****************************************************************/
    /* initialize sum of sizes of request buffers, expressed in 	*/
    /* number of bytes						*/
    /****************************************************************/

    total_buf_size =     DMAheader->shared_size 
			   - sizeof (struct DMAreq_header) 
			   - (NUM_DMAreq * sizeof (struct DMAreq)) ;



    /****************************************************************/
    /* initialize size of individual request buffers in words 	*/
    /****************************************************************/

    req_buf_size = (total_buf_size / NUM_DMAreq) / sizeof(short);

    /****************************************************************/
    /* initialze the address to the start of the dma buffers in 	*/
    /* each request entry header and the pointers into those 	*/
    /* buffers.  then make the DMAbuf pointer nil to signal that 	*/
    /* there is no display currently being built.			*/
    /****************************************************************/

    DMAbuf =  (u_short *) &DMAheader->DMAreq[NUM_DMAreq];
    for (i=0; i<NUM_DMAreq; i++) {
    	DMAheader->DMAreq[i].bufp = (char *) DMAbuf;
    	DMAbuf += req_buf_size;
    }
    DMAbuf = 0;
    /*
     * to prevent first Need_dma from flushing
     */
    dma_word = req_buf_max = (u_short *) req_buf_size;

    last_DMAtype = (-1);

}

/*
 * This routine will enable the FIFO capability of the VAXstar hardware. This routine
 * will set a flag which indicates that we want to use the FIFO for future 
 * operations.
 */

Enable_fifo()
{
	register VOLATILE struct fcc *sgfcc = fcc_cbcsr;

	int	i;		/* loop counter */

	/********************************/
	/* first set the FIFO flag	*/
	/********************************/

	FIFOflag = TRUE;

        /********************************************************/
        /* see if the FIFO iobuf needs to be mapped to the user  */
        /********************************************************/

        if (FIFOheader == 0)
                ioctl(fd_qdss, QD_MAPIOBUF, &FIFOheader, 0);
        *(unsigned long *) &sgfcc->cbcsr = (unsigned long) 0;
        *(unsigned long *) &sgfcc->put = (unsigned long ) 0;
        sgfcc->thresh = 0x0000;
        *(unsigned long *)&sgfcc->cbcsr |= 
		(unsigned long)(((sgfcc->icsr|ENTHRSH)<<16)|DL_ENB);

        /****************************************************************/
        /* initialize total number of request buffers in the request    */
        /* queue                                                        */
        /****************************************************************/

        FIFOheader->size = NUM_DMAreq;
        FIFOheader->used = FIFOheader->oldest = FIFOheader->newest = 0;


        /****************************************************************/
        /* initialize sum of sizes of request buffers, expressed in     */
        /* number of bytes                                              */
        /****************************************************************/

        total_buf_size =     FIFOheader->shared_size
                           - sizeof (struct FIFOreq_header)
                           - (NUM_DMAreq * sizeof (struct FIFOreq)) ;



        /****************************************************************/
        /* initialize size of individual request buffers in words and   */
        /* some FIFO related variables.                                 */
        /****************************************************************/

	req_buf_size = (total_buf_size / NUM_DMAreq) / sizeof(short);
	*sg_next_fifo = 0;  /* This points to next available word in FIFO */
	

        /****************************************************************/
        /* initialze the address to the start of the dma buffers in     */
        /* each request entry header and the pointers into those        */
        /* buffers.  then make the DMAbuf pointer nil to signal that    */
        /* there is no display currently being built.                   */
        /****************************************************************/

        DMAbuf = 0;
        /*
         * to prevent first Need_dma from flushing
         */
        dma_word = req_buf_max = (u_short *) req_buf_size;

        last_DMAtype = (-1);

}



Flush_dma (wait_flag)
    int	wait_flag;	/* wait for adder address completion before returning */
{
    register u_short *DMAbuf_ptr;

    /****************************************************************/
    /* *dga MUST be a register variable to ensure that the 		*/
    /* optimizer generates instructions that can be performed on 	*/
    /* device registers						*/
    /****************************************************************/

    register struct dga *dga = Dga;
    register VOLATILE struct  DMAreq_header	*DMAhdr = DMAheader;
    register u_short *p;
    register struct DMAreq	*reqst = (struct DMAreq	*) DMArequest;
    int cookie;

    struct timeval timeout;
    int selmask;

    /*
     * timeout needs to be small, to accomodate race condition in loop
     */
    timeout.tv_sec = (long) 0;
    timeout.tv_usec = (long) 17000;	/* 1/60 sec. */

#ifdef undef
    /* determine if there is really something to send */
    if ( dma_word - DMAbuf == 0 && wait_flag == TRUE)
	return;
#endif

    if ((wait_flag == TRUE) || (DMAbuf != 0)) {
    	/********************************************************/
    	/* if the wait_flag is true, then check the DMAbuf 	*/
    	/* pointer.  if the DMAbuf pointer is nil, then queue 	*/
    	/* a no-op DISPLIST request that will wait for the FIFO */
    	/* to be empty before it signals DMAdone.  if the 	*/
    	/* DMAbuf pointer is not nil, then change the done	*/
    	/* condition of the existing DMA request to be 		*/
    	/* FIFO_EMPTY.  this will ensure that dma activity is 	*/
    	/* completely finished					*/
    	/********************************************************/

    	if (wait_flag == TRUE)
    		if (DMAbuf == 0) {
    			Get_dma ( FIFO_EMPTY );
    			reqst = DMArequest;
    			Need_dma ( 3 );
    			*p++ = (PNT(REQUEST_ENABLE));
    			*p++ = (ADDRESS);
    			*p++ = (PNT(ADDRESS_COUNTER));
    			Confirm_dma();
    		} else {
    			reqst->DMAdone |= FIFO_EMPTY;
    			reqst->DMAdone &= ~COUNT_ZERO;
    		}

#ifdef undef
    	if (reqst->DMAtype == DISPLIST)
#endif
    		reqst->length = (dma_word - DMAbuf) * sizeof(short);

    	/********************************************************/
    	/* update pointers and counters in the DMA request 	*/
    	/* queue header						*/
    	/********************************************************/
    
    	DMA_PUTEND (DMAhdr);
    
    
    	/********************************************************/
    	/* test to see if there is a DMA in progress, or if 	*/
    	/* this is a "cold start"				*/
    	/********************************************************/
    
    	if (! DMA_ISACTIVE (DMAhdr) && ! DMA_ISEMPTY (DMAhdr))
    		{
    		/************************************************/
    		/* determine if transfers to/from ADDER need to	*/
    		/* temporarily inhibited.  this is a MUST if 	*/
    		/* we are switching from BTOP to DISPLIST mode.	*/
    		/* also, if we are changing modes than we need	*/
    		/* shut down DMA operations.			*/
    		/************************************************/

    		if ( reqst->DMAtype != last_DMAtype ) {
    			AdderPtr[REQUEST_ENABLE] = 0;
    			Dga->csr &= ~DMA_IE;
    			Dga->csr &= HALT_DMA_OPS;
    		}

    		/**********************/
    		/* mark DMA as active */
    		/**********************/

    		DMA_SETACTIVE (DMAhdr);

    		/************************************************/
    		/* disable interrupt since setting COUNT ZERO	*/
    		/* condition could trigger interrupt during	*/
    		/* hardware set up				*/
    		/************************************************/

    		dga->csr &= ~DMA_IE;


    		/********************************/
    		/* clear any error condition	*/
    		/********************************/

    		dga->csr |= (DMA_ERR);
    	
    		/************************************************/
    		/* set up condition bits (10, 9, and 8) in the 	*/
    		/* csr.  also, load appropriate value into 	*/
    		/* Adder's REQUEST_ENABLE register.		*/
    		/************************************************/

    		switch (reqst->DMAtype)
    			{
    			case DISPLIST:
    				dga->csr |= DL_ENB;
    				dga->csr &= ~(BTOP_ENB | BYTE_DMA);
    				AdderPtr[REQUEST_ENABLE] = RASTEROP_COMPLETE;
    				break;
    			case PTOB:
    				if (reqst->DMAdone & BYTE_PACK)
    					dga->csr |= (PTOB_ENB | BYTE_DMA);
    				else {
    					dga->csr |= PTOB_ENB;
    					dga->csr &= ~BYTE_DMA;
    				}
    				AdderPtr[REQUEST_ENABLE] = TX_READY;
    				break;
    			case BTOP:
    				if (reqst->DMAdone & BYTE_PACK) {
    					dga->csr &= ~DL_ENB;
    					dga->csr |= (BTOP_ENB | BYTE_DMA);
    				} else {
    					dga->csr |= BTOP_ENB;
    					dga->csr &= ~(BYTE_DMA | DL_ENB);
    				}
    				AdderPtr[REQUEST_ENABLE] = RX_READY;
    				break;
    			}


    		/************************************************/
    		/* record request DMAtype for future calls to	*/
    		/* this routine					*/
    		/************************************************/

    		last_DMAtype = reqst->DMAtype;

    		if (reqst->DMAdone & COUNT_ZERO)
    			/****************************************/
    			/* set the DMADONE bit when the dma 	*/
    			/* byte counter (dga->bytcnt_lo and 	*/
    			/* dga->bytcnt_hi) is 0			*/
    			/****************************************/

    			dga->csr &= ~SET_DONE_FIFO;

    		else if (reqst->DMAdone & FIFO_EMPTY)
    			/****************************************/
    			/* set the DMADONE bit when the dma 	*/
    			/* byte counter is zero, and when the 	*/
    			/* FIFO is empty			*/
    			/****************************************/

    			dga->csr |= SET_DONE_FIFO;

    		/************************************************/
    		/* calculate the next cookie.  it's the offset 	*/
    		/* of the buffer into the shared space plus 	*/
    		/* the value returned by the driver in 		*/
    		/* DMAheader->QBAreg				*/
    		/************************************************/
    	
    		cookie = ((int)DMAbuf-(int)DMAhdr) + (int)DMAhdr->QBAreg;

    		/********************************/
    		/* set up destination address	*/
    		/********************************/
    		dga->adrs_lo = (unsigned short) cookie;
    		dga->adrs_hi = (unsigned short) (cookie >> 16);
    	
    		/****************************************/
    		/* set up byte length of transfer	*/
    		/****************************************/
    		dga->bytcnt_lo = (unsigned short) reqst->length;
    		dga->bytcnt_hi = (unsigned short) 
    			(reqst->length) >> 16;

    		/********************************/
    		/* set up the interupt enable	*/
    		/********************************/

    		dga->csr |= DMA_IE;
    		}

    	/********************************************************/
    	/* if the wait flag is true, then wait for the DMA	*/
    	/* to complete.						*/
    	/********************************************************/
    	if (wait_flag == TRUE) {
            while (DMA_ISACTIVE (DMAheader)) {
        	selmask = 1 << fd_qdss;
    		select( fd_qdss + 1, 0, &selmask, 0, &timeout);
    	    }
    	}
    			
    }
    /*
     * wait for the last adder pixel address to be generated
     */
    if (wait_flag == TRUE)
	while ( (AdderPtr[STATUS] & ADDRESS) != ADDRESS) /* ADDRESS_COMPLETE */
	    ;
    DMAbuf = 0;
    dma_word = req_buf_max = (u_short *) req_buf_size;
}

Flush_fifo (wait_flag)
    int	wait_flag;	/* wait for adder address completion before returning */
{
    register u_short *DMAbuf_ptr;

    /****************************************************************/
    /* *dga MUST be a register variable to ensure that the 		*/
    /* optimizer generates instructions that can be performed on 	*/
    /* device registers						*/
    /****************************************************************/

    register struct dga *dga = Dga;
    register VOLATILE struct  DMAreq_header	*DMAhdr = DMAheader;
    register u_short *p;
    register VOLATILE struct fcc *sgfcc;
    u_short  fifo_address;
    register struct FIFOreq *reqst = (struct FIFOreq *) FIFOrequest;

    struct timeval timeout;
    int selmask;

    /*
     * timeout needs to be small, to accomodate race condition in loop
     */
    timeout.tv_sec = (long) 0;
    timeout.tv_usec = (long) 17000;	/* 1/60 sec. */

    sgfcc = fcc_cbcsr;
    if ((wait_flag == TRUE) || (DMAbuf != 0)) {
    	/********************************************************/
    	/* if the wait_flag is true, then check the DMAbuf 	*/
    	/* pointer.  if the DMAbuf pointer is nil, then queue 	*/
    	/* a no-op request that will wait for the FIFO to be 	*/
    	/* empty before it signals DMAdone.  if the DMAbuf 	*/
    	/* pointer is not nill, then change change the done 	*/
    	/* condition of the existing DMA request to be 		*/
    	/* FIFO_EMPTY.  this will ensure that dma activity is 	*/
    	/* completely finished					*/
    	/********************************************************/

    	if (wait_flag == TRUE)
    		if (DMAbuf == 0) {
    			while (sgfcc->fwused) ; /* ?? */
    			Need_dma ( 3 );
    			*p++ =  ( PNT(REQUEST_ENABLE) );
    			*p++ =  ( ADDRESS );
    			*p++ =  ( PNT ( ADDRESS_COUNTER ) );
    			Confirm_dma();
    		}
    
    	/**********************************************************/
    	/* adjust the pointer into the next available word in the */
    	/* FIFO (multiply dma_word by sizeof short (remember,     */
    	/* DMAbuf is a short).					*/
    	/********************************************************/
    
#ifdef undef
    	if (sg_fifo_type == DISPLIST)
#endif
    		*sg_next_fifo += (dma_word - DMAbuf);
    
    	/********************************************************/
    	/* test to see if there is a FIFO in progress, or if 	*/
    	/* this is a "cold start"				*/
    	/********************************************************/
    
    	if (*sg_int_flag == -1) {
    		/************************************************/
    		/* enable the adder for rasterop complete	*/
    		/************************************************/
    		if ( sg_fifo_type != last_DMAtype )
    			AdderPtr[REQUEST_ENABLE] = RASTEROP_COMPLETE;

    		/****************************************/
    		/* set up condition bits in the csr	*/
    		/****************************************/
    		fifo_address = sgfcc->cbcsr & 0x3;
    		switch (sg_fifo_type) {
    		case DISPLIST:
    		    if ((sg_fifo_type != last_DMAtype) &&
    		           (last_DMAtype != (-1))) {
    			sgfcc->cbcsr &= 0xE0FC;
    			sgfcc->cbcsr |= (DL_ENB | fifo_address);
    			AdderPtr[REQUEST_ENABLE] = TX_READY;
    		    }
    		    if (*change_section == 1)
    			break;
    		    sgfcc->put = *sg_next_fifo;
    		    break;

    		case PTOB:
    		    if (sg_fifo_type != last_DMAtype) {
    			while (sgfcc->fwused) ; /* ?? */
    			sgfcc->cbcsr &= 0xE0FC;
    		    }
    		    AdderPtr[REQUEST_ENABLE] = TX_READY;
        	    sgfcc->icsr &= ~ENTHRSH;
    		    sgfcc->put = *sg_next_fifo;
    		    sgfcc->thresh = 0x0000;
    		    *(unsigned long *)&sgfcc->cbcsr =
    		       (unsigned long)((ENTHRSH<<16)|sg_fifo_func|fifo_address);
    		    sg_fifo_func = 0;
    		    break;

    		case BTOP:
    		    if (sg_fifo_type != last_DMAtype) {
    			while (sgfcc->fwused) ; /* ?? */
    			sgfcc->cbcsr &= 0xE0FC;
    		    }

    		    AdderPtr[REQUEST_ENABLE] = RX_READY;
       		    sgfcc->icsr &= ~ENTHRSH;
		    sgfcc->thresh = (dma_word - DMAbuf) * 2;
    		    *(unsigned long *)&sgfcc->cbcsr =
    	   	       (unsigned long)((ENTHRSH<<16)|sg_fifo_func|fifo_address);
    		    sg_fifo_func = 0;
    		    break;
    		}
    		last_DMAtype = sg_fifo_type;
    	}

    	/********************************************************/
    	/* if the input flag is true, then wait for the FIFO    */
    	/* to complete.						*/
    	/********************************************************/
    	if (wait_flag == TRUE) {
    		while (*sg_int_flag != -1) ;
    		while ((sgfcc->fwused) || (*change_section == 1)) ; /* ?? */
    		while ( (AdderPtr[STATUS] & ADDRESS) != ADDRESS);
    	}
    }
    DMAbuf = 0;
    dma_word = req_buf_max = (u_short *) req_buf_size;
}


Get_dma (condition)
    short condition; /* to be put in new packet header's DMAdone field */
{
    struct timeval timeout;
    int selmask;

    /*
     * timeout needs to be small, to accomodate race condition in loop
     */
    timeout.tv_sec = (long) 1;
    timeout.tv_usec = (long) 0;

    /*
     * assume WAKE_QUEUEEMPTY is set
     */
    while ( DMA_ISFULL(DMAheader)) {
        selmask = 1 << fd_qdss;
        select( fd_qdss + 1, 0, &selmask, 0, &timeout);
    }

    DMArequest = DMA_PUTBEGIN (DMAheader);
    DMArequest->DMAtype = DISPLIST;
    DMArequest->DMAdone &= ~REQUEST_DONE;	/* A NOOP !!  XXX dwm */
    DMArequest->DMAdone = condition ;

    /*
     * reinit DMAbuf and the pointer into it
     */
    
    dma_word = DMAbuf = (u_short *) DMArequest->bufp;
    req_buf_max = DMAbuf + req_buf_size;
}

Get_fifo (condition)
short condition; /* to be put in new packet header's DMAdone field */
{
    register VOLATILE struct fcc *sgfcc;
    u_short cur_section;
    u_short	next_section;
    struct timeval timeout;
    int selmask;

    sgfcc = fcc_cbcsr;

    /*
     * Check for FIFO wrapping
     */

    cur_section = *sg_next_fifo >> 14;	/* current section of FIFO */
    next_section = (*sg_next_fifo + req_buf_size) >> 14; /* next section */
    if ((cur_section != next_section) || (*sg_next_fifo > 0x7FFF)) {
	while ((sgfcc->fwused) || (*change_section == 1)) ; /* ?? */
    	if (next_section == 2)
    		*sg_next_fifo = 0;
    	else
    		*sg_next_fifo = next_section * 0x4000;

    /*
     * Modify CBCSR register when going into a new section.
     */

    	sgfcc->cbcsr = (sgfcc->cbcsr + 1) & 0xFFFD;
    	*change_section = 1;
    	sgfcc->thresh = 0x0000;
    	*(unsigned long *)&sgfcc->cbcsr =
    	 	(unsigned long)((ENTHRSH << 16) | sgfcc->cbcsr);
    }
    /*
     * reinit DMAbuf and the pointer into it
     */
	
	sg_fifo_type = DISPLIST;


	dma_word = DMAbuf = (u_short *) SG_Fifo + (*sg_next_fifo & 0x3FFF);
	req_buf_max = DMAbuf + req_buf_size;
}
