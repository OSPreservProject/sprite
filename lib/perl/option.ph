if (!defined &_OPTION) {
    eval 'sub _OPTION {1;}';
    require 'cfuncproto.ph';
    eval 'sub OPT_CONSTANT {
        local($val) = @_;
        eval "((\'int\') $val)";
    }';
    eval 'sub OPT_FALSE {0;}';
    eval 'sub OPT_TRUE {1;}';
    eval 'sub OPT_INT {-1;}';
    eval 'sub OPT_STRING {-2;}';
    eval 'sub OPT_REST {-3;}';
    eval 'sub OPT_FLOAT {-4;}';
    eval 'sub OPT_FUNC {-5;}';
    eval 'sub OPT_GENFUNC {-6;}';
    eval 'sub OPT_DOC {-7;}';
    eval 'sub OPT_TIME {-8;}';
    eval 'sub OPT_ALLOW_CLUSTERING {1;}';
    eval 'sub OPT_OPTIONS_FIRST {2;}';
    eval 'sub Opt_Number {
        local($optionArray) = @_;
        eval "($sizeof{$optionArray}/$sizeof{($optionArray}[0]))";
    }';
}
1;
