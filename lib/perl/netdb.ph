if (!defined &_NETDB) {
    eval 'sub _NETDB {1;}';
    eval 'sub h_addr { &h_addr_list[0];}';
    eval 'sub HOST_NOT_FOUND {1;}';
    eval 'sub TRY_AGAIN {2;}';
    eval 'sub NO_RECOVERY {3;}';
    eval 'sub NO_DATA {4;}';
    eval 'sub NO_ADDRESS { &NO_DATA;}';
}
1;
