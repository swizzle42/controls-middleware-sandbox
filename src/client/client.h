#pragma once

#include <controls_middleware/packet.h>
#include <sys/uio.h>

#include <cstdint>
#include <string_view>

#include "flatbuffers/flatbuffers.h"

namespace controls_middleware {
class SensorClient {
 public:
  SensorClient(std::string_view ip_address, uint16_t port);
  ~SensorClient();

  // delete copy constructor and copy assignment as a
  // network client cannot belong to multiple sensors
  SensorClient(const SensorClient&) = delete;
  SensorClient& operator=(const SensorClient&) = delete;

  // allow moving the resource
  SensorClient(SensorClient&& other) noexcept;
  SensorClient& operator=(SensorClient&& other) noexcept;

  /**
   * @brief Transmit a pre-serialised payload
   *
   * @param builder: finished FlatBuffer builder
   * @param msg_type: type of message being sent
   * @param seq: sequence number
   */
  void send_packet(const flatbuffers::FlatBufferBuilder& builder,
                   const msg_type_enum msg_type, const uint32_t seq);

 private:
  // file descriptor/resource handle
  int m_socket_fd{-1};

  // send_all method
  // int send_all(const uint8_t* buf, int& len);
  int send_all(iovec* iov, int iovcnt);
};
}  // namespace controls_middleware
