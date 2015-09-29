if (!defined &_SYNCUSER) {
    eval 'sub _SYNCUSER {1;}';
    if (!defined &_SPRITE) {
    }
    if (!defined &KERNEL) {
    }
    if (!defined &LOCKDEP) {
	eval 'sub SYNC_MAX_PRIOR {1;}';
    }
    else {
	eval 'sub SYNC_MAX_PRIOR {30;}';
    }
    eval 'sub LOCK_MONITOR {( &void)  &Sync_GetLock( &LOCKPTR);}';
    eval 'sub UNLOCK_MONITOR {( &void)  &Sync_Unlock( &LOCKPTR);}';
    eval 'sub ENTRY {1;}';
    eval 'sub INTERNAL {1;}';
    eval 'sub Sync_Wait {
        local($conditionPtr, $wakeIfSignal) = @_;
        eval " &Sync_SlowWait($conditionPtr,  &LOCKPTR, $wakeIfSignal)";
    }';
    eval 'sub Sync_Broadcast {
        local($conditionPtr) = @_;
        eval " &if ((( &Sync_Condition *)$conditionPtr)-> &waiting ==  &TRUE) { ( &void) &Sync_SlowBroadcast(( &unsigned \'int\') $conditionPtr, &(( &Sync_Condition *)$conditionPtr)-> &waiting); }";
    }';
}
1;
