if (!defined &_VMSTAT) {
    eval 'sub _VMSTAT {1;}';
    if (defined &KERNEL) {
    }
    else {
	require 'kernel/vmMachStat.ph';
    }
}
1;
