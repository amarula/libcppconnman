#include <cstddef>
#include <optional>

namespace Amarula::DBus::GConnman {

constexpr const char* SERVICE = "net.connman";
constexpr const char* CLOCK_INTERFACE = "net.connman.Clock";
constexpr const char* MANAGER_PATH = "/";
constexpr const char* PROP_TIME_STR = "Time";
constexpr const char* PROP_TIMEUPDATES_STR = "TimeUpdates";
constexpr const char* PROP_TIMEZONE_STR = "Timezone";
constexpr const char* PROP_TIMEZONEUPDATES_STR = "TimezoneUpdates";
constexpr const char* PROP_TIMESERVERS_STR = "Timeservers";
constexpr const char* PROP_TIMESERVERSYNCED_STR = "TimeserverSynced";

template <typename T>
struct CallbackData {
   private:
    T* self;
    std::optional<size_t> counter{std::nullopt};

   public:
    CallbackData(T* selfPtr, std::optional<size_t> count)
        : self{selfPtr}, counter{count} {}
    auto getSelf() const -> T* { return self; }
    [[nodiscard]] auto getCounter() const -> std::optional<size_t> {
        return counter;
    }
};

}  // namespace Amarula::DBus::GConnman
