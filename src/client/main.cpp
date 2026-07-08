#include <chrono>
#include <cstdlib>
#include <iostream>

#include "client.h"
#include "controls_middleware/packet.h"

controls_middleware::SensorPacket generate_telemetry(uint16_t id,
                                                     float metric_value) {
  // create timestamp
  auto now = std::chrono::system_clock::now();
  auto duration = now.time_since_epoch();
  auto millis =
      std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();

  return controls_middleware::SensorPacket{
      .device_id = id,
      .status = 1,
      .padding = 0,
      .timestamp = static_cast<uint64_t>(millis),
      .value = metric_value};
}

int main(int argc, char *argv[]) {
  int id{101};

  if (argc > 1) {
    std::cout << "Sensor Begin with " << (argc - 1) << " arguments: ";
    for (int i = 1; i < argc; ++i) {
      std::cout << argv[i] << " ";
    }
    std::cout << std::endl;

    id = atoi(argv[1]);
  } else {
    std::cout << "Sensor Begin..." << std::endl;
  }

  try {
    controls_middleware::SensorClient client("127.0.0.1", 8080);
    std::cout << "Successful connection to Server.";

    auto payload = generate_telemetry(id, 42.123f);

    std::cout << "Sending payload..." << std::endl;
    client.send_packet(payload);

    std::cout << "Transmission successful. Exiting." << std::endl;

  } catch (const std::exception &e) {
    std::cerr << "Runtime error: " << e.what() << std::endl;
    return 1;
  }

  return 0;
}