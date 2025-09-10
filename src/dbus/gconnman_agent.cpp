
#include <gio/gio.h>
#include <glib.h>

#include <amarula/dbus/connman/gagent.hpp>
#include <iostream>
#include <stdexcept>
#include <string>

#include "gconnman_private.hpp"

namespace Amarula::DBus::G::Connman {
static constexpr const char *INTROSPECTION_XML =
    "<node>"
    "  <interface name='net.connman.Agent'>"
    "    <method name='Release'/>"
    "    <method name='RequestInput'>"
    "      <arg name='service' type='o' direction='in'/>"
    "      <arg name='fields' type='a{sv}' direction='in'/>"
    "      <arg name='return' type='a{sv}' direction='out'/>"
    "    </method>"
    "    <method name='ReportError'>"
    "      <arg name='service' type='o' direction='in'/>"
    "      <arg name='error' type='s' direction='in'/>"
    "    </method>"
    "    <method name='Cancel'/>"
    "  </interface>"
    "</node>";

Agent::Agent(GDBusConnection *connection, const std::string &path)
    : connection_{connection} {
    if (!path.empty()) {
        path_ = path;
    }
    GError *err = nullptr;
    node_info_ = g_dbus_node_info_new_for_xml(INTROSPECTION_XML, &err);
    if (node_info_ == nullptr) {
        std::string const msg =
            "Failed to parse introspection XML: " + std::string(err->message);
        g_error_free(err);
        throw std::runtime_error(msg);
    }

    registration_id_ = g_dbus_connection_register_object(
        connection, path_.c_str(), *(node_info_->interfaces), &INTERFACE_VTABLE,
        this, nullptr, &err);

    if (registration_id_ == 0) {
        std::string const msg =
            "Failed to register agent: " + std::string(err->message);
        g_error_free(err);
        g_dbus_node_info_unref(node_info_);
        throw std::runtime_error(msg);
    }
}
Agent::~Agent() {
    g_dbus_connection_unregister_object(connection_, registration_id_);
    g_dbus_node_info_unref(node_info_);
}

void Agent::on_method_call(GDBusConnection * /*connection*/,
                           const gchar * /*sender*/,
                           const gchar * /*object_path*/,
                           const gchar * /*interface_name*/,
                           const gchar *method_name, GVariant *parameters,
                           GDBusMethodInvocation *invocation,
                           gpointer user_data) {
    auto *self = static_cast<Agent *>(user_data);
    self->dispatch_method_call(invocation, method_name, parameters);
}

void Agent::dispatch_method_call(GDBusMethodInvocation *invocation,
                                 const gchar *method_name,
                                 GVariant *parameters) {
    if (g_strcmp0(method_name, "RequestInput") == 0 && request_input_cb_) {
        const gchar *service = nullptr;
        GVariant *fields = nullptr;

        GVariant *child_service = g_variant_get_child_value(parameters, 0);
        GVariant *child_fields = g_variant_get_child_value(parameters, 1);

        service = g_variant_get_string(child_service, nullptr);
        fields = g_variant_ref(child_fields);

        g_variant_unref(child_service);
        g_variant_unref(child_fields);

        auto *result = request_input_cb_(service, fields);

        GVariant *tuple = g_variant_new_tuple(&result, 1);

        g_dbus_method_invocation_return_value(invocation, tuple);
        g_variant_unref(fields);
        return;
    }

    if (g_strcmp0(method_name, "Cancel") == 0 && cancel_cb_) {
        cancel_cb_();
        return;
    }

    if (g_strcmp0(method_name, "Release") == 0 && release_cb_) {
        release_cb_();
        return;
    }

    if (g_strcmp0(method_name, "ReportError") == 0) {
        const gchar *service = nullptr;
        const gchar *error_str = nullptr;

        GVariant *child_service = g_variant_get_child_value(parameters, 0);
        GVariant *child_error = g_variant_get_child_value(parameters, 1);

        service = g_variant_get_string(child_service, nullptr);
        error_str = g_variant_get_string(child_error, nullptr);

        std::cerr << "ReportError:" << service << " " << error_str << "\n";

        g_variant_unref(child_service);
        g_variant_unref(child_error);

        return;
    }

    g_dbus_method_invocation_return_error_literal(
        invocation, G_IO_ERROR, G_IO_ERROR_NOT_SUPPORTED, "Unknown method");
}
}  // namespace Amarula::DBus::G::Connman
