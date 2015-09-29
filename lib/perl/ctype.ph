if (!defined &_CTYPE) {
    eval 'sub _CTYPE {1;}';
    require 'cfuncproto.ph';
    if (!defined &EOF) {
	eval 'sub EOF {(-1);}';
    }
    eval 'sub isalnum {
        local($char) = @_;
        eval "(( &_ctype_bits+1)[$char] & ( &CTYPE_UPPER| &CTYPE_LOWER| &CTYPE_DIGIT))";
    }';
    eval 'sub isalpha {
        local($char) = @_;
        eval "(( &_ctype_bits+1)[$char] & ( &CTYPE_UPPER| &CTYPE_LOWER))";
    }';
    eval 'sub iscntrl {
        local($char) = @_;
        eval "(( &_ctype_bits+1)[$char] &  &CTYPE_CONTROL)";
    }';
    eval 'sub isdigit {
        local($char) = @_;
        eval "(( &_ctype_bits+1)[$char] &  &CTYPE_DIGIT)";
    }';
    eval 'sub isgraph {
        local($char) = @_;
        eval "(( &_ctype_bits+1)[$char] & ( &CTYPE_UPPER| &CTYPE_LOWER| &CTYPE_DIGIT| &CTYPE_PUNCT))";
    }';
    eval 'sub islower {
        local($char) = @_;
        eval "(( &_ctype_bits+1)[$char] &  &CTYPE_LOWER)";
    }';
    eval 'sub isprint {
        local($char) = @_;
        eval "(( &_ctype_bits+1)[$char] &  &CTYPE_PRINT)";
    }';
    eval 'sub ispunct {
        local($char) = @_;
        eval "(( &_ctype_bits+1)[$char] &  &CTYPE_PUNCT)";
    }';
    eval 'sub isspace {
        local($char) = @_;
        eval "(( &_ctype_bits+1)[$char] &  &CTYPE_SPACE)";
    }';
    eval 'sub isupper {
        local($char) = @_;
        eval "(( &_ctype_bits+1)[$char] &  &CTYPE_UPPER)";
    }';
    eval 'sub isxdigit {
        local($char) = @_;
        eval "(( &_ctype_bits+1)[$char] & ( &CTYPE_DIGIT| &CTYPE_HEX_DIGIT))";
    }';
    eval 'sub isascii {
        local($i) = @_;
        eval "(($i & ~0x7f) == 0)";
    }';
    eval 'sub CTYPE_UPPER {0x01;}';
    eval 'sub CTYPE_LOWER {0x02;}';
    eval 'sub CTYPE_DIGIT {0x04;}';
    eval 'sub CTYPE_SPACE {0x08;}';
    eval 'sub CTYPE_PUNCT {0x10;}';
    eval 'sub CTYPE_PRINT {0x20;}';
    eval 'sub CTYPE_CONTROL {0x40;}';
    eval 'sub CTYPE_HEX_DIGIT {0x80;}';
}
1;
