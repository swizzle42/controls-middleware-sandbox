#include "client.h"

#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <cstring>
#include <stdexcept>
#include <string>

#include "controls_middleware/logging.h"

static const char* TAG = "sensor_client";

namespace controls_middleware {
SensorClient::SensorClient(std::string_view ip_address, uint16_t port) {
  // get address info
  int status;
  struct addrinfo hints, *client_info;

  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;

  if ((status = getaddrinfo(ip_address.data(), std::to_string(port).data(),
                            &hints, &client_info)) != 0) {
    LOG_ERROR(TAG) << "getaddrinfo: " << gai_strerror(status);
  }

  // create IPv4 TCP socket
  m_socket_fd = socket(client_info->ai_family, client_info->ai_socktype,
                       client_info->ai_protocol);
  if (m_socket_fd < 0) {
    LOG_ERROR(TAG) << "Failed to create client socket";
    throw std::runtime_error("Failed to create network socket");
  }

  // Connect to the server
  if (connect(m_socket_fd, client_info->ai_addr, client_info->ai_addrlen) < 0) {
    close(m_socket_fd);
    throw std::runtime_error("Failed to connect to the server on port " +
                             std::to_string(port));
  }

  LOG_INFO(TAG) << "client created on " << ip_address << ":" << port;
}

SensorClient::~SensorClient() {
  if (m_socket_fd > 0) {
    close(m_socket_fd);
  }
}

SensorClient::SensorClient(SensorClient&& other) noexcept
    : m_socket_fd{other.m_socket_fd} {
  other.m_socket_fd = -1;
}

SensorClient& SensorClient::operator=(SensorClient&& other) noexcept {
  if (this != &other) {
    if (m_socket_fd >= 0) {
      close(m_socket_fd);
    }

    m_socket_fd = other.m_socket_fd;
    other.m_socket_fd = -1;
  }

  return *this;
}

void SensorClient::send_packet(const sensor_packet& packet) {
  ssize_t bytes_sent = send(m_socket_fd, &packet, sizeof(sensor_packet), 0);

  if (bytes_sent < 0) {
    throw std::runtime_error("Network send operation failed");
  }

  if (static_cast<size_t>(bytes_sent) != sizeof(sensor_packet)) {
    throw std::runtime_error("Partial send occurred");
  }
}
}  // namespace controls_middleware
