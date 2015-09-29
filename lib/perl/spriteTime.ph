if (!defined &_SPRITETIME) {
    eval 'sub _SPRITETIME {1;}';
    if (!defined &_SPRITE) {
    }
    eval 'sub ONE_SECOND {1000000;}';
    eval 'sub TENTH_SECOND {100000;}';
    eval 'sub HUNDREDTH_SECOND {10000;}';
    eval 'sub ONE_MILLISECOND {1000;}';
    eval 'sub TIME_CVT_BUF_SIZE {30;}';
    eval 'sub Time_LT {
        local($time1, $time2) = @_;
        eval "((($time1). &seconds < ($time2). &seconds) || ((($time1). &seconds == ($time2). &seconds) && (($time1). &microseconds < ($time2). &microseconds)))";
    }';
    eval 'sub Time_LE {
        local($time1, $time2) = @_;
        eval "((($time1). &seconds < ($time2). &seconds) || ((($time1). &seconds == ($time2). &seconds) && (($time1). &microseconds <= ($time2). &microseconds)))";
    }';
    eval 'sub Time_EQ {
        local($time1, $time2) = @_;
        eval "((($time1). &seconds == ($time2). &seconds) && (($time1). &microseconds == ($time2). &microseconds))";
    }';
    eval 'sub Time_GE {
        local($time1, $time2) = @_;
        eval "((($time1). &seconds > ($time2). &seconds) || ((($time1). &seconds == ($time2). &seconds) && (($time1). &microseconds >= ($time2). &microseconds)))";
    }';
    eval 'sub Time_GT {
        local($time1, $time2) = @_;
        eval "((($time1). &seconds > ($time2). &seconds) || ((($time1). &seconds == ($time2). &seconds) && (($time1). &microseconds > ($time2). &microseconds)))";
    }';
}
1;
