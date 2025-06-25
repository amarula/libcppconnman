#pragma once

#include <gio/gio.h>

#include <condition_variable>
#include <iostream>
#include <mutex>
#include <stdexcept>
#include <string>
#include <thread>

namespace Amarula::DBus {

class GProxy {
    std::mutex mtx_;
    std::condition_variable cv_;
    bool running_{false};
    std::thread glib_thread_;
    unsigned int pending_calls_{0};
    const std::string bus_name_;
    const std::string object_path_;
    GDBusConnection* connection_ = nullptr;
    GMainLoop* loop_{nullptr};
    GError* error_ = nullptr;

   public:
    GProxy(const std::string& bus_name, const std::string& object_path);

    virtual ~GProxy();

    void start();
    void stop();
    void on_any_async_done();
    void on_any_async_start();

    [[nodiscard]] auto getConnection() const { return connection_; }

   protected:
    virtual void cleanup() {};
};

}  // namespace Amarula::DBus
