#
# special local.mk for server
#

CFLAGS	+= -DADMPATH=\"/dev/syslog\"
#include <../../common.mk>

# Additional include file paths
.PATH.h: $(X)/src/lib/Xau
