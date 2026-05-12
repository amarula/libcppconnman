
#include <glib.h>

#include <amarula/dbus/connman/gtechnology.hpp>
#include <amarula/dbus/gdbus.hpp>
#include <amarula/dbus/gproxy.hpp>
#include <amarula/log.hpp>
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

void Technology::setTethering(bool tethering, PropertiesSetCallback callback) {
    auto data = prepareCallback(std::move(callback));
    setProperty(TETHERING_STR,
                g_variant_new_boolean(static_cast<gboolean>(tethering)),
                nullptr, &Technology::finishAsyncCall, data.release());
}

void Technology::setTetheringIdentifier(const std::string& identifier,
                                        PropertiesSetCallback callback) {
    auto data = prepareCallback(std::move(callback));
    setProperty(TETHERINGIDENTIFIER_STR,
                g_variant_new_string(identifier.c_str()), nullptr,
                &Technology::finishAsyncCall, data.release());
}

void Technology::setTetheringPassphrase(const std::string& passphrase,
                                        PropertiesSetCallback callback) {
    auto data = prepareCallback(std::move(callback));
    setProperty(TETHERINGPASSPHRASE_STR,
                g_variant_new_string(passphrase.c_str()), nullptr,
                &Technology::finishAsyncCall, data.release());
}

void Technology::setTetheringFreq(const uint32_t frequency,
                                  PropertiesSetCallback callback) {
    auto data = prepareCallback(std::move(callback));
    setProperty(TETHERINGFREQ_STR, g_variant_new_uint32(frequency), nullptr,
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
    } else if (g_strcmp0(key, TETHERINGIDENTIFIER_STR) == 0U) {
        tethering_identifier_ = g_variant_get_string(value, nullptr);
    } else if (g_strcmp0(key, TETHERINGPASSPHRASE_STR) == 0U) {
        tethering_passphrase_ = g_variant_get_string(value, nullptr);
    } else {
        LCM_LOG("Unknown property for Technology: " << key << '\n');
    }
}

auto operator<<(std::ostream& ost, const TechProperties& obj) -> std::ostream& {
    ost << NAME_STR << ": " << obj.name_ << '\n';
    ost << TYPE_STR << ": " << TYPE_MAP.toString(obj.type_) << '\n';
    ost << POWERED_STR << ": " << std::boolalpha << obj.powered_ << '\n';
    ost << CONNECTED_STR << ": " << std::boolalpha << obj.connected_ << '\n';
    ost << TETHERING_STR << ": " << std::boolalpha << obj.tethering_ << '\n';
    if (obj.type_ == TechProperties::Type::Wifi) {
        ost << TETHERINGIDENTIFIER_STR << ": " << obj.tethering_identifier_
            << '\n';
        ost << TETHERINGPASSPHRASE_STR << ": " << obj.tethering_passphrase_
            << '\n';
        ost << TETHERINGFREQ_STR << ": " << obj.tethering_freq_ << " \n";
    }
    return ost;
}

}  // namespace Amarula::DBus::G::Connman
