if (!defined &_TK) {
    eval 'sub _TK {1;}';
    if (!defined &_TCL) {
	require 'tcl.ph';
    }
    if (!defined &_XLIB_H) {
	require 'X11/Xlib.ph';
    }
    if (defined( &USE_ANSI) && defined( &__STDC__)) {
	eval 'sub _ARGS_ {
	    local($x) = @_;
	    eval "$x";
	}';
    }
    else {
	eval 'sub _ARGS_ {
	    local($x) = @_;
	    eval "()";
	}';
	eval 'sub const {1;}';
    }
    if (!defined &_CLIENTDATA) {
	eval 'sub _CLIENTDATA {1;}';
    }
    eval 'sub TK_ARGV_CONSTANT {15;}';
    eval 'sub TK_ARGV_INT {16;}';
    eval 'sub TK_ARGV_STRING {17;}';
    eval 'sub TK_ARGV_UID {18;}';
    eval 'sub TK_ARGV_REST {19;}';
    eval 'sub TK_ARGV_FLOAT {20;}';
    eval 'sub TK_ARGV_FUNC {21;}';
    eval 'sub TK_ARGV_GENFUNC {22;}';
    eval 'sub TK_ARGV_HELP {23;}';
    eval 'sub TK_ARGV_CONST_OPTION {24;}';
    eval 'sub TK_ARGV_OPTION_VALUE {25;}';
    eval 'sub TK_ARGV_OPTION_NAME_VALUE {26;}';
    eval 'sub TK_ARGV_END {27;}';
    eval 'sub TK_ARGV_NO_DEFAULTS {0x1;}';
    eval 'sub TK_ARGV_NO_LEFTOVERS {0x2;}';
    eval 'sub TK_ARGV_NO_ABBREV {0x4;}';
    eval 'sub TK_ARGV_DONT_SKIP_FIRST_ARG {0x8;}';
    eval 'sub TK_CONFIG_BOOLEAN {1;}';
    eval 'sub TK_CONFIG_INT {2;}';
    eval 'sub TK_CONFIG_DOUBLE {3;}';
    eval 'sub TK_CONFIG_STRING {4;}';
    eval 'sub TK_CONFIG_UID {5;}';
    eval 'sub TK_CONFIG_COLOR {6;}';
    eval 'sub TK_CONFIG_FONT {7;}';
    eval 'sub TK_CONFIG_BITMAP {8;}';
    eval 'sub TK_CONFIG_BORDER {9;}';
    eval 'sub TK_CONFIG_RELIEF {10;}';
    eval 'sub TK_CONFIG_CURSOR {11;}';
    eval 'sub TK_CONFIG_SYNONYM {12;}';
    eval 'sub TK_CONFIG_END {13;}';
    eval 'sub Tk_Offset {
        local($type, $field) = @_;
        eval "((\'int\') ((\'char\' *) &(($type *) 0)->$field))";
    }';
    eval 'sub TK_CONFIG_ARGV_ONLY {1;}';
    eval 'sub TK_CONFIG_COLOR_ONLY {2;}';
    eval 'sub TK_CONFIG_MONO_ONLY {4;}';
    eval 'sub TK_CONFIG_USER_BIT {0x100;}';
    if (!defined &_TKINT) {
	require 'tkInt.ph';
    }
    eval 'sub TK_READABLE {1;}';
    eval 'sub TK_WRITABLE {2;}';
    eval 'sub TK_EXCEPTION {4;}';
    eval 'sub TK_WIDGET_DEFAULT_PRIO {20;}';
    eval 'sub TK_STARTUP_FILE_PRIO {40;}';
    eval 'sub TK_USER_DEFAULT_PRIO {60;}';
    eval 'sub TK_INTERACTIVE_PRIO {80;}';
    eval 'sub TK_MAX_PRIO {100;}';
    eval 'sub TK_RELIEF_RAISED {1;}';
    eval 'sub TK_RELIEF_FLAT {2;}';
    eval 'sub TK_RELIEF_SUNKEN {4;}';
    eval 'sub TK_NOTIFY_SHARE {20;}';
    eval 'sub Tk_Display {
        local($tkwin) = @_;
        eval "((( &TkWindow *) ($tkwin))-> &display)";
    }';
    eval 'sub Tk_ScreenNumber {
        local($tkwin) = @_;
        eval "((( &TkWindow *) ($tkwin))-> &screenNum)";
    }';
    eval 'sub Tk_Screen {
        local($tkwin) = @_;
        eval "( &ScreenOfDisplay( &Tk_Display($tkwin),  &Tk_ScreenNumber($tkwin)))";
    }';
    eval 'sub Tk_WindowId {
        local($tkwin) = @_;
        eval "((( &TkWindow *) ($tkwin))-> &window)";
    }';
    eval 'sub Tk_Name {
        local($tkwin) = @_;
        eval "((( &TkWindow *) ($tkwin))-> &nameUid)";
    }';
    eval 'sub Tk_Class {
        local($tkwin) = @_;
        eval "((( &TkWindow *) ($tkwin))-> &classUid)";
    }';
    eval 'sub Tk_PathName {
        local($tkwin) = @_;
        eval "((( &TkWindow *) ($tkwin))-> &pathName)";
    }';
    eval 'sub Tk_X {
        local($tkwin) = @_;
        eval "((( &TkWindow *) ($tkwin))-> &changes. &x)";
    }';
    eval 'sub Tk_Y {
        local($tkwin) = @_;
        eval "((( &TkWindow *) ($tkwin))-> &changes. &y)";
    }';
    eval 'sub Tk_Width {
        local($tkwin) = @_;
        eval "((( &TkWindow *) ($tkwin))-> &changes. &width)";
    }';
    eval 'sub Tk_Height {
        local($tkwin) = @_;
        eval "((( &TkWindow *) ($tkwin))-> &changes. &height)";
    }';
    eval 'sub Tk_Changes {
        local($tkwin) = @_;
        eval "(&(( &TkWindow *) ($tkwin))-> &changes)";
    }';
    eval 'sub Tk_Attributes {
        local($tkwin) = @_;
        eval "(&(( &TkWindow *) ($tkwin))-> &atts)";
    }';
    eval 'sub Tk_IsMapped {
        local($tkwin) = @_;
        eval "((( &TkWindow *) ($tkwin))-> &flags &  &TK_MAPPED)";
    }';
    eval 'sub Tk_ReqWidth {
        local($tkwin) = @_;
        eval "((( &TkWindow *) ($tkwin))-> &reqWidth)";
    }';
    eval 'sub Tk_ReqHeight {
        local($tkwin) = @_;
        eval "((( &TkWindow *) ($tkwin))-> &reqHeight)";
    }';
    eval 'sub Tk_InternalBorderWidth {
        local($tkwin) = @_;
        eval "((( &TkWindow *) ($tkwin))-> &internalBorderWidth)";
    }';
    eval 'sub Tk_Parent {
        local($tkwin) = @_;
        eval "(( &Tk_Window) ((( &TkWindow *) ($tkwin))-> &parentPtr))";
    }';
    eval 'sub Tk_DisplayName {
        local($tkwin) = @_;
        eval "((( &TkWindow *) ($tkwin))-> &dispPtr-> &name)";
    }';
}
1;
