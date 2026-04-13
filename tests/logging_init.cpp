#include <amarula/log.hpp>

namespace {
struct EnableLogging {
    EnableLogging() { Amarula::Log::enable(true); }
};

[[maybe_unused]] EnableLogging enable_logging;
}  // namespace
