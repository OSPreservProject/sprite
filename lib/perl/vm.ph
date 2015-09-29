if (!defined &_VMUSER) {
    eval 'sub _VMUSER {1;}';
    require 'sprite.ph';
    require 'vmStat.ph';
    if (defined &KERNEL) {
	require 'user/vmTypes.ph';
	require 'user/proc.ph';
    }
    else {
	require 'vmTypes.ph';
	require 'proc.ph';
    }
}
1;
