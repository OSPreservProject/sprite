if (!defined &WINDOW) {
    eval 'sub bool {\'char\';}';
    eval 'sub reg { &register;}';
    eval 'sub TRUE {(1);}';
    eval 'sub FALSE {(0);}';
    eval 'sub ERR {(0);}';
    eval 'sub OK {(1);}';
    eval 'sub _ENDLINE {001;}';
    eval 'sub _FULLWIN {002;}';
    eval 'sub _SCROLLWIN {004;}';
    eval 'sub _FLUSH {010;}';
    eval 'sub _FULLLINE {020;}';
    eval 'sub _IDLINE {040;}';
    eval 'sub _STANDOUT {0200;}';
    eval 'sub _NOCHANGE {-1;}';
    eval 'sub _puts {
        local($s) = @_;
        eval " &tputs($s, 0,  &_putchar)";
    }';
    eval 'sub WINDOW {\'struct _win_st\';}';
    if (defined &lint) {
	eval 'sub VOID {
	    local($x) = @_;
	    eval "( &__void__ = (\'int\') ($x))";
	}';
    }
    else {
	eval 'sub VOID {
	    local($x) = @_;
	    eval "($x)";
	}';
    }
    eval 'sub addch {
        local($ch) = @_;
        eval " &VOID( &waddch( &stdscr, $ch))";
    }';
    eval 'sub getch {
        eval " &VOID( &wgetch( &stdscr))";
    }';
    eval 'sub addbytes {
        local($da,$co) = @_;
        eval " &VOID( &waddbytes( &stdscr, $da,$co))";
    }';
    eval 'sub addstr {
        local($str) = @_;
        eval " &VOID( &waddbytes( &stdscr, $str,  &strlen($str)))";
    }';
    eval 'sub getstr {
        local($str) = @_;
        eval " &VOID( &wgetstr( &stdscr, $str))";
    }';
    eval 'sub move {
        local($y, $x) = @_;
        eval " &VOID( &wmove( &stdscr, $y, $x))";
    }';
    eval 'sub clear {
        eval " &VOID( &wclear( &stdscr))";
    }';
    eval 'sub erase {
        eval " &VOID( &werase( &stdscr))";
    }';
    eval 'sub clrtobot {
        eval " &VOID( &wclrtobot( &stdscr))";
    }';
    eval 'sub clrtoeol {
        eval " &VOID( &wclrtoeol( &stdscr))";
    }';
    eval 'sub insertln {
        eval " &VOID( &winsertln( &stdscr))";
    }';
    eval 'sub deleteln {
        eval " &VOID( &wdeleteln( &stdscr))";
    }';
    eval 'sub refresh {
        eval " &VOID( &wrefresh( &stdscr))";
    }';
    eval 'sub inch {
        eval " &VOID( &winch( &stdscr))";
    }';
    eval 'sub insch {
        local($c) = @_;
        eval " &VOID( &winsch( &stdscr,$c))";
    }';
    eval 'sub delch {
        eval " &VOID( &wdelch( &stdscr))";
    }';
    eval 'sub standout {
        eval " &VOID( &wstandout( &stdscr))";
    }';
    eval 'sub standend {
        eval " &VOID( &wstandend( &stdscr))";
    }';
    eval 'sub mvwaddch {
        local($win,$y,$x,$ch) = @_;
        eval " &VOID( &wmove($win,$y,$x)== &ERR? &ERR: &waddch($win,$ch))";
    }';
    eval 'sub mvwgetch {
        local($win,$y,$x) = @_;
        eval " &VOID( &wmove($win,$y,$x)== &ERR? &ERR: &wgetch($win))";
    }';
    eval 'sub mvwaddbytes {
        local($win,$y,$x,$da,$co) = @_;
        eval " &VOID( &wmove($win,$y,$x)== &ERR? &ERR: &waddbytes($win,$da,$co))";
    }';
    eval 'sub mvwaddstr {
        local($win,$y,$x,$str) = @_;
        eval " &VOID( &wmove($win,$y,$x)== &ERR? &ERR: &waddbytes($win,$str, &strlen($str)))";
    }';
    eval 'sub mvwgetstr {
        local($win,$y,$x,$str) = @_;
        eval " &VOID( &wmove($win,$y,$x)== &ERR? &ERR: &wgetstr($win,$str))";
    }';
    eval 'sub mvwinch {
        local($win,$y,$x) = @_;
        eval " &VOID( &wmove($win,$y,$x) ==  &ERR ?  &ERR :  &winch($win))";
    }';
    eval 'sub mvwdelch {
        local($win,$y,$x) = @_;
        eval " &VOID( &wmove($win,$y,$x) ==  &ERR ?  &ERR :  &wdelch($win))";
    }';
    eval 'sub mvwinsch {
        local($win,$y,$x,$c) = @_;
        eval " &VOID( &wmove($win,$y,$x) ==  &ERR ?  &ERR: &winsch($win,$c))";
    }';
    eval 'sub mvaddch {
        local($y,$x,$ch) = @_;
        eval " &mvwaddch( &stdscr,$y,$x,$ch)";
    }';
    eval 'sub mvgetch {
        local($y,$x) = @_;
        eval " &mvwgetch( &stdscr,$y,$x)";
    }';
    eval 'sub mvaddbytes {
        local($y,$x,$da,$co) = @_;
        eval " &mvwaddbytes( &stdscr,$y,$x,$da,$co)";
    }';
    eval 'sub mvaddstr {
        local($y,$x,$str) = @_;
        eval " &mvwaddstr( &stdscr,$y,$x,$str)";
    }';
    eval 'sub mvgetstr {
        local($y,$x,$str) = @_;
        eval " &mvwgetstr( &stdscr,$y,$x,$str)";
    }';
    eval 'sub mvinch {
        local($y,$x) = @_;
        eval " &mvwinch( &stdscr,$y,$x)";
    }';
    eval 'sub mvdelch {
        local($y,$x) = @_;
        eval " &mvwdelch( &stdscr,$y,$x)";
    }';
    eval 'sub mvinsch {
        local($y,$x,$c) = @_;
        eval " &mvwinsch( &stdscr,$y,$x,$c)";
    }';
    eval 'sub clearok {
        local($win,$bf) = @_;
        eval "($win-> &_clear = $bf)";
    }';
    eval 'sub leaveok {
        local($win,$bf) = @_;
        eval "($win-> &_leave = $bf)";
    }';
    eval 'sub scrollok {
        local($win,$bf) = @_;
        eval "($win-> &_scroll = $bf)";
    }';
    eval 'sub flushok {
        local($win,$bf) = @_;
        eval "($bf ? ($win-> &_flags |=  &_FLUSH):($win-> &_flags &= ~ &_FLUSH))";
    }';
    eval 'sub getyx {
        local($win,$y,$x) = @_;
        eval "$y = $win-> &_cury, $x = $win-> &_curx";
    }';
    eval 'sub winch {
        local($win) = @_;
        eval "($win-> &_y[$win-> &_cury][$win-> &_curx] & 0177)";
    }';
    eval 'sub raw {
        eval "( &_tty. &sg_flags|= &RAW,  &_pfast= &_rawmode= &TRUE,  &stty( &_tty_ch,& &_tty))";
    }';
    eval 'sub noraw {
        eval "( &_tty. &sg_flags&=~ &RAW, &_rawmode= &FALSE, &_pfast=!( &_tty. &sg_flags& &CRMOD), &stty( &_tty_ch,& &_tty))";
    }';
    eval 'sub cbreak {
        eval "( &_tty. &sg_flags |=  &CBREAK,  &_rawmode =  &TRUE,  &stty( &_tty_ch,& &_tty))";
    }';
    eval 'sub nocbreak {
        eval "( &_tty. &sg_flags &= ~ &CBREAK, &_rawmode= &FALSE, &stty( &_tty_ch,& &_tty))";
    }';
    eval 'sub crmode {
        eval " &cbreak()";
    }';
    eval 'sub nocrmode {
        eval " &nocbreak()";
    }';
    eval 'sub echo {
        eval "( &_tty. &sg_flags |=  &ECHO,  &_echoit =  &TRUE,  &stty( &_tty_ch, & &_tty))";
    }';
    eval 'sub noecho {
        eval "( &_tty. &sg_flags &= ~ &ECHO,  &_echoit =  &FALSE,  &stty( &_tty_ch, & &_tty))";
    }';
    eval 'sub nl {
        eval "( &_tty. &sg_flags |=  &CRMOD, &_pfast =  &_rawmode, &stty( &_tty_ch, & &_tty))";
    }';
    eval 'sub nonl {
        eval "( &_tty. &sg_flags &= ~ &CRMOD,  &_pfast =  &TRUE,  &stty( &_tty_ch, & &_tty))";
    }';
    eval 'sub savetty {
        eval "(( &void)  &gtty( &_tty_ch, & &_tty),  &_res_flg =  &_tty. &sg_flags)";
    }';
    eval 'sub resetty {
        eval "( &_tty. &sg_flags =  &_res_flg, ( &void)  &stty( &_tty_ch, & &_tty))";
    }';
    eval 'sub erasechar {
        eval "( &_tty. &sg_erase)";
    }';
    eval 'sub killchar {
        eval "( &_tty. &sg_kill)";
    }';
    eval 'sub baudrate {
        eval "( &_tty. &sg_ospeed)";
    }';
    eval 'sub unctrl {
        local($c) = @_;
        eval " &_unctrl[($c) & 0177]";
    }';
}
1;
