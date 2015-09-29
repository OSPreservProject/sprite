default2		:	
	(cd $(TM).md/gdb; make)
	mv /sprite/src/cmds/$(NAME)/$(TM).md/gdb/$(NAME) ./$(TM).md/$(NAME)

clean tidy cleanall	::
	(cd $(TM).md; make clean)

#include <$(SYSMAKEFILE)>
