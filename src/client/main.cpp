#include <chrono>
#include <csignal>
#include <cstdlib>
#include <iostream>
#include <string_view>
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

flatbuffers::Offset<controls_middleware::Message> generate_telemetry_message(
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

  auto message_offset = controls_middleware::CreateMessage(
      builder, controls_middleware::Payload_Telemetry,
      telemetry_offset.Union());

  return message_offset;
}

flatbuffers::Offset<controls_middleware::Message> generate_command_message(
    flatbuffers::FlatBufferBuilder &builder, int id, std::string_view cmd) {
  // create timestamp
  auto now = std::chrono::system_clock::now();
  auto duration = now.time_since_epoch();
  auto millis =
      std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();

  uint16_t device_id = id;
  uint64_t timestamp = static_cast<uint64_t>(millis);

  auto cmd_string = builder.CreateString(cmd);

  auto command_offset = controls_middleware::CreateCommand(
      builder, device_id, timestamp, cmd_string);

  auto message_offset = controls_middleware::CreateMessage(
      builder, controls_middleware::Payload_Command, command_offset.Union());

  return message_offset;
}

flatbuffers::Offset<controls_middleware::Message> generate_heartbeat_message(
    flatbuffers::FlatBufferBuilder &builder, int id) {
  // create timestamp
  auto now = std::chrono::system_clock::now();
  auto duration = now.time_since_epoch();
  auto millis =
      std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();

  uint16_t device_id = id;
  uint64_t timestamp = static_cast<uint64_t>(millis);

  auto heartbeat_offset =
      controls_middleware::CreateHeartbeat(builder, device_id, timestamp);

  auto message_offset = controls_middleware::CreateMessage(
      builder, controls_middleware::Payload_Heartbeat,
      heartbeat_offset.Union());

  return message_offset;
}

flatbuffers::Offset<controls_middleware::Message> generate_empty_message(
    flatbuffers::FlatBufferBuilder &builder) {
  return controls_middleware::CreateMessage(builder);
}

int main(int argc, char *argv[]) {
  signal(SIGINT, signal_handler);

  if (argc != 3) return -1;

  // get the client id number
  int id = atoi(argv[1]);

  // convert message type
  auto convert_msg_type = [](const int payload_type) {
    if (payload_type >= controls_middleware::Payload_MIN ||
        payload_type <= controls_middleware::Payload_MAX) {
      return static_cast<controls_middleware::Payload>(payload_type);
    } else {
      return controls_middleware::Payload_NONE;
    }
  };

  // get the client payload type
  auto payload_type = convert_msg_type(atoi(argv[2]));

  LOG_INFO(TAG) << "sensor begin with id=" << id;

  try {
    controls_middleware::SensorClient client("127.0.0.1", 8080);
    LOG_INFO(TAG) << "successful connection to server";

    // create message builder
    flatbuffers::FlatBufferBuilder builder(1024);

    while (true) {
      // generate the payload
      switch (payload_type) {
        case controls_middleware::Payload_Telemetry: {
          auto message = generate_telemetry_message(builder, id, 42.123f);
          builder.Finish(message);
          break;
        }

        case controls_middleware::Payload_Command: {
          auto message =
              generate_command_message(builder, id, "this is a command");
          builder.Finish(message);
          break;
        }

        case controls_middleware::Payload_Heartbeat: {
          auto message = generate_heartbeat_message(builder, id);
          builder.Finish(message);
          break;
        }

        default:
          auto message = generate_empty_message(builder);
          builder.Finish(message);
          break;
      }

      LOG_INFO(TAG) << "sending message";
      client.send_packet(builder, controls_middleware::msg_type_enum::TELEMETRY,
                         1);
      LOG_INFO(TAG) << "message sent successfully";

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