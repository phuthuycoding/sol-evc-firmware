/**
 * @file mqtt_topic_builder.h
 * @brief MQTT Topic Builder (shared utility)
 * @version 1.0.0
 *
 * Centralized topic construction for OCPP messages
 * Format: ocpp/{stationId}/{deviceId}/{category}/{connector}/{message_type}
 */

#ifndef MQTT_TOPIC_BUILDER_H
#define MQTT_TOPIC_BUILDER_H

#include <Arduino.h>
#include "../config/unified_config.h"

/**
 * @brief MQTT Topic Builder (stateless utility)
 *
 * Provides centralized topic construction to avoid duplication
 * Used by both MQTTClient and OCPPMessageHandler
 */
namespace MQTTTopicBuilder {

/**
 * @brief Build heartbeat topic
 * Format: ocpp/{station}/{device}/heartbeat
 */
void buildHeartbeat(char* buffer, size_t size, const DeviceConfig& config);

/**
 * @brief Build status notification topic
 * Format: ocpp/{station}/{device}/status/{connector}/status_notification
 */
void buildStatus(char* buffer, size_t size, const DeviceConfig& config, uint8_t connectorId);

/**
 * @brief Build meter values topic
 * Format: ocpp/{station}/{device}/meter/{connector}/meter_values
 */
void buildMeter(char* buffer, size_t size, const DeviceConfig& config, uint8_t connectorId);

/**
 * @brief Build transaction topic
 * Format: ocpp/{station}/{device}/transaction/{type}
 * @param type "start" or "stop"
 */
void buildTransaction(char* buffer, size_t size, const DeviceConfig& config, const char* type);

/**
 * @brief Build boot notification topic
 * Format: ocpp/{station}/{device}/event/0/boot_notification
 */
void buildBoot(char* buffer, size_t size, const DeviceConfig& config);

/**
 * @brief Build command subscription topic (wildcard)
 * Format: ocpp/{station}/{device}/cmd/+
 */
void buildCommand(char* buffer, size_t size, const DeviceConfig& config);

}  // namespace MQTTTopicBuilder

#endif  // MQTT_TOPIC_BUILDER_H
