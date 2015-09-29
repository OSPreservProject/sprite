if (!defined &__RPC_HEADER__) {
    eval 'sub __RPC_HEADER__ {1;}';
    require 'rpc/types.ph';
    require 'netinet/in.ph';
    require 'rpc/xdr.ph';
    require 'rpc/auth.ph';
    require 'rpc/clnt.ph';
    require 'rpc/rpc_msg.ph';
    require 'rpc/auth_unix.ph';
    require 'rpc/svc.ph';
    require 'rpc/svc_auth.ph';
    require 'rpc/netdb.ph';
}
1;
