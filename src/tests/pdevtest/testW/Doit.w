#!/sprite/cmds/csh -f

echo "$1 Bytes (sync)"
foreach rep (1 2 3 4 5)
    pdevtest.new -M 1 $2 >& /dev/null &
    sleep 1
    pdevtest.new -S -n 1000 -w -d $1
    sleep 2
end
