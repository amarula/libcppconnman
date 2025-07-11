#pragma once

#include <amarula/dbus/connman/gclock.hpp>
#include <amarula/dbus/connman/gmanager.hpp>
#include <amarula/dbus/connman/gservice.hpp>
#include <amarula/dbus/connman/gtechnology.hpp>
#include <amarula/dbus/gdbus.hpp>
#include <memory>
#include <mutex>

namespace Amarula::DBus::G::Connman {

class Connman {
    std::unique_ptr<DBus> dbus_;

   public:
    Connman();
    Connman(const Connman&) = delete;
    auto operator=(const Connman&) -> Connman& = delete;
    Connman(Connman&&) = delete;
    auto operator=(Connman&&) -> Connman& = delete;
    ~Connman() { dbus_.reset(); }

    [[nodiscard]] auto clock() const { return clock_; }
    [[nodiscard]] auto manager() const { return manager_; }

   private:
    std::mutex mtx_;
    std::shared_ptr<Clock> clock_{nullptr};
    std::shared_ptr<Manager> manager_{nullptr};
};

}  // namespace Amarula::DBus::G::Connman
