#include "amarula/dbus/gproxy.hpp"

#include <mutex>

namespace Amarula::DBus {

void GProxy::on_any_async_done() {
    if (pending_calls_-- == 1) {
        g_main_loop_quit(loop_);
    }
}

void GProxy::on_any_async_start() { ++pending_calls_; }

GProxy::GProxy(const std::string& bus_name, const std::string& object_path)
    : bus_name_(bus_name), object_path_(object_path) {
    connection_ = g_bus_get_sync(G_BUS_TYPE_SYSTEM, nullptr, &error_);
    if (connection_ == nullptr) {
        throw std::runtime_error("Failed to connect to DBus: " +
                                 std::string(error_->message));
    }

    GDBusProxy* proxy = g_dbus_proxy_new_sync(
        connection_, G_DBUS_PROXY_FLAGS_NONE, nullptr, bus_name_.c_str(),
        object_path_.c_str(), "org.freedesktop.DBus.Introspectable", nullptr,
        &error_);

    if (proxy == nullptr) {
        throw std::runtime_error("Failed to create proxy for bus name: " +
                                 std::string(error_->message));
    }

    GVariant* result =
        g_dbus_proxy_call_sync(proxy, "Introspect", nullptr,
                               G_DBUS_CALL_FLAGS_NONE, -1, nullptr, &error_);

    g_object_unref(proxy);

    if (result == nullptr) {
        throw std::runtime_error(
            "Failed to introspect object path or interface: " +
            std::string(error_->message));
    }

    g_variant_unref(result);
    start();
}

GProxy::~GProxy() {
    if (connection_ != nullptr) {
        g_object_unref(connection_);
    }
    if (error_ != nullptr) {
        g_error_free(error_);
    }

    {
        std::unique_lock<std::mutex> lock(mtx_);
        cv_.wait(lock, [this] { return running_; });
    }

    if (loop_ != nullptr && (g_main_loop_is_running(loop_) == TRUE) &&
        pending_calls_ == 0U) {
        g_main_loop_quit(loop_);
    }

    if (glib_thread_.joinable()) {
        glib_thread_.join();
    }
    cleanup();
}

void GProxy::start() {
    if (!running_) {
        glib_thread_ = std::thread([this]() {
            {
                std::lock_guard<std::mutex> const lock(mtx_);
                running_ = true;
                loop_ = g_main_loop_new(nullptr, FALSE);
            }
            cv_.notify_all();
            g_main_loop_run(loop_);
            g_main_loop_unref(loop_);
        });
    }
}
void GProxy::stop() {
    {
        std::unique_lock<std::mutex> lock(mtx_);
        cv_.wait(lock, [this] { return running_; });
        running_ = false;
    }

    if (loop_ != nullptr) {
        g_main_loop_quit(loop_);
        loop_ = nullptr;
    }
    if (glib_thread_.joinable()) {
        glib_thread_.join();
    }
}

}  // namespace Amarula::DBus