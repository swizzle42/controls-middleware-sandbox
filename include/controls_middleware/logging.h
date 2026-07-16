#pragma once

#include <sstream>
#include <string_view>

namespace logging {
enum class Level { Debug, Info, Warning, Error };

void set_level(Level level);
Level get_level();

class LogMessage {
 public:
  LogMessage(Level level, std::string_view tag);
  ~LogMessage();

  template <typename T>
  LogMessage& operator<<(const T& value) {
    if (m_enabled) m_stream << value;
    return *this;
  }

 private:
  Level m_level;
  std::string_view m_tag;
  std::ostringstream m_stream;
  bool m_enabled;
};

void write(Level level, std::string_view tag, std::string_view message);

}  // namespace logging

#define LOG_DEBUG(tag) logging::LogMessage(logging::Level::Debug, tag)
#define LOG_INFO(tag) logging::LogMessage(logging::Level::Info, tag)
#define LOG_WARN(tag) logging::LogMessage(logging::Level::Warning, tag)
#define LOG_ERROR(tag) logging::LogMessage(logging::Level::Error, tag)
