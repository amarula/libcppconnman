#pragma once
#include <amarula/dbus/gconnman_clock.hpp>
#include <amarula/dbus/gproxy.hpp>

namespace Amarula::DBus {

class Connman : public GProxy {
    ConnmanClock* clock_{nullptr};

    void cleanup() override { delete clock_; }

    void init();

   public:
    Connman();
    [[nodiscard]] auto clock() const -> ConnmanClock* { return clock_; }
};

}  // namespace Amarula::DBus
