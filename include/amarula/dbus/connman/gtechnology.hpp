#pragma once
#include <glib.h>

#include <amarula/dbus/gdbus.hpp>
#include <amarula/dbus/gproxy.hpp>
#include <cstdint>
#include <string>

namespace Amarula::DBus::G::Connman {
class Manager;

struct TechProperties {
   public:
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

    [[nodiscard]] auto getName() const { return name_; }
    [[nodiscard]] auto getType() const { return type_; }
    [[nodiscard]] auto isPowered() const { return powered_; }
    [[nodiscard]] auto isConnected() const { return connected_; }
    [[nodiscard]] auto isTethering() const { return tethering_; }
    [[nodiscard]] auto getTetheringFreq() const { return tethering_freq_; }
    void print() const;

   private:
    bool powered_ = false;
    bool connected_ = false;
    bool tethering_ = false;
    std::string name_;
    Type type_;
    int tethering_freq_{0};

    void update(const gchar* key, GVariant* value);

    friend class ConnmanTechnology;
    friend class DBusProxy<TechProperties>;
};

class Technology : public DBusProxy<TechProperties> {
   private:
    using DBusProxy::DBusProxy;
    Technology(DBus* dbus, const gchar* obj_path);

   public:
    using Properties = TechProperties;
    void setPowered(bool powered, PropertiesSetCallback callback = nullptr);
    void scan(PropertiesSetCallback callback = nullptr);

    friend class Manager;
};

}  // namespace Amarula::DBus::G::Connman
