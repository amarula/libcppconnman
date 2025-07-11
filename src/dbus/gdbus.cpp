#include <gio/gio.h>
#include <glib-object.h>
#include <glib.h>
#include <glibconfig.h>

#include <amarula/dbus/gdbus.hpp>
#include <mutex>
#include <stdexcept>
#include <string>

namespace Amarula::DBus::G {

void DBus::on_any_async_done() {
    std::lock_guard<std::mutex> const lock(mtx_);
    if (pending_calls_-- == 1 && !running_ && loop_ != nullptr) {
        g_main_loop_quit(loop_);
    }
}

void DBus::on_any_async_start() { ++pending_calls_; }

DBus::DBus(const std::string& bus_name, const std::string& object_path) {
    GError* error = nullptr;
    connection_ = g_bus_get_sync(G_BUS_TYPE_SYSTEM, nullptr, &error);
    if (connection_ == nullptr) {
        std::string const msg = error->message;
        g_clear_error(&error);
        throw std::runtime_error("Failed to connect to DBus: " + msg);
    }

    GDBusProxy* proxy = g_dbus_proxy_new_sync(
        connection_, G_DBUS_PROXY_FLAGS_NONE, nullptr, bus_name.c_str(),
        object_path.c_str(), "org.freedesktop.DBus.Introspectable", nullptr,
        &error);

    if (proxy == nullptr) {
        std::string const msg = error->message;
        g_clear_error(&error);
        throw std::runtime_error("Failed to create proxy for bus name: " + msg);
    }

    GVariant* result =
        g_dbus_proxy_call_sync(proxy, "Introspect", nullptr,
                               G_DBUS_CALL_FLAGS_NONE, -1, nullptr, &error);

    g_object_unref(proxy);

    if (result == nullptr) {
        std::string const msg = error->message;
        g_clear_error(&error);
        throw std::runtime_error(
            "Failed to introspect object path or interface: " +
            std::string(msg));
    }

    g_variant_unref(result);
    start();
}

DBus::~DBus() {
    {
        std::unique_lock<std::mutex> lock(mtx_);
        cv_.wait(lock, [this] { return running_; });
    }
    {
        std::lock_guard<std::mutex> const lock(mtx_);
        running_ = false;
    }
    if (loop_ != nullptr && (g_main_loop_is_running(loop_) == TRUE) &&
        pending_calls_ == 0U) {
        g_main_loop_quit(loop_);
    }
    if (glib_thread_.joinable()) {
        glib_thread_.join();
    }
    if (connection_ != nullptr) {
        g_object_unref(connection_);
    }
}

auto DBus::on_loop_started(gpointer user_data) -> gboolean {
    auto* self = static_cast<DBus*>(user_data);
    {
        std::lock_guard<std::mutex> const lock(self->mtx_);
        self->running_ = true;
    }
    self->cv_.notify_all();
    return G_SOURCE_REMOVE;  // run once
}

void DBus::start() {
    if (!running_) {
        glib_thread_ = std::thread([this]() {
            loop_ = g_main_loop_new(nullptr, FALSE);
            g_idle_add(&DBus::on_loop_started, this);
            g_main_loop_run(loop_);
            g_main_loop_unref(loop_);
            loop_ = nullptr;
        });
    }
}

}  // namespace Amarula::DBus::G