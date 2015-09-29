require 'stdio.ph';
require 'setjmp.ph';
sub TRUE {1;}
sub FALSE {0;}
sub OBJNULL {(( &object) &NULL);}
sub fix {
    local($x) = @_;
    eval "($x)-> &FIX. &FIXVAL";
}
sub SMALL_FIXNUM_LIMIT {1024;}
sub small_fixnum {
    local($i) = @_;
    eval "( &object)( &small_fixnum_table+ &SMALL_FIXNUM_LIMIT+($i))";
}
sub sf {
    local($x) = @_;
    eval "($x)-> &SF. &SFVAL";
}
sub lf {
    local($x) = @_;
    eval "($x)-> &LF. &LFVAL";
}
sub code_char {
    local($c) = @_;
    eval "( &object)( &character_table+($c))";
}
sub char_code {
    local($x) = @_;
    eval "($x)-> &ch. &ch_code";
}
sub char_font {
    local($x) = @_;
    eval "($x)-> &ch. &ch_font";
}
sub char_bits {
    local($x) = @_;
    eval "($x)-> &ch. &ch_bits";
}
sub s_fillp { &st_fillp;}
sub s_self { &st_self;}
sub type_of {
    local($obje) = @_;
    eval "(( &enum  &type)((( &object)($obje))-> &d. &t))";
}
sub endp {
    local($obje) = @_;
    eval " &endp1($obje)";
}
sub vs_org { &value_stack;}
sub vs_push {
    local($obje) = @_;
    eval "(* &vs_top++ = ($obje))";
}
sub vs_pop {(*-- &vs_top);}
sub vs_head { &vs_top[-1];}
sub vs_mark { &object * &old_vs_top =  &vs_top;}
sub vs_reset { &vs_top =  &old_vs_top;}
sub vs_check { &if ( &vs_top >=  &vs_limit)  &vs_overflow();;}
sub vs_check_push {
    local($obje) = @_;
    eval "( &vs_top >=  &vs_limit ? ( &object) &vs_overflow() : (* &vs_top++ = ($obje)))";
}
sub check_arg {
    local($n) = @_;
    eval " &if ( &vs_top -  &vs_base != ($n))  &check_arg_failed($n)";
}
sub MMcheck_arg {
    local($n) = @_;
    eval " &if ( &vs_top -  &vs_base < ($n))  &too_few_arguments();  &else  &if ( &vs_top -  &vs_base > ($n))  &too_many_arguments()";
}
sub vs_reserve {
    local($x) = @_;
    eval " &if( &vs_base+($x) >=  &vs_limit)  &vs_overflow();";
}
sub bds_org { &bind_stack;}
sub bds_check { &if ( &bds_top >=  &bds_limit)  &bds_overflow();}
sub bds_bind {
    local($sym, $val) = @_;
    eval "((++ &bds_top)-> &bds_sym = ($sym),  &bds_top-> &bds_val = ($sym)-> &s. &s_dbind, ($sym)-> &s. &s_dbind = ($val))";
}
sub bds_unwind1 {(( &bds_top-> &bds_sym)-> &s. &s_dbind =  &bds_top-> &bds_val, -- &bds_top);}
sub ihs_org { &ihs_stack;}
sub ihs_check { &if ( &ihs_top >=  &ihs_limit)  &ihs_overflow();}
sub ihs_push {
    local($function) = @_;
    eval "(++ &ihs_top)-> &ihs_function = ($function);  &ihs_top-> &ihs_base =  &vs_base";
}
sub ihs_pop {
    eval "( &ihs_top--)";
}
sub alloc_frame_id {
    eval " &alloc_object( &t_spice)";
}
sub frs_org { &frame_stack;}
sub frs_push {
    local($class, $val) = @_;
    eval " &if (++ &frs_top >=  &frs_limit)  &frs_overflow();  &frs_top-> &frs_lex =  &lex_env;  &frs_top-> &frs_bds_top =  &bds_top;  &frs_top-> &frs_class = ($class);  &frs_top-> &frs_val = ($val);  &frs_top-> &frs_ihs =  &ihs_top;  &setjmp( &frs_top-> &frs_jmpbuf)";
}
sub frs_pop {
    eval " &frs_top--";
}
sub MMcons {
    local($a,$d) = @_;
    eval " &make_cons(($a),($d))";
}
sub MMcar {
    local($x) = @_;
    eval "($x)-> &c. &c_car";
}
sub MMcdr {
    local($x) = @_;
    eval "($x)-> &c. &c_cdr";
}
sub CMPcar {
    local($x) = @_;
    eval "($x)-> &c. &c_car";
}
sub CMPcdr {
    local($x) = @_;
    eval "($x)-> &c. &c_cdr";
}
sub CMPcaar {
    local($x) = @_;
    eval "($x)-> &c. &c_car-> &c. &c_car";
}
sub CMPcadr {
    local($x) = @_;
    eval "($x)-> &c. &c_cdr-> &c. &c_car";
}
sub CMPcdar {
    local($x) = @_;
    eval "($x)-> &c. &c_car-> &c. &c_cdr";
}
sub CMPcddr {
    local($x) = @_;
    eval "($x)-> &c. &c_cdr-> &c. &c_cdr";
}
sub CMPcaaar {
    local($x) = @_;
    eval "($x)-> &c. &c_car-> &c. &c_car-> &c. &c_car";
}
sub CMPcaadr {
    local($x) = @_;
    eval "($x)-> &c. &c_cdr-> &c. &c_car-> &c. &c_car";
}
sub CMPcadar {
    local($x) = @_;
    eval "($x)-> &c. &c_car-> &c. &c_cdr-> &c. &c_car";
}
sub CMPcaddr {
    local($x) = @_;
    eval "($x)-> &c. &c_cdr-> &c. &c_cdr-> &c. &c_car";
}
sub CMPcdaar {
    local($x) = @_;
    eval "($x)-> &c. &c_car-> &c. &c_car-> &c. &c_cdr";
}
sub CMPcdadr {
    local($x) = @_;
    eval "($x)-> &c. &c_cdr-> &c. &c_car-> &c. &c_cdr";
}
sub CMPcddar {
    local($x) = @_;
    eval "($x)-> &c. &c_car-> &c. &c_cdr-> &c. &c_cdr";
}
sub CMPcdddr {
    local($x) = @_;
    eval "($x)-> &c. &c_cdr-> &c. &c_cdr-> &c. &c_cdr";
}
sub CMPcaaaar {
    local($x) = @_;
    eval "($x)-> &c. &c_car-> &c. &c_car-> &c. &c_car-> &c. &c_car";
}
sub CMPcaaadr {
    local($x) = @_;
    eval "($x)-> &c. &c_cdr-> &c. &c_car-> &c. &c_car-> &c. &c_car";
}
sub CMPcaadar {
    local($x) = @_;
    eval "($x)-> &c. &c_car-> &c. &c_cdr-> &c. &c_car-> &c. &c_car";
}
sub CMPcaaddr {
    local($x) = @_;
    eval "($x)-> &c. &c_cdr-> &c. &c_cdr-> &c. &c_car-> &c. &c_car";
}
sub CMPcadaar {
    local($x) = @_;
    eval "($x)-> &c. &c_car-> &c. &c_car-> &c. &c_cdr-> &c. &c_car";
}
sub CMPcadadr {
    local($x) = @_;
    eval "($x)-> &c. &c_cdr-> &c. &c_car-> &c. &c_cdr-> &c. &c_car";
}
sub CMPcaddar {
    local($x) = @_;
    eval "($x)-> &c. &c_car-> &c. &c_cdr-> &c. &c_cdr-> &c. &c_car";
}
sub CMPcadddr {
    local($x) = @_;
    eval "($x)-> &c. &c_cdr-> &c. &c_cdr-> &c. &c_cdr-> &c. &c_car";
}
sub CMPcdaaar {
    local($x) = @_;
    eval "($x)-> &c. &c_car-> &c. &c_car-> &c. &c_car-> &c. &c_cdr";
}
sub CMPcdaadr {
    local($x) = @_;
    eval "($x)-> &c. &c_cdr-> &c. &c_car-> &c. &c_car-> &c. &c_cdr";
}
sub CMPcdadar {
    local($x) = @_;
    eval "($x)-> &c. &c_car-> &c. &c_cdr-> &c. &c_car-> &c. &c_cdr";
}
sub CMPcdaddr {
    local($x) = @_;
    eval "($x)-> &c. &c_cdr-> &c. &c_cdr-> &c. &c_car-> &c. &c_cdr";
}
sub CMPcddaar {
    local($x) = @_;
    eval "($x)-> &c. &c_car-> &c. &c_car-> &c. &c_cdr-> &c. &c_cdr";
}
sub CMPcddadr {
    local($x) = @_;
    eval "($x)-> &c. &c_cdr-> &c. &c_car-> &c. &c_cdr-> &c. &c_cdr";
}
sub CMPcdddar {
    local($x) = @_;
    eval "($x)-> &c. &c_car-> &c. &c_cdr-> &c. &c_cdr-> &c. &c_cdr";
}
sub CMPcddddr {
    local($x) = @_;
    eval "($x)-> &c. &c_cdr-> &c. &c_cdr-> &c. &c_cdr-> &c. &c_cdr";
}
sub CMPfuncall { &funcall;}
sub cclosure_call { &funcall;}
sub Cnil {(( &object)& &Cnil_body);}
sub Ct {(( &object)& &Ct_body);}
sub CMPmake_fixnum {
    local($x) = @_;
    eval "(((( &FIXtemp=($x))+1024)&-2048)==0? &small_fixnum( &FIXtemp): &make_fixnum( &FIXtemp))";
}
sub Creturn {
    local($v) = @_;
    eval " &return(( &vs_top= &vs,($v)))";
}
sub Cexit { &return(( &vs_top= &vs,0));}
1;
