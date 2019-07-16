#pragma once
#include <libudev.h>
#include "udev_device.hpp"

namespace udevadm {

    class udev_monitor
    {
    public:
        //! 构造
        udev_monitor(struct ::udev_monitor *handle) : handle_(handle) { }

        //! 拷贝构造
        udev_monitor(const udev_monitor &other) : handle_(udev_monitor_ref(other.handle_)) { }

        //! 移动构造
        udev_monitor(udev_monitor &&other) noexcept : handle_(other.handle_) { other.handle_ = nullptr; }

        //! 析构
        virtual ~udev_monitor() noexcept {
            if (handle_) {
                udev_monitor_unref (handle_);
                handle_ = nullptr;
            }
        }

        //! 拷贝赋值
        udev_monitor& operator=(const udev_monitor &other) {
            if (handle_) udev_monitor_ref(handle_);
            handle_ = udev_monitor_ref(other.handle_);
            return *this;
        }

        //! 移动赋值
        udev_monitor& operator=(udev_monitor &&other) noexcept {
            if (handle_) udev_monitor_ref(handle_);
            handle_ = other.handle_;
            other.handle_ = nullptr;
            return *this;
        };

        operator bool() const { return handle_; }

    public:                     // 接口部分
        // 开始接收数据
        int enable_receiving() const {
            return udev_monitor_enable_receiving(handle_);
        }
        // 设置接接收 buffer size
        int set_receive_buffer_size(int size) const {
            return udev_monitor_set_receive_buffer_size(handle_, size);
        }
        // 获取文件句柄
        int get_fd() const {
            return udev_monitor_get_fd(handle_);
        }
        // 接收设备信息
        udev_device receive_device() const {
            return udev_monitor_receive_device(handle_);
        }
        // 监控过滤，更新，移除
        int filter_add_match_subsystem_devtype(const char * subsystem, const char * devtype) const {
            return udev_monitor_filter_add_match_subsystem_devtype(handle_, subsystem, devtype);
        }

        int filter_add_match_tag(const char *tag) const {
            return udev_monitor_filter_add_match_tag(handle_, tag);
        }

        int filter_update() const { return udev_monitor_filter_update(handle_); }

        int filter_remove() const { return udev_monitor_filter_remove(handle_); }

    private:
        struct ::udev_monitor *handle_ = nullptr;
    };


}  // Udev
