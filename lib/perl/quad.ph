if (!defined &_QUAD) {
    eval 'sub _QUAD {1;}';
    require 'stdio.ph';
    require 'sys/types.ph';
    eval 'sub QUAD_MOST_SIG {1;}';
    eval 'sub QUAD_LEAST_SIG {0;}';
    eval 'sub Quad_EQ {
        local($q1, $q2) = @_;
        eval "(($q1). &val[ &QUAD_LEAST_SIG] == ($q2). &val[ &QUAD_LEAST_SIG] && ($q1). &val[ &QUAD_MOST_SIG] == ($q2). &val[ &QUAD_MOST_SIG])";
    }';
}
1;
