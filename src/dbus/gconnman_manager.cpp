
#include <gio/gio.h>
#include <glib-object.h>
#include <glib.h>
#include <glibconfig.h>

#include <algorithm>
#include <amarula/dbus/connman/gagent.hpp>
#include <amarula/dbus/connman/gmanager.hpp>
#include <amarula/dbus/connman/gservice.hpp>
#include <amarula/dbus/connman/gtechnology.hpp>
#include <amarula/dbus/gdbus.hpp>
#include <amarula/dbus/gproxy.hpp>
#include <amarula/log.hpp>
#include <memory>
#include <mutex>
#include <ranges>
#include <string>
#include <utility>
#include <vector>

#include "gconnman_private.hpp"
#include "gdbus_private.hpp"

namespace Amarula::DBus::G::Connman {

using State = ManaProperties::State;
static constexpr EnumStringMap<State, 4> STATE_MAP{
    {{{State::Offline, "offline"},
      {State::Idle, "idle"},
      {State::Ready, "ready"},
      {State::Online, "online"}}}};

void ManaProperties::update(const gchar* key, GVariant* value) {
    if (g_strcmp0(key, OFFLINEMODE_STR) == 0) {
        offline_mode_ = (g_variant_get_boolean(value) == 1U);
    } else if (g_strcmp0(key, STATE_STR) == 0U) {
        state_ = STATE_MAP.fromString(g_variant_get_string(value, nullptr));
    } else {
        LCM_LOG("Unknown property for Manager: " << key << '\n');
    }
}

Manager::Manager(DBus* dbus, const std::string& agent_path)
    : DBusProxy(dbus, SERVICE, MANAGER_PATH, MANAGER_INTERFACE),
      agent_{
          std::unique_ptr<Agent>(new Agent(dbus->connection(), agent_path))} {
    setup_agent();
    get_technologies();
    get_services();

    g_signal_connect(proxy(), "g-signal::TechnologyRemoved",
                     G_CALLBACK(&Manager::on_technology_added_removed_cb),
                     this);
    g_signal_connect(proxy(), "g-signal::TechnologyAdded",
                     G_CALLBACK(&Manager::on_technology_added_removed_cb),
                     this);
    g_signal_connect(proxy(), "g-signal::ServicesChanged",
                     G_CALLBACK(&Manager::on_services_changed_cb), this);
}

void Manager::process_services_changed(
    const std::vector<std::string>& services_removed,
    const std::vector<std::pair<std::string, VariantPtr>>& services_changed) {
    for (const auto& object_path : services_removed) {
        services_.erase(std::remove_if(services_.begin(), services_.end(),
                                       [&object_path](const auto service) {
                                           return service->objPath() ==
                                                  object_path;
                                       }),
                        services_.end());
    }
    Manager::ProxyList<Service> new_order_of_services;
    for (const auto& [path, prop] : services_changed) {
        auto service_it = std::find_if(
            services_.begin(), services_.end(),
            [&path](const auto service) { return service->objPath() == path; });
        if (service_it != services_.end()) {
            new_order_of_services.push_back(*service_it);
            (*service_it)->updateProperties(prop.get());
        } else {
            auto proxy =
                std::shared_ptr<Service>(new Service(dbus(), path.c_str()));
            proxy->updateProperties(prop.get());
            new_order_of_services.push_back(proxy);
        }
    }
    services_ = new_order_of_services;
    services_changed_cb_(services_);
}

void Manager::setup_agent() {
    agent_->set_request_input_handler([this](const gchar* service_path,
                                             GVariant* fields) -> GVariant* {
        std::lock_guard<std::mutex> const lock(mtx_);
        GVariantBuilder builder;
        g_variant_builder_init(&builder, G_VARIANT_TYPE("a{sv}"));
        auto service_it = std::ranges::find_if(
            services_, [&service_path](const auto& service) {
                return service->objPath() == service_path;
            });

        if (service_it != services_.end()) {
            auto* parsed_fields = parse_fields(fields);
            auto input_requested = classify_input(parsed_fields);
            switch (input_requested) {
                case InputType::InputWpA2Passphrase:
                    if (request_input_passphrase_cb_ != nullptr) {
                        const auto passphrase =
                            request_input_passphrase_cb_(*service_it);
                        g_variant_builder_add(
                            &builder, "{sv}", "Passphrase",
                            g_variant_new_string(passphrase.second.c_str()));
                    }
                    break;
                case InputType::InputWpA2PassphraseWpsAlternative:
                    if (request_input_passphrase_cb_ != nullptr) {
                        const auto passphrase =
                            request_input_passphrase_cb_(*service_it);

                        g_variant_builder_add(
                            &builder, "{sv}",
                            passphrase.first ? "Passphrase" : "WPS",
                            g_variant_new_string(passphrase.second.c_str()));
                    }
                    break;
                case InputType::InputHiddenNetworkName:
                    if (request_input_hidden_network_name_cb_ != nullptr) {
                        const auto name =
                            request_input_hidden_network_name_cb_(*service_it);

                        GVariant* value = nullptr;
                        if (name.first) {
                            value = g_variant_new_string(name.second.c_str());
                        } else {
                            value = g_variant_new_fixed_array(
                                G_VARIANT_TYPE_BYTE, name.second.data(),
                                name.second.size(), sizeof(guchar));
                        }
                        g_variant_builder_add(&builder, "{sv}",
                                              name.first ? "Name" : "SSID",
                                              value);
                    }

                    break;
                case InputType::InputChallengeResponse:
                case InputType::InputWpaEnterprise:
                    if (request_input_wpa_enterprise_cb_ != nullptr) {
                        const auto identity_password =
                            request_input_wpa_enterprise_cb_(*service_it);

                        g_variant_builder_add(
                            &builder, "{sv}", "Identity",
                            g_variant_new_string(
                                identity_password.first.c_str()));

                        GVariant* value_string = nullptr;
                        if (identity_password.second.first) {
                            value_string = g_variant_new_string(
                                identity_password.second.second.c_str());
                        } else {
                            value_string = g_variant_new_fixed_array(
                                G_VARIANT_TYPE_BYTE,
                                identity_password.second.second.data(),
                                identity_password.second.second.size(),
                                sizeof(guchar));
                        }
                        g_variant_builder_add(&builder, "{sv}",
                                              identity_password.second.first
                                                  ? "Passphrase"
                                                  : "WPS",
                                              value_string);
                    }

                    break;
                case InputType::InputWispr:
                    if (request_input_wispr_enabled_cb_ != nullptr) {
                        const auto user_password =
                            request_input_wispr_enabled_cb_(*service_it);

                        g_variant_builder_add(
                            &builder, "{sv}", "Username",
                            g_variant_new_string(user_password.first.c_str()));

                        g_variant_builder_add(
                            &builder, "{sv}", "Password",
                            g_variant_new_string(user_password.second.c_str()));
                    }

                    break;
                default:
                    break;
            }
        }
        return g_variant_builder_end(&builder);
    });
}

void Manager::setOfflineMode(bool offline_mode,
                             PropertiesSetCallback callback) {
    auto data = prepareCallback(std::move(callback));
    setProperty(proxy(), OFFLINEMODE_STR,
                g_variant_new_boolean(static_cast<gboolean>(offline_mode)),
                nullptr, &Manager::finishAsyncCall, data.release());
}

auto Manager::dict_to_path_prop(GVariant* tuple)
    -> std::pair<std::string, VariantPtr> {
    const gchar* object_path = nullptr;

    GVariant* object_path_variant = g_variant_get_child_value(tuple, 0);
    object_path = g_variant_get_string(object_path_variant, nullptr);

    VariantPtr properties_dict{g_variant_get_child_value(tuple, 1),
                               &g_variant_unref};
    g_variant_unref(object_path_variant);
    return {std::string(object_path), std::move(properties_dict)};
}
template <class ProxyType>
auto Manager::dict_to_proxy(GVariant* tuple) -> std::shared_ptr<ProxyType> {
    const auto [path, properties] = dict_to_path_prop(tuple);
    auto proxy =
        std::shared_ptr<ProxyType>(new ProxyType(dbus(), path.c_str()));
    proxy->updateProperties(properties.get());

    return proxy;
}

template <class ProxyType>
auto Manager::arrays_to_proxies(GVariant* array_od) -> ProxyList<ProxyType> {
    ProxyList<ProxyType> proxies;

    if (g_variant_is_of_type(array_od, G_VARIANT_TYPE("a(oa{sv})")) != 0U) {
        GVariantIter iter;
        g_variant_iter_init(&iter, array_od);
        GVariant* item = nullptr;
        while ((item = g_variant_iter_next_value(&iter)) != nullptr) {
            const auto proxy = dict_to_proxy<ProxyType>(item);
            proxies.push_back(proxy);
            g_variant_unref(item);
        }
    }
    return proxies;
}

template <class ProxyType>
void Manager::get_proxies_cb(GObject* proxy, GAsyncResult* res,
                             gpointer user_data) {
    GError* error = nullptr;
    GVariant* out_properties = nullptr;
    auto* self = static_cast<Manager*>(user_data);

    const auto success =
        finish(G_DBUS_PROXY(proxy), res, &error, &out_properties);
    ProxyList<ProxyType> proxies;
    if (success) {
        proxies = self->template arrays_to_proxies<ProxyType>(out_properties);
        g_variant_unref(out_properties);
        std::lock_guard<std::mutex> const lock(self->mtx_);
        if constexpr (std::is_same_v<ProxyType, Service>) {
            self->services_ = proxies;
            if (!self->services_.empty()) {
                self->services_changed_cb_(self->services_);
            }
        } else {
            self->technologies_ = proxies;
            if (!self->technologies_.empty()) {
                self->technologies_changed_cb_(self->technologies_);
            }
        }

    } else {
        LCM_LOG(error->message << '\n');
        g_error_free(error);
    }
}

void Manager::get_technologies() {
    callMethod(proxy(), nullptr, GETTECHNOLOGIES_STR, nullptr,
               &Manager::get_proxies_cb<Technology>, this);
}

void Manager::get_services() {
    callMethod(proxy(), nullptr, GETSERVICES_STR, nullptr,
               &Manager::get_proxies_cb<Service>, this);
}

void Manager::registerAgent(const std::string& object_path,
                            PropertiesSetCallback callback) {
    auto data = prepareCallback(std::move(callback));
    GVariant* child = g_variant_new_object_path(object_path.c_str());
    GVariant* parameters = g_variant_new_tuple(&child, 1);
    callMethod(proxy(), nullptr, REGISTERAGENT_STR, parameters,
               &Manager::finishAsyncCall, data.release());
}

void Manager::unregisterAgent(const std::string& object_path,
                              PropertiesSetCallback callback) {
    auto data = prepareCallback(std::move(callback));
    GVariant* child = g_variant_new_object_path(object_path.c_str());
    GVariant* parameters = g_variant_new_tuple(&child, 1);
    callMethod(proxy(), nullptr, UNREGISTERAGENT_STR, parameters,
               &Manager::finishAsyncCall, data.release());
}

void Manager::on_technology_added_removed_cb(GDBusProxy* /*proxy*/,
                                             gchar* /*sender_name*/,
                                             gchar* signal_name,
                                             GVariant* parameters,
                                             gpointer user_data) {
    auto* self = static_cast<Manager*>(user_data);
    std::lock_guard<std::mutex> const lock(self->mtx_);
    if (g_strcmp0(signal_name, "g-signal::TechnologyAdded") == 0U) {
        const auto technology =
            self->template dict_to_proxy<Technology>(parameters);
        self->technologies_.push_back(technology);

    } else if (g_strcmp0(signal_name, "g-signal::TechnologyRemoved") == 0U) {
        const auto object_path =
            std::string(g_variant_get_string(parameters, nullptr));

        self->technologies_.erase(
            std::remove_if(self->technologies_.begin(),
                           self->technologies_.end(),
                           [&object_path](const auto& technology) {
                               return technology->objPath() == object_path;
                           }),
            self->technologies_.end());
    }
    self->technologies_changed_cb_(self->technologies_);
}

void Manager::on_services_changed_cb(GDBusProxy* /*proxy*/,
                                     gchar* /*sender_name*/,
                                     gchar* /*signal_name*/,
                                     GVariant* parameters, gpointer user_data) {
    auto* self = static_cast<Manager*>(user_data);

    GVariant* changed = nullptr;
    GVariant* removed = nullptr;

    changed = g_variant_get_child_value(parameters, 0);
    removed = g_variant_get_child_value(parameters, 1);

    std::vector<std::pair<std::string, VariantPtr>> services_changed;
    std::vector<std::string> services_removed;

    GVariantIter iter;
    g_variant_iter_init(&iter, changed);
    GVariant* item = nullptr;

    while ((item = g_variant_iter_next_value(&iter)) != nullptr) {
        services_changed.push_back(dict_to_path_prop(item));
        g_variant_unref(item);
    }
    g_variant_unref(changed);

    GVariantIter iter_removed;
    g_variant_iter_init(&iter_removed, removed);
    GVariant* removed_item = nullptr;
    while ((removed_item = g_variant_iter_next_value(&iter_removed)) !=
           nullptr) {
        const gchar* removed_path = g_variant_get_string(removed_item, nullptr);
        services_removed.emplace_back(removed_path);
        g_variant_unref(removed_item);
    }
    g_variant_unref(removed);

    std::lock_guard<std::mutex> const lock(self->mtx_);
    self->process_services_changed(services_removed, services_changed);
}

void Manager::onTechnologiesChanged(OnTechListChangedCallback callback) {
    if (callback != nullptr) {
        std::lock_guard<std::mutex> const lock(mtx_);
        technologies_changed_cb_ = std::move(callback);
    }
}

void Manager::onServicesChanged(OnServListChangedCallback callback) {
    if (callback != nullptr) {
        std::lock_guard<std::mutex> const lock(mtx_);
        services_changed_cb_ = std::move(callback);
    }
}

using FieldDescription = struct {
    gchar* field_name;
    gchar* type;
    gchar* requirement;
};

auto Manager::parse_fields(GVariant* fields) -> GPtrArray* {
    GPtrArray* array = g_ptr_array_new_with_free_func(g_free);

    GVariantIter iter;
    const gchar* field_name = nullptr;
    GVariant* inner_dict = nullptr;

    g_variant_iter_init(&iter, fields);

    while (g_variant_iter_next(&iter, "{&sv}", &field_name, &inner_dict) != 0) {
        FieldDescription* desc = g_new0(FieldDescription, 1);
        desc->field_name = g_strdup(field_name);

        GVariantIter inner_iter;
        const gchar* prop_name = nullptr;
        GVariant* prop_value_variant = nullptr;

        g_variant_iter_init(&inner_iter, inner_dict);
        while (g_variant_iter_next(&inner_iter, "{&s@v}", &prop_name,
                                   &prop_value_variant) != 0) {
            auto* prop_value = g_variant_get_variant(prop_value_variant);
            if (g_strcmp0(prop_name, "Type") == 0) {
                desc->type = g_variant_dup_string(prop_value, nullptr);
            } else if (g_strcmp0(prop_name, "Requirement") == 0) {
                desc->requirement = g_variant_dup_string(prop_value, nullptr);
            }
            g_variant_unref(prop_value);
            g_variant_unref(prop_value_variant);
        }
        g_variant_unref(inner_dict);

        g_ptr_array_add(array, desc);
    }
    return array;
}

auto Manager::classify_input(GPtrArray* fields) -> InputType {
    bool has_passphrase{false};
    bool has_wps{false};
    bool has_name{false};
    bool has_identity{false};
    bool has_username{false};
    bool has_password{false};
    bool has_challenge{false};

    for (guint i = 0; i < fields->len; ++i) {
        auto* desc =
            static_cast<FieldDescription*>(g_ptr_array_index(fields, i));
        if (g_strcmp0(desc->field_name, "Passphrase") == 0 &&
            g_strcmp0(desc->requirement, "mandatory") == 0) {
            has_passphrase = TRUE;
        }
        if (g_strcmp0(desc->field_name, "WPS") == 0) {
            has_wps = TRUE;
        }
        if (g_strcmp0(desc->field_name, "Name") == 0) {
            has_name = TRUE;
        }
        if (g_strcmp0(desc->field_name, "Identity") == 0) {
            has_identity = TRUE;
        }
        if (g_strcmp0(desc->field_name, "Username") == 0) {
            has_username = TRUE;
        }
        if (g_strcmp0(desc->field_name, "Password") == 0) {
            has_password = TRUE;
        }
        if (g_strcmp0(desc->field_name, "Passphrase") == 0 &&
            g_strcmp0(desc->type, "response") == 0) {
            has_challenge = TRUE;
        }
    }

    if (has_passphrase && !has_wps && !has_identity) {
        return InputType::InputWpA2Passphrase;
    }
    if (has_passphrase && has_wps) {
        return InputType::InputWpA2PassphraseWpsAlternative;
    }
    if (has_name) {
        return InputType::InputHiddenNetworkName;
    }
    if (has_identity && has_challenge) {
        return InputType::InputChallengeResponse;
    }
    if (has_identity && has_passphrase) {
        return InputType::InputWpaEnterprise;
    }
    if (has_username && has_password) {
        return InputType::InputWispr;
    }

    return InputType::InputUnknown;
}

}  // namespace Amarula::DBus::G::Connman
