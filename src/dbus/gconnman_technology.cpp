
#include <glib.h>

#include <amarula/dbus/connman/gtechnology.hpp>
#include <amarula/dbus/gdbus.hpp>
#include <amarula/dbus/gproxy.hpp>
#include <amarula/log.hpp>
#include <iostream>
#include <utility>

#include "gconnman_private.hpp"
#include "gdbus_private.hpp"

namespace Amarula::DBus::G::Connman {

using Type = TechProperties::Type;
static constexpr EnumStringMap<Type, 9> TYPE_MAP{
    {{{Type::Ethernet, "ethernet"},
      {Type::Wifi, "wifi"},
      {Type::Cellular, "cellular"},
      {Type::Bluetooth, "bluetooth"},
      {Type::Vpn, "vpn"},
      {Type::Wired, "wired"},
      {Type::P2p, "p2p"},
      {Type::Gps, "gps"},
      {Type::Gadget, "gadget"}}}};

Technology::Technology(DBus* dbus, const gchar* obj_path)
    : DBusProxy(dbus, SERVICE, obj_path, TECHNOLOGY_INTERFACE) {}

void Technology::setPowered(bool powered, PropertiesSetCallback callback) {
    auto data = prepareCallback(std::move(callback));
    setProperty(POWERED_STR,
                g_variant_new_boolean(static_cast<gboolean>(powered)), nullptr,
                &Technology::finishAsyncCall, data.release());
}

void Technology::scan(PropertiesSetCallback callback) {
    auto data = prepareCallback(std::move(callback));
    callMethod(nullptr, SCAN_STR, nullptr, &Technology::finishAsyncCall,
               data.release());
}

void TechProperties::update(const gchar* key, GVariant* value) {
    if (g_strcmp0(key, NAME_STR) == 0) {
        name_ = g_variant_get_string(value, nullptr);
    } else if (g_strcmp0(key, TYPE_STR) == 0U) {
        type_ = TYPE_MAP.fromString(g_variant_get_string(value, nullptr));

    } else if (g_strcmp0(key, POWERED_STR) == 0U) {
        powered_ = g_variant_get_boolean(value) == 1U;
    } else if (g_strcmp0(key, CONNECTED_STR) == 0U) {
        connected_ = g_variant_get_boolean(value) == 1U;
    } else if (g_strcmp0(key, TETHERING_STR) == 0U) {
        tethering_ = g_variant_get_boolean(value) == 1U;
    } else if (g_strcmp0(key, TETHERINGFREQ_STR) == 0U) {
        tethering_freq_ = g_variant_get_int32(value);
    } else {
        LCM_LOG("Unknown property for Technology: " << key << '\n');
    }
}

void TechProperties::print() const {
    LCM_LOG("@@@@@@@@@@ TechProperties: @@@@@@@@@@@@@@@\n");
    LCM_LOG(NAME_STR << ": " << name_ << '\n');
    LCM_LOG(TYPE_STR << ": " << TYPE_MAP.toString(type_) << '\n');
    LCM_LOG(POWERED_STR << ": " << std::boolalpha << powered_ << '\n');
    LCM_LOG(CONNECTED_STR << ": " << std::boolalpha << connected_ << '\n');
    LCM_LOG(TETHERING_STR << ": " << std::boolalpha << tethering_ << '\n');
    LCM_LOG(TETHERINGFREQ_STR << ": " << tethering_freq_ << " \n");
    LCM_LOG("@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@\n");
}

}  // namespace Amarula::DBus::G::Connman
