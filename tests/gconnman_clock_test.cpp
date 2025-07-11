#include <glib.h>
#include <gtest/gtest.h>

#include <amarula/dbus/connman/gclock.hpp>
#include <amarula/dbus/connman/gconnman.hpp>
#include <iostream>
#include <string>
#include <vector>

using Amarula::DBus::G::Connman::Connman;
using TimeUpdate = Amarula::DBus::G::Connman::ClockProperties::TimeUpdate;
using TimeZoneUpdate = TimeUpdate;

constexpr guint TEST_TIME = 1633036800;
constexpr auto TEST_TIME_ZONE = "America/Vancouver";
constexpr int SLEEP_DURATION_SECONDS = 3;

TEST(Connman, ClockSetTimeUpdates) {
    const Connman connman;
    connman.clock()->onPropertyChanged([](auto& props) {
        std::cout << "onPropertyChanged:\n";
        props.print();
    });
    connman.clock()->setTimeUpdates(
        TimeUpdate::Auto, [&connman](auto success) { EXPECT_TRUE(success); });
}

TEST(Connman, ClockSetTime) {
    const Connman connman;
    const guint time = TEST_TIME;
    connman.clock()->onPropertyChanged([](auto& props) {
        std::cout << "onPropertyChanged:\n";
        props.print();
    });
    connman.clock()->setTimeUpdates(
        TimeUpdate::Manual, [&connman](auto success) {
            EXPECT_TRUE(success);
            connman.clock()->setTime(time, [&connman](auto success) {
                EXPECT_TRUE(success);
                connman.clock()->getProperties([](auto& props) {
                    std::cout << "getProperties:\n";
                    props.print();
                });
            });
        });
}

TEST(Connman, ClockSetTimeServers1) {
    const Connman connman;
    connman.clock()->onPropertyChanged([](auto& props) {
        std::cout << "onPropertyChanged:\n";
        props.print();
    });
    const std::vector<std::string> servers = {"time1.google.com",
                                              "time2.google.com"};
    connman.clock()->setTimeServers(
        servers, [&connman](auto success) { EXPECT_TRUE(success); });
}

TEST(Connman, ClockSetTimeServers2) {
    const Connman connman;
    connman.clock()->onPropertyChanged([](auto& props) {
        std::cout << "onPropertyChanged:\n";
        props.print();
    });
    const std::vector<std::string> servers = {"time1.example.com",
                                              "time2.example.com"};
    connman.clock()->setTimeServers(servers, [&connman](auto success) {
        EXPECT_TRUE(success);
        connman.clock()->getProperties([](auto& props) {
            std::cout << "getProperties:\n";
            props.print();
        });
    });
}

TEST(Connman, ClockSetTimeZoneUpdates) {
    const Connman connman;
    connman.clock()->onPropertyChanged([](auto& props) {
        std::cout << "onPropertyChanged:\n";
        props.print();
    });
    connman.clock()->setTimeZoneUpdates(
        TimeZoneUpdate::Auto,
        [&connman](auto success) { EXPECT_TRUE(success); });
}

TEST(Connman, ClockSetTimeZone) {
    const Connman connman;
    connman.clock()->onPropertyChanged([](auto& props) {
        std::cout << "onPropertyChanged:\n";
        props.print();
    });
    connman.clock()->setTimeZoneUpdates(
        TimeZoneUpdate::Manual, [&connman](auto success) {
            EXPECT_TRUE(success);
            connman.clock()->setTimeZone(
                TEST_TIME_ZONE, [&connman](auto success) {
                    EXPECT_TRUE(success);
                    connman.clock()->getProperties([](auto& props) {
                        std::cout << "getProperties:\n";
                        props.print();
                    });
                });
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
    connman.clock()->getProperties([](auto& props) { props.print(); });
}
