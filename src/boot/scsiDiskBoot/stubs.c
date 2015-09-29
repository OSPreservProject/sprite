#include "sprite.h"
#include "machMon.h"

printf(format, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10)
    char *format;
    Address arg1;
    Address arg2;
    Address arg3;
    Address arg4;
    Address arg5;
    Address arg6;
    Address arg7;
    Address arg8;
    Address arg9;
    Address arg10;
{
#ifndef NO_PRINTF
    Mach_MonPrintf(format, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9,
		       arg10);
#endif
}

panic(string)
    char *string;
{
#ifndef NO_PRINTF
    Mach_MonPrintf("Panic: %s\n", string);
#endif
    (romVectorPtr->abortEntry)();

}
