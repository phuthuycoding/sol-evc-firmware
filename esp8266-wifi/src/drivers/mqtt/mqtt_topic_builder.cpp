/**
 * @file mqtt_topic_builder.cpp
 * @brief MQTT Topic Builder Implementation
 */

#include "drivers/mqtt/mqtt_topic_builder.h"

namespace MQTTTopicBuilder {

void buildHeartbeat(char* buffer, size_t size, const DeviceConfig& config) {
    snprintf(buffer, size, "ocpp/%s/%s/heartbeat",
             config.stationId, config.deviceId);
}

void buildStatus(char* buffer, size_t size, const DeviceConfig& config, uint8_t connectorId) {
    snprintf(buffer, size, "ocpp/%s/%s/status/%u/status_notification",
             config.stationId, config.deviceId, connectorId);
}

void buildMeter(char* buffer, size_t size, const DeviceConfig& config, uint8_t connectorId) {
    snprintf(buffer, size, "ocpp/%s/%s/meter/%u/meter_values",
             config.stationId, config.deviceId, connectorId);
}

void buildTransaction(char* buffer, size_t size, const DeviceConfig& config, const char* type) {
    snprintf(buffer, size, "ocpp/%s/%s/transaction/%s",
             config.stationId, config.deviceId, type);
}

void buildBoot(char* buffer, size_t size, const DeviceConfig& config) {
    snprintf(buffer, size, "ocpp/%s/%s/event/0/boot_notification",
             config.stationId, config.deviceId);
}

void buildCommand(char* buffer, size_t size, const DeviceConfig& config) {
    snprintf(buffer, size, "ocpp/%s/%s/cmd/+",
             config.stationId, config.deviceId);
}

}  // namespace MQTTTopicBuilder
