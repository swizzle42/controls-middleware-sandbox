#pragma once

#include <string_view>

namespace logging {
enum class Level { Debug, Info, Warning, Error };

void set_level(Level level);

void log(Level level, std::string_view tag, std::string_view message);

}  // namespace logging

#define LOG_DEBUG(tag, msg) logging::log(logging::Level::Debug, tag, msg)
#define LOG_INFO(tag, msg) logging::log(logging::Level::Info, tag, msg)
#define LOG_WARN(tag, msg) logging::log(logging::Level::Warning, tag, msg)
#define LOG_ERROR(tag, msg) logging::log(logging::Level::Error, tag, msg)
