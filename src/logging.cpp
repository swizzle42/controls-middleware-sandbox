#include "controls_middleware/logging.h"

#include <atomic>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <iostream>

namespace logging {
namespace {
std::atomic<Level> current_level = Level::Info;

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

LogMessage::LogMessage(Level level, std::string_view tag)
    : m_level(level), m_tag(tag), m_enabled(level >= get_level()) {}
LogMessage::~LogMessage() { write(m_level, m_tag, m_stream.str()); }

void write(Level level, std::string_view tag, std::string_view message) {
  if (level < current_level) return;

  auto now = std::chrono::system_clock::now();
  auto time = std::chrono::system_clock::to_time_t(now);

  std::cout << std::put_time(std::localtime(&time), "%H:%M:%S") << " ["
            << level_to_string(level) << "]"
            << " [" << tag << "] " << message << '\n';
}

void set_level(Level level) { current_level = level; }

Level get_level() { return current_level; }

}  // namespace logging
