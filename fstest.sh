#!/bin/bash

# iterating through bug numbers
for bug_num in {1..14}; do
    echo "Testing with bug number $bug_num"
    
    # mount with the right bug num
    ./lab4-bug.x86_64 -bug $bug_num -image test.img fs
    
    # checking dir contents
    names=$(ls fs/dir)
    names=$(echo $names)
    if [ "$names" != "file1 file2 file3" ] ; then
        echo "DIRNAMES fs/dir"
        exit 1
    fi

    # checking file contents
    cksum_output=$(cksum fs/file.1)
    if [ "$cksum_output" != "3660301625 900 fs/file.1" ] ; then 
        echo "CKSUM fs/file.1"
        exit 1
    fi

    # checking mode
    mode=$(stat -c '%f %s' fs/dir1)
    if [ "$mode" != "41ed 4096" ] ; then
        echo "MODE fs/dir1"
        exit 1
    fi

    # test reading w/ different block sizes
    dd_output=$(dd if=fs/file.1 bs=300 status=none | cksum)
    if [ "$dd_output" != "3660301625 900" ] ; then 
        echo "CKSUM fs/file.1"
        exit 1
    fi
    
done

exit 0

