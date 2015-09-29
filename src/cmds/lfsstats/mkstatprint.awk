BEGIN		{ skip = 1; }
/typedef struct Lfs_StatsVersion/ { skip = 0; }
/LfsLogStats/ { i = "log"; s = $2;  next;  }
/LfsCheckPointStats/ { i = "checkpoint"; s = $2;  next;  }
/LfsLogCleanStats/ { i = "cleaning"; s = $2;  next;  }
/LfsBlockIOStats/ { i = "blockio"; s = $2;  next;  }
/LfsDescStats/ { i = "desc"; s = $2;   next;  }
/LfsIndexStats/ { i = "index"; s = $2;   next;  }
/LfsFileLayoutStats/ { i = "layout"; s = $2;   next;  }
/LfsSegUsageStats/ { i = "segusage"; s = $2;   next;  }
/LfsCacheBackendStats/ { i = "backend"; s = $2; next;  }
/LfsDirLogStats/ { i = "dirlog"; s = $2; next;  }
/padding/	{ next; }
/#undef LFSCOUNT/ { skip = 1; next; }
/LFSCOUNT/	{ if (skip) next; print "printf(\"" s "." substr($2,1,length($2)-1) " %d\\n\", statsPtr->" i "." substr($2,1,length($2)-1) ".low);" ; next;} 
