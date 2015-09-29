if (!defined &_INET) {
    eval 'sub _INET {1;}';
    eval 'sub INET_STREAM_NAME_FORMAT {"/hosts/%s/netTCP";}';
    eval 'sub INET_DGRAM_NAME_FORMAT {"/hosts/%s/netUDP";}';
    eval 'sub INET_RAW_NAME_FORMAT {"/hosts/%s/netIP";}';
    eval 'sub INET_PRIV_PORTS {1024;}';
    eval 'sub INET_SERVER_PORTS {5000;}';
}
1;
