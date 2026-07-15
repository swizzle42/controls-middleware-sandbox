#pragma once

#include <cstdint>

namespace controls_middleware {

const static uint16_t magic_val = 0xFACE;

enum class msg_type_enum : uint8_t { TELEMETRY = 1 };

#pragma pack(push, 1)
struct control_frame_header {
  uint16_t magic;           // 2 bytes
  uint8_t version;          // 1 byte
  msg_type_enum msg_type;   // 1 byte
  uint32_t seq_num;         // 4 bytes
  uint32_t payload_length;  // 4 bytes
};  // 8 bytes
#pragma pack(pop)

struct sensor_packet {
  uint16_t device_id;  // 2 bytes
  uint8_t status;      // 1 byte
  uint64_t timestamp;  // 8 bytes
  float value;         // 4 bytes
};  // 16 bytes

}  // namespace controls_middleware
