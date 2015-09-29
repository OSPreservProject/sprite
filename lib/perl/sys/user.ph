if (!defined &_SYS_USER_H) {
    eval 'sub _SYS_USER_H {1;}';
    require 'sys/param.ph';
    require 'sys/time.ph';
    require 'sys/resource.ph';
    require 'sys/exec.ph';
    require 'errno.ph';
}
1;
