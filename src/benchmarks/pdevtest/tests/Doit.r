#!/sprite/cmds/csh -f

echo "$1 Bytes"
foreach rep (1 2 3 4 5)
    pdevtest.new -M 1 >& /dev/null &
    sleep 1
    pdevtest.new -S -n 1000 -r -d $1
    sleep 2
end
