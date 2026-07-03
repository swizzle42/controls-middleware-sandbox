#pragma once

#include <cstdint>

namespace controls_middleware {
// ensure the compiler doesn't add invisible padding bytes
#pragma pack(push, 1)
struct SensorPacket {
  uint16_t device_id;  // Device ID (2 bytes)
  uint8_t status;      // 1 byte
  uint8_t padding;     // 1 byte
  uint64_t timestamp;  // 8 bytes
  float value;         // 4 bytes
};
#pragma pack(pop)

}  // namespace controls_middleware
