/*
 * tmp_ctl:
 * pretend to be Sequent DYNIX tmp_ctl system call
 */


#include "sprite.h"
#include "sys.h"
#include "errno.h"
#include "compatInt.h"
#include "tmp_ctl.h"

int
tmp_ctl(command, processor)
	int command, processor;

{
	int status;
	Sys_MachineInfo machInfo;

	switch (command) {
	case TMP_NENG:
		status = Sys_GetMachineInfo(sizeof(machInfo), &machInfo);
		if (SUCCESS == status) {
			return machInfo.processors;
		}
		break;
	case TMP_OFFLINE:
		status = Sched_IdleProcessor(processor);
		break;
	case TMP_ONLINE:
		status = Sched_StartProcessor(processor);
		break;
	case TMP_QUERY:
		/* no way to get processor info... yet */
		errno = EINVAL;
		return UNIX_ERROR;
	default:
		errno = EINVAL;
		return UNIX_ERROR;
	}
	if (SUCCESS != status) {
		errno = Compat_MapCode(status);
		return UNIX_ERROR;
	} else {
		return UNIX_SUCCESS;
	}
}





