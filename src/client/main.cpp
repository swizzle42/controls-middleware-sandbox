#include <chrono>
#include <csignal>
#include <cstdlib>
#include <iostream>
#include <thread>

#include "client.h"
#include "controls_middleware/logging.h"
#include "controls_middleware/packet.h"
#include "flatbuffers/flatbuffers.h"
#include "telemetry_generated.h"

static const char *TAG = "SensorClient";

void signal_handler(int sig) {
  LOG_INFO(TAG) << "signal caught (" << sig << "), exiting...";
  exit(sig);
}

flatbuffers::Offset<controls_middleware::Telemetry> generate_telemetry(
    flatbuffers::FlatBufferBuilder &builder, int id, float metric_value) {
  // create timestamp
  auto now = std::chrono::system_clock::now();
  auto duration = now.time_since_epoch();
  auto millis =
      std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();

  uint16_t device_id = id;
  uint8_t status = 1;
  uint64_t timestamp = static_cast<uint64_t>(millis);
  float value = metric_value;

  auto telemetry_offset = controls_middleware::CreateTelemetry(
      builder, device_id, status, timestamp, value);

  return telemetry_offset;
}

int main(int argc, char *argv[]) {
  signal(SIGINT, signal_handler);

  int id = argc > 1 ? atoi(argv[1]) : 1;
  LOG_INFO(TAG) << "sensor begin with id=" << id;

  try {
    controls_middleware::SensorClient client("127.0.0.1", 8080);
    LOG_INFO(TAG) << "successful connection to server";

    // create telemetry builder
    flatbuffers::FlatBufferBuilder builder(1024);

    while (true) {
      // generate the payload
      auto telemetry = generate_telemetry(builder, id, 42.123f);
      builder.Finish(telemetry);

      LOG_INFO(TAG) << "sending payload";
      client.send_packet(builder, controls_middleware::msg_type_enum::TELEMETRY,
                         1);
      LOG_INFO(TAG) << "payload sent successfully";

      // clear the builder
      builder.Clear();
      std::this_thread::sleep_for(std::chrono::milliseconds(2000));
    }

  } catch (const std::exception &e) {
    std::cerr << "Runtime error: " << e.what() << std::endl;
    return 1;
  }

  return 0;
}