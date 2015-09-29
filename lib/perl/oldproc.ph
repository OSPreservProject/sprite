if (!defined &_PROCUSER) {
    eval 'sub _PROCUSER {1;}';
    require 'spriteTime.ph';
    require 'sig.ph';
    require 'kernel/mach.ph';
    if (defined &KERNEL) {
	require 'user/vm.ph';
    }
    else {
	require 'vm.ph';
    }
    eval 'sub PROC_TERM_EXITED {1;}';
    eval 'sub PROC_TERM_DETACHED {2;}';
    eval 'sub PROC_TERM_SIGNALED {3;}';
    eval 'sub PROC_TERM_DESTROYED {4;}';
    eval 'sub PROC_TERM_SUSPENDED {5;}';
    eval 'sub PROC_TERM_RESUMED {6;}';
    eval 'sub PROC_BAD_STACK {1;}';
    eval 'sub PROC_BAD_PSW {2;}';
    eval 'sub PROC_VM_READ_ERROR {3;}';
    eval 'sub PROC_VM_WRITE_ERROR {4;}';
    eval 'sub PROC_MY_PID {(( &Proc_PID) 0xffffffff);}';
    eval 'sub PROC_MY_HOSTID {(( &unsigned \'int\') 0xffffffff);}';
    eval 'sub PROC_INDEX_MASK {0x000000FF;}';
    eval 'sub Proc_PIDToIndex {
        local($pid) = @_;
        eval "($pid &  &PROC_INDEX_MASK)";
    }';
    eval 'sub PROC_ALL_PROCESSES {(( &Proc_PID) 0);}';
    eval 'sub PROC_NO_FAMILY {( &Proc_PID) -1;}';
    eval 'sub Proc_In_A_Family {
        local($familyID) = @_;
        eval "($familyID !=  &PROC_NO_FAMILY)";
    }';
    eval 'sub PROC_SUPER_USER_ID {0;}';
    eval 'sub PROC_NO_ID {-1;}';
    eval 'sub PROC_MIN_PRIORITY {-2;}';
    eval 'sub PROC_MAX_PRIORITY {2;}';
    eval 'sub PROC_NO_INTR_PRIORITY {2;}';
    eval 'sub PROC_HIGH_PRIORITY {1;}';
    eval 'sub PROC_NORMAL_PRIORITY {0;}';
    eval 'sub PROC_LOW_PRIORITY {-1;}';
    eval 'sub PROC_VERY_LOW_PRIORITY {-2;}';
    eval 'sub PROC_KERNEL {0x00001;}';
    eval 'sub PROC_USER {0x00002;}';
    eval 'sub PROC_DEBUGGED {0x00004;}';
    eval 'sub PROC_DEBUG_ON_EXEC {0x00008;}';
    eval 'sub PROC_SINGLE_STEP_FLAG {0x00010;}';
    eval 'sub PROC_DEBUG_WAIT {0x00020;}';
    eval 'sub PROC_MIG_PENDING {0x00040;}';
    eval 'sub PROC_DONT_MIGRATE {0x00080;}';
    eval 'sub PROC_FOREIGN {0x00100;}';
    eval 'sub PROC_DIEING {0x00200;}';
    eval 'sub PROC_LOCKED {0x00400;}';
    eval 'sub PROC_NO_VM {0x00800;}';
    eval 'sub PROC_MIGRATING {0x01000;}';
    eval 'sub PROC_MIGRATION_DONE {0x02000;}';
    eval 'sub PROC_ON_DEBUG_LIST {0x04000;}';
    eval 'sub PROC_REMOTE_EXEC_PENDING {0x08000;}';
    eval 'sub PROC_MIG_ERROR {0x10000;}';
    eval 'sub PROC_WAIT_BLOCK {0x1;}';
    eval 'sub PROC_WAIT_FOR_SUSPEND {0x2;}';
    eval 'sub PROC_NUM_GENERAL_REGS {16;}';
    eval 'sub PROC_MAX_ENVIRON_NAME_LENGTH {512;}';
    eval 'sub PROC_MAX_ENVIRON_VALUE_LENGTH {512;}';
    eval 'sub PROC_MAX_ENVIRON_SIZE {100;}';
    eval 'sub PROC_MAX_INTERPRET_SIZE {80;}';
    eval 'sub PROC_TIMER_REAL {0;}';
    eval 'sub PROC_MAX_TIMER { &PROC_TIMER_REAL;}';
    eval 'sub PROC_PCB_ARG_LENGTH {256;}';
    eval 'sub PROC_MIG_IMPORT_NEVER {0;}';
    eval 'sub PROC_MIG_IMPORT_ROOT {0x00000001;}';
    eval 'sub PROC_MIG_IMPORT_ALL {0x00000003;}';
    eval 'sub PROC_MIG_IMPORT_ANYINPUT {0x00000010;}';
    eval 'sub PROC_MIG_IMPORT_ANYLOAD {0x00000020;}';
    eval 'sub PROC_MIG_IMPORT_ALWAYS {( &PROC_MIG_IMPORT_ANYINPUT |  &PROC_MIG_IMPORT_ANYLOAD);}';
    eval 'sub PROC_MIG_EXPORT_NEVER {0;}';
    eval 'sub PROC_MIG_EXPORT_ROOT {0x00010000;}';
    eval 'sub PROC_MIG_EXPORT_ALL {0x00030000;}';
    eval 'sub PROC_MIG_ALLOW_DEFAULT {( &PROC_MIG_IMPORT_ALL |  &PROC_MIG_EXPORT_ALL);}';
}
1;
