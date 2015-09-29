if (!defined &_IN_VAR) {
    eval 'sub _IN_VAR {1;}';
    eval 'sub ia_addr { &ia_ifa. &ifa_addr;}';
    eval 'sub ia_broadaddr { &ia_ifa. &ifa_broadaddr;}';
    eval 'sub ia_dstaddr { &ia_ifa. &ifa_dstaddr;}';
    eval 'sub ia_ifp { &ia_ifa. &ifa_ifp;}';
    eval 'sub IA_SIN {
        local($ia) = @_;
        eval "((\'struct sockaddr_in\' *)(&((\'struct in_ifaddr\' *)$ia)-> &ia_addr))";
    }';
    eval 'sub IFA_ROUTE {0x01;}';
    if (defined &KERNEL) {
    }
}
1;
