

#include <glib.h>

#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace Amarula::DBus::G {

template <typename Enum, std::size_t N>
class EnumStringMap {
   public:
    using Pair = std::pair<Enum, std::string_view>;

    constexpr explicit EnumStringMap(std::array<Pair, N> init) : data_{init} {}

    [[nodiscard]] constexpr auto to_string(Enum enum_value) const
        -> std::string_view {
        for (auto&& [e, s] : data_) {
            if (e == enum_value) {
                return s;
            }
        }
        throw std::runtime_error("Invalid enum value");
    }

    [[nodiscard]] constexpr auto from_string(std::string_view str) const
        -> Enum {
        for (auto&& [e, s] : data_) {
            if (s == str) {
                return e;
            }
        }
        throw std::runtime_error("Invalid string value");
    }

   private:
    std::array<Pair, N> data_;
};

template <typename T = std::string, std::size_t N = 0>
auto as_to_vector(GVariant* array_s, const EnumStringMap<T, N>* enum_map =
                                         nullptr) -> std::vector<T> {
    GVariantIter str_iter;
    std::vector<T> container;
    if (g_variant_is_of_type(array_s, G_VARIANT_TYPE("as")) != 0U) {
        g_variant_iter_init(&str_iter, array_s);
        GVariant* item = nullptr;
        while ((item = g_variant_iter_next_value(&str_iter)) != nullptr) {
            const gchar* item_str = g_variant_get_string(item, nullptr);
            if constexpr (std::is_enum_v<T>) {
                container.emplace_back(enum_map->from_string(item_str));
            } else {
                container.emplace_back(item_str);
            }
            g_variant_unref(item);
        }
    }
    return container;
}

using VariantPtr = std::unique_ptr<GVariant, decltype(&g_variant_unref)>;

static inline auto vector_to_as(const std::vector<std::string>& vec)
    -> VariantPtr {
    GVariantBuilder builder;
    g_variant_builder_init(&builder, G_VARIANT_TYPE("as"));

    for (const auto& string : vec) {
        g_variant_builder_add_value(&builder,
                                    g_variant_new_string(string.c_str()));
    }

    GVariant* var = g_variant_builder_end(&builder);
    return VariantPtr{g_variant_ref_sink(var), &g_variant_unref};
}

}  // namespace Amarula::DBus::G
