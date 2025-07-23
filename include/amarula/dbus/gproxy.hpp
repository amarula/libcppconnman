#pragma once

#include <gio/gio.h>
#include <glib-object.h>
#include <glib.h>

#include <amarula/dbus/gdbus.hpp>
#include <any>
#include <array>
#include <cstddef>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <string>

namespace Amarula::DBus::G {

template <class Properties>
class DBusProxy : public std::enable_shared_from_this<DBusProxy<Properties>> {
   public:
    using PropertiesCallback =
        std::function<void(const Properties& properties)>;
    using PropertiesSetCallback = std::function<void(bool success)>;

   private:
    std::mutex mtx_;
    std::mutex cb_mtx_;
    size_t callback_counter_{0U};
    GDBusProxy* proxy_;
    DBus* dbus_;
    std::map<size_t, std::any> callbacks_;
    Properties props_;
    PropertiesCallback on_property_changed_user_cb_{nullptr};

    void update_property(GVariant* prop) {
        GVariant* key_variant = g_variant_get_child_value(prop, 0);
        const gchar* key = g_variant_get_string(key_variant, nullptr);
        GVariant* value = g_variant_get_child_value(prop, 1);
        GVariant* variant = g_variant_get_variant(value);
        props_.update(key, variant);
        g_variant_unref(key_variant);
        g_variant_unref(variant);
        g_variant_unref(value);
    }

   protected:
    void updateProperties(GVariant* properties) {
        GVariantIter* iter = g_variant_iter_new(properties);
        GVariant* prop = nullptr;

        while ((prop = g_variant_iter_next_value(iter)) != nullptr) {
            update_property(prop);
            g_variant_unref(prop);
        }
        g_variant_iter_free(iter);
    }

    static auto finish(GDBusProxy* proxy, GAsyncResult* res, GError** error,
                       GVariant** out_properties = nullptr) {
        GVariant* ret = nullptr;
        ret = g_dbus_proxy_call_finish(proxy, res, error);
        if (ret != nullptr) {
            if (out_properties != nullptr) {
                *out_properties = g_variant_get_child_value(ret, 0);
            }
            g_variant_unref(ret);
            return true;
        }
        return false;
    }

    static void on_properties_changed_cb(
        GDBusProxy* /*proxy*/, gchar* /*sender_name*/, gchar* /*signal_name*/,
        GVariant* parameters /*string name, variant value*/,
        gpointer user_data) {
        auto self = static_cast<DBusProxy*>(user_data);
        self->update_property(parameters);
        if (self->on_property_changed_user_cb_) {
            std::lock_guard<std::mutex> const lock(self->cb_mtx_);
            self->on_property_changed_user_cb_(self->props_);
        }
    }

   public:
    DBusProxy(const DBusProxy&) = delete;
    auto operator=(const DBusProxy&) = delete;
    DBusProxy(DBusProxy&&) = delete;
    auto operator=(DBusProxy&&) = delete;
    virtual ~DBusProxy() {
        if (proxy_ != nullptr) {
            g_signal_handlers_disconnect_matched(proxy_, G_SIGNAL_MATCH_FUNC, 0,
                                                 0, nullptr, nullptr, nullptr);
            g_object_unref(proxy_);
        }
    }

    [[nodiscard]] auto properties() {
        std::lock_guard<std::mutex> lock(mtx_);
        return props_;
    }

    [[nodiscard]] auto proxy() const { return proxy_; }
    [[nodiscard]] auto dbus() const { return dbus_; }
    [[nodiscard]] auto objPath() const {
        const auto* path = g_dbus_proxy_get_object_path(proxy_);
        return path != nullptr ? std::string(path) : std::string();
    }

    void getProperties(PropertiesCallback callback = nullptr) {
        auto data = prepareCallback(std::move(callback));
        call_method(proxy_, nullptr, "GetProperties", nullptr,
                    &DBusProxy::get_property_cb, data.release());
    }

    void onPropertyChanged(const PropertiesCallback& callback) {
        if (callback != nullptr) {
            std::lock_guard<std::mutex> const lock(cb_mtx_);
            on_property_changed_user_cb_ = std::move(callback);
        }
    }

   protected:
    struct CallbackData {
       private:
        std::shared_ptr<DBusProxy> self_;
        std::optional<size_t> counter_{std::nullopt};

       public:
        CallbackData(std::shared_ptr<DBusProxy> self_ptr,
                     std::optional<size_t> count)
            : self_{self_ptr}, counter_{count} {}
        auto getSelf() const { return self_; }
        [[nodiscard]] auto getCounter() const { return counter_; }
    };

    template <class CallBackType, typename... Args>
    void executeCallBack(const std::optional<size_t>& counter, Args&&... args) {
        std::unique_lock<std::mutex> lock(mtx_);

        if (counter) {
            const auto callback =
                std::any_cast<CallBackType>(callbacks_[counter.value()]);
            lock.unlock();
            callback(std::forward<Args>(args)...);
            lock.lock();
            callbacks_.erase(counter.value());
        }

        dbus_->on_any_async_done();
    }

    explicit DBusProxy(DBus* dbus, const gchar* name, const gchar* obj_path,
                       const gchar* interface_name)
        : dbus_{dbus} {
        GError* err = nullptr;

        proxy_ = g_dbus_proxy_new_sync(dbus_->connection(),
                                       G_DBUS_PROXY_FLAGS_NONE, nullptr, name,
                                       obj_path, interface_name, nullptr, &err);
        if (proxy_ == nullptr) {
            std::string const msg =
                "Failed to create proxy: " + std::string(err->message);
            g_error_free(err);
            throw std::runtime_error(msg);
        }
        g_signal_connect(proxy_, "g-signal::PropertyChanged",
                         G_CALLBACK(&DBusProxy::on_properties_changed_cb),
                         this);
    }

    template <typename T>
    auto prepareCallback(T callback) {
        std::lock_guard<std::mutex> const lock(mtx_);
        dbus_->on_any_async_start();
        std::optional<size_t> counter{std::nullopt};
        if (callback) {
            size_t index = ++callback_counter_;
            counter = index;
            callbacks_[counter.value()] = std::move(callback);
        }
        auto self = this->shared_from_this();
        return std::make_unique<CallbackData>(self, counter);
    }

    static void finishAsyncCall(GObject* proxy, GAsyncResult* res,
                                gpointer user_data) {
        GError* error = nullptr;

        const auto success = finish(G_DBUS_PROXY(proxy), res, &error);
        if (!success) {
            std::cerr << error->message << '\n';
            g_error_free(error);
        }

        std::unique_ptr<CallbackData> data(
            static_cast<CallbackData*>(user_data));
        auto self = data->getSelf();
        const auto counter = data->getCounter();
        self->template executeCallBack<PropertiesSetCallback>(counter, success);
    }

    static void get_property_cb(GObject* proxy, GAsyncResult* res,
                                gpointer user_data) {
        GError* error = nullptr;
        GVariant* out_properties = nullptr;
        std::unique_ptr<CallbackData> data(
            static_cast<CallbackData*>(user_data));
        auto self = data->getSelf();
        const auto counter = data->getCounter();
        const auto success =
            finish(G_DBUS_PROXY(proxy), res, &error, &out_properties);

        if (success) {
            self->updateProperties(out_properties);
            g_variant_unref(out_properties);
        } else {
            std::cerr << error->message << '\n';
            g_error_free(error);
        }
        self->template executeCallBack<PropertiesCallback>(counter,
                                                           self->props_);
    }

    static void set_property(GDBusProxy* proxy, const gchar* arg_name,
                             GVariant* arg_value, GCancellable* cancellable,
                             GAsyncReadyCallback callback, gpointer user_data

    ) {
        std::array<GVariant*, 2> tuple_elements{
            g_variant_new_string(arg_name), g_variant_new_variant(arg_value)};

        GVariant* parameters = g_variant_new_tuple(tuple_elements.data(), 2);
        g_dbus_proxy_call(proxy, "SetProperty", parameters,
                          G_DBUS_CALL_FLAGS_NONE, -1, cancellable, callback,
                          user_data);
    }

    static void call_method(GDBusProxy* proxy, GCancellable* cancellable,
                            const gchar* arg_name, GVariant* parameters,
                            GAsyncReadyCallback callback, gpointer user_data) {
        g_dbus_proxy_call(
            proxy, arg_name,
            (parameters != nullptr) ? parameters
                                    : g_variant_new_tuple(nullptr, 0),
            G_DBUS_CALL_FLAGS_NONE, -1, cancellable, callback, user_data);
    }
};

}  // namespace Amarula::DBus::G
