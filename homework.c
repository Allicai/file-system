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


#define ROOT_INODE_NUM 1

// helper method to translate a path to its inode number
int path_to_inode(const char *path) {

    // if the path is the root dir, return the root inode #
    if (strcmp(path, "/") == 0)
        return ROOT_INODE_NUM;

    // start from root directory's inode #
    int current_inode_num = ROOT_INODE_NUM;

    // navigate with given
    while (*path != '\0') {
        // get the current component of the path
        const char *next_separator = strchr(path, '/');
        if (next_separator == NULL)
            next_separator = path + strlen(path);

        // search for the current component in the dir entries
        int found = 0;
        for (int i = 0; i < FS_BLOCK_SIZE / sizeof(struct fs_dirent); i++) {
            struct fs_dirent entry;
            block_read(&entry, current_inode_num, 1);
            if (entry.valid && strncmp(entry.name, path, next_separator - path) == 0 &&
                (strlen(entry.name) == next_separator - path)) {
                // moving to the next dir
                current_inode_num = entry.inode; 
                found = 1;
                break;
            }
        }

        // -1 if nothing is found
        if (!found)
            return -1;

        // getting the next part of the path after a '/'
        path = next_separator;
        if (*path == '/')
            path++;
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
    /* TODO: your code here */

    // initialize
    
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
    return -EOPNOTSUPP;
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
    return -EOPNOTSUPP;
}

/* operations vector. Please don't rename it, or else you'll break things
 */
struct fuse_operations fs_ops = {
    .init = fs_init,
    .getattr = fs_getattr,
    .readdir = fs_readdir,
    .read = fs_read,
};

