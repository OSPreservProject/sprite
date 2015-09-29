if (!defined &_CMD) {
    eval 'sub _CMD {1;}';
    require 'stdio.ph';
    if (!defined &_CLIENTDATA) {
	eval 'sub _CLIENTDATA {1;}';
    }
    eval 'sub CMD_OK {0;}';
    eval 'sub CMD_AMBIGUOUS {1;}';
    eval 'sub CMD_NOT_FOUND {2;}';
    eval 'sub CMD_PARTIAL {3;}';
    eval 'sub CMD_UNBOUND {4;}';
    eval 'sub CMD_BAD_ARGS {5;}';
    eval 'sub CMD_ERROR {6;}';
    eval 'sub CMD_VAR_LOOP {7;}';
    if (defined &NOTDEF) {
    }
}
1;
