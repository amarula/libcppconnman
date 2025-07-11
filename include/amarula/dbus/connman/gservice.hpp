#pragma once
#include <glib.h>

#include <amarula/dbus/gdbus.hpp>
#include <amarula/dbus/gproxy.hpp>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace Amarula::DBus::G::Connman {
class Manager;
struct ServProperties;

class GVariantParser {
   public:
    GVariantParser() = default;
    virtual ~GVariantParser() = default;

    GVariantParser(const GVariantParser&) = default;
    auto operator=(const GVariantParser&) -> GVariantParser& = default;
    GVariantParser(GVariantParser&&) = default;
    auto operator=(GVariantParser&&) -> GVariantParser& = default;

   protected:
    void parse(GVariant* variant);

    virtual void update(const gchar* /*key*/, GVariant* /*value*/) {};
};

class IPv4 : public GVariantParser {
   public:
    enum class Method : uint8_t {
        Off = 0,
        Dhcp,
        Manual,
        Auto,
    };

    void print() const;

    [[nodiscard]] auto getMethod() const { return method_; }
    [[nodiscard]] auto getAddress() const { return address_; }
    [[nodiscard]] auto getNetmask() const { return netmask_; }
    [[nodiscard]] auto getGateway() const { return gateway_; }

   private:
    Method method_{};
    std::string address_;
    std::string netmask_;
    std::string gateway_;
    explicit IPv4(GVariant* variant) { parse(variant); };
    void update(const gchar* key, GVariant* value) override;

    friend class ServProperties;
};

struct IPv6 : public GVariantParser {
   public:
    enum class Method : uint8_t {
        Off = 0,
        K6to4,
        Manual,
        Fixed,
        Auto,
    };
    enum class Privacy : uint8_t { Disabled = 0, Enabled, Preferred };
    void print() const;
    [[nodiscard]] auto getMethod() const { return method_; }
    [[nodiscard]] auto getAddress() const { return address_; }
    [[nodiscard]] auto getGateway() const { return gateway_; }
    [[nodiscard]] auto getPrivacy() const { return privacy_; }
    [[nodiscard]] auto getPrefixLength() const { return prefix_length_; }

   private:
    Method method_{};
    std::string address_;
    std::string gateway_;
    Privacy privacy_{};
    uint8_t prefix_length_{0U};
    explicit IPv6(GVariant* variant) { parse(variant); };
    void update(const gchar* key, GVariant* value) override;

    friend class ServProperties;
};

struct Ethernet : public GVariantParser {
   public:
    enum class Method : uint8_t { Manual = 0, Auto };
    void print() const;
    [[nodiscard]] auto getMethod() const { return method_; }
    [[nodiscard]] auto getInterface() const { return interface_; }
    [[nodiscard]] auto getAddress() const { return address_; }
    [[nodiscard]] auto getMtu() const { return mtu_; }

   private:
    Method method_{};
    std::string interface_;
    std::string address_;
    uint16_t mtu_{0U};
    explicit Ethernet(GVariant* variant) { parse(variant); };
    void update(const gchar* key, GVariant* value) override;

    friend class ServProperties;
};

struct Provider : public GVariantParser {
   public:
    void print() const;
    [[nodiscard]] auto getHost() const { return host_; }
    [[nodiscard]] auto getDomain() const { return domain_; }
    [[nodiscard]] auto getName() const { return name_; }
    [[nodiscard]] auto getType() const { return type_; }

   private:
    std::string host_;
    std::string domain_;
    std::string name_;
    std::string type_;
    explicit Provider(GVariant* variant) { parse(variant); };
    void update(const gchar* key, GVariant* value) override;

    friend class ServProperties;
};

struct Proxy : public GVariantParser {
   public:
    enum class Method : uint8_t { Direct = 0, Auto, Manual };
    void print() const;
    [[nodiscard]] auto getMethod() const { return method_; }
    [[nodiscard]] auto getUrl() const { return url_; }
    [[nodiscard]] auto getServers() const { return servers_; }
    [[nodiscard]] auto getExcludes() const { return excludes_; }

   private:
    Method method_{};
    std::string url_;
    std::vector<std::string> servers_;
    std::vector<std::string> excludes_;
    explicit Proxy(GVariant* variant) { parse(variant); };
    void update(const gchar* key, GVariant* value) override;

    friend class ServProperties;
};

struct ServProperties {
   public:
    enum class State : uint8_t {
        Idle = 0,
        Failure,
        Ready,
        Online,
        Disconnect,
        Association,
        Configuration
    };
    enum class Type : uint8_t {
        Ethernet = 0,
        Wifi,
        Cellular,
        Bluetooth,
        Vpn,
        Wired,
        P2p,
        Gps,
        Gadget
    };
    enum class Security : uint8_t {
        None = 0,
        Wep,
        Psk,
        Ieee8021x,
        Wps,
        WpsAdvertising
    };
    enum class Error : uint8_t {
        None = 0,
        OutOfRange,
        PinMissing,
        DhcpFailed,
        ConnectFailed,
        LoginFailed,
        AuthFailed,
        InvalidKey,
        Blocked,
        OnlineCheckFailed
    };

    void print() const;
    [[nodiscard]] auto getState() const { return state_; }
    [[nodiscard]] auto getType() const { return type_; }
    [[nodiscard]] auto getSecurity() const { return security_; }
    [[nodiscard]] auto getStrength() const { return strength_; }
    [[nodiscard]] auto getError() const { return error_; }
    [[nodiscard]] auto isAutoconnect() const { return autoconnect_; }
    [[nodiscard]] auto isMDNSEnabled() const { return mdns_; }
    [[nodiscard]] auto isFavorite() const { return favorite_; }
    [[nodiscard]] auto isImmutable() const { return immutable_; }
    [[nodiscard]] auto isRoaming() const { return roaming_; }
    [[nodiscard]] auto getIPv4() const { return ipv4_; }
    [[nodiscard]] auto getIPv6() const { return ipv6_; }
    [[nodiscard]] auto getEthernet() const { return ethernet_; }
    [[nodiscard]] auto getProvider() const { return provider_; }
    [[nodiscard]] auto getProxy() const { return proxy_; }
    [[nodiscard]] auto getName() const { return name_; }
    [[nodiscard]] auto getNameservers() const { return name_servers_; }
    [[nodiscard]] auto getDomains() const { return domains_; }
    [[nodiscard]] auto getTimeservers() const { return time_servers_; }

   private:
    State state_{};
    Error error_{};
    std::string name_;
    Type type_{};
    std::optional<std::vector<Security>> security_;
    std::optional<std::vector<std::string>> name_servers_;
    std::optional<std::vector<std::string>> name_servers_conf_;
    std::optional<std::vector<std::string>> domains_;
    std::optional<std::vector<std::string>> time_servers_;
    bool autoconnect_{false};
    bool mdns_{false};
    bool favorite_{false};
    bool immutable_{false};
    bool roaming_{false};
    uint8_t strength_{0U};
    std::optional<IPv4> ipv4_{std::nullopt};
    std::optional<IPv6> ipv6_{std::nullopt};
    std::optional<Ethernet> ethernet_{std::nullopt};
    std::optional<Provider> provider_{std::nullopt};
    std::optional<Proxy> proxy_{std::nullopt};

    void update(const gchar* key, GVariant* value);

    friend class ConnmanService;
    friend class DBusProxy<ServProperties>;
};

class Service : public DBusProxy<ServProperties> {
   private:
    using DBusProxy::DBusProxy;
    Service(DBus* dbus, const gchar* obj_path);

   public:
    using Properties = ServProperties;
    void connect(PropertiesSetCallback callback = nullptr);
    void disconnect(PropertiesSetCallback callback = nullptr);
    void remove(PropertiesSetCallback callback = nullptr);
    void setAutoconnect(bool autoconnect,
                        PropertiesSetCallback callback = nullptr);
    void setNameServers(const std::vector<std::string>& name_servers,
                        PropertiesSetCallback callback = nullptr);
    friend class Manager;
};

}  // namespace Amarula::DBus::G::Connman
