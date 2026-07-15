#include "client.h"

#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>

#include <cstring>
#include <stdexcept>
#include <string>

#include "controls_middleware/logging.h"
#include "flatbuffers/flatbuffers.h"

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
  // create the payload
  flatbuffers::FlatBufferBuilder builder;
  auto telemetry_offset = CreateTelemetry(
      builder, packet.device_id, packet.status, packet.timestamp, packet.value);
  builder.Finish(telemetry_offset);

  // get the serialised data
  const uint8_t* payload_ptr = builder.GetBufferPointer();
  const uint32_t payload_size = builder.GetSize();

  // populate the header
  control_frame_header header;
  header.magic = htons(magic_val);
  header.version = 1;
  header.msg_type = msg_type_enum::TELEMETRY;
  header.seq_num = htonl(1);
  header.payload_length = htonl(payload_size);

  LOG_INFO(TAG) << "sending header with magic value: " << std::hex << header.magic;

  // create an iovec
  struct iovec iov[2];

  // first chunk: header
  iov[0].iov_base = &header;
  iov[0].iov_len = sizeof(control_frame_header);

  // second chunk: payload
  iov[1].iov_base = const_cast<uint8_t*>(payload_ptr);
  iov[1].iov_len = payload_size;

  // send the data
  if (send_all(iov, 2) == -1) {
    LOG_ERROR(TAG) << "failed to send complete packet";
    throw std::runtime_error("SensorClient::send_packet failed");
  }
}

int SensorClient::send_all(iovec* iov, int iovcnt) {
  size_t total_sent = 0;
  size_t total_to_send = 0;

  // find out how much we need to send
  for (int i = 0; i < iovcnt; ++i) {
    total_to_send += iov[i].iov_len;
  }

  while (total_sent < total_to_send) {
    ssize_t n = writev(m_socket_fd, iov, iovcnt);

    // there's been an error on write
    if (n < 0) {
      if (errno == EINTR) continue;
      return -1;
    }
    // no bytes have been written, connection closed
    if (n == 0) {
      return -1;
    }
    // otherwise, increment the number of bytes written
    total_sent += n;

    size_t remaining = n;
    while (remaining > 0 && iovcnt > 0) {
      if (remaining >= iov[0].iov_len) {
        remaining -= iov[0].iov_len;
        iov++;
        iovcnt--;
      } else {
        iov[0].iov_base = static_cast<char*>(iov[0].iov_base) + remaining;
        iov[0].iov_len -= remaining;
        remaining = 0;
      }
    }
  }

  return 0;
}

}  // namespace controls_middleware
