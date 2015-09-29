#
# testlib.tcl --
#
# Test support routines.  Some of these are based on routines provided with
# standard Tcl.
#------------------------------------------------------------------------------
# Copyright 1992 Karl Lehenbauer and Mark Diekhans.
#
# Permission to use, copy, modify, and distribute this software and its
# documentation for any purpose and without fee is hereby granted, provided
# that the above copyright notice appear in all copies.  Karl Lehenbauer and
# Mark Diekhans make no representations about the suitability of this
# software for any purpose.  It is provided "as is" without express or
# implied warranty.
#------------------------------------------------------------------------------
# $Id: testlib.tcl,v 2.0 1992/10/16 04:49:22 markd Rel $
#------------------------------------------------------------------------------
#


rename unknown SAVED_UNKNOWN

#
# 
#
proc OutTestError {test_name test_description contents_of_test
                   passing_int_result passing_result int_result result} {
    set int(0) TCL_OK
    set int(1) TCL_ERROR
    set int(2) TCL_RETURN
    set int(3) TCL_BREAK
    set int(4) TCL_CONTINUE

    puts stdout "==== $test_name $test_description"
    puts stdout "==== Contents of test case:"
    puts stdout "$contents_of_test"
    puts stdout "==== Result was: $int($int_result)"
    puts stdout "$result"
    puts stdout "---- Result should have been: $int($passing_int_result)"
    puts stdout "$passing_result"
    puts stdout "---- $test_name FAILED" 

}

proc test {test_name test_description contents_of_test passing_results} {
    set answer [uplevel $contents_of_test]
    if {$answer != $passing_results}  { 
        puts stdout "==== $test_name $test_description"
        puts stdout "==== Contents of test case:"
        puts stdout "$contents_of_test"
        puts stdout "==== Result was:"
        puts stdout "$answer"
	puts stdout "---- Result should have been:"
	puts stdout "$passing_results"
	puts stdout "---- $test_name FAILED" 
    }
}

proc Test {test_name test_description contents_of_test passing_int_result
           passing_result} {
    set int_result [catch {uplevel $contents_of_test} result]

    if {($int_result != $passing_int_result) ||
        ($result != $passing_result)} {
        OutTestError $test_name $test_description $contents_of_test \
                     $passing_int_result $passing_result $int_result $result
    }
}

#
# Compare result against case-insensitive regular expression.
#

proc TestReg {test_name test_description contents_of_test passing_int_result
              passing_result} {
    set int_result [catch {uplevel $contents_of_test} result]

    if {($int_result != $passing_int_result) ||
        ![regexp -nocase $passing_result $result]} {
        OutTestError $test_name $test_description $contents_of_test \
                     $passing_int_result $passing_result $int_result $result
    }
}

proc dotests {file args} {
    global TESTS
    set savedTests $TESTS
    set TESTS $args
    source $file
    set TESTS $savedTests
}

# Genenerate a unique file record that can be verified.  The record
# grows quite large to test the dynamic buffering in the file I/O.

proc GenRec {id} {
    return [format "Key:%04d {This is a test of file I/O (%d)} KeyX:%04d %s" \
                    $id $id $id [replicate :@@@@@@@@: $id]]
}

