/* Unix(TM) CD-ROM file copy utility  by
		Matthew B. Hornbeck, Director of Technical Services
		Copyright 1989, 1990, 1991, 1992 by Young Minds, Incorporated
		June 30, 1992.
File : CD_COPY.C

Note : On HP-UX machines, you must link this program using the BSD
	compatible library (i.e. "cc -o cd_link cd_link.c /usr/lib/libBSD.a")
*/

#define TRANSLATION_FILE	"00_TRANS.TBL;1"
#define RRIP_TRANSLATION_FILE	"00_TRANS.TBL"

#ifdef sun
#define REALPATH
#endif

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <malloc.h>
#include <sys/types.h>
#include <sys/dir.h>
#include <sys/stat.h>

#ifdef REALPATH
#include <sys/param.h>
#else
#define MAXPATHLEN	4096
#endif

#ifdef M_XENIX
#define getwd(x)	getcwd(x,2047)
#endif

#define MAX_TRANS_TYPES	(4)
#ifndef FALSE
#define FALSE	(0)
#define TRUE	(!FALSE)
#endif

/*
**	Bit masks for setting flags bit-vector from
**	command line args.
*/
#define RECURSE		(1)
#define ROCK_RIDGE	(2)

char*
get_program_name( argv0 )
	char	*argv0;
{
	char	*program_name;

	/*
	**	Gets the component of the command that
	**	occurs after the last '/'.
	*/
	program_name = strrchr( argv0, '/' );
	if (program_name == NULL)
		program_name = argv0;
	else
		program_name++;
	return( program_name );
}

void
usage( program_name, arg_list, error_message)
	char	*program_name;
	char	*arg_list;
	char	*error_message;
{
	fprintf( stderr, "Usage: %s %s\n", program_name, arg_list );
	fprintf( stderr, "\t%s\n", error_message );
}

typedef struct dir_element {
	struct dir_element *next;
	char *cd_path;
	char *new_path;
} dir_elem;

static dir_elem *head = NULL, *tail = NULL;

int 
push_dir (cd_path, new_path)
	char *cd_path, *new_path;

{
	dir_elem *tmp;

	if (head == NULL)
	  {	if ((head = tmp = (dir_elem *) malloc (sizeof (dir_elem))) == NULL)
		  {	fprintf (stderr, "Unable to allocate dir_element buffer!\n");
			return FALSE;
		  }
	  }
	else
	  {	if ((tail->next = tmp = (dir_elem *) malloc (sizeof (dir_elem))) == NULL)
		  {	fprintf (stderr, "Unable to allocate dir_element buffer!\n");
			return FALSE;
		  }
	  }
	tmp->next = NULL;
	if ((tmp->cd_path = (char *) malloc (strlen (cd_path) + 1)) == NULL)
	  {	fprintf (stderr, "Unable to allocate cd_path of dir_element!\n");
		return FALSE;
	  }
	strcpy (tmp->cd_path, cd_path);
	if ((tmp->new_path = (char *) malloc (strlen (new_path) + 1)) == NULL)
	  {	fprintf (stderr, "Unable to allocate new_path of dir_element!\n");
		return FALSE;
	  }
	strcpy (tmp->new_path, new_path);
	tail = tmp;
	return TRUE;
}

dir_elem *
dequeue_dir()

{	
	dir_elem *tmp;

	if (head == NULL)
	  {	tail = NULL;
		return NULL;
	  }
	tmp = head;
	head = tmp->next;
	tmp->next = NULL;
	return tmp;
}

void
free_dir (dir)
	dir_elem *dir;

{
	free (dir->cd_path);
	free (dir->new_path);
	free (dir);
}

void
translate_name( name, trans_type)
	char	*name;
	int		trans_type;
{
	int	i;

	/*
	**	Changes the name given according to one of the algorithms
	**	below.  The algorithm used is selected via the trans_type
	**	arguement to this function.
	*/
	switch( trans_type ) {
		case 0:
			/*
			**	No translation.  Use original name.
			*/
			break;

		case 1:
			/*
			**	All lower case.
			*/
			for (i = 0; i < strlen(name) ; i ++) {
				if (isupper (name [i]))
					name [i] = tolower (name [i]);
				else
					name [i] = name [i];
			}
			break;
		case 2:
			/*
			**	All lower case.  Strip ";version_no".
			*/
			for (i = 0; (name[i] != ';') && (i < strlen( name )); i ++) {
				if (isupper (name [i]))
					name [i] = tolower (name [i]);
				else 
					name [i] = name [i];
			}
			name[i] = '\0';
			break;
		case 3:
			/*
			**	All lower case.  Replace ";version_no" with "-version_no".
			*/
			for (i = 0; i < strlen( name ); i ++) {
				if ( name[i] == ';' )
					name[i] = '-';
				else if (isupper (name [i]))
					name [i] = tolower (name [i]);
				else 
					name [i] = name [i];
			}
			name[i] = '\0';
			break;
		default:
			fprintf(stderr, "translate_name: Unknown translation type.\n");
			exit( 1 );
	}
}

FILE*
open_trans( cd_path, trans_name, trans_type )
	char	*cd_path;
	char	*trans_name;
	int		*trans_type;
{
	FILE	*fp;
	char	*new_name;
	char	*name;
	int		i;

	/*
	**	Get space for resolved file name which consistes of the cd_path
	**	and the translated trans_name (new_name) concatinated together.
	*/
	if ((name = malloc( strlen( trans_name ) + strlen(cd_path) + 2 )) == NULL) {
		fprintf(stderr, "Error: Malloc failed.\n");
		exit( 1 );
	}
	/*
	**	Get space to put the translated trans_name.
	*/
	if ((new_name = malloc( strlen( trans_name ) + 1 )) == NULL) {
		fprintf(stderr, "Error: Malloc failed.\n");
		exit( 1 );
	}
	/*
	**	translate the trans_name using the translation type
	**	that was previously found, or if first time translation
	**	type defaults to 0.
	*/
	strcpy( new_name, trans_name );
	translate_name( new_name, *trans_type );
	/*
	**	Concatinate translated name and cd_path to get resolved name.
	*/
	sprintf( name, "%s/%s", cd_path, new_name );
	/*
	**	Attempt to open the file.
	**	If fopen fails then I will try some other translation types on
	**	on the trans_name.
	*/
	if ((fp = fopen (name, "rt")) != NULL) {
		free( new_name );
		free( name );
		return( fp );
	}
	/*
	**	Try translation types on trans_name until I can either
	**	open the file successfully or I run out of translation
	**	types.
	*/
	for (i = 0; i < MAX_TRANS_TYPES; i++) {
		strcpy( new_name, trans_name );
		translate_name( new_name, i );
		sprintf( name, "%s/%s", cd_path, new_name );
		if ((fp = fopen (name, "rt")) != NULL) {
			*trans_type = i;
			free( new_name );
			free( name );
			return( fp );
		}
	}
	/*
	**	Failed to open the file.
	**	Return NULL file descriptor to signal error.
	*/
	free( new_name );
	free( name );
	return( NULL );
}

int
rrip_proc_dir( cd_path, new_path, recurse )
	char	*cd_path;
	char	*new_path;
	int		recurse;
{
	FILE			*fp;
	char			line[MAXPATHLEN];
	char			file_name[MAXPATHLEN];
	char			link_buf[MAXPATHLEN];
	char			trans_name[MAXPATHLEN];
	char			link_name [MAXPATHLEN];
	char			new_name [MAXPATHLEN];
	char			resolved_name [MAXPATHLEN];
	char			type;
	int				num_fields;
	char			command[MAXPATHLEN];

	/*
	**	For each directory entry.  Get its type.  
	**	Depending on its type make a directory or symbolic link.
	**	If the type is a directory and directory recursion was
	**	asked for on the command line, then push it onto the
	**	stack to be proccessed later.
	*/
	sprintf( trans_name, "%s/%s", cd_path, RRIP_TRANSLATION_FILE );
	if ((fp = fopen( trans_name, "rt" )) == NULL ) {
		fprintf (stderr, "Unable to open translation file %s!\n", trans_name);
		return FALSE;
	}
	while (fgets( line, sizeof( line ), fp) != NULL) {
		/*
		**	Get the type of the file,
		**	the file name and the link name if this entry is a link.
		*/
		strcpy( link_name, "" );
		num_fields = sscanf( line, "%c %*s %s %s", 
											&type, file_name, link_name );

		if (strcmp( file_name, ".") == 0)
			continue;
		if (strcmp( file_name, "..") == 0)
			continue;
		sprintf (new_name, "%s/%s", new_path, file_name);
		switch (type) {
			case 'F' :	
				sprintf( command, "cp \"%s/%s\" %s", cd_path, 
														file_name, new_name);
				if (system ( command ) < 0)
					fprintf (stderr, "Unable to copy %s to %s!\n", 
												link_name, new_name);
					break;

			case 'L' :	
#ifdef M_XENIX
				fprintf (stderr, "Unable to make link %s to %s!\n", 
					link_name, new_name);
#else
				if (symlink (link_name, new_name) != 0)
					fprintf (stderr, "Unable to make link %s to %s!\n", 
						link_name, new_name);
#endif
					break;

			case 'D' :	
				mkdir (new_name, 0777);
				if (recurse) {
					sprintf (link_name, "%s/%s", cd_path, file_name);
					push_dir (link_name, new_name);
				}
				break;

			case 'M' :	
				mkdir (new_name, 0777);
				if (recurse) {	
					sprintf (link_buf, "%s/%s", cd_path, link_name);
#ifdef REALPATH
					realpath (link_buf, resolved_name);
#else
					strcpy (resolved_name, link_buf);
#endif
					push_dir (resolved_name, new_name);
				}
				break;
			default:
				fprintf(stderr, "proc_dir:	Unknown file type.\n");
				exit( 1 );
		}
	}
	fclose (fp);
	return TRUE;
}

int
iso9660_proc_dir( cd_path, new_path, recurse )
	char	*cd_path;
	char	*new_path;
	int		recurse;
{
	FILE			*fp;
	char			line [4096], link_name [4096], new_name [4096];
	char			trans_name [4096], resolved_name [MAXPATHLEN];
	char			type, elem1 [65], elem2 [2048], elem3 [2048];
	int 			line_cnt, elem_cnt, i, j;
	static int		trans_type = 0;
	char			command[MAXPATHLEN];

	sprintf (trans_name, "%s/%s", cd_path, TRANSLATION_FILE);
	if ((fp = open_trans( cd_path, TRANSLATION_FILE, &trans_type )) == NULL)
	  {	
		fprintf (stderr, "Unable to open file %s!\n", trans_name);
		return FALSE;
	  }
	line_cnt = 0;
	while (fgets (line, sizeof (line), fp) != NULL) {
		line_cnt ++;
		if ((strlen (line) < 19) || (line [1] != ' ') || (line [strlen (line) - 1] != '\n'))
		  {	fprintf (stderr, "Invalid %s file!?!\n", trans_name);
			exit (1);
		  }
		type = line [0];

		/*
		**	Get the ISO name.
		*/
		for (i = 2, j = 0; (line [i] != ' ') && (line [i] != '\t'); i ++, j ++)
			elem1 [j] = line [i];

		elem1 [j] = '\0';

		/*
		**	translate name to the same format that was required
		**	in order to open the "00_TRANS.TBL;1".
		*/
		translate_name( elem1, trans_type );
		/*
		**	Skip past white space.
		*/
		while ((line [i] == ' ') || (line [i] == '\t'))
			i ++;

		/*
		**	Get the unix name.
		*/
		for (j = 0; (line [i] != '\t') && (line [i] != '\n'); i ++, j ++)
			elem2 [j] = line [i];
		elem2 [j] = '\0';

		elem_cnt = 2;
		j = 0;
		if (line [i] == '\t') {
			/*
			**	Get name of file that this name is a link to 
			**	if this is a link.
			*/
			for (i ++; line [i] != '\n'; i ++, j ++)
				elem3 [j] = line [i];
			elem_cnt ++;
		}
		elem3 [j] = '\0';
		if ((line_cnt == 1) && (strcmp (elem1, ".") == 0))
			continue;
		if ((line_cnt == 2) && (strcmp (elem1, "..") == 0))
			continue;
		sprintf (new_name, "%s/%s", new_path, elem2);
		switch (type) {
			case 'F' :	
				sprintf( command, "cp \"%s/%s\" %s", cd_path, elem1, new_name);
				if (system ( command ) < 0)
					fprintf (stderr, "Unable to copy %s to %s!\n", 
																elem1, elem2);
					break;

			case 'L' :	
#ifdef M_XENIX
				fprintf (stderr, "Unable to make link %s to %s!\n", 
					elem1, elem3);
#else
				if (symlink (elem3, new_name) != 0)
					fprintf (stderr, "Unable to make link %s to %s!\n", 
						elem1, elem3);
#endif
					break;

			case 'D' :	
				mkdir (new_name, 0777);
				if (recurse) {
					sprintf (link_name, "%s/%s", cd_path, elem1);
					push_dir (link_name, new_name);
				}
				break;

			case 'M' :	
				mkdir (new_name, 0777);
				if (recurse) {	
					sprintf (link_name, "%s/%s", cd_path, elem3);
#ifdef REALPATH
					realpath (link_name, resolved_name);
#else
					strcpy (resolved_name, link_name);
#endif
					push_dir (resolved_name, new_name);
				}
				break;
			default:
				fprintf(stderr, "proc_dir:	Unknown file type.\n");
				exit( 1 );
		}
	}
	fclose (fp);
	return TRUE;
}

int 
proc_dir (cd_path, new_path, flags)
	char *cd_path, *new_path;
	int flags;

{	
	int	recurse;

	/*
	**	If command line arguement "-r" was specified then
	**	recurse down subdirectories.
	*/
	if ((flags & RECURSE) != 0)
		recurse = TRUE;
	else
		recurse = FALSE;

	/*
	**	If command line arguement "-R" was specified then
	**	ignore the 00_TRANS.TBL and create links with the 
	**	same names as the names that exist in the directory
	**	entries on the disk.
	**
	**	This is most useful on Rock Ridge disks where the
	**	name in the directory entry is the name that should
	**	be used.
	*/
	if ((flags & ROCK_RIDGE) != 0 )
		rrip_proc_dir( cd_path, new_path, recurse );
	else
		iso9660_proc_dir( cd_path, new_path, recurse );
}

int main (argc, argv)
int argc;
char *argv [];

{	dir_elem	*cur_dir;
	char		cd_pathname [2048];
	char		target_dirname[2048];
	char		resolved_cd_name [MAXPATHLEN];
	char		resolved_dir_name [MAXPATHLEN];
	int			flags;

	int		this_arg = 1;
	int		switch_count;
	char	*program_name;
	char	error_message[80];
	char	*arg_list = "[-rR] cd_pathname [target_dir]";

	fprintf (stderr, 
		"cd_link : Copyright 1989, 1990, 1991, 1992 By Young Minds, Incoporated\n");

	/*	Extract program name from first arguement.	*/
	program_name = get_program_name(argv[0]);

	/*	Process arguements	*/
	flags = 0;
	cd_pathname[0] = '\0';
	target_dirname[0] = '\0';
	while (this_arg < argc) {
		/*	Process switches	*/
		if (argv[this_arg][0] == '-') {
			switch_count = 1;
			while (argv[this_arg][switch_count] != '\0') {
				switch (argv[this_arg][switch_count]) {
					case 'r' :
						/*
						**	If command line arguement "-r" was specified then
						**	recurse down subdirectories.
						*/
						flags |= RECURSE;
						break;
					case 'R' :
						/*
						**	If command line arguement "-R" was specified then
						**	ignore the 00_TRANS.TBL and create links with the 
						**	same names as the names that exist in the directory
						**	entries on the disk.
						**
						**	This is most useful on Rock Ridge disks where the
						**	name in the directory entry is the name that should
						**	be used.
						*/
						flags |= ROCK_RIDGE;
						break;
					default :
						sprintf(error_message, "Unknown switch: -%c", 
												argv[this_arg][switch_count]);
						usage( program_name, arg_list, error_message );
						exit(1);
						break;
				}
				switch_count++;
			}
		}
		/*	Process everything else.	*/
		else {
			/*	Get input file name.	*/
			if (cd_pathname[0] != '\0') {
				if (target_dirname[0] != '\0') {
					/*	
					**	If already gotten then an error exists 
					**	in the command line.
					*/
					sprintf( error_message, "Invalid arguement: %s", 
															argv[this_arg] );
					usage( program_name, arg_list, error_message );
					exit(1);
				}
				else
					if (argv [this_arg] [0] == '/')
						strcpy (target_dirname, argv [this_arg]);
					else {	
						getwd (target_dirname);
						strcat (target_dirname, "/");
						strcat (target_dirname, argv [this_arg]);
	  				}
			}
			else {
				if (argv [this_arg] [0] == '/')
					strcpy (cd_pathname, argv [this_arg]);
				else {	
					getwd (cd_pathname);
					strcat (cd_pathname, "/");
					strcat (cd_pathname, argv [this_arg]);
	  			}
			}
		}
		this_arg++;
	}

	/*
	**	If there was an input file specified then use that file for
	**	input.  Otherwise, get input from stdin.
	*/ 
	if (cd_pathname[0] == '\0') {
		sprintf( error_message, "Missing cd_pathname.");
		usage( program_name, arg_list, error_message );
		exit(1);
	}

	/*
	**	If there was an output file specified then use that file for
	**	output.  Otherwise, put output to stdout.
	*/
	if (target_dirname[0] == '\0') {
		getwd (target_dirname);
	}


#ifdef REALPATH
	realpath (cd_pathname, resolved_cd_name);
	realpath (target_dirname, resolved_dir_name);
#else
	strcpy (resolved_cd_name, cd_pathname);
	strcpy (resolved_dir_name, target_dirname);
#endif

	push_dir ( resolved_cd_name, resolved_dir_name );
	while ((cur_dir = dequeue_dir ()) != NULL)
	  {	
		proc_dir (cur_dir->cd_path, cur_dir->new_path, flags);
		free_dir (cur_dir);
	  }
	return 0;
}
