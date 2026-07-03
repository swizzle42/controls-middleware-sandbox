#include <chrono>
#include <iostream>
#include <thread>

#include "server.h"

int main() {
  std::cout << "Starting Broker Server." << std::endl;

  try {
    controls_middleware::SensorServer server("127.0.0.1", 8080);

    auto on_packet_recv = [](const controls_middleware::SensorPacket& packet) {
      std::cout << "\n[BROKER_DATA_INGEST]------------\n"
                << "Device ID: " << packet.device_id << "\n"
                << "Metric Value: " << packet.value << "\n"
                << "Timestamp: " << packet.timestamp << "\n"
                << "Status: " << static_cast<int>(packet.status) << "\n";
    };

    server.start(on_packet_recv);
    std::cout << "Server started in the background..." << std::endl;

    std::cout << "Control room main loop alive. Awaiting sensor telemetry...\n";
    std::this_thread::sleep_for(std::chrono::seconds(30));

    std::cout << "Shutting down control room broker." << std::endl;

  } catch (std::exception& e) {
    std::cerr << "Fatal server exception: " << e.what() << std::endl;
    return 1;
  }

  return 0;
}