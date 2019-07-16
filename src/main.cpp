#include <mntent.h>
#include <sys/mount.h>
#include <sys/epoll.h>
#define _FILE_OFFSET_BITS 64
#define FUSE_USE_VERSION 26
#include <fuse/fuse.h>

#include <thread>
#include <iostream>
#include <set>

#include <udev/udev.hpp>
#include <log/logger.hpp>
#include <thread/threadpool.hpp>

#include "fs_mon.h"

utils::log::logger logger ("/tmp/fsmonitor_.log", 1024*1024*64, 2);
utils::thread::threadpool pool;

std::set<std::string> fsmd_mounts;
std::set<std::string> plan_mounts;

#define PROC_MOUNTS_PATH "/proc/mounts"

int assert_removable_media(const char *mnt_fsname, const char *mnt_dir)
{
    udevadm::udev udev;
    udevadm::udev_enumerate enumerate = udev.enumerate_new();

    enumerate.add_match_subsystem("block");
    enumerate.add_match_property("DEVNAME", mnt_fsname);
    // 浏览设备
    enumerate.scan_devices();
    udevadm::udev_list_entry devices = enumerate.get_list_entry();
    do {
        udevadm::udev_device dev = udev.device_new_from_syspath(devices.get_name().c_str());
        if (dev) {
            if (dev.get_property_value("DEVTYPE") == "partition") {
                if (dev.get_property_value("ID_BUS") == "usb")
                    return 0;
                // ID_BUS=ata 的固态硬盘 做如下识别
                if (strncmp(mnt_dir, "/media/", 7) == 0)
                    return 0;
            }
        }
    } while(++devices);
    return -1;
}
int scan_mntent()
{
    fsmd_mounts.clear();
    plan_mounts.clear();
    FILE *fp = setmntent(PROC_MOUNTS_PATH, "r");
    if (!fp) {
        LERROR("semntent failed!");
        return -errno;
    }
    struct mntent *m;
    while ((m = getmntent(fp))) {
        // 已被监控目录
        if (strncmp(m->mnt_fsname, "fsmd", 4) == 0) {
            fsmd_mounts.insert(m->mnt_dir);
        }
        if (strncmp(m->mnt_fsname, "/dev/", 5) == 0) {
            if (0 == assert_removable_media(m->mnt_fsname, m->mnt_dir)) {
                LDEBUG("{} 是移动存储设备,挂载点 : {}", m->mnt_fsname, m->mnt_dir);
                plan_mounts.insert(m->mnt_dir);
            }
            if (strncmp(m->mnt_fsname, "/dev/mapper/", strlen("/dev/mapper/")) == 0) {
                LDEBUG("{} 是特殊适配移动存储设备,挂载点 : {}", m->mnt_fsname, m->mnt_dir);
                plan_mounts.insert(m->mnt_dir);
            }

        }
    }
    endmntent(fp);
    for (auto &e : plan_mounts) {
        if (fsmd_mounts.count(e) == 0) {
            LDEBUG("监控目录 : {}", e.c_str());
            fsm_monitor_fork(e.c_str());
            //pool.commit(fsm_monitor, e.c_str());
        }
    }
    return 0;
}

int devices_monitor()
{
    // 浏览现有已挂在移动存储设备设备
    scan_mntent();
    int mounts_fd = open(PROC_MOUNTS_PATH, O_RDONLY);
    if (mounts_fd < 0) {
        LERROR("open {} error : %s", PROC_MOUNTS_PATH, strerror(errno));
        return -errno;
    }
    int epoll_fd = epoll_create(1);
    struct epoll_event ev;
    ev.data.fd = mounts_fd;
    ev.events = EPOLLPRI;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, mounts_fd, &ev) < 0) {
        close(mounts_fd);
        close(epoll_fd);
        LERROR("epoll add {} EPOLLPRI err : {}", PROC_MOUNTS_PATH, strerror(errno));
        return -errno;
    }
    while (1) {
        int event_num = epoll_wait(epoll_fd, &ev, 1, -1);
        if (event_num < 0) {
            close(mounts_fd);
            close(epoll_fd);
            LERROR("epoll wait err : {}", strerror(errno));
            return -errno;
        }
        if (ev.data.fd == mounts_fd && ev.events & EPOLLPRI) {
            scan_mntent();
        }
    }
}
int devices_umount()
{
    FILE *fp = setmntent(PROC_MOUNTS_PATH, "r");
    if (!fp) {
        LERROR("semntent failed!");
        return -errno;
    }
    struct mntent *m;
    while ((m = getmntent(fp))) {
        // 已被监控目录
        if (strncmp(m->mnt_fsname, "fsmd", 4) == 0) {
            umount2(m->mnt_dir, MNT_FORCE);
        }
    }
    endmntent(fp);
}
int main(int argc, char *argv[])
{
    LDEBUG("main begin ...");
    fsm_init();
    devices_umount();
    devices_monitor();
    LDEBUG("main end ...");
    return 0;
}
