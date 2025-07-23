#pragma once
#include <gio/gio.h>
#include <glib-object.h>
#include <glib.h>

#include <amarula/dbus/connman/gagent.hpp>
#include <amarula/dbus/connman/gservice.hpp>
#include <amarula/dbus/connman/gtechnology.hpp>
#include <amarula/dbus/gdbus.hpp>
#include <amarula/dbus/gproxy.hpp>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

namespace Amarula::DBus::G::Connman {
class Connman;

struct ManaProperties {
   public:
    enum class State : uint8_t { Offline = 0, Idle, Ready, Online };
    [[nodiscard]] auto isOfflineMode() const { return offline_mode_; }
    [[nodiscard]] auto getState() const { return state_; }

   private:
    bool offline_mode_{false};
    State state_{};

    void update(const gchar* key, GVariant* value);

    friend class Manager;
    friend class DBusProxy<ManaProperties>;
};

class Manager : public DBusProxy<ManaProperties> {
   public:
    template <class T>
    using ProxyList = std::vector<std::shared_ptr<T>>;

    template <class T>
    using OnProxyListChangedCallback =
        std::function<void(const ProxyList<T>& proxies)>;
    using OnTechListChangedCallback = OnProxyListChangedCallback<Technology>;
    using OnServListChangedCallback = OnProxyListChangedCallback<Service>;

    /*
     * Should follow
     * https://git.kernel.org/pub/scm/network/connman/connman.git/tree/doc/agent-api.txt
     * examples
     */

    /*
     * One can pass passphrase or WPS alternative, the bool indicates
     * whether the passphrase is provided.
     */
    using OnRequestInputPassphraseCallback =
        std::function<std::pair<bool, std::string>(std::shared_ptr<Service>)>;
    /*
     * One can pass Name or SSID alternative, the bool indicates
     * whether the Name is provided.
     */
    using OnRequestInputHiddenNetworkNameCallback =
        OnRequestInputPassphraseCallback;
    /*
     * One can pass passphrase or WPS alternative, the bool indicates
     * whether the passphrase is provided.
     */
    using OnRequestInputWPAEnterpriseCallback =
        std::function<std::pair<std::string, std::pair<bool, std::string>>(
            std::shared_ptr<Service>)>;

    using OnRequestInputWISPrEnabledCallback =
        std::function<std::pair<std::string, std::string>(
            std::shared_ptr<Service>)>;
    using OnTechnologiesChangedCallback =
        std::function<void(const Manager::ProxyList<Technology>&)>;
    using OnServicesChangedCallback =
        std::function<void(const Manager::ProxyList<Service>&)>;

    [[nodiscard]] auto services() {
        std::lock_guard<std::mutex> const lock(mtx_);
        return services_;
    }

    [[nodiscard]] auto technologies() {
        std::lock_guard<std::mutex> const lock(mtx_);
        return technologies_;
    }

    void onRequestInputPassphrase(OnRequestInputPassphraseCallback callback) {
        std::lock_guard<std::mutex> const lock(mtx_);
        request_input_passphrase_cb_ = std::move(callback);
    }

    void onRequestInputHiddenNetworkName(
        OnRequestInputHiddenNetworkNameCallback callback) {
        std::lock_guard<std::mutex> const lock(mtx_);
        request_input_hidden_network_name_cb_ = std::move(callback);
    }

    void onRequestInputInputWPAEnterprise(
        OnRequestInputWPAEnterpriseCallback callback) {
        std::lock_guard<std::mutex> const lock(mtx_);
        request_input_wpa_enterprise_cb_ = std::move(callback);
    }

    void onRequestInputInputWISPrEnabled(
        OnRequestInputWISPrEnabledCallback callback) {
        std::lock_guard<std::mutex> const lock(mtx_);
        request_input_wispr_enabled_cb_ = std::move(callback);
    }

    void registerAgent(const std::string& object_path,
                       PropertiesSetCallback callback = nullptr);
    void unregisterAgent(const std::string& object_path,
                         PropertiesSetCallback callback = nullptr);
    void setOfflineMode(bool offline_mode,
                        PropertiesSetCallback callback = nullptr);

    void onTechnologiesChanged(OnTechListChangedCallback callback);
    void onServicesChanged(OnServListChangedCallback callback);

    auto internalAgentPath() const -> std::string { return agent_->path_; };

   private:
    enum class InputType : std::uint8_t {
        InputUnknown = 0,
        InputWpA2Passphrase,
        InputWpA2PassphraseWpsAlternative,
        InputHiddenNetworkName,
        InputWpaEnterprise,
        InputWispr,
        InputChallengeResponse,
    };

    ProxyList<Service> services_;
    ProxyList<Technology> technologies_;

    std::mutex mtx_;
    std::unique_ptr<Agent> agent_{nullptr};

    using VariantPtr = std::unique_ptr<GVariant, decltype(&g_variant_unref)>;

    OnRequestInputPassphraseCallback request_input_passphrase_cb_;
    OnRequestInputHiddenNetworkNameCallback
        request_input_hidden_network_name_cb_;
    OnRequestInputWPAEnterpriseCallback request_input_wpa_enterprise_cb_;
    OnRequestInputWISPrEnabledCallback request_input_wispr_enabled_cb_;
    OnTechnologiesChangedCallback technologies_changed_cb_{
        [](const Manager::ProxyList<Technology>&) {}};
    OnServicesChangedCallback services_changed_cb_{
        [](const Manager::ProxyList<Service>&) {}};

    explicit Manager(DBus* dbus, const std::string& agent_path = std::string());

    using DBusProxy::DBusProxy;

    template <class ProxyType>
    static void get_proxies_cb(GObject* proxy, GAsyncResult* res,
                               gpointer user_data);
    static void on_technology_added_removed_cb(GDBusProxy* proxy,
                                               gchar* sender_name,
                                               gchar* signal_name,
                                               GVariant* parameters,
                                               gpointer user_data);
    static void on_services_changed_cb(GDBusProxy* proxy, gchar* sender_name,
                                       gchar* signal_name, GVariant* parameters,
                                       gpointer user_data);
    static auto classify_input(GPtrArray* fields) -> InputType;
    static auto parse_fields(GVariant* fields) -> GPtrArray*;
    static auto dict_to_path_prop(GVariant* tuple)
        -> std::pair<std::string, VariantPtr>;

    template <class ProxyType>
    auto arrays_to_proxies(GVariant* array_od) -> ProxyList<ProxyType>;
    template <class ProxyType>
    auto dict_to_proxy(GVariant* tuple) -> std::shared_ptr<ProxyType>;
    void get_technologies();
    void get_services();
    void setup_agent();
    void process_services_changed(
        const std::vector<std::string>& services_removed,
        const std::vector<std::pair<std::string, VariantPtr>>&
            services_changed);

    friend class Connman;
};

}  // namespace Amarula::DBus::G::Connman
