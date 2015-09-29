#
# This awk script is intended to read as input the file lfsStat.h and 
# output C fprintf to printout the stats structure.
#
BEGIN		{ skip = 1; }
/typedef struct Lfs_StatsVersion/ { skip = 0; }
/LfsLogStats/ 	     { 
			field = "log"; 
			st = "Lfs_Stats.LfsLogStats.";  
			next;  
		     }
/LfsCheckPointStats/ { 
			field = "checkpoint"; 
			st = "Lfs_Stats.LfsCheckPointStats."; 
			next; 
		     }
/LfsLogCleanStats/ { 
			field = "cleaning"; 
			st = "LfsLogStats.LfsLogCleanStats."; 
			next;  
		   }
/LfsBlockIOStats/  { 
			field = "blockio"; 
			st = "LfsLogStats.LfsLogCleanStats.";
			next;
		   }
/LfsDescStats/     { 
			field = "desc"; 
			st = "LfsLogStats.LfsDescStats.";
			next;
		   }
/LfsIndexStats/   { 
			field = "index"; 
			st = "LfsLogStats.LfsIndexStats.";
			next;
		  }
/LfsFileLayoutStats/ { 
			field = "layout"; 
			st = "LfsLogStats.LfsFileLayoutStats.";
			next;
		     }
/LfsSegUsageStats/ { 
			field = "segusage"; 
			st = "LfsLogStats.LfsSegUsageStats.";
			next;
		     }

/LfsCacheBackendStats/ { 
			field = "backend"; 
			st = "LfsLogStats.LfsCacheBackendStats.";
			next;
		     }
/LfsDirLogStats/     { 
			field = "dirlog"; 
			st = "LfsLogStats.LfsDirLogStats.";
			next;
		     }
/padding/	    { next; }
/#undef LFSCOUNT/   { skip = 1; next; }
/LFSCOUNT/	    { 
			if (skip) 
			    next;
			name = substr($2,1,length($2)-1);
			var = "statsPtr->" field "." name;
			print "if (printzero || " var ".low) " 
			print "\tfprintf(outFile,\"" st  field "." name ".low %u\\n\"," var ".low);"
			print "if (printzero || " var ".high) " 
			print "\tfprintf(outFile,\"" st  field "." name ".high %u\\n\"," var ".high);"
			next;
		    }
