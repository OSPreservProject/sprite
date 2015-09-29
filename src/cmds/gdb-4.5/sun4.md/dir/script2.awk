$1 == $1 { 
	print "touch:", $1
        command = sprintf("touch %s", $1)
	system(command)
}
