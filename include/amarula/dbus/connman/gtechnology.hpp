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
    [[nodiscard]] auto getTetheringIdentifier() const {
        return tethering_identifier_;
    }
    [[nodiscard]] auto getTetheringPassphrase() const {
        return tethering_passphrase_;
    }
    friend auto operator<<(std::ostream& ostr,
                           const TechProperties& object) -> std::ostream&;

   private:
    bool powered_ = false;
    bool connected_ = false;
    bool tethering_ = false;
    std::string name_;
    std::string tethering_identifier_;
    std::string tethering_passphrase_;
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
    void setTethering(bool tethering, PropertiesSetCallback callback = nullptr);
    void setTetheringIdentifier(const std::string& identifier,
                                PropertiesSetCallback callback = nullptr);
    void setTetheringPassphrase(const std::string& passphrase,
                                PropertiesSetCallback callback = nullptr);
    void setTetheringFreq(int frequency,
                          PropertiesSetCallback callback = nullptr);
    void scan(PropertiesSetCallback callback = nullptr);

    friend class Manager;
};

}  // namespace Amarula::DBus::G::Connman
