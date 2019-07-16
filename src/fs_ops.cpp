#include <cstring>

#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>

#include <libgen.h>
#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#ifdef HAVE_SYS_XATTR_H
#include <sys/xattr.h>
#endif

#include <log/logger.hpp>
#include "fs_ops.h"

#define FSM_DATA ((struct fsm_state *) fuse_get_context()->private_data)

static void get_relative_path(char *rpath, const char *path)
{
    if (path[0] == '/') {
        if (strlen(path) == 1)
            strcpy(rpath, ".");
        else
            strcpy(rpath, path + 1);
    } else {
        strcpy(rpath, path);
    }
}
static void get_absolute_path(char *apath, const char *path)
{
    strcpy(apath, FSM_DATA->rootdir);
    strncat(apath, path, PATH_MAX);
}
int check_permission(const char *op)
{
    fuse_context *ctx = fuse_get_context();
    LDEBUG("fuse_context : [{}] [{}] [{}]", ctx->pid, ctx->uid, ctx->gid);
    char temp[BUFSIZ] = {0};
    char path[BUFSIZ] = {0};
    ssize_t size = 0;
    snprintf(temp, BUFSIZ - 1, "/proc/%d/exe", ctx->pid);
    if ((size = readlink(temp, path, BUFSIZ)) < 0) {
        return -1;
    }
    path[size] = '\0';
    LDEBUG("process name [{}]", path);
    if (strstr(path, "VRVZYJCOMM"))
        return 0;
    snprintf(temp, BUFSIZ - 1, "/proc/%d/exe", getpgid(ctx->pid));
    size = 0;
    if ((size = readlink(temp, path, BUFSIZ)) < 0) {
        return -1;
    }
    path[size] = '\0';
    LDEBUG("group process name [{}]", path);
    if (strstr(path, "VRVZYJCOMM"))
        return 0;

    return -1;
}

/** Initialize filesystem */
void *fs_init(struct fuse_conn_info *info)
{
    int fd = FSM_DATA->rootfd;
    if (fchdir(fd) < 0) {
        LDEBUG("fs_init error...");
        abort();
    } else {
        close(fd);
    }
    // 必须要有返回值，不然 fuse_get_context 将获取不到该值
    return FSM_DATA;
}

int fs_getattr(const char *path, struct stat *stbuf)
{
    int retstat = -EACCES;
    char rpath[PATH_MAX] = {0}, apath[PATH_MAX] = {0};
    get_relative_path(rpath, path);
    get_absolute_path(apath, path);
    LDEBUG("[{}] [{}] [{}]", path, rpath, apath);
    if (strcmp(rpath, ".") != 0) {
        if (check_permission("getattr")) return retstat;
    }
    retstat = lstat(rpath, stbuf);
    if (retstat < 0) retstat = -errno;
    return retstat;
}

int fs_readlink(const char *path, char *link, size_t size)
{
    int retstat = -EACCES;
    char rpath[PATH_MAX] = {0}, apath[PATH_MAX] = {0};
    get_relative_path(rpath, path);
    get_absolute_path(apath, path);
    LDEBUG("[{}] [{}] [{}]", path, rpath, apath);
    if (strcmp(rpath, ".") != 0)
        if (check_permission("readlink")) return retstat;
    retstat = readlink(rpath, link, size);
    if (retstat < 0) retstat = -errno;
    return retstat;
}

int fs_mknod(const char *path, mode_t mode, dev_t dev)
{
    int retstat = -EACCES;
    char rpath[PATH_MAX] = {0}, apath[PATH_MAX] = {0};
    get_relative_path(rpath, path);
    get_absolute_path(apath, path);
    LDEBUG("[{}] [{}] [{}]", path, rpath, apath);
    if (check_permission("mknod")) return retstat;

    if (S_ISREG(mode)) {
        retstat = open(rpath, O_CREAT | O_EXCL | O_WRONLY, mode);
        if (retstat >= 0)
            retstat = close(retstat);
    } else {
        if (S_ISFIFO(mode))
            retstat = mkfifo(rpath, mode);
        else
            retstat = mknod(rpath, mode, dev);
    }
    return retstat;
}

/** Create a directory */
int fs_mkdir(const char *path, mode_t mode)
{
    int retstat = -EACCES;
    char rpath[PATH_MAX] = {0}, apath[PATH_MAX] = {0};
    get_relative_path(rpath, path);
    get_absolute_path(apath, path);
    LDEBUG("[{}] [{}] [{}]", path, rpath, apath);
    if (check_permission("mkdir")) return retstat;
    retstat = mkdir(rpath, mode);
    if (retstat < 0) retstat = -errno;
    return retstat;
}

/** Remove a file */
int fs_unlink(const char *path)
{
    int retstat = -EACCES;
    char rpath[PATH_MAX] = {0}, apath[PATH_MAX] = {0};
    get_relative_path(rpath, path);
    get_absolute_path(apath, path);
    LDEBUG("[{}] [{}] [{}]", path, rpath, apath);
    if (check_permission("unlink")) return retstat;
    retstat = unlink(rpath);
    if (retstat < 0) retstat = -errno;
    return retstat;
}

/** Remove a directory */
int fs_rmdir(const char *path)
{
    int retstat = -EACCES;
    char rpath[PATH_MAX] = {0}, apath[PATH_MAX] = {0};
    get_relative_path(rpath, path);
    get_absolute_path(apath, path);
    LDEBUG("[{}] [{}] [{}]", path, rpath, apath);
    if (check_permission("rmdir")) return retstat;
    retstat = rmdir(rpath);
    if (retstat < 0) retstat = -errno;
    return retstat;
}

/** Create a symbolic link */
int fs_symlink(const char *path, const char *link)
{
    int retstat = -EACCES;
    char rpath[PATH_MAX] = {0}, apath[PATH_MAX] = {0};
    get_relative_path(rpath, path);
    get_absolute_path(apath, path);
    LDEBUG("[{}] [{}] [{}]", path, rpath, apath);
    if (check_permission("symlink")) return retstat;
    retstat = symlink(rpath, link);
    if (retstat < 0) retstat = -errno;
    return retstat;
}

/** Rename a file */
// both path and newpath are fs-relative
int fs_rename(const char *path, const char *newpath)
{
    int retstat = -EACCES;
    char rpath[PATH_MAX] = {0}, apath[PATH_MAX] = {0};
    char new_rpath[PATH_MAX] = {0}, new_apath[PATH_MAX] = {0};
    get_relative_path(rpath, path);
    get_absolute_path(apath, path);
    get_relative_path(new_rpath, newpath);
    get_absolute_path(new_apath, newpath);
    LDEBUG("[{}] [{}] [{}]", path, rpath, apath);
    LDEBUG("[{}] [{}] [{}]", newpath, new_rpath, new_apath);
    if (check_permission("rename")) return retstat;
    retstat = rename(rpath, new_rpath);
    if (retstat < 0) retstat = -errno;
    return retstat;
}

/** Create a hard link to a file */
int fs_link(const char *path, const char *newpath)
{
    int retstat = -EACCES;
    char rpath[PATH_MAX] = {0}, apath[PATH_MAX] = {0};
    char new_rpath[PATH_MAX] = {0}, new_apath[PATH_MAX] = {0};
    get_relative_path(rpath, path);
    get_absolute_path(apath, path);
    get_relative_path(new_rpath, newpath);
    get_absolute_path(new_apath, newpath);
    LDEBUG("[{}] [{}] [{}]", path, rpath, apath);
    LDEBUG("[{}] [{}] [{}]", newpath, new_rpath, new_apath);
    if (check_permission("link")) return retstat;
    retstat = link(rpath, new_rpath);
    if (retstat < 0) retstat = -errno;
    return retstat;
}

/** Change the permission bits of a file */
int fs_chmod(const char *path, mode_t mode)
{
    int retstat = -EACCES;
    char rpath[PATH_MAX] = {0}, apath[PATH_MAX] = {0};
    get_relative_path(rpath, path);
    get_absolute_path(apath, path);
    LDEBUG("[{}] [{}] [{}]", path, rpath, apath);
    if (check_permission("chmod")) return retstat;
    retstat = chmod(rpath, mode);
    if (retstat < 0) retstat = -errno;
    return retstat;
}

/** Change the owner and group of a file */
int fs_chown(const char *path, uid_t uid, gid_t gid)
{
    int retstat = -EACCES;
    char rpath[PATH_MAX] = {0}, apath[PATH_MAX] = {0};
    get_relative_path(rpath, path);
    get_absolute_path(apath, path);
    LDEBUG("[{}] [{}] [{}]", path, rpath, apath);
    if (check_permission("chown")) return retstat;
    retstat = chown(rpath, uid, gid);
    if (retstat < 0) retstat = -errno;
    return retstat;
}

/** Change the size of a file */
int fs_truncate(const char *path, off_t newsize)
{
    int retstat = -EACCES;
    char rpath[PATH_MAX] = {0}, apath[PATH_MAX] = {0};
    get_relative_path(rpath, path);
    get_absolute_path(apath, path);
    LDEBUG("[{}] [{}] [{}]", path, rpath, apath);
    if (check_permission("truncate")) return retstat;
    retstat = truncate(rpath, newsize);
    if (retstat < 0) retstat = -errno;
    return retstat;
}

/** Change the access and/or modification times of a file */
/* note -- I'll want to change this as soon as 2.6 is in debian testing */
int fs_utime(const char *path, struct utimbuf *ubuf)
{
    int retstat = -EACCES;
    char rpath[PATH_MAX] = {0}, apath[PATH_MAX] = {0};
    get_relative_path(rpath, path);
    get_absolute_path(apath, path);
    LDEBUG("[{}] [{}] [{}]", path, rpath, apath);
    if (check_permission("utime")) return retstat;
    retstat = utime(rpath, ubuf);
    if (retstat < 0) retstat = -errno;
    return retstat;
}

/** File open operation */
int fs_open(const char *path, struct fuse_file_info *fi)
{
    int retstat = -EACCES;
    int fd;
    char rpath[PATH_MAX] = {0}, apath[PATH_MAX] = {0};
    get_relative_path(rpath, path);
    get_absolute_path(apath, path);
    LDEBUG("[{}] [{}] [{}]", path, rpath, apath);
    if (check_permission("open")) return retstat;
    // if the open call succeeds, my retstat is the file descriptor,
    // else it's -errno.  I'm making sure that in that case the saved
    // file descriptor is exactly -1.
    fd = open(rpath, fi->flags);
    if (fd < 0)
        retstat = -errno;
    else
        retstat = 0;
    fi->fh = fd;
    return retstat;
}

/** Read data from an open file */
int fs_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
{
    int retstat = -EACCES;
    char rpath[PATH_MAX] = {0}, apath[PATH_MAX] = {0};
    get_relative_path(rpath, path);
    get_absolute_path(apath, path);
    LDEBUG("[{}] [{}] [{}]", path, rpath, apath);
    if (check_permission("read")) return retstat;
    retstat = pread(fi->fh, buf, size, offset);
    if (retstat < 0) retstat = -errno;
    return retstat;
}

/** Write data to an open file */
int fs_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
{
    int retstat = -EACCES;
    char rpath[PATH_MAX] = {0}, apath[PATH_MAX] = {0};
    get_relative_path(rpath, path);
    get_absolute_path(apath, path);
    LDEBUG("[{}] [{}] [{}]", path, rpath, apath);
    if (check_permission("write")) return retstat;
    retstat = pwrite(fi->fh, buf, size, offset);
    if (retstat < 0) retstat = -errno;
    return retstat;
}
/** Get file system statistics */
int fs_statfs(const char *path, struct statvfs *statv)
{
    int retstat = -EACCES;
    char rpath[PATH_MAX] = {0}, apath[PATH_MAX] = {0};
    get_relative_path(rpath, path);
    get_absolute_path(apath, path);
    LDEBUG("[{}] [{}] [{}]", path, rpath, apath);
    if (check_permission("statfs")) return retstat;
    retstat = statvfs(rpath, statv);
    if (retstat < 0) retstat = -errno;
    return retstat;
}
/** Possibly flush cached data */
int fs_flush(const char *path, struct fuse_file_info *fi)
{
    char rpath[PATH_MAX] = {0}, apath[PATH_MAX] = {0};
    get_relative_path(rpath, path);
    get_absolute_path(apath, path);
    LDEBUG("[{}] [{}] [{}]", path, rpath, apath);
    return 0;
}

/** Release an open file */
int fs_release(const char *path, struct fuse_file_info *fi)
{
    int retstat = -EACCES;
    char rpath[PATH_MAX] = {0}, apath[PATH_MAX] = {0};
    get_relative_path(rpath, path);
    get_absolute_path(apath, path);
    LDEBUG("[{}] [{}] [{}]", path, rpath, apath);
    if (check_permission("release")) return retstat;
    retstat = close(fi->fh);
    if (retstat < 0) retstat = -errno;
    return retstat;
}
/** Synchronize file contents */
int fs_fsync(const char *path, int datasync, struct fuse_file_info *fi)
{
    int retstat = -EACCES;
    char rpath[PATH_MAX] = {0}, apath[PATH_MAX] = {0};
    get_relative_path(rpath, path);
    get_absolute_path(apath, path);
    LDEBUG("[{}] [{}] [{}]", path, rpath, apath);
    if (check_permission("fsync")) return retstat;
#ifdef HAVE_FDATASYNC
    if (datasync)
        retstat = fdatasync(fi->fh);
    else
#endif
        retstat = fsync(fi->fh);
    if (retstat < 0) retstat = -errno;
    return retstat;
}

#ifdef HAVE_SYS_XATTR_H

/** Set extended attributes */
int fs_setxattr(const char *path, const char *name, const char *value, size_t size, int flags)
{
    return lsetxattr(path, name, value, size, flags);
}

/** Get extended attributes */
int fs_getxattr(const char *path, const char *name, char *value, size_t size)
{
    return lgetxattr(path, name, value, size);
}

/** List extended attributes */
int fs_listxattr(const char *path, char *list, size_t size)
{
    return llistxattr(path, list, size);
}

/** Remove extended attributes */
int fs_removexattr(const char *path, const char *name)
{
    return lremovexattr(path, name);
}
#endif

/** Open directory */
int fs_opendir(const char *path, struct fuse_file_info *fi)
{
    DIR *dp;
    int retstat = -EACCES;
    char rpath[PATH_MAX] = {0}, apath[PATH_MAX] = {0};
    get_relative_path(rpath, path);
    get_absolute_path(apath, path);
    LDEBUG("[{}] [{}] [{}]", path, rpath, apath);
    if (check_permission("opendir")) return retstat;
    dp = opendir(rpath);
    if (dp == NULL)
        retstat = -errno;
    else
        retstat = 0;
    fi->fh = (intptr_t) dp;
    return retstat;
}

/** Read directory */
int fs_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset,
               struct fuse_file_info *fi)
{
    DIR *dp;
    struct dirent *de;
    int retstat = -EACCES;
    char rpath[PATH_MAX] = {0}, apath[PATH_MAX] = {0};
    get_relative_path(rpath, path);
    get_absolute_path(apath, path);
    LDEBUG("[{}] [{}] [{}]", path, rpath, apath);
    if (check_permission("readdir")) return retstat;
    dp = (DIR *) (uintptr_t) fi->fh;
    de = readdir(dp);
    if (de == 0) {
        retstat = -errno;
    } else {
        do {
            struct stat st;
            memset(&st, 0, sizeof(st));
            st.st_ino = de->d_ino;
            st.st_mode = de->d_type << 12;
            if (filler(buf, de->d_name, NULL, 0) != 0) {
                return -ENOMEM;
            }
        } while ((de = readdir(dp)) != NULL);
        retstat = 0;
    }
    return retstat;
}

/** Release directory */
int fs_releasedir(const char *path, struct fuse_file_info *fi)
{
    int retstat = -EACCES;
    char rpath[PATH_MAX] = {0}, apath[PATH_MAX] = {0};
    get_relative_path(rpath, path);
    get_absolute_path(apath, path);
    LDEBUG("[{}] [{}] [{}]", path, rpath, apath);
    if (check_permission("releasedir")) return retstat;
    retstat = closedir((DIR *) (uintptr_t) fi->fh);
    if (retstat < 0) retstat = -errno;
    return retstat;
}

/** Synchronize directory contents */
int fs_fsyncdir(const char *path, int datasync, struct fuse_file_info *fi)
{
    char rpath[PATH_MAX] = {0}, apath[PATH_MAX] = {0};
    get_relative_path(rpath, path);
    get_absolute_path(apath, path);
    return 0;
}

/** Clean up filesystem */
void fs_destroy(void *userdata)
{
    LDEBUG("fs_destory ...");
}

/** Check file access permissions */
int fs_access(const char *path, int mask)
{
    int retstat = -EACCES;
    char rpath[PATH_MAX] = {0}, apath[PATH_MAX] = {0};
    get_relative_path(rpath, path);
    get_absolute_path(apath, path);
    LDEBUG("[{}] [{}] [{}]", path, rpath, apath);
    if (check_permission("access")) return retstat;
    retstat = access(rpath, mask);
    if (retstat < 0) retstat = -errno;
    return retstat;
}
/** Change the size of an open file */
int fs_ftruncate(const char *path, off_t offset, struct fuse_file_info *fi)
{
    int retstat = -EACCES;
    char rpath[PATH_MAX] = {0}, apath[PATH_MAX] = {0};
    get_relative_path(rpath, path);
    get_absolute_path(apath, path);
    LDEBUG("[{}] [{}] [{}]", path, rpath, apath);
    if (check_permission("ftruncate")) return retstat;
    retstat = ftruncate(fi->fh, offset);
    if (retstat < 0) retstat = -errno;
    return retstat;
}

/** Get attributes from an open file */
int fs_fgetattr(const char *path, struct stat *statbuf, struct fuse_file_info *fi)
{
    if (!strcmp(path, "/"))
        return fs_getattr(path, statbuf);
    int retstat = -EACCES;
    char rpath[PATH_MAX] = {0}, apath[PATH_MAX] = {0};
    get_relative_path(rpath, path);
    get_absolute_path(apath, path);
    LDEBUG("[{}] [{}] [{}]", path, rpath, apath);
    if (check_permission("fgetattr")) return retstat;
    retstat = fstat(fi->fh, statbuf);
    if (retstat < 0) retstat = -errno;
    return retstat;
}
