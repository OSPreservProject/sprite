if (!defined &_TD) {
    eval 'sub _TD {1;}';
    require 'sprite.ph';
    require 'fmt.ph';
    eval 'sub TD_BREAK {1;}';
    eval 'sub TD_GOT_CARRIER {2;}';
    eval 'sub TD_LOST_CARRIER {3;}';
    eval 'sub TD_RAW_START_BREAK {1;}';
    eval 'sub TD_RAW_STOP_BREAK {2;}';
    eval 'sub TD_RAW_SET_DTR {3;}';
    eval 'sub TD_RAW_CLEAR_DTR {4;}';
    eval 'sub TD_RAW_SHUTDOWN {5;}';
    eval 'sub TD_RAW_OUTPUT_READY {6;}';
    eval 'sub TD_RAW_FLUSH_OUTPUT {7;}';
    eval 'sub TD_RAW_FLOW_CHARS {8;}';
    eval 'sub TD_RAW_SET_BAUD_RATE {9;}';
    eval 'sub TD_RAW_GET_BAUD_RATE {10;}';
    eval 'sub TD_COOKED_SIGNAL {21;}';
    eval 'sub TD_COOKED_READS_OK {22;}';
    eval 'sub TD_COOKED_WRITES_OK {23;}';
}
1;
