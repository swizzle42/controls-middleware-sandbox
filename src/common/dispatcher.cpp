#include "controls_middleware/dispatcher.h"

#include <iostream>

#include "controls_middleware/logging.h"

static const char* TAG = "dispatcher";

namespace controls_middleware {

void dispatcher(const Message& message) {
  switch (message.payload_type()) {
    case Payload_Telemetry: {
      auto telemetry = message.payload_as_Telemetry();
      LOG_INFO(TAG) << "[BROKER_DATA_INGEST]------------[TYPE: TELEMETRY]\n"
                    << "Device ID: " << telemetry->device_id() << "\n"
                    << "Metric Value: " << telemetry->value() << "\n"
                    << "Timestamp: " << telemetry->timestamp() << "\n"
                    << "Status: " << static_cast<int>(telemetry->status())
                    << "\n\n";
      break;
    }

    case Payload_Command: {
      auto command = message.payload_as_Command();
      LOG_INFO(TAG) << "\n[BROKER_DATA_INGEST]------------[TYPE: COMMAND]\n"
                    << "Device ID: " << command->device_id() << "\n"
                    << "Timestamp: " << command->timestamp() << "\n"
                    << "Command: " << command->cmd()->c_str() << "\n"
                    << "\n\n";
      break;
    }

    case Payload_Heartbeat: {
      auto heartbeat = message.payload_as_Heartbeat();
      LOG_INFO(TAG) << "\n[BROKER_DATA_INGEST]------------[TYPE: HEARTBEAT]\n"
                    << "Device ID: " << heartbeat->device_id() << "\n"
                    << "Timestamp: " << heartbeat->timestamp() << "\n"
                    << "\n\n";
      break;
    }

    case Payload_NONE:
    default: {
      LOG_WARN(TAG) << "unknown message type";
      break;
    }
  }
}

}  // namespace controls_middleware
