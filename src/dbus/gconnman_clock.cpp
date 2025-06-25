// clang-format off
#include "gconnman_definitions.hpp"
// clang-format on
#include <amarula/dbus/gconnman.hpp>
#include <amarula/dbus/gconnman_clock.hpp>

#include "net_connman_proxy_gdbus_generated.h"

namespace Amarula::DBus {

ConnmanClock::~ConnmanClock() {
    if (clock_proxy_ != nullptr) {
        g_object_unref(clock_proxy_);
    }
}

ConnmanClock::ConnmanClock(GProxy* proxy) : gproxy_{proxy} {
    GError* err = nullptr;
    clock_proxy_ = net_connman_clock_proxy_new_sync(
        gproxy_->getConnection(), G_DBUS_PROXY_FLAGS_NONE, GConnman::SERVICE,
        GConnman::MANAGER_PATH, nullptr, &err);
    if (clock_proxy_ == nullptr) {
        throw std::runtime_error("Failed to create Connman Clock proxy: " +
                                 std::string(err->message));
        g_error_free(err);
    }
}

void ConnmanClock::updateProperties(GVariant* properties,
                                    const std::optional<size_t>& counter) {
    if (g_variant_is_of_type(properties, G_VARIANT_TYPE("a{sv}")) == 0U) {
        std::cerr << "Invalid GVariant type\n";
        return;
    }
    GVariantIter* iter = g_variant_iter_new(properties);

    GVariant* prop = nullptr;
    ClockProperties props;

    while ((prop = g_variant_iter_next_value(iter)) != nullptr) {
        const gchar* key =
            g_variant_get_string(g_variant_get_child_value(prop, 0), nullptr);
        GVariant* value = g_variant_get_child_value(prop, 1);
        GVariant* variant = g_variant_get_variant(value);
        if (g_strcmp0(key, GConnman::PROP_TIME_STR) == 0) {
            props.time = g_variant_get_uint64(variant);

        } else if (g_strcmp0(key, GConnman::PROP_TIMEUPDATES_STR) == 0U) {
            props.timeUpdates = g_variant_get_string(variant, nullptr);
        } else if (g_strcmp0(key, GConnman::PROP_TIMEZONE_STR) == 0U) {
            props.timezone = g_variant_get_string(variant, nullptr);
        } else if (g_strcmp0(key, GConnman::PROP_TIMEZONEUPDATES_STR) == 0U) {
            props.timezoneUpdates = g_variant_get_string(variant, nullptr);
        } else if (g_strcmp0(key, GConnman::PROP_TIMESERVERS_STR) == 0U) {
            const gchar* str = nullptr;
            GVariantIter strIter;
            if (g_variant_is_of_type(variant, G_VARIANT_TYPE("as")) != 0U) {
                g_variant_iter_init(&strIter, variant);
                GVariant* server = nullptr;
                while ((server = g_variant_iter_next_value(&strIter)) !=
                       nullptr) {
                    const gchar* server_str =
                        g_variant_get_string(server, nullptr);
                    props.timeservers.emplace_back(server_str);
                    g_variant_unref(server);
                }
            }
        } else if (g_strcmp0(key, GConnman::PROP_TIMESERVERSYNCED_STR) == 0U) {
            props.timeServerSynced = g_variant_get_boolean(variant) == 1U;
        } else {
            std::cerr << "Unknown property: " << key << '\n';
        }
        g_variant_unref(variant);
        g_variant_unref(value);
        g_variant_unref(prop);
    }
    if (counter) {
        std::unique_lock<std::mutex> lock(mtx_);
        const auto callback = std::any_cast<ClockPropertiesCallback>(
            properties_callbacks_[counter.value()]);
        lock.unlock();
        callback(props);
        if (counter.value() != 0U) {
            lock.lock();
            properties_callbacks_.erase(counter.value());
        }
    }

    g_variant_iter_free(iter);
}

void ConnmanClock::set_property_cb(GObject* proxy, GAsyncResult* res,
                                   gpointer user_data) {
    std::unique_ptr<GConnman::CallbackData<ConnmanClock>> data(
        static_cast<GConnman::CallbackData<ConnmanClock>*>(user_data));
    ConnmanClock* self = data->getSelf();
    const auto counter = data->getCounter();

    GError* error = nullptr;
    const bool success = net_connman_clock_call_set_property_finish(
                             NET_CONNMAN_CLOCK(proxy), res, &error) == 1U;
    if (!success) {
        std::cerr << "Failed to set clock property: " << error->message << '\n';
        g_error_free(error);
    }
    if (counter) {
        std::unique_lock<std::mutex> lock(self->mtx_);
        const auto callback = std::any_cast<ClockPropertiesSetCallback>(
            self->properties_callbacks_[counter.value()]);
        lock.unlock();
        callback(success);

        if (counter.value() != 0U) {
            lock.lock();
            self->properties_callbacks_.erase(counter.value());
        }
    }
    std::unique_lock<std::mutex> const lock(self->mtx_);
    self->gproxy_->on_any_async_done();
}

void ConnmanClock::get_property_cb(GObject* proxy, GAsyncResult* res,
                                   gpointer user_data) {
    std::unique_ptr<GConnman::CallbackData<ConnmanClock>> data(
        static_cast<GConnman::CallbackData<ConnmanClock>*>(user_data));
    ConnmanClock* self = data->getSelf();
    const auto counter = data->getCounter();
    GError* error = nullptr;
    GVariant* out_properties = nullptr;
    if (net_connman_clock_call_get_properties_finish(
            NET_CONNMAN_CLOCK(proxy), &out_properties, res, &error) == 0U) {
        std::cerr << "Failed to get clock properties: " << error->message
                  << '\n';
        g_error_free(error);
    } else {
        self->updateProperties(out_properties, counter);
        g_variant_unref(out_properties);
    }
    std::unique_lock<std::mutex> const lock(self->mtx_);
    self->gproxy_->on_any_async_done();
}

void ConnmanClock::on_properties_changed_cb(
    GDBusProxy* /*proxy*/, GVariant* changed_properties,
    const gchar* const* /*invalidated_properties*/, gpointer user_data) {
    auto* self = static_cast<ConnmanClock*>(user_data);
    // The ClockProperties passed to the callback will only have the changed
    // properties correctly set. The rest will be default initialized. This have
    // to be rethinked if we want to use this callback
    self->updateProperties(changed_properties, 0U);
}

void ConnmanClock::setTime(unsigned int time,
                           ClockPropertiesSetCallback callback) {
    const auto call_counter = prepareCallback(std::move(callback));
    auto data = std::make_unique<GConnman::CallbackData<ConnmanClock>>(
        this, call_counter);

    net_connman_clock_call_set_property(
        NET_CONNMAN_CLOCK(clock_proxy_), GConnman::PROP_TIME_STR,
        g_variant_new_variant(g_variant_new_uint64(time)), nullptr,
        static_cast<GAsyncReadyCallback>(&ConnmanClock::set_property_cb),
        data.release());
}

void ConnmanClock::setTimeZone(const std::string& timezone,
                               ClockPropertiesSetCallback callback) {
    const auto call_counter = prepareCallback(std::move(callback));
    auto data = std::make_unique<GConnman::CallbackData<ConnmanClock>>(
        this, call_counter);
    net_connman_clock_call_set_property(
        NET_CONNMAN_CLOCK(clock_proxy_), GConnman::PROP_TIMEZONE_STR,
        g_variant_new_variant(g_variant_new_string(timezone.c_str())), nullptr,
        static_cast<GAsyncReadyCallback>(&ConnmanClock::set_property_cb),
        data.release());
}

void ConnmanClock::setTimeUpdates(const std::string& timeUpdates,
                                  ClockPropertiesSetCallback callback) {
    const auto call_counter = prepareCallback(std::move(callback));
    auto data = std::make_unique<GConnman::CallbackData<ConnmanClock>>(
        this, call_counter);
    net_connman_clock_call_set_property(
        NET_CONNMAN_CLOCK(clock_proxy_), GConnman::PROP_TIMEUPDATES_STR,
        g_variant_new_variant(g_variant_new_string(timeUpdates.c_str())),
        nullptr,
        static_cast<GAsyncReadyCallback>(&ConnmanClock::set_property_cb),
        data.release());
}

void ConnmanClock::setTimeZoneUpdates(const std::string& timeZoneUpdates,
                                      ClockPropertiesSetCallback callback) {
    const auto call_counter = prepareCallback(std::move(callback));
    auto data = std::make_unique<GConnman::CallbackData<ConnmanClock>>(
        this, call_counter);
    net_connman_clock_call_set_property(
        NET_CONNMAN_CLOCK(clock_proxy_), GConnman::PROP_TIMEZONEUPDATES_STR,
        g_variant_new_variant(g_variant_new_string(timeZoneUpdates.c_str())),
        nullptr,
        static_cast<GAsyncReadyCallback>(&ConnmanClock::set_property_cb),
        data.release());
}

void ConnmanClock::setTimeServers(const std::vector<std::string>& servers,
                                  ClockPropertiesSetCallback callback) {
    const auto call_counter = prepareCallback(std::move(callback));
    auto data = std::make_unique<GConnman::CallbackData<ConnmanClock>>(
        this, call_counter);
    GVariantBuilder builder;
    g_variant_builder_init(&builder, G_VARIANT_TYPE("as"));
    for (const auto& server : servers) {
        GVariant* str_variant = g_variant_new_string(server.c_str());
        g_variant_builder_add_value(&builder, str_variant);
    }
    GVariant* servers_variant = g_variant_builder_end(&builder);
    net_connman_clock_call_set_property(
        NET_CONNMAN_CLOCK(clock_proxy_), GConnman::PROP_TIMESERVERS_STR,
        g_variant_new_variant(servers_variant), nullptr,
        static_cast<GAsyncReadyCallback>(&ConnmanClock::set_property_cb),
        data.release());
    g_variant_builder_clear(&builder);
}

void ConnmanClock::getProperties(ClockPropertiesCallback callback) {
    const auto call_counter = prepareCallback(std::move(callback));
    auto data = std::make_unique<GConnman::CallbackData<ConnmanClock>>(
        this, call_counter);
    net_connman_clock_call_get_properties(
        NET_CONNMAN_CLOCK(clock_proxy_), nullptr,
        static_cast<GAsyncReadyCallback>(&ConnmanClock::get_property_cb),
        data.release());
}

void ConnmanClock::onPropertiesChanged(ClockPropertiesCallback callback) {
    if (callback == nullptr) {
        std::cerr << "Callback for properties changed cannot be null.\n";
        return;
    }

    {
        std::unique_lock<std::mutex> const lock(mtx_);
        properties_callbacks_[0U] = std::move(callback);
    }

    g_signal_connect(NET_CONNMAN_CLOCK(clock_proxy_), "g-properties-changed",
                     G_CALLBACK(&ConnmanClock::on_properties_changed_cb), this);
}

}  // namespace Amarula::DBus