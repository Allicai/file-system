#!/bin/bash

# test dir contents
test_dir_contents() {
    dir="$1"
    expected_names="$2"

    names=$(ls $dir)
    names=$(echo $names | tr '\n' ' ') # converting newlines into spaces???

    if [ "$names" != "$expected_names" ]; then
        echo "DIRNAMES $dir"
        exit 1
    fi
}

# test file checksums
test_file_checksum() {
    file="$1"
    expected_checksum="$2"
    result=$(cksum $file)

    if [ "$result" != "$expected_checksum" ]; then
        echo "CKSUM $file"
        exit 1
    fi
}

# test file attributes
test_file_attributes() {
    file="$1"
    expected_output="$2"

    result=$(stat -c '%f %s' $file)

    if [ "$result" != "$expected_output" ]; then
        echo "MODE $file"
        exit 1
    fi
}

# test dir contents
test_dir_contents "fs/" "dir dir1 dir2 dir-with-long-name file.1 file.2"
test_dir_contents "fs/dir" "file1 file2 file3"
test_dir_contents "fs/dir1" "long-file-name subdir"
test_dir_contents "fs/dir1/subdir" ""
test_dir_contents "fs/dir-with-long-name" "2nd-file-with-long-name"
test_dir_contents "fs/dir2" "twenty-six-character-name twenty-seven-character-name"

# test file checksums
test_file_checksum "fs/file.1" "2178593158 900 fs/file.1"
test_file_checksum "fs/file.2" "3891739079 2000 fs/file.2"
test_file_checksum "fs/dir1/long-file-name" "2928220301 1025 fs/dir1/long-file-name"
test_file_checksum "fs/dir2/twenty-seven-character-name" "3761062583 100 fs/dir2/twenty-seven-character-name"
test_file_checksum "fs/dir2/twenty-six--character-name" "3611217367 100 fs/dir2/twenty-six--character-name"
test_file_checksum "fs/dir/file1" "2893082884 100 fs/dir/file1"
test_file_checksum "fs/dir/file2" "327293107 1200 fs/dir/2"
test_file_checksum "fs/dir/file3" "926221923 10111 fs/dir/file3"
test_file_checksum "fs/dir-with-long-name/2nd-file-with-long-name" "558398486 200 fs/dir-with-long-name/2nd-file-with-long-name"

# test file attributes
test_file_attributes "fs/" "41ff 4096"
test_file_attributes "fs/dir" "41ff 4096"
test_file_attributes "fs/dir/file1" "81ff 100"
test_file_attributes "fs/dir/file2" "81ed 1200"
test_file_attributes "fs/dir/file3" "81ff 10111"
test_file_attributes "fs/dir1" "41ed 4096"
test_file_attributes "fs/dir1/long-file-name" "81ff 1025"
test_file_attributes "fs/dir1/subdir" "41ed 4096"
test_file_attributes "fs/file.1" "81ff 900"
test_file_attributes "fs/dir-with-long-name" "41ed 4096"
test_file_attributes "fs/dir-with-long-name/2nd-file-with-long-name" "81ff 200"
test_file_attributes "fs/dir2" "41ed 4096"
test_file_attributes "fs/dir2/twenty-seven-character-name" "81ff 100"
test_file_attributes "fs/dir2/twenty-six--character-name" "81ff 100"
test_file_attributes "fs/file.2" "81a0 2000"

exit 0