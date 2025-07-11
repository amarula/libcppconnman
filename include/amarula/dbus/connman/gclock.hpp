#pragma once
#include <glib.h>

#include <amarula/dbus/gdbus.hpp>
#include <amarula/dbus/gproxy.hpp>
#include <cstdint>
#include <string>
#include <vector>

namespace Amarula::DBus::G::Connman {

class Connman;

struct ClockProperties {
   public:
    enum class TimeUpdate : uint8_t { Manual = 0, Auto };
    using TimeZoneUpdate = TimeUpdate;

    [[nodiscard]] auto getTime() const { return time_; }
    [[nodiscard]] auto getTimeUpdates() const { return time_updates_; }
    [[nodiscard]] auto getTimezone() const { return timezone_; }
    [[nodiscard]] auto getTimezoneUpdates() const { return timezone_updates_; }
    [[nodiscard]] auto getTimeServers() const { return time_servers_; }
    [[nodiscard]] auto isTimeServerSynced() const {
        return time_server_synced_;
    }

    void print() const;

   private:
    uint64_t time_{0};
    TimeUpdate time_updates_{};
    std::string timezone_;
    TimeZoneUpdate timezone_updates_{};
    std::vector<std::string> time_servers_;
    bool time_server_synced_{false};

    void update(const gchar* key, GVariant* value);

    friend class Clock;
    friend class DBusProxy<ClockProperties>;
};

class Clock : public DBusProxy<ClockProperties> {
   private:
    explicit Clock(DBus* dbus);

    using DBusProxy::DBusProxy;

   public:
    using Properties = ClockProperties;
    void setTime(uint64_t time, PropertiesSetCallback callback = nullptr);
    void setTimeZone(const std::string& timezone,
                     PropertiesSetCallback callback = nullptr);
    void setTimeUpdates(Properties::TimeUpdate time_updates,
                        PropertiesSetCallback callback = nullptr);
    void setTimeZoneUpdates(Properties::TimeZoneUpdate time_zone_updates,
                            PropertiesSetCallback callback = nullptr);
    void setTimeServers(const std::vector<std::string>& servers,
                        PropertiesSetCallback callback = nullptr);
    friend class Connman;
};

}  // namespace Amarula::DBus::G::Connman
