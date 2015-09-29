#include <sys/param.h>

main()
{
    char buf[MAXPATHLEN+1];
    getwd(buf);
    buf[MAXPATHLEN]='\0';
    printf("%s\n",buf);
}
