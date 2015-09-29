sub TZDIR {"/etc/zoneinfo";}
sub TZDEFAULT {"localtime";}
sub TZ_MAX_TIMES {370;}
sub NOSOLAR {1;}
if (!defined &NOSOLAR) {
    eval 'sub TZ_MAX_TYPES {256;}';
}
else {
    eval 'sub TZ_MAX_TYPES {10;}';
}
sub TZ_MAX_CHARS {50;}
sub SECS_PER_MIN {60;}
sub MINS_PER_HOUR {60;}
sub HOURS_PER_DAY {24;}
sub DAYS_PER_WEEK {7;}
sub DAYS_PER_NYEAR {365;}
sub DAYS_PER_LYEAR {366;}
sub SECS_PER_HOUR {( &SECS_PER_MIN *  &MINS_PER_HOUR);}
sub SECS_PER_DAY {(('long')  &SECS_PER_HOUR *  &HOURS_PER_DAY);}
sub MONS_PER_YEAR {12;}
sub TM_SUNDAY {0;}
sub TM_MONDAY {1;}
sub TM_TUESDAY {2;}
sub TM_WEDNESDAY {3;}
sub TM_THURSDAY {4;}
sub TM_FRIDAY {5;}
sub TM_SATURDAY {6;}
sub TM_JANUARY {0;}
sub TM_FEBRUARY {1;}
sub TM_MARCH {2;}
sub TM_APRIL {3;}
sub TM_MAY {4;}
sub TM_JUNE {5;}
sub TM_JULY {6;}
sub TM_AUGUST {7;}
sub TM_SEPTEMBER {8;}
sub TM_OCTOBER {9;}
sub TM_NOVEMBER {10;}
sub TM_DECEMBER {11;}
sub TM_SUNDAY {0;}
sub TM_YEAR_BASE {1900;}
sub EPOCH_YEAR {1970;}
sub EPOCH_WDAY { &TM_THURSDAY;}
sub isleap {
    local($y) = @_;
    eval "((($y) % 4) == 0 && (($y) % 100) != 0 || (($y) % 400) == 0)";
}
1;
