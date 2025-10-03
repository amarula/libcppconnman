#pragma once

#include <atomic>
#include <iostream>
#include <mutex>
#include <sstream>

namespace Amarula {

class Log {
   public:
    static void enable(bool enabled) {
        state().store(enabled, std::memory_order_relaxed);
    }

    static auto isEnabled() -> bool {
        return state().load(std::memory_order_relaxed);
    }

    static auto mutex() -> std::mutex& {
        static std::mutex mutex;
        return mutex;
    }

   private:
    static auto state() -> std::atomic<bool>& {
#ifdef LCM_LOG_DEFAULT_ENABLED
        static std::atomic<bool> flag{true};
#else
        static std::atomic<bool> flag{false};
#endif
        return flag;
    }
};

}  // namespace Amarula

#ifndef LCM_LOG_STREAM
#define LCM_LOG_STREAM std::cout
#endif

#define LCM_LOG(...)                                                         \
    do {                                                                     \
        if (::Amarula::Log::isEnabled()) {                                   \
            std::ostringstream _lcm_oss;                                     \
            _lcm_oss << __VA_ARGS__;                                         \
            std::lock_guard<std::mutex> _lcm_guard(::Amarula::Log::mutex()); \
            LCM_LOG_STREAM << _lcm_oss.str();                                \
        }                                                                    \
    } while (0)
