if (!defined &_MATH_H) {
    eval 'sub _MATH_H {1;}';
    if (defined &__GNUC__) {
	eval 'sub _CONST_FUNC { &const;}';
    }
    else {
	eval 'sub _CONST_FUNC {1;}';
    }
    eval 'sub M_LN2 {0.69314718055994530942;}';
    eval 'sub M_PI {3.14159265358979323846;}';
    eval 'sub M_SQRT2 {1.41421356237309504880;}';
    eval 'sub M_E {2.7182818284590452354;}';
    eval 'sub M_LOG2E {1.4426950408889634074;}';
    eval 'sub M_LOG10E {0.43429448190325182765;}';
    eval 'sub M_LN10 {2.30258509299404568402;}';
    eval 'sub M_PI_2 {1.57079632679489661923;}';
    eval 'sub M_PI_4 {0.78539816339744830962;}';
    eval 'sub M_1_PI {0.31830988618379067154;}';
    eval 'sub M_2_PI {0.63661977236758134308;}';
    eval 'sub M_2_SQRTPI {1.12837916709551257390;}';
    eval 'sub M_SQRT1_2 {0.70710678118654752440;}';
    eval 'sub _POLY1 {
        local($x, $c) = @_;
        eval "(($c)[0] * ($x) + ($c)[1])";
    }';
    eval 'sub _POLY2 {
        local($x, $c) = @_;
        eval "( &_POLY1(($x), ($c)) * ($x) + ($c)[2])";
    }';
    eval 'sub _POLY3 {
        local($x, $c) = @_;
        eval "( &_POLY2(($x), ($c)) * ($x) + ($c)[3])";
    }';
    eval 'sub _POLY4 {
        local($x, $c) = @_;
        eval "( &_POLY3(($x), ($c)) * ($x) + ($c)[4])";
    }';
    eval 'sub _POLY5 {
        local($x, $c) = @_;
        eval "( &_POLY4(($x), ($c)) * ($x) + ($c)[5])";
    }';
    eval 'sub _POLY6 {
        local($x, $c) = @_;
        eval "( &_POLY5(($x), ($c)) * ($x) + ($c)[6])";
    }';
    eval 'sub _POLY7 {
        local($x, $c) = @_;
        eval "( &_POLY6(($x), ($c)) * ($x) + ($c)[7])";
    }';
    eval 'sub _POLY8 {
        local($x, $c) = @_;
        eval "( &_POLY7(($x), ($c)) * ($x) + ($c)[8])";
    }';
    eval 'sub _POLY9 {
        local($x, $c) = @_;
        eval "( &_POLY8(($x), ($c)) * ($x) + ($c)[9])";
    }';
    if (defined &__STDC__) {
	if (defined( &sun3) && !defined( &__STRICT_ANSI__) && !defined( &__SOFT_FLOAT__)) {
	    require 'math-68881.ph';
	}
	else {
	}
    }
    else {
    }
    if (!defined &HUGE_VAL) {
	eval 'sub HUGE_VAL {1.701411733192644270 &e38;}';
    }
    eval 'sub HUGE { &HUGE_VAL;}';
}
1;
