#pragma once

#include <gio/gio.h>
#include <glib.h>

#include <condition_variable>
#include <mutex>
#include <string>
#include <thread>

namespace Amarula::DBus::G {

class DBus {
    std::mutex mtx_;
    std::condition_variable cv_;
    bool running_{false};
    std::thread glib_thread_;
    unsigned int pending_calls_{0};
    GDBusConnection* connection_ = nullptr;
    GMainLoop* loop_{nullptr};
    GMainContext* ctx_{nullptr};

    static auto on_loop_started(gpointer user_data) -> gboolean;

   public:
    DBus(const std::string& bus_name, const std::string& object_path);

    DBus(const DBus&) = delete;
    auto operator=(const DBus&) -> DBus& = delete;
    DBus(DBus&&) = delete;
    auto operator=(DBus&&) -> DBus& = delete;
    virtual ~DBus();

    void start();
    void onAnyAsyncDone();
    void onAnyAsyncStart();

    [[nodiscard]] auto connection() const { return connection_; }
    [[nodiscard]] auto context() { return ctx_; }
};

}  // namespace Amarula::DBus::G
