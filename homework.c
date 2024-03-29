/*
 * file:        homework.c
 * description: skeleton file for CS 5600 system
 *
 * CS 5600, Computer Systems, Northeastern CCIS
 * Peter Desnoyers, November 2019
 * updated by CS3650 staff, March 2024
 */

#define FUSE_USE_VERSION 30
#define _FILE_OFFSET_BITS 64

#include <stdlib.h>
#include <stddef.h>
#include <unistd.h>
#include <fuse3/fuse.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <assert.h>
#include <sys/stat.h>

#include "fs3650.h"

/* disk access. All access is in terms of 4KB blocks; read and
 * write functions return 0 (success) or -EIO.
 */
extern int block_read(void *buf, int lba, int nblks);
extern int block_write(void *buf, int lba, int nblks);

/* Path splitting - interface is similar to the parsing function in Lab 2.
 *    char buf[PATH_MAX], *pathv[20];
 *    int n = split(path, &pathv, 20, &buf, sizeof(buf));
 */
int split(const char *path, char **pathv, int pathv_len, char *buf, int buf_len)
{
    char *p = buf, **pv = pathv;
    *pv++ = p;
    path++;

    if (*path == 0) return 0; // path is "/"

    while (pv < (pathv+pathv_len) && (p < buf+buf_len+1) && *path != 0) {
        char c = *path++;
        if (c == '/') {
            *p++ = 0;
            *pv++ = p;
        }
        else
            *p++ = c;
    }
    *p++ = 0;
    return pv-pathv;
}

/* init - this is called once by the FUSE framework at startup.
 * it reads the superblock into 'super'
 */
struct fs_super super;

void* fs_init(struct fuse_conn_info *conn, struct fuse_config *cfg)
{
    block_read(&super, 0, 1);   /* read superblock */
    assert(super.magic == FS_MAGIC); /* confirm magic number */
    cfg->direct_io = 1;         /* allow non-block-sized reads */

    return NULL;
}

/* Note on path translation errors:
 * In addition to the method-specific errors listed below, almost
 * every method can return one of the following errors if it fails to
 * locate a file or directory corresponding to a specified path.
 *
 * ENOENT - a component of the path doesn't exist.
 * ENOTDIR - an intermediate component of the path (e.g. 'a' or 'b' in
 *           /a/b/c) is not a directory
 */

/* Exercises: You will need to implement the following functions:
 *   fs_getattr
 *   fs_readdir
 *   fs_read
 */

// inode 1 is the root dir
// inode 0 is invalid (super)
#define ROOT_INODE_NUM 1

// helper method to translate a path to its inode number as suggested
int path_to_inode(const char *path) {
    // if the path is the root dir, return the root inode #
    if (strcmp(path, "/") == 0)
        return ROOT_INODE_NUM;

    // start from root directory's inode #
    int current_inode_num = ROOT_INODE_NUM;
    char *pathv[MAX_PATH_NAMES];
    char path_buf[MAX_PATH_BYTES];
    

    // navigate through each component of the path
    int paths = split(path, pathv, MAX_PATH_NAMES, path_buf, MAX_PATH_BYTES);
    for (int i = 0; i < paths; i++) {
        // get the current component of the path
        const char *current_component = pathv[i];

        // search for the current component in the directory entries
        int found = 0;
        struct fs_inode current_inode;
        block_read(&current_inode, current_inode_num, 1);
        for (int j = 0; j < (FS_BLOCK_SIZE / sizeof(struct fs_dirent)); j++) {
            struct fs_dirent entry[FS_BLOCK_SIZE / sizeof(struct fs_dirent)];
            block_read(entry, current_inode.ptrs[j], 1);
            for (int k = 0; k< (FS_BLOCK_SIZE / sizeof(struct fs_dirent)); k++) {
                if (entry[k].valid && strcmp(entry[k].name, current_component) == 0) {
                    // found the current component in the directory
                    current_inode_num = entry[k].inode;
                    found = 1;
                    break;
                }
            
            }
        }

        // if current component not found, return error
        if (!found)
            return -ENOENT;
    }

    // return the inode number of the final part of the path
    return current_inode_num;
}


/* Exercise 1:
 * getattr - get file or directory attributes. For a description of
 *  the fields in 'struct stat', see 'man lstat'.
 *
 * You should:
 *  1. parse the path given by "const char *path",
 *     find the inode of the specified file,
 *  2. copy inode's information to "struct stat",
 *     Note - for several fields in 'struct stat' there is
 *         no corresponding information in our file system:
 *          st_nlink - always set it to 1
 *          st_atime, st_ctime - set to same value as st_mtime
 *  3. and return:
 *     * success - return 0
 *     * errors - path translation, ENOENT
 *
 *  hints:
 *  - the helper function "split(...)" is useful
 *  - "block_read(...)" is useful; it reads of data from disk
 *  - it will be useful to create your own helper function to
 *    translate a path to an inode number
 */
int fs_getattr(const char *path, struct stat *sb, struct fuse_file_info *fi)
{
    int inode_num = path_to_inode(path);
    if (inode_num == -ENOENT)
        return -ENOENT; // no such file or dir

    // read the corresponding inode
    struct fs_inode inode;
    block_read(&inode, inode_num, 1);

    // fill struct with inode info
    sb->st_mode = inode.mode;
    sb->st_uid = inode.uid;
    sb->st_gid = inode.gid;
    sb->st_size = inode.size;
    sb->st_nlink = 1; // always 1
    // last access and last modification are presumably the same time
    sb->st_atime = inode.mtime; // last access time
    sb->st_mtime = inode.mtime; // last mod time
    sb->st_ctime = inode.ctime; // creation time

    return 0; // return 0
    
}


/* Exercise 2:
 * readdir - get directory contents.
 *
 * call the 'filler' function once for each valid entry in the
 * directory, as follows:
 *     filler(buf, <name>, <statbuf>, 0, 0)
 * where
 *   - "ptr" is the second argument
 *   - <name> is the name of the file/dir (the name in the direntry)
 *   - <statbuf> is a pointer to struct stat, just like in getattr.
 *
 * success - return 0
 * errors - path resolution, ENOTDIR, ENOENT
 *
 * hints:
 *   - this process is similar to the fs_getattr:
 *     -- you will walk file system to find the dir pointed by "path",
 *     -- then you need to call "filler" for each of
 *        the *valid* entry in this dir
 *   - you can ignore "struct fuse_file_info *fi" (also apply to later Exercises)

 */
int fs_readdir(const char *path, void *ptr, fuse_fill_dir_t filler, off_t offset,
               struct fuse_file_info *fi, enum fuse_readdir_flags flags)
{
    /* TODO: your code here */

    int inode_num = path_to_inode(path);
    if (inode_num == -ENOENT)
        return -ENOENT; // no such file or dir
    
    //read the dir inode
    struct fs_inode dir_inode;
    block_read(&dir_inode, inode_num, 1);

    // check if it's a dir
    // using the testing function from piazza post @532
    if (!(S_ISDIR(dir_inode.mode)))
        return -ENOTDIR; // not a dir

    // iterate throguh all the blocks of the directory
    for (int i = 0; i < sizeof(dir_inode.ptrs) / sizeof(dir_inode.ptrs[0]); i++) {
        char block_data[FS_BLOCK_SIZE];
        block_read(block_data, dir_inode.ptrs[i], 1);
    

        // iterate dir entries and add them to the filler
        for (int j = 0; i < FS_BLOCK_SIZE / sizeof(struct fs_dirent); j++) {
            struct fs_dirent entry;
            memcpy(&entry, block_data + j * sizeof(struct fs_dirent), sizeof(struct fs_dirent));

            if (entry.valid) {
                if (filler(ptr, entry.name, NULL, 0, 0) != 0)
                    return -ENOMEM; // the buffer is full
            }
        }
    }

    return 0; // success :D
}

/* Exercise 3:
 * read - read data from an open file.
 * should return the number of bytes requested, except:
 *   - if offset >= file len, return 0
 *   - if offset+len > file len, return bytes from offset to EOF
 *   - on error, return <0
 * Errors - path resolution, ENOENT, EISDIR
 */
int fs_read(const char *path, char *buf, size_t len, off_t offset,
            struct fuse_file_info *fi)
{
    /* TODO: your code here */
    
    int inode_num = path_to_inode(path);
    if (inode_num == -ENOENT)
        return -ENOENT; // no such file or dir
    
    //read the file inode
    struct fs_inode file_inode;
    block_read(&file_inode, inode_num, 1);

    // check if it's a file
    if (!(S_ISREG(file_inode.mode)))
        return -EISDIR; // not a file

    // calculate the number of bytes available to read from the file
    size_t available_bytes = file_inode.size - offset;
    if (available_bytes <= 0)
        return 0; // offset is at or beyond the end of the file

    // determine the number of bytes to read based on len and available bytes
    size_t bytes_to_read = (len < available_bytes) ? len : available_bytes;

    // calculate the block index and offset within the block for the start of reading
    int block_index = offset / FS_BLOCK_SIZE;
    int block_offset = offset % FS_BLOCK_SIZE;

    // read data from the file into the buffer
    size_t bytes_read = 0;
    while (bytes_read < bytes_to_read) {
        // read data from the current block
        char block_data[FS_BLOCK_SIZE];
        block_read(block_data, file_inode.ptrs[block_index], 1);

        // calculate the number of bytes to copy from the block to the buffer
        size_t bytes_to_copy = (bytes_to_read - bytes_read < FS_BLOCK_SIZE - block_offset) ?
                               bytes_to_read - bytes_read : FS_BLOCK_SIZE - block_offset;

        // copy data from block_data to buf
        memcpy(buf + bytes_read, block_data + block_offset, bytes_to_copy);

        // update variables for next iteration
        bytes_read += bytes_to_copy;
        block_index++;
        block_offset = 0; // for subsequent blocks, start reading from the beginning

        // if all bytes have been read or we've reached the end of the file, break
        if (bytes_read >= bytes_to_read || bytes_read + offset >= file_inode.size)
            break;
    }

    return bytes_to_read; // return the number of bytes read
}

/* operations vector. Please don't rename it, or else you'll break things
 */
struct fuse_operations fs_ops = {
    .init = fs_init,
    .getattr = fs_getattr,
    .readdir = fs_readdir,
    .read = fs_read,
};
