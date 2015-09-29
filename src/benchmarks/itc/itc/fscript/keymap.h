/*************************************************************\
* 							      *
* 	James Gosling, 1984				      *
* 	Copyright (c) 1984 IBM				      *
* 							      *
* Structure definitions for keystroke mapping tables.  These  *
* are used by the window manager.			      *
* 							      *
\*************************************************************/


struct LoadedKeymap {
    int PrefixTableLength;	/* The number of PrefixTable entries */
    struct PrefixTable {
	int PackedPrefixKeys,	/* The keystrokes that correspond to this
				   entry.  Eg. for the table that maps the
				   keystrokes prefixed by ^X this would have
				   the value 'x'&037 */
            FirstKey,		/* The lowest character value mapped by this
				   table */
            NumberOfKeys;	/* The number of characters in this table */
    } PrefixTable[1 /* flex */];
    /* int offsets[] */		/* The offset to an offset is computed by
				   adding the character value with the length
				   of all preceeding tables */
    /* char strings[][] */
};
