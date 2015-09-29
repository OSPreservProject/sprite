# The following variable should be redefined from the command-line using
# the format "rdist -f [this makefile] -d DIR=`pwd`".
DIR = /sprite/src/cmds/${COMMAND}

# To exclude particular file(s), define EXCEPT on the command line.
EXCEPT = ()

EXCEPT_PAT = (\\~\$ \\^#.*  /#.* \\.o\$ \\.a\$ \\.bak\$ /Mx\\.)
ALL_EXCEPT = (${DIR}/{RCS,LOCK.make,version.h,tags,TAGS,${EXCEPT}})
RHOST = (allspice)
BACKUP = /sprite/backup


${DIR} -> ${RHOST} install -y ${BACKUP}${DIR} ;
        except_pat ${EXCEPT_PAT};
	except ${ALL_EXCEPT};

