#include <glib.h>

#include <amarula/dbus/connman/gservice.hpp>
#include <amarula/dbus/gdbus.hpp>
#include <amarula/dbus/gproxy.hpp>
#include <amarula/log.hpp>
#include <cstdint>
#include <optional>
#include <utility>

#include "gconnman_private.hpp"
#include "gdbus_private.hpp"

namespace Amarula::DBus::G::Connman {

using Type = ServProperties::Type;
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

using State = ServProperties::State;
static constexpr EnumStringMap<State, 7> STATE_MAP{
    {{{State::Association, "association"},
      {State::Configuration, "configuration"},
      {State::Disconnect, "disconnect"},
      {State::Failure, "failure"},
      {State::Idle, "idle"},
      {State::Online, "online"},
      {State::Ready, "ready"}}}};

using Security = ServProperties::Security;
static constexpr EnumStringMap<Security, 6> SECURITY_MAP{
    {{{Security::None, "none"},
      {Security::Wep, "wep"},
      {Security::Psk, "psk"},
      {Security::Ieee8021x, "ieee8021x"},
      {Security::Wps, "wps"},
      {Security::WpsAdvertising, "wps_advertising"}}}};

using Error = ServProperties::Error;
static constexpr EnumStringMap<Error, 10> ERROR_MAP{
    {{{Error::None, ""},
      {Error::OutOfRange, "out-of-range"},
      {Error::PinMissing, "pin-missing"},
      {Error::DhcpFailed, "dhcp-failed"},
      {Error::ConnectFailed, "connect-failed"},
      {Error::LoginFailed, "login-failed"},
      {Error::AuthFailed, "auth-failed"},
      {Error::InvalidKey, "invalid-key"},
      {Error::Blocked, "blocked"},
      {Error::OnlineCheckFailed, "online-check-failed"}}}};

static constexpr EnumStringMap<IPv4::Method, 4> IPV4_METHOD_MAP{
    {{{IPv4::Method::Off, "off"},
      {IPv4::Method::Dhcp, "dhcp"},
      {IPv4::Method::Manual, "manual"},
      {IPv4::Method::Auto, "auto"}}}};

static constexpr EnumStringMap<Proxy::Method, 3> PROXY_METHOD_MAP{
    {{{Proxy::Method::Direct, "direct"},
      {Proxy::Method::Manual, "manual"},
      {Proxy::Method::Auto, "auto"}}}};

static constexpr EnumStringMap<Ethernet::Method, 2> ETHERNET_METHOD_MAP{
    {{{Ethernet::Method::Manual, "manual"}, {Ethernet::Method::Auto, "auto"}}}};

static constexpr EnumStringMap<IPv6::Method, 5> IPV6_METHOD_MAP{
    {{{IPv6::Method::Off, "off"},
      {IPv6::Method::K6to4, "6to4"},
      {IPv6::Method::Manual, "manual"},
      {IPv6::Method::Fixed, "fixed"},
      {IPv6::Method::Auto, "auto"}}}};

static constexpr EnumStringMap<IPv6::Privacy, 4> IPV6_PRIVACY_MAP{
    {{{IPv6::Privacy::Disabled, "disabled"},
      {IPv6::Privacy::Enabled, "enabled"},
      {IPv6::Privacy::Preferred, "preferred"},
      {IPv6::Privacy::Preferred, "prefered"}}}};

Service::Service(DBus* dbus, const gchar* obj_path)
    : DBusProxy(dbus, SERVICE, obj_path, SERVICE_INTERFACE) {}

void Service::connect(PropertiesSetCallback callback) {
    auto data = prepareCallback(std::move(callback));
    callMethod(nullptr, CONNECT_STR, nullptr, &Service::finishAsyncCall,
               data.release());
}

void Service::disconnect(PropertiesSetCallback callback) {
    auto data = prepareCallback(std::move(callback));
    callMethod(nullptr, DISCONNECT_STR, nullptr, &Service::finishAsyncCall,
               data.release());
}

void Service::remove(PropertiesSetCallback callback) {
    auto data = prepareCallback(std::move(callback));
    callMethod(nullptr, REMOVE_STR, nullptr, &Service::finishAsyncCall,
               data.release());
}

void Service::setAutoconnect(const bool autoconnect,
                             PropertiesSetCallback callback) {
    auto data = prepareCallback(std::move(callback));
    setProperty(AUTOCONNECT_STR,
                g_variant_new_boolean(static_cast<gboolean>(autoconnect)),
                nullptr, &Service::finishAsyncCall, data.release());
}

void Service::setNameServers(const std::vector<std::string>& name_servers,
                             PropertiesSetCallback callback) {
    auto data = prepareCallback(std::move(callback));
    auto variant = vector_to_as(name_servers);
    setProperty(NAMESERVERS_CONFIGURATION_STR, variant.get(), nullptr,
                &Service::finishAsyncCall, data.release());
}

void IPv4::update(const gchar* key, GVariant* value) {
    if (g_strcmp0(key, METHOD_STR) == 0U) {
        method_ =
            IPV4_METHOD_MAP.fromString(g_variant_get_string(value, nullptr));
    } else if (g_strcmp0(key, ADDRESS_STR) == 0U) {
        address_ = g_variant_get_string(value, nullptr);
    } else if (g_strcmp0(key, NETMASK_STR) == 0U) {
        netmask_ = g_variant_get_string(value, nullptr);
    } else if (g_strcmp0(key, GATEWAY_STR) == 0U) {
        gateway_ = g_variant_get_string(value, nullptr);
    } else {
        LCM_LOG("Unknown property for IPv4: " << key << '\n');
    }
}

void IPv6::update(const gchar* key, GVariant* value) {
    if (g_strcmp0(key, METHOD_STR) == 0U) {
        method_ =
            IPV6_METHOD_MAP.fromString(g_variant_get_string(value, nullptr));
    } else if (g_strcmp0(key, ADDRESS_STR) == 0U) {
        address_ = g_variant_get_string(value, nullptr);
    } else if (g_strcmp0(key, GATEWAY_STR) == 0U) {
        gateway_ = g_variant_get_string(value, nullptr);
    } else if (g_strcmp0(key, PRIVACY_STR) == 0U) {
        privacy_ =
            IPV6_PRIVACY_MAP.fromString(g_variant_get_string(value, nullptr));
    } else if (g_strcmp0(key, PREFIXLENGTH_STR) == 0U) {
        prefix_length_ = static_cast<uint8_t>(g_variant_get_byte(value));
    } else {
        LCM_LOG("Unknown property for IPv6: " << key << '\n');
    }
}

void GVariantParser::parse(GVariant* variant) {
    GVariantIter* iter = g_variant_iter_new(variant);
    GVariant* prop = nullptr;

    while ((prop = g_variant_iter_next_value(iter)) != nullptr) {
        GVariant* key_variant = g_variant_get_child_value(prop, 0);
        const gchar* key = g_variant_get_string(key_variant, nullptr);
        GVariant* wrapped = g_variant_get_child_value(prop, 1);
        GVariant* value = g_variant_get_variant(wrapped);

        update(key, value);

        g_variant_unref(key_variant);
        g_variant_unref(wrapped);
        g_variant_unref(value);
        g_variant_unref(prop);
    }
    g_variant_iter_free(iter);
}

void Ethernet::update(const gchar* key, GVariant* value) {
    if (g_strcmp0(key, METHOD_STR) == 0U) {
        method_ = ETHERNET_METHOD_MAP.fromString(
            g_variant_get_string(value, nullptr));
    } else if (g_strcmp0(key, INTERFACE_STR) == 0U) {
        interface_ = g_variant_get_string(value, nullptr);
    } else if (g_strcmp0(key, ADDRESS_STR) == 0U) {
        address_ = g_variant_get_string(value, nullptr);
    } else if (g_strcmp0(key, MTU_STR) == 0U) {
        mtu_ = static_cast<uint16_t>(g_variant_get_uint16(value));
    } else {
        LCM_LOG("Unknown property for Ethernet: " << key << '\n');
    }
}

void Provider::update(const gchar* key, GVariant* value) {
    if (g_strcmp0(key, HOST_STR) == 0U) {
        host_ = g_variant_get_string(value, nullptr);
    } else if (g_strcmp0(key, DOMAIN_STR) == 0U) {
        domain_ = g_variant_get_string(value, nullptr);
    } else if (g_strcmp0(key, NAME_STR) == 0U) {
        name_ = g_variant_get_string(value, nullptr);
    } else if (g_strcmp0(key, TYPE_STR) == 0U) {
        type_ = g_variant_get_string(value, nullptr);
    } else {
        LCM_LOG("Unknown property for Provider: " << key << '\n');
    }
}

void Proxy::update(const gchar* key, GVariant* value) {
    if (g_strcmp0(key, METHOD_STR) == 0U) {
        method_ =
            PROXY_METHOD_MAP.fromString(g_variant_get_string(value, nullptr));
    } else if (g_strcmp0(key, URL_STR) == 0U) {
        url_ = g_variant_get_string(value, nullptr);
    } else if (g_strcmp0(key, SERVERS_STR) == 0U) {
        servers_ = as_to_vector(value);
    } else if (g_strcmp0(key, EXCLUDES_STR) == 0U) {
        excludes_ = as_to_vector(value);
    } else {
        LCM_LOG("Unknown property for Proxy: " << key << '\n');
    }
}

void ServProperties::update(const gchar* key, GVariant* value) {
    if (g_strcmp0(key, NAME_STR) == 0) {
        name_ = g_variant_get_string(value, nullptr);
    } else if (g_strcmp0(key, TYPE_STR) == 0U) {
        type_ = TYPE_MAP.fromString(g_variant_get_string(value, nullptr));
    } else if (g_strcmp0(key, STATE_STR) == 0U) {
        state_ = STATE_MAP.fromString(g_variant_get_string(value, nullptr));
    } else if (g_strcmp0(key, ERROR_STR) == 0U) {
        error_ = ERROR_MAP.fromString(g_variant_get_string(value, nullptr));
    } else if (g_strcmp0(key, FAVORITE_STR) == 0U) {
        favorite_ = g_variant_get_boolean(value) == 1U;
    } else if (g_strcmp0(key, IMMUTABLE_STR) == 0U) {
        immutable_ = g_variant_get_boolean(value) == 1U;
    } else if (g_strcmp0(key, AUTOCONNECT_STR) == 0U) {
        autoconnect_ = g_variant_get_boolean(value) == 1U;
    } else if (g_strcmp0(key, MDNS_STR) == 0U) {
        mdns_ = g_variant_get_boolean(value) == 1U;
    } else if (g_strcmp0(key, STRENGTH_STR) == 0U) {
        strength_ = g_variant_get_byte(value);
    } else if (g_strcmp0(key, IPV4_STR) == 0U) {
        ipv4_ = (g_variant_n_children(value) != 0)
                    ? std::optional<IPv4>(IPv4(value))
                    : std::nullopt;
    } else if (g_strcmp0(key, IPV6_STR) == 0U) {
        ipv6_ = (g_variant_n_children(value) != 0)
                    ? std::optional<IPv6>(IPv6(value))
                    : std::nullopt;
    } else if (g_strcmp0(key, ETHERNET_STR) == 0U) {
        ethernet_ = (g_variant_n_children(value) != 0)
                        ? std::optional<Ethernet>(Ethernet(value))
                        : std::nullopt;
    } else if (g_strcmp0(key, PROVIDER_STR) == 0U) {
        provider_ = (g_variant_n_children(value) != 0)
                        ? std::optional<Provider>(Provider(value))
                        : std::nullopt;
    } else if (g_strcmp0(key, PROXY_STR) == 0U) {
        proxy_ = (g_variant_n_children(value) != 0)
                     ? std::optional<Proxy>(Proxy(value))
                     : std::nullopt;
    } else if (g_strcmp0(key, SECURITY_STR) == 0U) {
        security_ = (g_variant_n_children(value) != 0)
                        ? std::optional<std::vector<Security>>(
                              as_to_vector<Security>(value, &SECURITY_MAP))
                        : std::nullopt;
    } else if (g_strcmp0(key, NAMESERVERS_STR) == 0U) {
        name_servers_ =
            (g_variant_n_children(value) != 0)
                ? std::optional<std::vector<std::string>>(as_to_vector(value))
                : std::nullopt;
    } else if (g_strcmp0(key, NAMESERVERS_CONFIGURATION_STR) == 0U) {
        name_servers_conf_ =
            (g_variant_n_children(value) != 0)
                ? std::optional<std::vector<std::string>>(as_to_vector(value))
                : std::nullopt;
    } else if (g_strcmp0(key, DOMAINS_STR) == 0U) {
        domains_ =
            (g_variant_n_children(value) != 0)
                ? std::optional<std::vector<std::string>>(as_to_vector(value))
                : std::nullopt;
    } else if (g_strcmp0(key, TIMESERVERS_STR) == 0U) {
        time_servers_ =
            (g_variant_n_children(value) != 0)
                ? std::optional<std::vector<std::string>>(as_to_vector(value))
                : std::nullopt;
    } else {
        LCM_LOG("Unknown or empty property for Service: " << key << '\n');
    }
}

auto operator<<(std::ostream& ost, const ServProperties& obj) -> std::ostream& {
    ost << "State: " << STATE_MAP.toString(obj.state_) << '\n';
    if (obj.error_ != Error::None) {
        ost << "Error: " << ERROR_MAP.toString(obj.error_) << '\n';
    }

    ost << "Name: " << obj.name_ << '\n';
    ost << "Type: " << TYPE_MAP.toString(obj.type_) << '\n';
    ost << "Strength: " << static_cast<int>(obj.strength_) << '\n';
    ost << "AutoConnect: " << std::boolalpha << obj.autoconnect_ << '\n';
    ost << "mDNS: " << obj.mdns_ << '\n';
    ost << "Favorite: " << obj.favorite_ << '\n';
    ost << "Immutable: " << obj.immutable_ << '\n';
    ost << "Roaming: " << obj.roaming_ << '\n';

    if (obj.security_) {
        ost << "Security: ";
        for (const auto& sec : obj.security_.value()) {
            ost << SECURITY_MAP.toString(sec) << ' ';
        }
        ost << '\n';
    }

    if (obj.name_servers_) {
        ost << "Nameservers: ";
        for (const auto& nserver : obj.name_servers_.value()) {
            ost << nserver << ' ';
        }
        ost << '\n';
    }

    if (obj.name_servers_conf_) {
        ost << "Nameservers.Configuration: ";
        for (const auto& nserver : obj.name_servers_conf_.value()) {
            ost << nserver << ' ';
        }
        ost << '\n';
    }

    if (obj.domains_) {
        ost << "Domains: ";
        for (const auto& domain : obj.domains_.value()) {
            ost << domain << ' ';
        }
        ost << '\n';
    }

    if (obj.time_servers_) {
        ost << "TimeServers: ";
        for (const auto& tserver : obj.time_servers_.value()) {
            ost << tserver << ' ';
        }
        ost << '\n';
    }

    if (obj.ipv4_) {
        ost << obj.ipv4_.value();
    }

    if (obj.ipv6_) {
        ost << obj.ipv6_.value();
    }

    if (obj.ethernet_) {
        ost << obj.ethernet_.value();
    }

    if (obj.provider_) {
        ost << obj.provider_.value();
    }

    if (obj.proxy_) {
        ost << obj.proxy_.value();
    }

    return ost;
}

auto operator<<(std::ostream& ost, const Ethernet& obj) -> std::ostream& {
    ost << "Ethernet:\n";
    ost << "  Method: " << ETHERNET_METHOD_MAP.toString(obj.method_) << '\n';
    ost << "  Interface: " << obj.interface_ << '\n';
    ost << "  Address: " << obj.address_ << '\n';
    ost << "  MTU: " << obj.mtu_ << '\n';
    return ost;
}

auto operator<<(std::ostream& ost, const IPv4& obj) -> std::ostream& {
    ost << "IPv4:\n";
    ost << "  Method: " << IPV4_METHOD_MAP.toString(obj.method_) << '\n';
    ost << "  Address: " << obj.address_ << '\n';
    ost << "  Netmask: " << obj.netmask_ << '\n';
    ost << "  Gateway: " << obj.gateway_ << '\n';
    return ost;
}

auto operator<<(std::ostream& ost, const Provider& obj) -> std::ostream& {
    ost << "Provider:\n";
    ost << "  Host: " << obj.host_ << '\n';
    ost << "  Domain: " << obj.domain_ << '\n';
    ost << "  Name: " << obj.name_ << '\n';
    ost << "  Type: " << obj.type_ << '\n';
    return ost;
}

auto operator<<(std::ostream& ost, const IPv6& obj) -> std::ostream& {
    ost << "IPv6:\n";
    ost << "  Method: " << IPV6_METHOD_MAP.toString(obj.method_) << '\n';
    ost << "  Address: " << obj.address_ << '\n';
    ost << "  Gateway: " << obj.gateway_ << '\n';
    ost << "  Privacy: " << static_cast<int>(obj.privacy_) << '\n';
    ost << "  Prefix Length: " << static_cast<int>(obj.prefix_length_) << '\n';
    return ost;
}

auto operator<<(std::ostream& ost, const Proxy& obj) -> std::ostream& {
    ost << "Proxy:\n";
    ost << "  Method: " << PROXY_METHOD_MAP.toString(obj.method_) << '\n';
    ost << "  URL: " << obj.url_ << '\n';
    ost << "  Servers: ";
    for (const auto& server : obj.servers_) {
        ost << server << ' ';
    }
    ost << '\n';

    ost << "  Excludes: ";
    for (const auto& exclude : obj.excludes_) {
        ost << exclude << ' ';
    }
    ost << '\n';
    return ost;
}
}  // namespace Amarula::DBus::G::Connman
