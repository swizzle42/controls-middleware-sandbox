#pragma once

#include <atomic>
#include <cstdint>
#include <functional>
#include <string_view>
#include <thread>
#include <vector>

#include "controls_middleware/packet.h"

namespace controls_middleware {
class SensorServer {
 public:
  using PacketCallback = std::function<void(const SensorPacket&)>;

  // constructor: prepare listening socket but don't start the loop yet
  SensorServer(std::string_view ip_address, uint16_t port);

  // destructor: ensure background thread is stopped and all sockets closed
  ~SensorServer();

  // prevent copying
  SensorServer(const SensorServer&) = delete;
  SensorServer& operator=(const SensorServer&) = delete;

  /**
   * @brief start the server executation loop in a background thread
   */
  void start(PacketCallback callback);

  /**
   * @brief stop the server manually
   */
  void stop();

 private:
  // internal listen loop
  void listen_loop(std::stop_token stop_token, PacketCallback callback);

  int m_listen_fd{-1};            // primary public socket
  std::vector<int> m_client_fds;  // dynamic list of active client sockets
  std::jthread m_worker_thread;   // handle to background worker
  std::atomic<bool> m_is_running{false};  // thread-safe flag tracking state
};

}  // namespace controls_middleware
