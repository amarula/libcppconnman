#include <gtest/gtest.h>

#include <amarula/dbus/gdbus.hpp>
#include <stdexcept>

using Amarula::DBus::G::DBus;

TEST(DBus, Initialization) {
    EXPECT_NO_THROW(
        { const DBus dbus("org.freedesktop.DBus", "/org/freedesktop/DBus"); });
    EXPECT_THROW(
        { const DBus dbus("invalid.bus.name", "/invalid/object/path"); },
        std::runtime_error);
}
