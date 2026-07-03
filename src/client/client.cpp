#include "client.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <stdexcept>
#include <string>

namespace controls_middleware {
SensorClient::SensorClient(std::string_view ip_address, uint16_t port) {
  // create IPv4 TCP socket
  m_socket_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (m_socket_fd < 0) {
    throw std::runtime_error("Failed to create network socket");
  }

  // Configure the target endpoint address structure
  struct sockaddr_in server_addr{};
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(port);

  // Convert the target IP to the appropriate network format
  if (inet_pton(AF_INET, ip_address.data(), &server_addr.sin_addr) <= 0) {
    close(m_socket_fd);
    throw std::runtime_error("Invalid IP address string format: " +
                             std::string(ip_address));
  }

  // Connect to the server
  if (connect(m_socket_fd, reinterpret_cast<struct sockaddr*>(&server_addr),
              sizeof(server_addr)) < 0) {
    close(m_socket_fd);
    throw std::runtime_error("Failed to connect to the server on port " +
                             std::to_string(port));
  }
}

SensorClient::~SensorClient() {
  if (m_socket_fd > 0) {
    close(m_socket_fd);
  }
}

void SensorClient::send_packet(const sensor_packet& packet) {
  ssize_t bytes_sent = write(m_socket_fd, &packet, sizeof(sensor_packet));

  if (bytes_sent < 0) {
    throw std::runtime_error("Network write operation failed");
  }

  if (static_cast<size_t>(bytes_sent) != sizeof(sensor_packet)) {
    throw std::runtime_error("Partial write occurred");
  }
}
}  // namespace controls_middleware
