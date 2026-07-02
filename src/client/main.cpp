#include <chrono>
#include <iostream>

#include "client.h"
#include "controls_middleware/packet.h"

controls_middleware::sensor_packet generate_telemetry(uint16_t id,
                                                      float metric_value) {
  // create timestamp
  auto now = std::chrono::system_clock::now();
  auto duration = now.time_since_epoch();
  auto millis =
      std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();

  return controls_middleware::sensor_packet{
      .device_id = id,
      .status = 1,
      .padding = 0,
      .timestamp = static_cast<uint64_t>(millis),
      .value = metric_value};
}

int main() {
  std::cout << "Sensor Begin..." << std::endl;

  try {
    controls_middleware::SensorClient client("127.0.0.1", 8080);
    std::cout << "Successful connection to Server.";

    auto payload = generate_telemetry(101, 42.123f);

    std::cout << "Sending payload..." << std::endl;
    client.send_packet(payload);

    std::cout << "Transmission successful. Exiting.";

  } catch (const std::exception &e) {
    std::cerr << "Runtime error: " << e.what() << std::endl;
    return 1;
  }

  return 0;
}