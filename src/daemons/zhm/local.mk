# pidfile is put in /hosts/<machine>
CFLAGS +=  -DPIDFILE=\"zhm.pid\" -L/sprite/lib/$(TM).md -DMACHINE=\"$(TM)\" -Usparc
LIBS += -lzephyr -lcom_err -lss
NOOPTIMIZATION = no -O
#include <$(SYSMAKEFILE)>
