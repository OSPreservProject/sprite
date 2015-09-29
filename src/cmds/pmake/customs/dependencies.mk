# DO NOT DELETE THIS LINE -- make depend depends on it.
arch.o          :  ../src/make.h ../lib/include/sprite.h
arch.o          : ../lib/lst/lst.h config.h ../src/nonints.h
arch.o          : ../lib/include/hash.h ../lib/include/list.h
compat.o        :  ../src/make.h ../lib/include/sprite.h
compat.o        : ../lib/lst/lst.h config.h ../src/nonints.h
cond.o          :  ../src/make.h
cond.o          :  ../lib/include/sprite.h
cond.o          :  ../lib/lst/lst.h config.h
cond.o          : ../src/nonints.h ../lib/include/buf.h
dir.o           :  ../src/make.h
dir.o           : ../lib/include/sprite.h 
dir.o           : ../lib/lst/lst.h config.h ../src/nonints.h
dir.o           : ../lib/include/hash.h ../lib/include/list.h
make.o          : ../src/make.h 
make.o          : ../lib/include/sprite.h 
make.o          : ../lib/lst/lst.h config.h ../src/nonints.h
job.o           :  ../src/make.h ../lib/include/sprite.h
job.o           : ../lib/lst/lst.h config.h ../src/nonints.h customs.h
job.o           :  rpc.h 
main.o          :  ../src/make.h ../lib/include/sprite.h
main.o          : ../lib/lst/lst.h config.h ../src/nonints.h
main.o          : ../lib/include/option.h
parse.o         :   ../src/make.h
parse.o         :  ../lib/include/sprite.h
parse.o         : ../lib/lst/lst.h config.h ../src/nonints.h
parse.o         : ../lib/include/buf.h
suff.o          :  ../src/make.h 
suff.o          : ../lib/include/sprite.h 
suff.o          : ../lib/lst/lst.h config.h ../src/nonints.h
suff.o          : ../lib/include/bit.h
targ.o          :   ../src/make.h
targ.o          :  ../lib/include/sprite.h
targ.o          :  ../lib/lst/lst.h config.h
targ.o          : ../src/nonints.h ../lib/include/hash.h ../lib/include/list.h
rmt.o           : ../src/make.h 
rmt.o           : ../lib/include/sprite.h 
rmt.o           : ../lib/lst/lst.h config.h ../unix/config.h ../src/nonints.h
rmt.o           : customs.h  
rmt.o           :  rpc.h 
str.o           : ../src/make.h 
str.o           : ../lib/include/sprite.h 
str.o           : ../lib/lst/lst.h config.h ../src/nonints.h
var.o           :  ../src/make.h 
var.o           : ../lib/include/sprite.h ../lib/lst/lst.h config.h
var.o           : ../src/nonints.h ../lib/include/buf.h
customs.o       : ../lib/include/sprite.h customsInt.h customs.h
customs.o       :   rpc.h
mca.o           : customsInt.h ../lib/include/sprite.h customs.h
mca.o           :   rpc.h
mca.o           :  ../lib/lst/lst.h
mca.o           :   log.h
avail.o         : customsInt.h ../lib/include/sprite.h customs.h
avail.o         :   rpc.h
import.o        : customsInt.h ../lib/include/sprite.h customs.h
import.o        :   rpc.h
import.o        :  ../lib/lst/lst.h log.h
election.o      : customsInt.h ../lib/include/sprite.h customs.h
election.o      :   rpc.h
election.o      :  log.h 
rpc.o           :   rpc.h
xdr.o           : customs.h  
xdr.o           :   rpc.h log.h
log.o           : customsInt.h ../lib/include/sprite.h customs.h
log.o           :   rpc.h
log.o           :  log.h 
swap.o          : customsInt.h ../lib/include/sprite.h customs.h
swap.o          :   rpc.h
host.o          : customs.h  
host.o          :   rpc.h
export.o        :   customs.h
export.o        :  rpc.h
importquota.o   : customs.h  
importquota.o   :   rpc.h
reginfo.o       : customs.h  
reginfo.o       :   rpc.h
logd.o          : customs.h  
logd.o          :   rpc.h log.h
cctrl.o         : customs.h  
cctrl.o         :   rpc.h
customslib.o    : customs.h  
customslib.o    :   rpc.h
