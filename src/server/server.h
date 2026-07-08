#pragma once

#include <poll.h>

#include <atomic>
#include <cstdint>
#include <functional>
#include <string_view>
#include <thread>
#include <vector>

#include "controls_middleware/packet.h"

namespace controls_middleware {

// context for client data buffer
typedef struct {
  std::vector<uint8_t> buffer;
  size_t read_ptr{0};
} buffer_ctx_t;

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
  /**
   * @brief internal listen loop
   */
  void listen_loop(std::stop_token stop_token, PacketCallback callback);

  /**
   * @brief process the client message buffer
   */
  std::vector<SensorPacket> process_client_buffer(buffer_ctx_t& context);

  /**
   * @brief handle new connections and stage them
   */
  void handle_new_connection(pollfd& listen_slot,
                             std::vector<pollfd>& clients_to_add);

  /**
   * @brief handle client events
   */
  void handle_client_event(const pollfd& client_slot,
                           const PacketCallback& callback,
                           std::vector<int>& fds_to_remove);

  /**
   * @brief apply any staged changes to the list of active sockets
   */
  void apply_staged_updates(std::vector<int>& fds_to_remove,
                            std::vector<pollfd>& clients_to_add);

  std::vector<pollfd> m_monitor_list;  // list of active sockets
  std::unordered_map<int, buffer_ctx_t>
      m_buffers;                          // buffers for each active socket
  std::jthread m_worker_thread;           // handle to background worker
  std::atomic<bool> m_is_running{false};  // thread-safe flag tracking state
};

}  // namespace controls_middleware
