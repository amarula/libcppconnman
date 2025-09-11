
#include <glib.h>

#include <amarula/dbus/connman/gservice.hpp>
#include <amarula/dbus/gdbus.hpp>
#include <amarula/dbus/gproxy.hpp>
#include <cstdint>
#include <iostream>
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

static constexpr EnumStringMap<IPv6::Privacy, 3> IPV6_PRIVACY_MAP{
    {{{IPv6::Privacy::Disabled, "disabled"},
      {IPv6::Privacy::Enabled, "enabled"},
      {IPv6::Privacy::Preferred, "preferred"}}}};

Service::Service(DBus* dbus, const gchar* obj_path)
    : DBusProxy(dbus, SERVICE, obj_path, SERVICE_INTERFACE) {}

void Service::connect(PropertiesSetCallback callback) {
    auto data = prepareCallback(std::move(callback));
    call_method(proxy(), nullptr, CONNECT_STR, nullptr,
                &Service::finishAsyncCall, data.release());
}

void Service::disconnect(PropertiesSetCallback callback) {
    auto data = prepareCallback(std::move(callback));
    call_method(proxy(), nullptr, DISCONNECT_STR, nullptr,
                &Service::finishAsyncCall, data.release());
}

void Service::remove(PropertiesSetCallback callback) {
    auto data = prepareCallback(std::move(callback));
    call_method(proxy(), nullptr, REMOVE_STR, nullptr,
                &Service::finishAsyncCall, data.release());
}

void Service::setAutoconnect(const bool autoconnect,
                             PropertiesSetCallback callback) {
    auto data = prepareCallback(std::move(callback));
    set_property(proxy(), AUTOCONNECT_STR,
                 g_variant_new_boolean(static_cast<gboolean>(autoconnect)),
                 nullptr, &Service::finishAsyncCall, data.release());
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
        std::cerr << "Unknown property for IPv4: " << key << '\n';
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
        std::cerr << "Unknown property for IPv6: " << key << '\n';
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
        std::cerr << "Unknown property for Ethernet: " << key << '\n';
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
        std::cerr << "Unknown property for Provider: " << key << '\n';
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
        std::cerr << "Unknown property for Proxy: " << key << '\n';
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
        std::cerr << "Unknown or empty property for Service: " << key << '\n';
    }
}

void ServProperties::print() const {
    std::cout << "@@@@@@@@@@ ServProperties: @@@@@@@@@@@@@@@\n";
    std::cout << "State: " << STATE_MAP.toString(state_) << '\n';
    if (error_ != Error::None) {
        std::cout << "Error: " << ERROR_MAP.toString(error_) << '\n';
    }

    std::cout << "Name: " << name_ << '\n';
    std::cout << "Type: " << TYPE_MAP.toString(type_) << '\n';
    std::cout << "Strength: " << static_cast<int>(strength_) << '\n';
    std::cout << "AutoConnect: " << std::boolalpha << autoconnect_ << '\n';
    std::cout << "mDNS: " << mdns_ << '\n';
    std::cout << "Favorite: " << favorite_ << '\n';
    std::cout << "Immutable: " << immutable_ << '\n';
    std::cout << "Roaming: " << roaming_ << '\n';

    if (security_) {
        std::cout << "Security: ";
        for (const auto& sec : security_.value()) {
            std::cout << SECURITY_MAP.toString(sec) << ' ';
        }
        std::cout << '\n';
    }

    if (name_servers_) {
        std::cout << "Nameservers: ";
        for (const auto& nserver : name_servers_.value()) {
            std::cout << nserver << ' ';
        }
        std::cout << '\n';
    }

    if (domains_) {
        std::cout << "Domains: ";
        for (const auto& domain : domains_.value()) {
            std::cout << domain << ' ';
        }
        std::cout << '\n';
    }

    if (time_servers_) {
        std::cout << "TimeServers: ";
        for (const auto& tserver : time_servers_.value()) {
            std::cout << tserver << ' ';
        }
        std::cout << '\n';
    }

    if (ipv4_) {
        ipv4_.value().print();
    }

    if (ipv6_) {
        ipv6_.value().print();
    }

    ethernet_.value().print();

    if (provider_) {
        provider_.value().print();
    }

    if (proxy_) {
        proxy_.value().print();
    }

    std::cout << "@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@\n";
}

void Ethernet::print() const {
    std::cout << "Ethernet:\n";
    std::cout << "  Method: " << ETHERNET_METHOD_MAP.toString(method_) << '\n';
    std::cout << "  Interface: " << interface_ << '\n';
    std::cout << "  Address: " << address_ << '\n';
    std::cout << "  MTU: " << mtu_ << '\n';
}

void IPv4::print() const {
    std::cout << "IPv4:\n";
    std::cout << "  Method: " << IPV4_METHOD_MAP.toString(method_) << '\n';
    std::cout << "  Address: " << address_ << '\n';
    std::cout << "  Netmask: " << netmask_ << '\n';
    std::cout << "  Gateway: " << gateway_ << '\n';
}

void Provider::print() const {
    std::cout << "Provider:\n";
    std::cout << "  Host: " << host_ << '\n';
    std::cout << "  Domain: " << domain_ << '\n';
    std::cout << "  Name: " << name_ << '\n';
    std::cout << "  Type: " << type_ << '\n';
}

void IPv6::print() const {
    std::cout << "IPv6:\n";
    std::cout << "  Method: " << IPV6_METHOD_MAP.toString(method_) << '\n';
    std::cout << "  Address: " << address_ << '\n';
    std::cout << "  Gateway: " << gateway_ << '\n';
    std::cout << "  Privacy: " << static_cast<int>(privacy_) << '\n';
    std::cout << "  Prefix Length: " << static_cast<int>(prefix_length_)
              << '\n';
}

void Proxy::print() const {
    std::cout << "Proxy:\n";
    std::cout << "  Method: " << PROXY_METHOD_MAP.toString(method_) << '\n';
    std::cout << "  URL: " << url_ << '\n';
    std::cout << "  Servers: ";
    for (const auto& server : servers_) {
        std::cout << server << ' ';
    }
    std::cout << '\n';

    std::cout << "  Excludes: ";
    for (const auto& exclude : excludes_) {
        std::cout << exclude << ' ';
    }
    std::cout << '\n';
}
}  // namespace Amarula::DBus::G::Connman
