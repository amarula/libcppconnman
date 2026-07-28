#pragma once
#include <gio/gio.h>
#include <glib.h>

#include <amarula/dbus/gdbus.hpp>
#include <functional>
#include <string>
#include <utility>

namespace Amarula::DBus::G::Connman {
class Connman;

class Agent {
    GDBusNodeInfo *node_info_;
    guint registration_id_{0};
    DBus *dbus_;
    std::string path_{"/net/amarula/gconnman/agent"};

    explicit Agent(DBus *dbus, const std::string &path = std::string());

    using RequestInputCallback =
        std::function<GVariant *(const gchar *service, GVariant *fields)>;
    using CancelCallback = std::function<void()>;
    using ReleaseCallback = std::function<void()>;
    /*
     * Returning true asks connman to retry the connection with the credentials
     * it already has, by replying net.connman.Agent.Error.Retry. The callback
     * runs on the D-Bus dispatch thread while connman waits for the reply, so
     * it must not block.
     *
     * The retry budget belongs to this callback: connman reconnects
     * immediately, with no backoff and no attempt limit of its own, so a
     * callback that always returns true against a service that keeps failing
     * retries forever. Count the attempts and return false once you give up.
     */
    using ReportErrorCallback =
        std::function<bool(const gchar *service, const gchar *error)>;

    void set_request_input_handler(RequestInputCallback callback) {
        request_input_cb_ = std::move(callback);
    }

    void set_cancel_handler(CancelCallback callback) {
        cancel_cb_ = std::move(callback);
    }

    void set_release_handler(ReleaseCallback callback) {
        release_cb_ = std::move(callback);
    }

    void set_report_error_handler(ReportErrorCallback callback) {
        report_error_cb_ = std::move(callback);
    }

    RequestInputCallback request_input_cb_;
    CancelCallback cancel_cb_;
    ReleaseCallback release_cb_;
    ReportErrorCallback report_error_cb_;

    static void on_method_call(GDBusConnection *connection, const gchar *sender,
                               const gchar *object_path,
                               const gchar *interface_name,
                               const gchar *method_name, GVariant *parameters,
                               GDBusMethodInvocation *invocation,
                               gpointer user_data);

    void dispatch_method_call(GDBusMethodInvocation *invocation,
                              const gchar *method_name, GVariant *parameters);

    constexpr static const GDBusInterfaceVTable INTERFACE_VTABLE{
        Agent::on_method_call, nullptr, nullptr, {nullptr}};

   public:
    Agent(const Agent &) = delete;
    auto operator=(const Agent &) -> Agent & = delete;
    Agent(Agent &&) = delete;
    auto operator=(Agent &&) -> Agent & = delete;
    ~Agent();
    friend class Manager;
};

}  // namespace Amarula::DBus::G::Connman
