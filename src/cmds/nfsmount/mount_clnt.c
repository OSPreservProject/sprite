#include <rpc/rpc.h>
#include <sys/time.h>
#include "mount.h"

static struct timeval TIMEOUT = { 25, 0 };

VoidPtr 
mountproc_null_1(argp, clnt)
	VoidPtr argp;
	CLIENT *clnt;
{
	static char res;

	bzero(&res, sizeof(res));
	if (clnt_call(clnt, MOUNTPROC_NULL, xdr_void, argp, xdr_void, &res, TIMEOUT) != RPC_SUCCESS) {
		return (NULL);
	}
	return ((VoidPtr)&res);
}


fhstatus *
mountproc_mnt_1(argp, clnt)
	dirpath *argp;
	CLIENT *clnt;
{
	static fhstatus res;

	bzero(&res, sizeof(res));
	if (clnt_call(clnt, MOUNTPROC_MNT, xdr_dirpath, argp, xdr_fhstatus, &res, TIMEOUT) != RPC_SUCCESS) {
		return (NULL);
	}
	return (&res);
}


mountlist *
mountproc_dump_1(argp, clnt)
	VoidPtr argp;
	CLIENT *clnt;
{
	static mountlist res;

	bzero(&res, sizeof(res));
	if (clnt_call(clnt, MOUNTPROC_DUMP, xdr_void, argp, xdr_mountlist, &res, TIMEOUT) != RPC_SUCCESS) {
		return (NULL);
	}
	return (&res);
}


VoidPtr 
mountproc_umnt_1(argp, clnt)
	dirpath *argp;
	CLIENT *clnt;
{
	static char res;

	bzero(&res, sizeof(res));
	if (clnt_call(clnt, MOUNTPROC_UMNT, xdr_dirpath, argp, xdr_void, &res, TIMEOUT) != RPC_SUCCESS) {
		return (NULL);
	}
	return ((VoidPtr)&res);
}


VoidPtr 
mountproc_umntall_1(argp, clnt)
	VoidPtr argp;
	CLIENT *clnt;
{
	static char res;

	bzero(&res, sizeof(res));
	if (clnt_call(clnt, MOUNTPROC_UMNTALL, xdr_void, argp, xdr_void, &res, TIMEOUT) != RPC_SUCCESS) {
		return (NULL);
	}
	return ((VoidPtr)&res);
}


exports *
mountproc_export_1(argp, clnt)
	VoidPtr argp;
	CLIENT *clnt;
{
	static exports res;

	bzero(&res, sizeof(res));
	if (clnt_call(clnt, MOUNTPROC_EXPORT, xdr_void, argp, xdr_exports, &res, TIMEOUT) != RPC_SUCCESS) {
		return (NULL);
	}
	return (&res);
}


exports *
mountproc_exportall_1(argp, clnt)
	VoidPtr argp;
	CLIENT *clnt;
{
	static exports res;

	bzero(&res, sizeof(res));
	if (clnt_call(clnt, MOUNTPROC_EXPORTALL, xdr_void, argp, xdr_exports, &res, TIMEOUT) != RPC_SUCCESS) {
		return (NULL);
	}
	return (&res);
}

