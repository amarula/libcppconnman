#include <glib.h>
#include <gtest/gtest.h>

#include <amarula/dbus/connman/gclock.hpp>
#include <amarula/dbus/connman/gconnman.hpp>
#include <iostream>
#include <string>
#include <vector>

#include "qt_thread_bundle.hpp"

using Amarula::DBus::G::Connman::Connman;
using TimeUpdate = Amarula::DBus::G::Connman::ClockProperties::TimeUpdate;
using TimeZoneUpdate = TimeUpdate;

constexpr guint TEST_TIME = 1633036800;
constexpr auto TEST_TIME_ZONE = "America/Vancouver";
constexpr int SLEEP_DURATION_SECONDS = 3;

TEST(Connman, ClockSetTimeUpdates) {
    const QtThreadBundle qt_thread_bundle;
    const Connman connman;
    connman.clock()->onPropertyChanged(
        [main_tid = qt_thread_bundle.main_tid,
         loop_tid = qt_thread_bundle.loop_tid](auto& props) {
            const auto callback_tid = std::this_thread::get_id();
            EXPECT_NE(callback_tid, main_tid);
            EXPECT_NE(callback_tid, loop_tid);
            std::cout << "onPropertyChanged:\n";
            props.print();
        });
    connman.clock()->setTimeUpdates(
        TimeUpdate::Auto, [&connman, main_tid = qt_thread_bundle.main_tid,
                           loop_tid = qt_thread_bundle.loop_tid](auto success) {
            const auto callback_tid = std::this_thread::get_id();
            EXPECT_NE(callback_tid, main_tid);
            EXPECT_NE(callback_tid, loop_tid);
            EXPECT_TRUE(success);
        });
}

TEST(Connman, ClockSetTime) {
    const QtThreadBundle qt_thread_bundle;
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
    const QtThreadBundle qt_thread_bundle;
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
    const QtThreadBundle qt_thread_bundle;
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
    const QtThreadBundle qt_thread_bundle;
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
    const QtThreadBundle qt_thread_bundle;
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
    EXPECT_NO_THROW({
        const QtThreadBundle qt_thread_bundle;
        const Connman connman;
    });
}
TEST(Connman, clockGetPropertiesNull) {
    const QtThreadBundle qt_thread_bundle;
    const Connman connman;
    connman.clock()->getProperties();
}

TEST(Connman, ClockGetProperties) {
    const QtThreadBundle qt_thread_bundle;
    const Connman connman;
    connman.clock()->getProperties(
        [main_tid = qt_thread_bundle.main_tid,
         loop_tid = qt_thread_bundle.loop_tid](auto& props) {
            const auto callback_tid = std::this_thread::get_id();
            EXPECT_NE(callback_tid, main_tid);
            EXPECT_NE(callback_tid, loop_tid);
            props.print();
        });
}
