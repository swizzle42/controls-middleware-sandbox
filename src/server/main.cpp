#include <atomic>
#include <chrono>
#include <csignal>
#include <iostream>
#include <thread>

#include "controls_middleware/logging.h"
#include "server.h"

static const char* TAG = "broker";

std::atomic_bool running = true;

void signal_handler(int) { running = false; }

int main() {
  logging::set_level(logging::Level::Debug);
  LOG_INFO(TAG) << "begin initialisation";

  std::signal(SIGINT, signal_handler);
  std::signal(SIGTERM, signal_handler);

  try {
    controls_middleware::SensorServer server("127.0.0.1", 8080);

    auto on_packet_recv = [](const controls_middleware::sensor_packet& packet) {
      std::cout << "\n[BROKER_DATA_INGEST]------------\n"
                << "Device ID: " << packet.device_id << "\n"
                << "Metric Value: " << packet.value << "\n"
                << "Timestamp: " << packet.timestamp << "\n"
                << "Status: " << static_cast<int>(packet.status) << "\n\n";
    };

    server.start(on_packet_recv);
    LOG_INFO(TAG) << "sensor server started in the background";

    LOG_INFO(TAG) << "main loop alive, awaiting sensor telemetry...";
    while (running) {
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    LOG_INFO(TAG) << "shutting down broker server";
    server.stop();

  } catch (std::exception& e) {
    std::cerr << "Fatal server exception: " << e.what() << std::endl;
    return 1;
  }

  return 0;
}