#pragma once
#include <gio/gio.h>
#include <glib.h>

#include <functional>
#include <utility>

namespace Amarula::DBus::G::Connman {
class Connman;

class Agent {
    GDBusNodeInfo *node_info_;
    guint registration_id_{0};
    GDBusConnection *connection_{nullptr};
    static constexpr const auto AGENT_PATH{"/net/amarula/gconnman/agent"};

    explicit Agent(GDBusConnection *connection);

    using RequestInputCallback =
        std::function<GVariant *(const gchar *service, GVariant *fields)>;
    using CancelCallback = std::function<void()>;
    using ReleaseCallback = std::function<void()>;
    using ReportErrorCallback =
        std::function<void(const gchar *service, const gchar *error)>;

    void set_request_input_handler(RequestInputCallback callback) {
        request_input_cb_ = std::move(callback);
    }

    void set_cancel_handler(CancelCallback callback) {
        cancel_cb_ = std::move(callback);
    }

    void set_release_handler(ReleaseCallback callback) {
        release_cb_ = std::move(callback);
    }

    RequestInputCallback request_input_cb_;
    CancelCallback cancel_cb_;
    ReleaseCallback release_cb_;

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
