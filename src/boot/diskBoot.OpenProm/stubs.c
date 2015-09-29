#include "sprite.h"
#include "machMon.h"
#include "boot.h"

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
    Mach_MonPrintf(format, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9,
		       arg10);
}

void panic(string)
    char *string;
{
    Mach_MonPrintf("Panic: %s\n", string);
    (romVectorPtr->abortEntry)();

}
