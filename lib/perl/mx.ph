if (!defined &_MX) {
    eval 'sub _MX {1;}';
    if (!defined &_XLIB_H_) {
	require 'X11/Xlib.ph';
    }
    require 'stdio.ph';
    eval 'sub MX_POS_EQUAL {
        local($a, $b) = @_;
        eval "((($a). &lineIndex == ($b). &lineIndex) && (($a). &charIndex == ($b). &charIndex))";
    }';
    eval 'sub MX_POS_LESS {
        local($a, $b) = @_;
        eval "((($a). &lineIndex < ($b). &lineIndex) || ((($a). &lineIndex == ($b). &lineIndex) && (($a). &charIndex < ($b). &charIndex)))";
    }';
    eval 'sub MX_POS_LEQ {
        local($a, $b) = @_;
        eval "((($a). &lineIndex < ($b). &lineIndex) || ((($a). &lineIndex == ($b). &lineIndex) && (($a). &charIndex <= ($b). &charIndex)))";
    }';
    eval 'sub MX_UNDO {1;}';
    eval 'sub MX_DELETE {2;}';
    eval 'sub MX_NO_TITLE {8;}';
    eval 'sub TX_NO_TITLE {1;}';
    eval 'sub MX_BEFORE {1;}';
    eval 'sub MX_AFTER {2;}';
    eval 'sub MX_REVERSE {1;}';
    eval 'sub MX_GRAY {2;}';
    eval 'sub MX_UNDERLINE {3;}';
    eval 'sub MX_BOX {4;}';
    eval 'sub UNDO_ID_LENGTH {60;}';
    eval 'sub UNDO_NAME_LENGTH {600;}';
}
1;
