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
#include "flatbuffers/flatbuffers.h"
#include "telemetry_generated.h"

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

void SensorServer::start(TelemetryCallback callback) {
  m_worker_thread = std::jthread(&SensorServer::listen_loop, this, callback);
  m_is_running = true;
}

void SensorServer::stop() {
  if (m_worker_thread.joinable()) m_worker_thread.request_stop();
}

void SensorServer::listen_loop(std::stop_token stop_token,
                               TelemetryCallback callback) {
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
                                       const TelemetryCallback& callback,
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
        ctx.rx_buffer.insert(ctx.rx_buffer.end(), scratchpad,
                             scratchpad + bytes_read);

        process_client_buffer(ctx, callback);
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

int SensorServer::process_client_buffer(buffer_ctx_t& context,
                                        const TelemetryCallback& callback) {
  // greedily process Telemetry data from the client buffer
  while (true) {
    // if we dont' have enough data to produce a control frame header, break
    if (context.rx_buffer.size() < sizeof(control_frame_header)) {
      break;
    }

    // read the header data
    control_frame_header header;
    std::memcpy(&header, context.rx_buffer.data(),
                sizeof(control_frame_header));

    // convert from network to host format
    uint16_t magic = ntohs(header.magic);
    uint32_t payload_len = ntohl(header.payload_length);

    // validate the magic number
    if (magic != magic_val) {
      LOG_ERROR(TAG) << "protocol violation: bad magic bytes 0x" << std::hex
                     << magic << ". Returning early.";
      return -1;
    }

    // check if we have the complete flatbuffer
    uint32_t total_packet_size = sizeof(control_frame_header) + payload_len;
    if (context.rx_buffer.size() < total_packet_size) {
      break;
    }

    const uint8_t* payload_ptr =
        context.rx_buffer.data() + sizeof(control_frame_header);

    // verify the data
    flatbuffers::Verifier verifier(payload_ptr, payload_len);
    if (VerifyTelemetryBuffer(verifier)) {
      auto telemetry = GetTelemetry(payload_ptr);

      LOG_DEBUG(TAG) << "data verified, delegating to callback";
      callback(*telemetry);
    }

    // clear the frame from the buffer
    context.rx_buffer.erase(context.rx_buffer.begin(),
                            context.rx_buffer.begin() + total_packet_size);
  }

  return 0;
}

}  // namespace controls_middleware
