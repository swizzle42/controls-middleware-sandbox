#include "controls_middleware/logging.h"

#include <chrono>
#include <ctime>
#include <iomanip>
#include <iostream>

namespace logging {
namespace {
Level current_level = Level::Debug;

const char* level_to_string(Level level) {
  switch (level) {
    case Level::Debug:
      return "DEBUG";
    case Level::Info:
      return "INFO";
    case Level::Warning:
      return "WARN";
    case Level::Error:
      return "ERROR";
    default:
      return "";
  }
}
}  // namespace

void set_level(Level level) { current_level = level; }

void log(Level level, std::string_view tag, std::string_view message) {
  if (level < current_level) return;

  auto now = std::chrono::system_clock::now();
  auto time = std::chrono::system_clock::to_time_t(now);

  std::cout << std::put_time(std::localtime(&time), "%H:%M:%S") << " ["
            << level_to_string(level) << "]"
            << " [" << tag << "] " << message << '\n';
}
}  // namespace logging
