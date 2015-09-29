
int  Inquiry();
void moveMedium();
void modeSel();
void pos();
void prevRem();
void sendCmd();

typedef struct PositionRobotCommand {
#if BYTE_ORDER == BIG_ENDIAN
    unsigned char command;
    unsigned char unitNumber		:3;
    unsigned char reserved		:5;
    unsigned char transpElemAddr[2];
    unsigned char destAddr[2];
    unsigned char reserved2[2];
    unsigned char reserved3		:7;
    unsigned char invert		:1;
    unsigned char reserved4;
#else /* BYTE_ORDER == LITTLE_ENDIAN */
    unsigned char command;
    unsigned char reserved		:5;
    unsigned char unitNumber		:3;
    unsigned char transpElemAddr[2];
    unsigned char destAddr[2];
    unsigned char reserved2[2];
    unsigned char invert		:1;
    unsigned char reserved3		:7;
    unsigned char reserved4;
#endif
} PositionRobotCommand;


typedef struct PreventRemovalCommand {
#if BYTE_ORDER == BIG_ENDIAN
    unsigned char command;
    unsigned char unitNumber		:3;
    unsigned char reserved		:5;
    unsigned char reserved2[2];
    unsigned char reserved3		:7;
    unsigned char prevent		:1;
    unsigned char reserved4;
#else /* BYTE_ORDER == LITTLE_ENDIAN */
    unsigned char command;
    unsigned char reserved		:5;
    unsigned char unitNumber		:3;
    unsigned char reserved2[2];
    unsigned char prevent		:1;
    unsigned char reserved3		:7;
    unsigned char reserved4;
#endif
} PreventRemovalCommand;

typedef struct MoveMediumCommand {
#if BYTE_ORDER == BIG_ENDIAN
    unsigned char command;
    unsigned char unitNumber	:3;
    unsigned char reserved	:5;
    unsigned char transpElemAddr[2];
    unsigned char sourceAddr[2];
    unsigned char destAddr[2];
    unsigned char reserved2[2];
    unsigned char reserved3	:7;
    unsigned char invert	:1;
    unsigned char eePos		:2;
    unsigned char reserved4	:4;
    unsigned char finalBits	:2; 	/* These are fixed at 0. */
#else /* BYTE_ORDER == LITTLE_ENDIAN */
    unsigned char command;
    unsigned char reserved	:5;
    unsigned char unitNumber	:3;
    unsigned char transpElemAddr[2];
    unsigned char sourceAddr[2];
    unsigned char destAddr[2];
    unsigned char reserved2[2];
    unsigned char invert	:1;
    unsigned char reserved3	:7;
    unsigned char finalBits	:2;	/* These are fixed at 0. */
    unsigned char reserved4	:4;
    unsigned char eePos		:2;
#endif
} MoveMediumCommand;


typedef struct ModeSelectCommand {
#if BYTE_ORDER == BIG_ENDIAN
    unsigned char command;		/* 0x15, for Mode Select */
    unsigned char unitNumber	:3;	/* Logical Unit Number to which
					 * to pass the command. */
    unsigned char pageFormat	:1;	/* 1 == SCSI-2;
					 * Must be 1 for Exabyte robot. */
    unsigned char reserved	:3;	
    unsigned char savedPage	:1;	/* 0 == Changes are not permanent.
					 * 1 == Changes are permanent
					 * (stored in non-volatile memory).*/
    unsigned char reserved2[2];
    unsigned char paramListLength;	/* Length of the entire parameter
					 * list. */
    unsigned char vendorUnique	:2;	
    unsigned char reserved3	:4;
    unsigned char flag		:1;	/* Interrupt after linked command. */
    unsigned char link		:1;	/* Another command follows. */
#else /* BYTE_ORDER == LITTLE_ENDIAN */
    unsigned char command;		/* 0x15, for Mode Select */
    unsigned char savedPage	:1;	/* 0 == Changes are not permanent.
					 * 1 == Changes are permanent
					 * (stored in non-volatile memory).*/
    unsigned char reserved	:3;
    unsigned char pageFormat	:1;	/* 1 == SCSI-2;
					 * Must be 1 for Exabyte robot. */
    unsigned char unitNumber	:3;	/* Logical Unit Number to which
					 * to pass the command. */
    unsigned char reserved2[2];
    unsigned char paramListLength;	/* Length of the entire parameter
					 * list. */
    unsigned char link		:1;	/* Another command follows. */
    unsigned char flag		:1;	/* Interrupt after linked command. */
    unsigned char reserved3	:4;
    unsigned char vendorUnique	:2;
#endif
} ModeSelectCommand;


typedef struct ExbRobotModeSelVendorUnique {
#if BYTE_ORDER == BIG_ENDIAN
    unsigned char reserved	:2;
    unsigned char pageCode	:6;	/* Identifies the vendor unique
					 * parameter list. Must be 0x0. */
    unsigned char paramListLength;	/* Length of the vendor unique
					 * parameter list. It is equal to
					 * 62. */
    unsigned char aInit		:1;	/* 0 == Perform INITIALIZE ELEMENT
					 *      STATUS (0xE7) only when the
					 *      command is issued.
					 * 1 == Perform INITIALIZE ELEMENT
					 *      STATUS after power-up. */
    unsigned char uInit		:1;	/* 0 == Do not perform INIT ELEMENT
					 *      STATUS on a cartridge each time
					 *      it is handled by the CHM.
					 * 1 == Perform INIT ELEMENT STATUS
					 *      each time. */
    unsigned char parity	:1;	/* 0 == Enable SCSI bus parity checking.
					 * 1 == Disable SCSI bus parity checking. */
    unsigned char reserved2	:2;	
    unsigned char notReadyDispCntrl:1;	/* 0 == "Not ready" message
					 *      is flashing.
					 * 1 == Message is steady. */
    unsigned char mesgDispCntrl	:2;	/* Determines how displayMessage
					 * is displayed.
					 * 0 == Flashing 8-char display.
					 * 1 == Steady 8-char display.
					 * 2 == Scrolling display (up to 60 chars). */
    unsigned char reserved3;
    char displayMessage[60];		/* Specifies the ready message that
					 * appears on the operator display when
					 * the robot is ready for operation. */
#else /* BYTE_ORDER == LITTLE_ENDIAN */
    unsigned char pageCode	:6;	/* Identifies the vendor unique
					 * parameter list. Must be 0x0. */
    unsigned char reserved	:2;

    unsigned char paramListLength;	/* Length of the vendor unique
					 * parameter list. It is equal to
					 * 62. */
    unsigned char mesgDispCntrl	:2;	/* Determines how displayMessage
					 * is displayed.
					 * 0 == Flashing 8-char display.
					 * 1 == Steady 8-char display.
					 * 2 == Scrolling display (up to 60 chars). */
    unsigned char notReadyDispCntrl	:1;	/* 0 == "Not ready" message
						 *      is flashing.
						 * 1 == Message is steady. */
    unsigned char reserved2	:2;
    unsigned char parity	:1;	/* 0 == Enable SCSI bus parity checking.
					 * 1 == Disable SCSI bus parity checking. */

    unsigned char uInit		:1;	/* 0 == Do not perform INIT ELEMENT
					 *      STATUS on a cartridge each time
					 *      it is handled by the CHM.
					 * 1 == Perform INIT ELEMENT STATUS
					 *      each time. */
    unsigned char aInit		:1;	/* 0 == Perform INITIALIZE ELEMENT
					 *      STATUS (0xE7) only when the
					 *      command is issued.
					 * 1 == Perform INITIALIZE ELEMENT
					 *      STATUS after power-up. */
    unsigned char reserved3;
    char displayMessage[60];		/* Specifies the ready message that
					 * appears on the operator display when
					 * the robot is ready for operation. */
#endif
} ExbRobotModeSelVendorUnique;
    


/* Minor differences between ModeSelect and ModeSense. */

typedef struct ExbRobotElemAddrAssign {
#if BYTE_ORDER == BIG_ENDIAN
    unsigned char pageSalvable	:1;	/* Specifies that the EXB-120 is
					 * capable of saving this page
					 * to non-volatile memory. */
    unsigned char reserved	:1;
    unsigned char pageCode	:6;	/* Identifies the Element Address
					 * Assignment parameter list. The
					 * value must be 0x1d. */
#else /* BYTE_ORDER == LITTLE_ENDIAN */
    unsigned char pageCode	:6;	/* Identifies the Element Address
					 * Assignment parameter list. The
					 * value must be 0x1d. */
    unsigned char reserved	:1;
    unsigned char pageSalvable	:1;	/* Specifies that the EXB-120 is
					 * capable of saving this page
					 * to non-volatile memory. */
#endif    
    unsigned char paramListLength;	/* Length of the element address
					 * assignment parameter list. */
    unsigned char transpElemAddr[2];	/* This identifies the address of
					 * the robot arm. */
    unsigned char transpElemCount[2];	/* Number of robot arms. Valid value
					 * for this field is 1. */
    unsigned char firstStorElemAddr[2]; /* Identifies the starting address
					 * of the data cartridge storage
					 * locations. */
    unsigned char storElemCount[2];	/* The number of data cartridge
					 * storage locations within
					 * EXB-120. Max == 116. */
    unsigned char firstEeElemAddr[2];	/* Address of the first Entry Exit Port. */
    unsigned char eeElemCount[2];	/* Number of Entry Exit Ports.
					 * (only valid == 1)*/
    unsigned char firstTapeDriveAddr[2];
                                        /* First address  */
    unsigned char tapeDriveCount[2];	/* Number of tape drives. Ranges from 1-4. */
    unsigned char reserved2[2];		
} ExbRobotElemAddrAssign;


typedef struct InquiryCommand {
#if BYTE_ORDER == BIG_ENDIAN
    unsigned char command;		/* 0x12 for SCSI Inquiry. */

    unsigned char unitNumber	:3;     /* Logical Unit Number to which
				         * to pass the command. */
    unsigned char reserved	:4;     
    unsigned char evpd		:1;     /* Enable Vital Product Data. Selects
					 * The type of inquiry data requested
					 * by the initiator. */
    unsigned char pageCode;		
    unsigned char reserved2;		
    unsigned char allocLength;		/* The number of bytes that the
					 * initiator has allocated for
					 * data returned from the Inquiry
					 * command. */
    unsigned char vendorUnique	:2; 	/* Vendor Unique bits. */
    unsigned char reserved3	:4;
    unsigned char flag		:1; 	/* Interrupt after linked command. */
    unsigned char link		:1; 	/* Another command follows. */
#else /* BYTE_ORDER == LITTLE_ENDIAN */
    unsigned char command;		/* 0x12 for SCSI Inquiry. */

    unsigned char evpd		:1;	/* Enable Vital Product Data. Selects
					 * The type of inquiry data requested
					 * by the initiator. */
    unsigned char reserved	:4;
    unsigned char unitNumber	:3;	/* Logical Unit Number to which
					 * to pass the command. */
    unsigned char pageCode;
    unsigned char reserved2;
    unsigned char allocLength;		/* The number of bytes that the
					 * initiator has allocated for data
					 * returned from the Inquiry command.
					 */
    unsigned char link		:1;	/* Another command follows. */
    unsigned char flag		:1;	/* Interrupt after linked command. */
    unsigned char reserved3	:4;
    unsigned char vendorUnique	:2;	/* Vendor Unique bits. */
#endif
} InquiryCommand;


typedef struct ModeSenseCommand {
#if BYTE_ORDER == BIG_ENDIAN
    unsigned char command;		/* 0x1a for Mode Sense. */
    unsigned char unitNumber	:3;	/* Logical Unit Number to which to
					 * pass the command. */
    unsigned char reserved	:1;
    unsigned char disableBlkDescrptr:1;	/* Not used on Exabyte EXB-120. */
    unsigned char reserved2	:3;
    unsigned char pageControl	:2;	/* Defines the type of parameters
					 * that are to be returned.
					 * 0 == Current values.
					 * 1 == Values which as changeable.
					 * 2 == Default values.
					 * 3 == Saved values. */
    unsigned char pageCode	:6;	/* Specifies which pages are to be
					 * returned.
					 * 0x1d == Element Address Assgnmt Pg.
					 * 0x1e == Transport Geometry Pg.
					 * 0x1f == Device Capabilities Pg.
					 * 0x0  == Vendor Unique Pg.
					 * 0x3f == All pages (in the above order). */
    unsigned char reserved3;
    unsigned char allocLength;		/* Specifies the maximum length
					 * of the parameter list to be
					 * returned. Max == 112 (0x70). */
    unsigned char vendorUnique	:2;	
    unsigned char reserved4	:4;
    unsigned char flag		:1;	/* Interrupt after linked command. */
    unsigned char link		:1;	/* Another command follows. */
#else /* BYTE_ORDER == LITTLE_ENDIAN */
    unsigned char command;		/* 0x1a for Mode Sense. */
    unsigned char reserved2	:3;
    unsigned char disableBlkDescrptr:1;	/* Not used on Exabyte EXB-120. */    
    unsigned char reserved	:1;
    unsigned char unitNumber	:3;	/* Logical Unit Number to which to
					 * pass the command. */
    unsigned char pageCode	:6;	/* Specifies which pages are to be
					 * returned.
					 * 0x1d == Element Address Assgnmt Pg.
					 * 0x1e == Transport Geometry Pg.
					 * 0x1f == Device Capabilities Pg.
					 * 0x0  == Vendor Unique Pg.
					 * 0x3f == All pages (in the above order). */
    unsigned char pageControl	:2;	/* Defines the type of parameters
					 * that are to be returned.
					 * 0 == Current values.
					 * 1 == Values which as changeable.
					 * 2 == Default values.
					 * 3 == Saved values. */
    unsigned char reserved3;
    unsigned char allocLength;		/* Specifies the maximum length
					 * of the parameter list to be
					 * returned. Max == 112 (0x70). */
    unsigned char link		:1;	/* Another command follows. */
    unsigned char flag		:1;	/* Interrupt after linked command. */
    unsigned char reserved4	:4;
    unsigned char vendorUnique	:2;
#endif
} ModeSenseCommand;

typedef struct ExbRobotParamListHeader {
    unsigned char senseDataLength;
    unsigned char reserved[3];
} ExbRobotParamListHeader;


typedef struct ReserveCommand {
#if BYTE_ORDER == BIG_ENDIAN
    unsigned char command;		/* 0x16 for Reserve Command. */
    unsigned char unitNumber	:3;	/* Logical Unit Number to which to
					 * pass the command. */
    unsigned char thirdParty	:1;
    unsigned char thirdPartyDevID:3;
    unsigned char extent	:1;	/* 0 == The device is reserved.
					 * 1 == A series of elements,
					 *      identified by the reserveID
					 *      field and specified by the
					 *      element list descriptor
					 *      are reserved. A minimum of
					 *      six bytes must be sent by
					 *      the initiator. */
    unsigned char reserveID;
    unsigned char elemListLength[2];
    unsigned char vendorUnique	:2;
    unsigned char reserved	:4;
    unsigned char flag		:1;
    unsigned char link		:1;
#else /* BYTE_ORDER == LITTLE_ENDIAN */
    unsigned char command;
    unsigned char extent	:1;	/* 0 == The device as a whole
					 *      is reserved.
					 * 1 == A series of elements,
					 *      identified by the reserveID
					 *      field and specified by the
					 *      element list descriptor
					 *      are reserved. A minimum of
					 *      six bytes must be sent by
					 *      the initiator. */
    unsigned char thirdPartyDevID:3;
    unsigned char thirdParty	:1;
    unsigned char unitNumber	:3;	/* Logical Unit Number to which to
					 * pass the command. */
    unsigned char reserveID;
    unsigned char elemListLength[2];
    unsigned char link		:1;    
    unsigned char flag		:1;
    unsigned char reserved	:4;
    unsigned char vendorUnique	:2;
#endif
} ReserveCommand;


typedef struct ElemListDescriptor {
    unsigned char reserved[2];
    unsigned char elemCount[2];		/* The number of elements of
					 * a specific type to be reserved. */
    unsigned char elemAddr[2];		/* The address of the element or
					 * starting address of a series
					 * of elements to be reserved. */
} ElemListDescriptor;
    

/* end of robot.h */



