#include <glib.h>
#include <glibconfig.h>

#include <amarula/dbus/connman/gagent.hpp>
#include <amarula/dbus/connman/gclock.hpp>
#include <amarula/dbus/connman/gconnman.hpp>
#include <amarula/dbus/connman/gmanager.hpp>
#include <amarula/dbus/connman/gservice.hpp>
#include <amarula/dbus/gdbus.hpp>
#include <memory>

#include "gconnman_private.hpp"

namespace Amarula::DBus::G::Connman {

Connman::Connman()
    : dbus_(std::make_unique<DBus>(SERVICE, OBJECT_PATH)),
      clock_{std::shared_ptr<Clock>(new Clock(dbus_.get()))},
      manager_{std::shared_ptr<Manager>(new Manager(dbus_.get()))} {
    clock_->getProperties();
}

}  // namespace Amarula::DBus::G::Connman
