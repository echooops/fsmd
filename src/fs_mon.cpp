#include <sys/wait.h>

#include <log/logger.hpp>
#include "fs_ops.h"
#include "fs_mon.h"
static struct fuse_operations fsm_ops;

int fsm_init()
{
    fsm_ops.init = fs_init;
    fsm_ops.getattr = fs_getattr;
    fsm_ops.readlink = fs_readlink;
    fsm_ops.getdir = nullptr;
    fsm_ops.mknod = fs_mknod;
    fsm_ops.mkdir = fs_mkdir;
    fsm_ops.symlink = fs_symlink;
    fsm_ops.rename = fs_rename;
    fsm_ops.link = fs_link;
    fsm_ops.chmod = fs_chmod;
    fsm_ops.truncate = fs_truncate;
    fsm_ops.utime = fs_utime;
    fsm_ops.open = fs_open;
    fsm_ops.read = fs_read;
    fsm_ops.write = fs_write;
    fsm_ops.statfs = fs_statfs;
    fsm_ops.flush = fs_flush;
    fsm_ops.release = fs_release;
    fsm_ops.fsync = fs_fsync;
#ifdef HAVE_SYS_XATTR_H
    fsm_ops.setxattr = fs_setxattr;
    fsm_ops.getxattr = fs_getxattr;
    fsm_ops.listxattr = fs_listxattr;
    fsm_ops.removexattr = fs_removexattr;
#endif
    fsm_ops.opendir = fs_opendir;
    fsm_ops.readdir = fs_readdir;
    fsm_ops.release = fs_release;
    fsm_ops.fsyncdir = fs_fsyncdir;
    fsm_ops.init = fs_init;
    fsm_ops.destroy = fs_destroy;
    fsm_ops.access = fs_access;
    fsm_ops.ftruncate = fs_ftruncate;
    fsm_ops.fgetattr = fs_fgetattr;
    return 0;
}
int fsm_monitor_fork(const char *path)
{
    pid_t id = fork();
    if (id < 0) {
        LERROR("fork error : [{}]", strerror(errno));
        return -errno;
    } else if (id == 0) {
        // 子进程
        fsm_state *fsm_data = (struct fsm_state *)malloc(sizeof(struct fsm_state));
        if (fsm_data == NULL) {
            LERROR("fsm_monitor malloc : {}", strerror(errno));
            return -errno;
        }
        fsm_data->rootdir = realpath(path, NULL);
        fsm_data->rootfd = open(fsm_data->rootdir, 0);
        LDEBUG("[{}] [{}]", fsm_data->rootdir, fsm_data->rootfd);

        int argc = 4;
        char *argv[5] = {nullptr};
        argv[0] = strdup("fsmd");
        argv[1] = strdup("-o");
        argv[2] = strdup("nonempty,allow_other");
        argv[3] = strdup(fsm_data->rootdir);
        argv[4] = nullptr;
        fuse_main(argc, argv, &fsm_ops, fsm_data);
        for (int i = 0; i < argc; i++)
            free(argv[i]);
        free(fsm_data);
        _exit(127);
    } else {
        int status;
        LDEBUG("DEBUG : [{}]", __LINE__);
        while(waitpid(id, &status, 0) < 0)
            if (errno != EINTR) return -1;
        LDEBUG("DEBUG : [{}]", __LINE__);
        return status;
    }
}
void fsm_monitor(const char *path)
{
    fsm_state *fsm_data = (struct fsm_state *)malloc(sizeof(struct fsm_state));
    if (fsm_data == NULL) {
        LERROR("fsm_monitor malloc : {}", strerror(errno));
        return;
    }
    fsm_data->rootdir = realpath(path, NULL);
    fsm_data->rootfd = open(fsm_data->rootdir, 0);
    LDEBUG("[{}] [{}]", fsm_data->rootdir, fsm_data->rootfd);

    char *mountpoint = nullptr;
    int argc = 4;
    char *argv[5] = {nullptr};
    argv[0] = strdup("fsmd");
    argv[1] = strdup("-o");
    argv[2] = strdup("nonempty,allow_other");
    argv[3] = strdup(fsm_data->rootdir);
    argv[4] = nullptr;
    struct fuse_args args = FUSE_ARGS_INIT(argc, argv);
    fuse_parse_cmdline(&args, &mountpoint, NULL, NULL);
    struct fuse_chan *chan = fuse_mount(mountpoint, &args);
    struct fuse *f = fuse_new(chan, &args, &fsm_ops, sizeof(fsm_ops), fsm_data);
    LDEBUG("fuse_loop ...");
    if (fuse_loop(f) == 0) {
        LDEBUG("fuse_exit...");
        fuse_exit(f);
    }
    if (chan) {
        LDEBUG("fuse_unmount ...");
        fuse_unmount(mountpoint, chan);
    }
    if (f) {
        LDEBUG("fuse_destroy ...");
        fuse_destroy(f);
    }
    LDEBUG("fuse_opt_free_args ...");
    fuse_opt_free_args(&args);
    for (int i = 0; i < argc; i++)
        free(argv[i]);
    free(mountpoint);
}
