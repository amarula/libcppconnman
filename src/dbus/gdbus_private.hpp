

#include <glib.h>

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

    [[nodiscard]] constexpr std::string_view to_string(Enum enum_value) const {
        for (auto&& [e, s] : data_) {
            if (e == enum_value) {
                return s;
            }
        }
        throw std::runtime_error("Invalid enum value");
    }

    [[nodiscard]] constexpr Enum from_string(std::string_view str) const {
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
auto as_to_vector(GVariant* array_s, const EnumStringMap<T, N>* enumMap =
                                         nullptr) -> std::vector<T> {
    GVariantIter strIter;
    std::vector<T> container;
    if (g_variant_is_of_type(array_s, G_VARIANT_TYPE("as")) != 0U) {
        g_variant_iter_init(&strIter, array_s);
        GVariant* item = nullptr;
        while ((item = g_variant_iter_next_value(&strIter)) != nullptr) {
            const gchar* item_str = g_variant_get_string(item, nullptr);
            if constexpr (std::is_enum_v<T>) {
                container.emplace_back(enumMap->from_string(item_str));
            } else {
                container.emplace_back(item_str);
            }
            g_variant_unref(item);
        }
    }
    return container;
}

}  // namespace Amarula::DBus::G
