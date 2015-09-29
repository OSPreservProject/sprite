if (!defined &_RPCUSER) {
    eval 'sub _RPCUSER {1;}';
    require 'cfuncproto.ph';
    require 'sprite.ph';
    require 'spriteTime.ph';
    eval 'sub RPC_MAX_NAME_LENGTH {100;}';
    eval 'sub TEST_RPC_ECHO {1;}';
    eval 'sub TEST_RPC_SEND {2;}';
    eval 'sub TEST_RPC_BOUNCE {3;}';
}
1;
