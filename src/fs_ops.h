#pragma once
#define _FILE_OFFSET_BITS 64
#define FUSE_USE_VERSION 26
#include <fuse.h>


struct fsm_state {
    int rootfd;
    char *rootdir;
};

void *fs_init(struct fuse_conn_info *info);

int fs_getattr(const char *path, struct stat *stbuf);

int fs_readlink(const char *path, char *link, size_t size);

int fs_mknod(const char *path, mode_t mode, dev_t dev);
/** Create a directory */
int fs_mkdir(const char *path, mode_t mode);
/** Remove a file */
int fs_unlink(const char *path);
/** Remove a directory */
int fs_rmdir(const char *path);
/** Create a symbolic link */
int fs_symlink(const char *path, const char *link);
/** Rename a file */
// both path and newpath are fs-relative
int fs_rename(const char *path, const char *newpath);
/** Create a hard link to a file */
int fs_link(const char *path, const char *newpath);
/** Change the permission bits of a file */
int fs_chmod(const char *path, mode_t mode);
/** Change the owner and group of a file */
int fs_chown(const char *path, uid_t uid, gid_t gid);
/** Change the size of a file */
int fs_truncate(const char *path, off_t newsize);
/** Change the access and/or modification times of a file */
/* note -- I'll want to change this as soon as 2.6 is in debian testing */
int fs_utime(const char *path, struct utimbuf *ubuf);
/** File open operation */
int fs_open(const char *path, struct fuse_file_info *fi);
/** Read data from an open file */
int fs_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi);
/** Write data to an open file */
int fs_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi);
/** Get file system statistics */
int fs_statfs(const char *path, struct statvfs *statv);
/** Possibly flush cached data */
int fs_flush(const char *path, struct fuse_file_info *fi);
/** Release an open file */
int fs_release(const char *path, struct fuse_file_info *fi);
/** Synchronize file contents */
int fs_fsync(const char *path, int datasync, struct fuse_file_info *fi);
#ifdef HAVE_SYS_XATTR_H
/** Set extended attributes */
int fs_setxattr(const char *path, const char *name, const char *value, size_t size, int flags);
/** Get extended attributes */
int fs_getxattr(const char *path, const char *name, char *value, size_t size);
/** List extended attributes */
int fs_listxattr(const char *path, char *list, size_t size);
/** Remove extended attributes */
int fs_removexattr(const char *path, const char *name);
#endif
/** Open directory */
int fs_opendir(const char *path, struct fuse_file_info *fi);
/** Read directory */
int fs_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi);
/** Release directory */
int fs_releasedir(const char *path, struct fuse_file_info *fi);
/** Synchronize directory contents */
int fs_fsyncdir(const char *path, int datasync, struct fuse_file_info *fi);
/** Clean up filesystem */
void fs_destroy(void *userdata);
/** Check file access permissions */
int fs_access(const char *path, int mask);
/** Change the size of an open file */
int fs_ftruncate(const char *path, off_t offset, struct fuse_file_info *fi);
/** Get attributes from an open file */
int fs_fgetattr(const char *path, struct stat *statbuf, struct fuse_file_info *fi);
