#pragma once

#include <controls_middleware/packet.h>
#include <sys/uio.h>

#include <cstdint>
#include <string_view>

#include "telemetry_generated.h"

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
   * @brief Transmit a pre-packed data frame
   */
  void send_packet(const sensor_packet& packet);

 private:
  // file descriptor/resource handle
  int m_socket_fd{-1};

  // send_all method
  // int send_all(const uint8_t* buf, int& len);
  int send_all(iovec* iov, int iovcnt);
};
}  // namespace controls_middleware
