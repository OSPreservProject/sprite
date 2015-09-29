if (!defined &_HASH) {
    eval 'sub _HASH {1;}';
    if (!defined &_LIST) {
	require 'list.ph';
    }
    if (!defined &_SPRITE) {
	require 'sprite.ph';
    }
    eval 'sub HASH_STRING_KEYS {0;}';
    eval 'sub HASH_ONE_WORD_KEYS {1;}';
    eval 'sub Hash_GetValue {
        local($h) = @_;
        eval "(($h)-> &clientData)";
    }';
    eval 'sub Hash_SetValue {
        local($h, $val) = @_;
        eval "(($h)-> &clientData = ( &ClientData) ($val))";
    }';
    eval 'sub Hash_Size {
        local($n) = @_;
        eval "((($n) + $sizeof{\'int\'} - 1) / $sizeof{\'int\'})";
    }';
}
1;
