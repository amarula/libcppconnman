#include <amarula/dbus/gconnman.hpp>

#include "gconnman_definitions.hpp"
#include "net_connman_proxy_gdbus_generated.h"
namespace Amarula::DBus {

Connman::Connman()
    : GProxy(GConnman::SERVICE, "/net/connman"),
      clock_(new ConnmanClock(this)) {}

}  // namespace Amarula::DBus