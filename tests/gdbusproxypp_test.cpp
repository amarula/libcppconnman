#include <gtest/gtest.h>

#include <amarula/dbus/gproxy.hpp>

TEST(GProxy, Initialization) {
    EXPECT_NO_THROW({
        Amarula::DBus::GProxy dbus("org.freedesktop.DBus",
                                   "/org/freedesktop/DBus");
    });
    EXPECT_THROW(
        {
            const Amarula::DBus::GProxy dbus("invalid.bus.name",
                                             "/invalid/object/path");
        },
        std::runtime_error);
}
