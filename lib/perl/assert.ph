require 'cfuncproto.ph';
if (defined &assert) {
}
if (defined &_assert) {
}
if (!defined &NDEBUG) {
    if (defined &KERNEL) {
	if (defined &__STDC__) {
	    eval 'sub _assert {
	        local($ex) = @_;
	        eval "{  &if (!($ex)) {  &panic( \\"Assertion failed: (\\" #$ex \\") file \\\\\\"%s\\\\\\", line %d\\\\n\\",  &__FILE__,  &__LINE__);}}";
	    }';
	}
	else {
	    eval 'sub _assert {
	        local($ex) = @_;
	        eval "{  &if (!($ex)) {  &panic( \\"Assertion failed: file \\\\\\"%s\\\\\\", line %d\\\\n\\",  &__FILE__,  &__LINE__);}}";
	    }';
	}
    }
    else {
	if (defined &__STDC__) {
	    eval 'sub _assert {
	        local($ex) = @_;
	        eval "{  &if (!($ex)) {  &__eprintf( \\"Assertion failed: (\\" #$ex \\") line %d of \\\\\\"%s\\\\\\"\\\\n\\",  &__LINE__,  &__FILE__);  &abort();}}";
	    }';
	}
	else {
	    eval 'sub _assert {
	        local($ex) = @_;
	        eval "{  &if (!($ex)) {  &__eprintf( \\"Assertion failed: line %d of \\\\\\"%s\\\\\\"\\\\n\\",  &__LINE__,  &__FILE__);  &abort();}}";
	    }';
	}
    }
    eval 'sub assert {
        local($ex) = @_;
        eval " &_assert($ex)";
    }';
}
else {
    eval 'sub _assert {
        local($ex) = @_;
        eval "";
    }';
    eval 'sub assert {
        local($ex) = @_;
        eval "";
    }';
}
1;
