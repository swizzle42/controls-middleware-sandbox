#include "server.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <iostream>
#include <stdexcept>
#include <string>

namespace controls_middleware {
SensorServer::SensorServer(std::string_view ip_address, uint16_t port) {
  // create a TCP socket
  m_listen_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (m_listen_fd < 0) {
    throw std::runtime_error("Failed to create listener socket");
  }

  // configure the listener socket
  int opt{1};
  setsockopt(m_listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

  // configure the target endpoint address structure
  struct sockaddr_in listener_addr{};
  listener_addr.sin_family = AF_INET;
  listener_addr.sin_port = htons(port);

  // handle ip address
  if (inet_pton(AF_INET, ip_address.data(), &listener_addr.sin_addr) <= 0) {
    close(m_listen_fd);
    throw std::runtime_error("Invalid IP address string format: " +
                             std::string(ip_address));
  }

  // bind to the listener address to the socket
  if (bind(m_listen_fd, reinterpret_cast<struct sockaddr*>(&listener_addr),
           sizeof(listener_addr)) < 0) {
    close(m_listen_fd);
    throw std::runtime_error("Failed to bind the socket to port " +
                             std::to_string(port));
  }

  if (listen(m_listen_fd, 5) < 0) {
    close(m_listen_fd);
    throw std::runtime_error("Failed to listen on port " +
                             std::to_string(port));
  }
}

SensorServer::~SensorServer() {
  if (m_listen_fd >= 0) {
    close(m_listen_fd);
  }

  for (int fd : m_client_fds) {
    if (fd >= 0) {
      close(fd);
    }
  }
}

void SensorServer::start(PacketCallback callback) {
  m_worker_thread = std::jthread(&SensorServer::listen_loop, this, callback);
  m_is_running = true;
}

void SensorServer::listen_loop(std::stop_token stop_token, PacketCallback callback) {
  while(!stop_token.stop_requested()) {
    int client_fd = accept(m_listen_fd, nullptr, nullptr);

    // if we fail to accept the client connection
    if (client_fd < 0) {
      // if a stop has been requested, we just break
      if (stop_token.stop_requested()) {
        break;
      }

      // otherwise, we need to handle the error (TODO)
      continue;
    }

    // add the client FD to our list of active clients
    m_client_fds.push_back(client_fd);

    // prepare to recieve data and read
    SensorPacket incoming_packet {};
    ssize_t bytes_read = read(client_fd, &incoming_packet, sizeof(SensorPacket));
    if (bytes_read == sizeof(SensorPacket)) {
      // we have read a SensorPacket, defer to the callback
      callback(incoming_packet);
    } else {
      std::cerr << "Failed to read packet.";
    }

    // clean up the client communication channel
    close(client_fd);
    m_client_fds.pop_back();
  }

  // the listen loop is exiting
  m_is_running = false;
  return;
}

}  // namespace controls_middleware
