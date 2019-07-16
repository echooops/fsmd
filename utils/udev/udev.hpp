#pragma once
#include <libudev.h>
#include "udev_monitor.hpp"
#include "udev_enumerate.hpp"
#include "udev_device.hpp"


namespace udevadm {

    class udev
    {
    public:
        // 默认构造函数
        udev() : handle_(udev_new()) { };

        // 拷贝构造
        udev(const udev &other) : handle_(udev_ref(other.handle_)) { };

        // 移动构造
        udev(udev &&other) noexcept : handle_(other.handle_) { other.handle_ = nullptr; };

        // 析构函数
        virtual ~udev() noexcept {
            if (handle_) {
                udev_unref(handle_);
                handle_ = nullptr;
            }
        }

        // 拷贝赋值函数
        udev& operator=(const udev &other) {
            if (handle_) udev_unref(handle_);
            handle_ = udev_ref(other.handle_);
            return *this;
        }

        // 移动赋值函数
        udev& operator=(udev &&other) noexcept {
            if (handle_) udev_unref(handle_);
            handle_ = other.handle_;
            other.handle_ = nullptr;
            return *this;
        }

        operator bool() const { return handle_; }

    public:                     // 接口部分

        void *get_userdata() const { return udev_get_userdata(handle_); }

        void set_userdata(void *userdata) const { return udev_set_userdata(handle_, userdata); }

        // 创建 udev 监控  name = ["udev", "kernel"]
        udev_monitor monitor_new_from_netlink(const char* name = "udev") const {
            return udev_monitor_new_from_netlink(handle_, name);
        }
        // 创建 udev 枚举列表
        udev_enumerate enumerate_new() const {
            return udev_enumerate_new(handle_);
        }
        // 创建 device
        udev_device device_new_from_syspath(const char * syspath) const {
            return udev_device_new_from_syspath(handle_, syspath);
        }
        udev_device device_new_from_devnum(char type, dev_t devnum) const {
            return udev_device_new_from_devnum(handle_, type, devnum);
        }
        udev_device device_new_from_subsystem_sysname(const char * subsystem, const char * sysname) const {
            return udev_device_new_from_subsystem_sysname(handle_, subsystem, sysname);
        }
        udev_device device_new_from_environment() const {
            return udev_device_new_from_environment(handle_);
        }

    private:
        struct ::udev *handle_;
    };
}  // udev
