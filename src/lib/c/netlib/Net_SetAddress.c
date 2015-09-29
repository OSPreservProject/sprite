/* 
 * Net_SetAddress.c --
 *
 *	Routines to convert Net_Address's to and from network specific
 *	address types.
 *
 * Copyright 1992 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that this copyright
 * notice appears in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 */

#ifndef lint
static char rcsid[] = "$Header: /sprite/lib/forms/RCS/proto.c,v 1.6 92/03/02 15:29:56 bmiller Exp $ SPRITE (Berkeley)";
#endif /* not lint */

#include <sprite.h>
#include <net.h>


/*
 *----------------------------------------------------------------------
 *
 * Net_SetAddress --
 *
 *	Converts the network-specific address into a Net_Address.
 *
 * Results:
 *	SUCCESS if the conversion was successful, FAILURE otherwise
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
Net_SetAddress(type, specificPtr, addrPtr)
    Net_AddressType		type;		/* Type of specific address. */
    Address			specificPtr;	/* The specific address. */
    Net_Address			*addrPtr;	/* Result. */
{
    switch(type) {				
	case NET_ADDRESS_ETHER:				
	    addrPtr->type = NET_ADDRESS_ETHER;		
	    NET_ETHER_ADDR_COPY((*(Net_EtherAddress *) specificPtr),
		addrPtr->address.ether);			
	    break;						
	case NET_ADDRESS_ULTRA:					
	    addrPtr->type = NET_ADDRESS_ULTRA;			
	    Net_UltraAddrCopy((*(Net_UltraAddress *) specificPtr),
		addrPtr->address.ultra);			
	    break;						
	case NET_ADDRESS_FDDI:					
	    addrPtr->type = NET_ADDRESS_FDDI;			
	    NET_FDDI_ADDR_COPY((*(Net_FDDIAddress *) specificPtr),
		addrPtr->address.fddi);				
	    break;						
	case NET_ADDRESS_INET:					
	    addrPtr->type = NET_ADDRESS_INET;			
	    Net_InetAddrCopy((*(Net_InetAddress *) specificPtr),
		addrPtr->address.inet);			
	    break;
	default:
	    return FAILURE;
    }			
    return SUCCESS;
}

/*
 *----------------------------------------------------------------------
 *
 * Net_GetAddress --
 *
 *	Converts a Net_Address into a network-specific address.
 *
 * Results:
 *	SUCCESS if the conversion was successful, FAILURE otherwise
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
Net_GetAddress(addrPtr, specificPtr)
    Net_Address		*addrPtr;	/* Address to convert. */
    Address		specificPtr;	/* Result. */
{
    switch(addrPtr->type) {				
	case NET_ADDRESS_ETHER:				
	    NET_ETHER_ADDR_COPY(addrPtr->address.ether, 
		(*(Net_EtherAddress *) specificPtr));	
	    break;					
	case NET_ADDRESS_ULTRA:				
	    Net_UltraAddrCopy(addrPtr->address.ultra, 	
		(*(Net_UltraAddress *) specificPtr));	
	    break;					
	case NET_ADDRESS_FDDI:				
	    NET_FDDI_ADDR_COPY(addrPtr->address.fddi, 	
		(*(Net_FDDIAddress *) specificPtr));	
	    break;					
	case NET_ADDRESS_INET:				
	    Net_InetAddrCopy(addrPtr->address.inet, 	
		(*(Net_InetAddress *) specificPtr));	
	    break;					
	default:
	    return FAILURE;
    }							
    return SUCCESS;
}


