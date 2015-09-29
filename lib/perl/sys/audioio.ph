sub AUDIOGETREG { &_IOWR(ord('i'),1,'struct audio_ioctl');}
sub AUDIOSETREG { &_IOW(ord('i'),2,'struct audio_ioctl');}
sub AUDIOREADSTART { &_IO("1",3);}
sub AUDIOSTOP { &_IO("1",4);}
sub AUDIOPAUSE { &_IO("1",5);}
sub AUDIORESUME { &_IO("1",6);}
sub AUDIOREADQ { &_IOR("1",7,'int');}
sub AUDIOWRITEQ { &_IOR("1",8,'int');}
sub AUDIOGETQSIZE { &_IOR("1",9,'int');}
sub AUDIOSETQSIZE { &_IOW("1",10,'int');}
1;
