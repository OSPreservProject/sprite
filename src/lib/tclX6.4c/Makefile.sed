# convert include lines
s/^include \(.*\)/#include "\1"/g

# get rid of '-' before MAKEFLAGS
s/-\(\$(MAKEFLAGS)\)/\1/g

# add a default target
s/^all/default all/g


