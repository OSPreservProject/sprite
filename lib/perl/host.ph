if (!defined &_HOST) {
    eval 'sub _HOST {1;}';
    if (!defined &_TYPES) {
	require 'sys/types.ph';
    }
    if (!defined &_IN) {
	require 'netinet/in.ph';
    }
    require 'sys/stat.ph';
    eval 'sub HOST_ETHER_ADDRESS_SIZE {6;}';
}
1;
