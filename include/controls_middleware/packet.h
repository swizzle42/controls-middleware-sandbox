#pragma once

#include <cstdint>

namespace controls_middleware {

struct header {
  uint16_t magic;     // 2 bytes
  uint8_t version;    // 1 byte
  uint8_t type;       // 1 byte
  uint32_t length;    // 4 bytes
};  // 8 bytes

struct sensor_packet {
  uint16_t device_id;  // 2 bytes
  uint8_t status;      // 1 byte
  uint8_t padding;     // 1 byte
  uint64_t timestamp;  // 8 bytes
  float value;         // 4 bytes
};  // 16 bytes

}  // namespace controls_middleware
