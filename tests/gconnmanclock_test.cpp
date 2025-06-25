#include <gtest/gtest.h>

#include <amarula/dbus/gconnman.hpp>
#include <iomanip>
using Amarula::DBus::Connman;
using Amarula::DBus::ConnmanClock;

constexpr guint kTestTime = 1633036800;
constexpr const char* kTestTimeZone = "America/Vancouver";
constexpr int kSleepDurationSeconds = 30;

namespace {
void print_clock_properties(
    Amarula::DBus::ConnmanClock::ClockProperties& props) {
    std::cout << "Time: " << props.time << " (";
    const auto timeValue = static_cast<std::time_t>(props.time);
    std::cout << std::put_time(std::localtime(&timeValue), "%Y-%m-%d %H:%M:%S")
              << ")\n";
    std::cout << "Time Updates: " << props.timeUpdates << '\n';
    std::cout << "Timezone: " << props.timezone << '\n';
    std::cout << "Timezone Updates: " << props.timezoneUpdates << '\n';
    std::cout << "Time Server Synced: "
              << (props.timeServerSynced ? "Yes" : "No") << '\n';
    std::cout << "Timeservers: ";
    for (const auto& server : props.timeservers) {
        std::cout << server << ' ';
    }
    std::cout << '\n';
}
}  // namespace

TEST(Connman, ClockSetTime) {
    const Connman connman;
    const guint time = kTestTime;
    connman.clock()->setTimeUpdates(
        ConnmanClock::PROP_TIMEUPDATES_MANUAL, [&connman](auto success) {
            std::cout << "Time updates set to manual: "
                      << (success ? "Success" : "Failure") << '\n';
            connman.clock()->setTime(time, [&connman](auto success) {
                std::cout << "Time set to " << kTestTime << ": "
                          << (success ? "Success" : "Failure") << '\n';
                connman.clock()->getProperties(
                    [](auto& props) { print_clock_properties(props); });
            });
        });
}

TEST(Connman, ClockSetTimeZone) {
    const Connman connman;
    connman.clock()->setTimeZoneUpdates(
        ConnmanClock::PROP_TIMEUPDATES_MANUAL, [&connman](auto success) {
            std::cout << "TimeZone updates set to manual: "
                      << (success ? "Success" : "Failure") << '\n';
            connman.clock()->setTimeZone(
                kTestTimeZone, [&connman](auto success) {
                    std::cout << "TimeZone set to " << kTestTimeZone << ": "
                              << (success ? "Success" : "Failure") << '\n';
                    connman.clock()->getProperties(
                        [](auto& props) { print_clock_properties(props); });
                });
        });
}

TEST(Connman, ClockSetTimeServers) {
    const Connman connman;
    const std::vector<std::string> servers = {"time1.example.com",
                                              "time2.example.com"};
    connman.clock()->setTimeServers(servers, [&connman](auto success) {
        std::cout << "Time servers set: " << (success ? "Success" : "Failure")
                  << '\n';
        connman.clock()->getProperties(
            [](auto& props) { print_clock_properties(props); });
    });
}

TEST(Connman, ClockProxyInitialization) {
    EXPECT_NO_THROW({ const Connman connman; });
}
TEST(Connman, clockGetPropertiesNull) {
    const Connman connman;
    connman.clock()->getProperties();
}

TEST(Connman, ClockGetProperties) {
    const Connman connman;
    connman.clock()->getProperties(
        [](auto& props) { print_clock_properties(props); });
}

TEST(Connman, ClockOnPropertiesChanged) {
    const Connman connman;

    connman.clock()->onPropertiesChanged(
        [](auto& props) { print_clock_properties(props); });
    std::this_thread::sleep_for(std::chrono::seconds(kSleepDurationSeconds));
}
