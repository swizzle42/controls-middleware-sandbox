#include "server.h"

#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <algorithm>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <string>

#include "controls_middleware/logging.h"

static const char* TAG = "sensor_server";

namespace controls_middleware {
SensorServer::SensorServer(std::string_view ip_address, uint16_t port) {
  int listener_fd = get_listener_socket(ip_address, std::to_string(port));

  // otherwise, the listener socket is initialise so add to monitor list
  pollfd listener{.fd = listener_fd,
                  .events = POLLIN,  // check for readability
                  .revents = 0};

  m_monitor_list.push_back(listener);

  LOG_DEBUG(TAG) << "initialisation complete";
  LOG_DEBUG(TAG) << "created socket on " << ip_address << ":" << port;
}

SensorServer::~SensorServer() {
  stop();
  // on destruction, close any and all monitored files
  for (auto& pfd : m_monitor_list) {
    if (pfd.fd >= 0) {
      close(pfd.fd);
    }
  }
}

int SensorServer::get_listener_socket(std::string_view address,
                                      std::string_view port) {
  int listener_fd;
  int yes{1};
  int status;

  struct addrinfo hints, *listener_info, *p;

  // get a socket and bind it
  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;

  if ((status = getaddrinfo(address.data(), port.data(), &hints,
                            &listener_info)) != 0) {
    LOG_ERROR(TAG) << "getaddrinfo: " << gai_strerror(status);
    throw std::runtime_error("Failed to get server info");
  }

  for (p = listener_info; p != nullptr; p = p->ai_next) {
    listener_fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
    if (listener_fd < 0) {
      continue;
    }

    setsockopt(listener_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

    if (bind(listener_fd, p->ai_addr, p->ai_addrlen) < 0) {
      close(listener_fd);
      continue;
    }

    break;
  }

  if (p == nullptr) {
    throw std::runtime_error("failed to create and bind to socket");
  }

  freeaddrinfo(listener_info);

  if (listen(listener_fd, 10) == -1) {
    throw std::runtime_error("failed to listen on port");
  }

  return listener_fd;
}

void SensorServer::start(PacketCallback callback) {
  m_worker_thread = std::jthread(&SensorServer::listen_loop, this, callback);
  m_is_running = true;
}

void SensorServer::stop() {
  if (m_worker_thread.joinable()) m_worker_thread.request_stop();
}

void SensorServer::listen_loop(std::stop_token stop_token,
                               PacketCallback callback) {
  while (!stop_token.stop_requested()) {
    // staging vectors
    std::vector<pollfd> clients_to_add;
    std::vector<int> fds_to_remove;

    // poll monitored connections
    int ready = poll(m_monitor_list.data(), m_monitor_list.size(), 100);

    // if ready is -1, we have an error
    if (ready == -1) {
      if (errno == EINTR) continue;
      LOG_ERROR(TAG) << "Failed to poll open sockets";
      throw std::runtime_error("Failed to poll open sockets.");
    }

    for (size_t i = 0; i < m_monitor_list.size(); ++i) {
      auto& slot = m_monitor_list[i];

      // if there are no returned events for the slot, continue to the next
      if (slot.revents == 0) continue;

      if (i == 0) {
        LOG_DEBUG(TAG) << "Event on Listening Socket.";
        handle_new_connection(slot, clients_to_add);
      } else {
        LOG_DEBUG(TAG) << "Event on Client Socket.";
        handle_client_event(slot, callback, fds_to_remove);
      }
    }

    if (!fds_to_remove.empty() || !clients_to_add.empty()) {
      LOG_DEBUG(TAG) << "Modifying server clients.";
      apply_staged_updates(fds_to_remove, clients_to_add);
    }
  }
  m_is_running = false;
}

void SensorServer::handle_new_connection(pollfd& listen_slot,
                                         std::vector<pollfd>& clients_to_add) {
  if (!(listen_slot.revents & POLLIN)) return;

  int new_client_fd = accept4(listen_slot.fd, nullptr, nullptr, SOCK_NONBLOCK);
  if (new_client_fd == -1) return;

  // add the client to the staging vector
  clients_to_add.push_back(
      pollfd{.fd = new_client_fd, .events = POLLIN, .revents = 0});

  // create an entry for the clients message buffer
  m_buffers[new_client_fd] = buffer_ctx_t{};

  LOG_DEBUG(TAG) << "Added new connection to Server.";
}

void SensorServer::handle_client_event(const pollfd& client_slot,
                                       const PacketCallback& callback,
                                       std::vector<int>& fds_to_remove) {
  if (client_slot.revents & (POLLERR | POLLHUP)) {
    fds_to_remove.push_back(client_slot.fd);
    return;
  }

  if (client_slot.revents & POLLIN) {
    uint8_t scratchpad[64];
    ssize_t bytes_read =
        recv(client_slot.fd, scratchpad, sizeof(scratchpad), 0);

    if (bytes_read > 0) {
      LOG_DEBUG(TAG) << "Bytes read from client";
      auto it = m_buffers.find(client_slot.fd);
      if (it != m_buffers.end()) {
        auto& ctx = it->second;
        ctx.buffer.insert(ctx.buffer.end(), scratchpad,
                          scratchpad + bytes_read);

        for (auto& packet : process_client_buffer(ctx)) {
          callback(packet);
        }
      }
    } else {
      LOG_DEBUG(TAG) << "Client signaled change but sent no bytes. Closing.";
      fds_to_remove.push_back(client_slot.fd);
    }
  }
}

void SensorServer::apply_staged_updates(std::vector<int>& fds_to_remove,
                                        std::vector<pollfd>& clients_to_add) {
  // process removal
  for (int fd : fds_to_remove) {
    LOG_DEBUG(TAG) << "Removing client connection (fd=" << fd << ")";
    close(fd);
    m_buffers.erase(fd);

    m_monitor_list.erase(
        std::remove_if(
            m_monitor_list.begin(), m_monitor_list.end(),
            [fd](const pollfd& monitor_item) { return monitor_item.fd == fd; }),
        m_monitor_list.end());
  }

  // process addition
  if (!m_monitor_list.empty()) {
    LOG_DEBUG(TAG) << "Adding client connections";
    m_monitor_list.insert(m_monitor_list.end(), clients_to_add.begin(),
                          clients_to_add.end());
  }
}

std::vector<sensor_packet> SensorServer::process_client_buffer(
    buffer_ctx_t& context) {
  // we need to greedily take from the front of the buffer in increments of
  // sizeof(sensor_packet)
  std::vector<sensor_packet> packets;

  // NOTE: blocking unitl we process all the packets in the client buffer
  while ((context.buffer.size() - context.read_ptr) >= sizeof(sensor_packet)) {
    // get the start of the packet
    const uint8_t* packet_ptr = context.buffer.data() + context.read_ptr;

    // cast the packet
    const sensor_packet& packet =
        *reinterpret_cast<const sensor_packet*>(packet_ptr);
    // add the packet to our return vector
    packets.push_back(packet);

    // consume the frame size in the tracking pointer
    context.read_ptr += sizeof(sensor_packet);

    LOG_DEBUG(TAG) << "packet constructed and added to buffer";
  }

  // compaction once we've extracted packets
  client_buffer_compaction(context, (sizeof(sensor_packet) * 4));

  return packets;
}

void SensorServer::client_buffer_compaction(
    controls_middleware::buffer_ctx_t& context, size_t buffer_max_size) {
  if (context.read_ptr == context.buffer.size()) {
    // if we have read up to the end of the buffer we can clear
    context.buffer.clear();
    context.read_ptr = 0;

    LOG_DEBUG(TAG) << "client buffer cleared";
  } else if (context.read_ptr >= buffer_max_size) {
    // otherwise, if we've read over 4 frames, we can move the unread data to
    // the start of the buffer and begin from idx=0
    size_t unread_bytes = context.buffer.size() - context.read_ptr;
    std::memmove(context.buffer.data(),
                 context.buffer.data() + context.read_ptr, unread_bytes);
    context.buffer.resize(unread_bytes);
    context.read_ptr = 0;

    LOG_DEBUG(TAG) << "client buffer resized";
  }
}

}  // namespace controls_middleware
