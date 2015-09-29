default :
	cd $(MACHINE).md; pmake -M

clean :
	cd $(MACHINE).md; pmake -M $(.TARGET)

install : installman
	cd $(MACHINE).md; pmake -M install DESTDIR=/sprite/cmds

installman :
	update dist/documentation/bash.1 /sprite/man/cmds/bash.man

