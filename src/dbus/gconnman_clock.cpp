#include <glib.h>

#include <amarula/dbus/connman/gclock.hpp>
#include <amarula/dbus/gdbus.hpp>
#include <amarula/dbus/gproxy.hpp>
#include <amarula/log.hpp>
#include <ctime>
#include <iomanip>
#include <string>
#include <utility>
#include <vector>

#include "gconnman_private.hpp"
#include "gdbus_private.hpp"

namespace Amarula::DBus::G::Connman {

using TimeUpdate = ClockProperties::TimeUpdate;

static constexpr EnumStringMap<TimeUpdate, 2> TIME_UPDATE_MAP{
    {{{ClockProperties::TimeUpdate::Manual, "manual"},
      {ClockProperties::TimeUpdate::Auto, "auto"}}}};

using TimeZoneUpdate = ClockProperties::TimeZoneUpdate;
static constexpr EnumStringMap<TimeZoneUpdate, 2> TIME_ZONE_UPDATE_MAP{
    {{{ClockProperties::TimeZoneUpdate::Manual, "manual"},
      {ClockProperties::TimeZoneUpdate::Auto, "auto"}}}};

void ClockProperties::update(const gchar* key, GVariant* value) {
    if (g_strcmp0(key, TIME_STR) == 0) {
        time_ = g_variant_get_uint64(value);
    } else if (g_strcmp0(key, TIMEUPDATES_STR) == 0U) {
        time_updates_ =
            TIME_UPDATE_MAP.fromString(g_variant_get_string(value, nullptr));
    } else if (g_strcmp0(key, TIMEZONE_STR) == 0U) {
        timezone_ = g_variant_get_string(value, nullptr);
    } else if (g_strcmp0(key, TIMEZONEUPDATES_STR) == 0U) {
        timezone_updates_ = TIME_ZONE_UPDATE_MAP.fromString(
            g_variant_get_string(value, nullptr));
    } else if (g_strcmp0(key, TIMESERVERS_STR) == 0U) {
        time_servers_ = as_to_vector(value);
    } else if (g_strcmp0(key, TIMESERVERSYNCED_STR) == 0U) {
        time_server_synced_ = g_variant_get_boolean(value) == 1U;
    } else {
        LCM_LOG("Unknown property for Clock: " << key << '\n');
    }
}

Clock::Clock(DBus* dbus)
    : DBusProxy(dbus, SERVICE, MANAGER_PATH, CLOCK_INTERFACE) {}

void Clock::setTime(uint64_t time, PropertiesSetCallback callback) {
    auto data = prepareCallback(std::move(callback));
    setProperty(TIME_STR, g_variant_new_uint64(time), nullptr,
                &Clock::finishAsyncCall, data.release());
}

void Clock::setTimeZone(const std::string& timezone,
                        PropertiesSetCallback callback) {
    auto data = prepareCallback(std::move(callback));
    setProperty(TIMEZONE_STR, g_variant_new_string(timezone.c_str()), nullptr,
                &Clock::finishAsyncCall, data.release());
}

void Clock::setTimeUpdates(const Properties::TimeUpdate time_updates,
                           PropertiesSetCallback callback) {
    auto data = prepareCallback(std::move(callback));
    setProperty(
        TIMEUPDATES_STR,
        g_variant_new_string((TIME_UPDATE_MAP.toString(time_updates)).data()),
        nullptr, &Clock::finishAsyncCall, data.release());
}

void Clock::setTimeZoneUpdates(
    const Properties::TimeZoneUpdate time_zone_updates,
    PropertiesSetCallback callback) {
    auto data = prepareCallback(std::move(callback));
    setProperty(TIMEZONEUPDATES_STR,
                g_variant_new_string(
                    (TIME_ZONE_UPDATE_MAP.toString(time_zone_updates)).data()),
                nullptr, &Clock::finishAsyncCall, data.release());
}

void Clock::setTimeServers(const std::vector<std::string>& servers,
                           PropertiesSetCallback callback) {
    auto data = prepareCallback(std::move(callback));
    GVariantBuilder builder;
    g_variant_builder_init(&builder, G_VARIANT_TYPE("as"));
    for (const auto& server : servers) {
        GVariant* str_variant = g_variant_new_string(server.c_str());
        g_variant_builder_add_value(&builder, str_variant);
    }
    GVariant* servers_variant = g_variant_builder_end(&builder);
    setProperty(TIMESERVERS_STR, servers_variant, nullptr,
                &Clock::finishAsyncCall, data.release());
    g_variant_builder_clear(&builder);
}

auto operator<<(std::ostream& ost,
                const ClockProperties& obj) -> std::ostream& {
    ost << TIME_STR << ": " << obj.time_ << " (";
    const auto time_value = static_cast<std::time_t>(obj.time_);
    ost << std::put_time(std::localtime(&time_value), "%Y-%m-%d %H:%M:%S")
        << ")\n";
    ost << TIMEUPDATES_STR << ": "
        << TIME_UPDATE_MAP.toString(obj.time_updates_) << '\n';
    ost << TIMEZONE_STR << ": " << obj.timezone_ << '\n';
    ost << TIMEZONEUPDATES_STR << ": "
        << TIME_ZONE_UPDATE_MAP.toString(obj.timezone_updates_) << '\n';
    ost << TIMESERVERSYNCED_STR << ": " << std::boolalpha
        << obj.time_server_synced_ << '\n';
    ost << TIMESERVERS_STR << ": ";
    for (const auto& server : obj.time_servers_) {
        ost << server << ' ';
    }
    ost << '\n';
    return ost;
}

}  // namespace Amarula::DBus::G::Connman
