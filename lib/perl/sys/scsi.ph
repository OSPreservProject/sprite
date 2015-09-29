if (!defined &_SCSI_H) {
    eval 'sub _SCSI_H {1;}';
    require 'machparam.ph';
    if (!defined &BYTE_ORDER) {
    }
    eval 'sub SCSI_TEST_UNIT_READY {0x00;}';
    eval 'sub SCSI_REZERO_UNIT {0x01;}';
    eval 'sub SCSI_REQUEST_SENSE {0x03;}';
    eval 'sub SCSI_FORMAT_UNIT {0x04;}';
    eval 'sub SCSI_REASSIGN_BLOCKS {0x07;}';
    eval 'sub SCSI_READ {0x08;}';
    eval 'sub SCSI_WRITE {0x0a;}';
    eval 'sub SCSI_SEEK {0x0b;}';
    eval 'sub SCSI_INQUIRY {0x12;}';
    eval 'sub SCSI_MODE_SELECT {0x15;}';
    eval 'sub SCSI_RESERVE_UNIT {0x16;}';
    eval 'sub SCSI_RELEASE_UNIT {0x17;}';
    eval 'sub SCSI_COPY {0x18;}';
    eval 'sub SCSI_MODE_SENSE {0x1A;}';
    eval 'sub SCSI_START_STOP {0x1b;}';
    eval 'sub SCSI_RECV_DIAG_RESULTS {0x1c;}';
    eval 'sub SCSI_SEND_DIAGNOSTIC {0x1d;}';
    eval 'sub SCSI_PREVENT_ALLOW {0x1e;}';
    eval 'sub SCSI_READ_CAPACITY {0x25;}';
    eval 'sub SCSI_READ_EXT {0x28;}';
    eval 'sub SCSI_WRITE_EXT {0x2a;}';
    eval 'sub SCSI_SEEK_EXT {0x2b;}';
    eval 'sub SCSI_WRITE_VERIFY {0x2e;}';
    eval 'sub SCSI_VERIFY_EXT {0x2f;}';
    eval 'sub SCSI_SEARCH_HIGH {0x30;}';
    eval 'sub SCSI_SEARCH_EQUAL {0x31;}';
    eval 'sub SCSI_SEARCH_LOW {0x32;}';
    eval 'sub SCSI_SET_LIMITS {0x33;}';
    eval 'sub SCSI_COMPARE {0x39;}';
    eval 'sub SCSI_COPY_VERIFY {0x3a;}';
    eval 'sub SCSI_REWIND {0x01;}';
    eval 'sub SCSI_READ_BLOCK_LIMITS {0x05;}';
    eval 'sub SCSI_TRACK_SELECT {0x0b;}';
    eval 'sub SCSI_READ_REVERSE {0x0f;}';
    eval 'sub SCSI_WRITE_EOF {0x10;}';
    eval 'sub SCSI_SPACE {0x11;}';
    eval 'sub SCSI_VERIFY {0x13;}';
    eval 'sub SCSI_READ_BUFFER {0x14;}';
    eval 'sub SCSI_ERASE_TAPE {0x19;}';
    eval 'sub SCSI_LOAD_UNLOAD {0x1b;}';
    eval 'sub SCSI_LOCATE {0x2b;}';
    eval 'sub SCSI_READ_POSITION {0x34;}';
    if ( &BYTE_ORDER ==  &BIG_ENDIAN) {
    }
    else {
    }
    if ( &BYTE_ORDER ==  &BIG_ENDIAN) {
    }
    else {
    }
    eval 'sub SCSI_RESERVED_STATUS {
        local($byte) = @_;
        eval "($byte&0x80)";
    }';
    if ( &BYTE_ORDER ==  &BIG_ENDIAN) {
    }
    else {
    }
    eval 'sub SCSI_NO_SENSE_DATA {0x00;}';
    eval 'sub SCSI_NOT_READY {0x04;}';
    eval 'sub SCSI_NOT_LOADED {0x09;}';
    eval 'sub SCSI_INSUF_CAPACITY {0x0a;}';
    eval 'sub SCSI_HARD_DATA_ERROR {0x11;}';
    eval 'sub SCSI_WRITE_PROTECT {0x17;}';
    eval 'sub SCSI_CORRECTABLE_ERROR {0x18;}';
    eval 'sub SCSI_FILE_MARK {0x1c;}';
    eval 'sub SCSI_INVALID_COMMAND {0x20;}';
    eval 'sub SCSI_UNIT_ATTENTION {0x30;}';
    eval 'sub SCSI_END_OF_MEDIA {0x34;}';
    if ( &BYTE_ORDER ==  &BIG_ENDIAN) {
    }
    else {
    }
    eval 'sub SCSI_CLASS7_NO_SENSE {0;}';
    eval 'sub SCSI_CLASS7_RECOVERABLE {1;}';
    eval 'sub SCSI_CLASS7_NOT_READY {2;}';
    eval 'sub SCSI_CLASS7_MEDIA_ERROR {3;}';
    eval 'sub SCSI_CLASS7_HARDWARE_ERROR {4;}';
    eval 'sub SCSI_CLASS7_ILLEGAL_REQUEST {5;}';
    eval 'sub SCSI_CLASS7_MEDIA_CHANGE {6;}';
    eval 'sub SCSI_CLASS7_UNIT_ATTN {6;}';
    eval 'sub SCSI_CLASS7_WRITE_PROTECT {7;}';
    eval 'sub SCSI_CLASS7_BLANK_CHECK {8;}';
    eval 'sub SCSI_CLASS7_VENDOR {9;}';
    eval 'sub SCSI_CLASS7_POWER_UP_FAILURE {10;}';
    eval 'sub SCSI_CLASS7_ABORT {11;}';
    eval 'sub SCSI_CLASS7_EQUAL {12;}';
    eval 'sub SCSI_CLASS7_OVERFLOW {13;}';
    eval 'sub SCSI_CLASS7_RESERVED_14 {14;}';
    eval 'sub SCSI_CLASS7_RESERVED_15 {15;}';
    eval 'sub SCSI_MAX_SENSE_LEN {64;}';
    if ( &BYTE_ORDER ==  &BIG_ENDIAN) {
	if (defined &notdef) {
	}
    }
    else {
    }
    eval 'sub SCSI_DISK_TYPE {0;}';
    eval 'sub SCSI_TAPE_TYPE {1;}';
    eval 'sub SCSI_PRINTER_TYPE {2;}';
    eval 'sub SCSI_HOST_TYPE {3;}';
    eval 'sub SCSI_WORM_TYPE {4;}';
    eval 'sub SCSI_ROM_TYPE {5;}';
    eval 'sub SCSI_SCANNER_TYPE {6;}';
    eval 'sub SCSI_OPTICAL_MEM_TYPE {7;}';
    eval 'sub SCSI_MEDIUM_CHANGER_TYPE {8;}';
    eval 'sub SCSI_COMMUNICATIONS_TYPE {9;}';
    eval 'sub SCSI_NODEVICE_TYPE {0x7f;}';
    if ( &BYTE_ORDER ==  &BIG_ENDIAN) {
    }
    else {
    }
    if ( &BYTE_ORDER ==  &BIG_ENDIAN) {
    }
    else {
    }
    if ( &BYTE_ORDER ==  &BIG_ENDIAN) {
    }
    else {
    }
    if ( &BYTE_ORDER ==  &BIG_ENDIAN) {
    }
    else {
    }
    if ( &BYTE_ORDER ==  &BIG_ENDIAN) {
    }
    else {
    }
    if ( &BYTE_ORDER ==  &BIG_ENDIAN) {
    }
    else {
    }
    if ( &BYTE_ORDER ==  &BIG_ENDIAN) {
    }
    else {
    }
    if ( &BYTE_ORDER ==  &BIG_ENDIAN) {
    }
    else {
    }
    if ( &BYTE_ORDER ==  &BIG_ENDIAN) {
    }
    else {
    }
}
1;
