#include <gtest/gtest.h>

#include <amarula/dbus/connman/gconnman.hpp>
#include <amarula/dbus/connman/gservice.hpp>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <string>

#include "thread_bundle.hpp"

using Amarula::DBus::G::Connman::Connman;

namespace {

/*
 * Short on purpose: connman answers a Service.Connect() call only once the
 * agent has answered ReportError, so an agent that never completes the
 * invocation is indistinguishable from a hang. These calls must come back
 * well within the D-Bus default reply timeout of 25s.
 */
constexpr int CALL_TIMEOUT_MS = 2000;

/*
 * The service list is filled by the asynchronous GetServices() issued when the
 * Manager is built, so a test that needs a service has to wait for the
 * ServicesChanged callback instead of reading Manager::services() right away.
 */
constexpr int SERVICES_TIMEOUT_MS = 5000;

constexpr const char* AGENT_INTERFACE = "net.connman.Agent";
constexpr const char* RETRY_ERROR = "net.connman.Agent.Error.Retry";

auto call_agent(GDBusConnection* bus, const std::string& path,
                const gchar* method, GVariant* args, GError** error)
    -> GVariant* {
    return g_dbus_connection_call_sync(
        bus, g_dbus_connection_get_unique_name(bus), path.c_str(),
        AGENT_INTERFACE, method, args, nullptr, G_DBUS_CALL_FLAGS_NONE,
        CALL_TIMEOUT_MS, nullptr, error);
}

/*
 * The Agent constructor exports the agent object on our own bus connection
 * when the Manager is built, so these tests call it directly and never go
 * through connmand - Manager::registerAgent() is a separate step that only
 * tells connmand which path to call. Like the other connman tests they still
 * need a running connmand, because constructing a Manager does. Skip rather
 * than fail so the suite stays usable on a host without it.
 */
auto connman_available(GDBusConnection* bus) -> bool {
    GVariant* reply = g_dbus_connection_call_sync(
        bus, "org.freedesktop.DBus", "/org/freedesktop/DBus",
        "org.freedesktop.DBus", "NameHasOwner",
        g_variant_new("(s)", "net.connman"), G_VARIANT_TYPE("(b)"),
        G_DBUS_CALL_FLAGS_NONE, CALL_TIMEOUT_MS, nullptr, nullptr);
    if (reply == nullptr) {
        return false;
    }

    gboolean has_owner = FALSE;
    g_variant_get(reply, "(b)", &has_owner);
    g_variant_unref(reply);

    return has_owner == TRUE;
}

auto system_bus_or_skip() -> GDBusConnection* {
    GDBusConnection* bus = g_bus_get_sync(G_BUS_TYPE_SYSTEM, nullptr, nullptr);
    if (bus == nullptr) {
        return nullptr;
    }
    return connman_available(bus) ? bus : nullptr;
}

}  // namespace

TEST(ConnmanAgent, ReportErrorIsAnswered) {
    GDBusConnection* bus = system_bus_or_skip();
    if (bus == nullptr) {
        GTEST_SKIP() << "connmand not available on the system bus";
    }

    const ThreadBundle thread_bundle;
    const Connman connman;
    const auto manager = connman.manager();

    GError* error = nullptr;

    // No handler installed: the agent must still answer, and must not ask
    // connman to retry.
    GVariant* reply =
        call_agent(bus, manager->internalAgentPath(), "ReportError",
                   g_variant_new("(os)", "/net/connman/service/does_not_exist",
                                 "invalid-key"),
                   &error);

    ASSERT_NE(reply, nullptr)
        << "ReportError was not answered within " << CALL_TIMEOUT_MS
        << "ms: " << (error != nullptr ? error->message : "");
    g_variant_unref(reply);
}

TEST(ConnmanAgent, ReportErrorRequestsRetry) {
    GDBusConnection* bus = system_bus_or_skip();
    if (bus == nullptr) {
        GTEST_SKIP() << "connmand not available on the system bus";
    }

    /*
     * Declared before the Connman instance so that they outlive the callbacks
     * capturing them: ~Connman() waits for the pending asynchronous calls,
     * whose callbacks run on the library GLib thread.
     */
    std::mutex mtx;
    std::condition_variable services_cv;
    std::string discovered_service_path;
    std::string reported_service_path;
    std::string reported_error;

    const ThreadBundle thread_bundle;
    const Connman connman;
    const auto manager = connman.manager();

    manager->onServicesChanged(
        [&mtx, &services_cv, &discovered_service_path](const auto& services) {
            {
                std::lock_guard<std::mutex> const lock(mtx);
                if (discovered_service_path.empty() && !services.empty()) {
                    discovered_service_path = services.front()->objPath();
                }
            }
            services_cv.notify_all();
        });

    /*
     * The agent answers Retry only for a service it knows about, so wait for
     * the Manager service list to be populated. The waiting has to happen here
     * and not inside the callback: the callback runs on the GLib thread that
     * also dispatches the agent method calls, so a synchronous call from there
     * would never be answered.
     */
    std::string service_path;
    {
        std::unique_lock<std::mutex> lock(mtx);
        services_cv.wait_for(lock,
                             std::chrono::milliseconds(SERVICES_TIMEOUT_MS),
                             [&discovered_service_path] {
                                 return !discovered_service_path.empty();
                             });
        service_path = discovered_service_path;
    }

    // The callback is missed when GetServices() completes before it is
    // installed, so fall back to what the Manager already holds.
    if (service_path.empty()) {
        const auto services = manager->services();
        if (!services.empty()) {
            service_path = services.front()->objPath();
        }
    }

    if (service_path.empty()) {
        GTEST_SKIP() << "No connman services available";
    }

    manager->onReportError([&mtx, &reported_service_path, &reported_error](
                               const auto& service, const std::string& error) {
        std::lock_guard<std::mutex> const lock(mtx);
        reported_service_path =
            service != nullptr ? service->objPath() : std::string();
        reported_error = error;
        return true;
    });

    GError* error = nullptr;
    GVariant* reply = call_agent(
        bus, manager->internalAgentPath(), "ReportError",
        g_variant_new("(os)", service_path.c_str(), "invalid-key"), &error);

    ASSERT_EQ(reply, nullptr) << "Expected " << RETRY_ERROR;
    ASSERT_NE(error, nullptr);

    gchar* remote_error = g_dbus_error_get_remote_error(error);
    EXPECT_STREQ(remote_error, RETRY_ERROR);
    g_free(remote_error);
    g_error_free(error);

    std::lock_guard<std::mutex> const lock(mtx);
    EXPECT_EQ(reported_service_path, service_path);
    EXPECT_EQ(reported_error, "invalid-key");
}

TEST(ConnmanAgent, CancelIsAnswered) {
    GDBusConnection* bus = system_bus_or_skip();
    if (bus == nullptr) {
        GTEST_SKIP() << "connmand not available on the system bus";
    }

    const ThreadBundle thread_bundle;
    const Connman connman;
    const auto manager = connman.manager();

    GError* error = nullptr;
    GVariant* reply = call_agent(bus, manager->internalAgentPath(), "Cancel",
                                 nullptr, &error);

    ASSERT_NE(reply, nullptr)
        << "Cancel was not answered within " << CALL_TIMEOUT_MS
        << "ms: " << (error != nullptr ? error->message : "");
    g_variant_unref(reply);
}
