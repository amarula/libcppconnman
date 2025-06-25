#pragma once
#include <amarula/dbus/gproxy.hpp>
#include <any>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <vector>
namespace Amarula::DBus {
class Connman;

class ConnmanClock {
   public:
    struct ClockProperties {
        uint32_t time = 0;
        std::string timeUpdates;
        std::string timezone;
        std::string timezoneUpdates;
        std::vector<std::string> timeservers;
        bool timeServerSynced;
    };
    constexpr static const char* PROP_TIMEUPDATES_AUTO = "auto";
    constexpr static const char* PROP_TIMEUPDATES_MANUAL = "manual";
    using ClockPropertiesCallback = std::function<void(ClockProperties&)>;
    using ClockPropertiesSetCallback = std::function<void(bool success)>;

   private:
    size_t callback_counter_{0U};
    std::mutex mtx_;
    gpointer clock_proxy_{nullptr};
    GProxy* gproxy_{nullptr};
    std::map<size_t, std::any> properties_callbacks_;
    template <typename T>
    auto prepareCallback(T callback) {
        std::unique_lock<std::mutex> const lock(mtx_);
        gproxy_->on_any_async_start();
        std::optional<size_t> call_counter = std::nullopt;
        if (callback != nullptr) {
            size_t index = ++callback_counter_;
            call_counter = index;
            properties_callbacks_[index] = std::move(callback);
        }
        return call_counter;
    }
    void updateProperties(GVariant* properties,
                          const std::optional<size_t>& counter);
    static void get_property_cb(GObject* proxy, GAsyncResult* res,
                                gpointer user_data);
    static void set_property_cb(GObject* proxy, GAsyncResult* res,
                                gpointer user_data);
    static void on_properties_changed_cb(
        GDBusProxy* proxy, GVariant* changed_properties,
        const gchar* const* invalidated_properties, gpointer user_data);
    explicit ConnmanClock(GProxy* proxy);

   public:
    ConnmanClock(const ConnmanClock&) = delete;
    auto operator=(const ConnmanClock&) = delete;
    ConnmanClock(ConnmanClock&&) = delete;
    auto operator=(ConnmanClock&&) = delete;

    ~ConnmanClock();

    void setTime(unsigned int time,
                 ClockPropertiesSetCallback callback = nullptr);
    void setTimeZone(const std::string& timezone,
                     ClockPropertiesSetCallback callback = nullptr);
    void setTimeUpdates(const std::string& timeUpdates,
                        ClockPropertiesSetCallback callback = nullptr);
    void setTimeZoneUpdates(const std::string& timeZoneUpdates,
                            ClockPropertiesSetCallback callback = nullptr);
    void setTimeServers(const std::vector<std::string>& servers,
                        ClockPropertiesSetCallback callback = nullptr);
    void getProperties(ClockPropertiesCallback callback = nullptr);
    void onPropertiesChanged(ClockPropertiesCallback callback);
    [[nodiscard]] auto getGProxy() const { return gproxy_; }

    friend class Connman;
};

}  // namespace Amarula::DBus
