
/*
** The machine dependent statistics routines.
*/
#include "sprite.h"
#include "status.h"
#include "fs.h"
#include "fsCmd.h"
#include "sysStats.h"
#include "proc.h"
#include "vm.h"
#include "kernel/sched.h"
#include "kernel/fsStat.h"
#include "kernel/vm.h"
#include "stdio.h"
#include "ccMachStat.h"

ReturnStatus
InitMachStats(stats)
MachStats	*stats;
{
	return(SUCCESS);
}

ReturnStatus
GetMachStats(stats)
MachStats	*stats;
{
	return(SUCCESS);
}

PrintMachStats(startStats, endStats)
MachStats       *startStats;
MachStats       *endStats;
{
}
