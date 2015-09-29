#ifndef PCF_H

#define	PCF_FILE_VERSION	(('p'<<24)|('c'<<16)|('f'<<8)|1)

#define	PCF_DEFAULT_FORMAT	0x00000000
#define	PCF_COMPRESSED_METRICS	0x00000100

typedef struct {
	Mask label;
	int format;	/* format of data */
	int size;	/* length in bytes for this table */
	int start;	/* byte offset from front of file for this table */
} TableDesc;

extern	Mask	pcfReadFont(/* file, ppFont, ppCS, tables, params */);
#endif /* PCF_H */
