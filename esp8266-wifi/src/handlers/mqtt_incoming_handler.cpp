/**
 * @file mqtt_incoming_handler.cpp
 * @brief MQTT Incoming Message Handler Implementation
 */

#include "handlers/mqtt_incoming_handler.h"
#include "utils/logger.h"
#include <string.h>

void MQTTIncomingHandler::execute(
    const char* topic,
    const char* payload,
    uint16_t length,
    STM32Communicator& stm32,
    const DeviceConfig& config
) {
    LOG_INFO("MQTTIn", "RX: %s -> %.*s", topic, length, payload);

    // Check if this is a command topic for this device
    if (!isCommandTopic(topic, config)) {
        LOG_WARN("MQTTIn", "Topic not for this device, ignoring");
        return;
    }

    // Forward to STM32 via UART
    forwardToSTM32(topic, payload, length, stm32);
}

bool MQTTIncomingHandler::isCommandTopic(const char* topic, const DeviceConfig& config) {
    // Expected format: ocpp/{stationId}/{deviceId}/cmd/...
    char expectedPrefix[128];
    snprintf(expectedPrefix, sizeof(expectedPrefix),
             "ocpp/%s/%s/cmd/",
             config.stationId,
             config.deviceId);

    return strncmp(topic, expectedPrefix, strlen(expectedPrefix)) == 0;
}

void MQTTIncomingHandler::forwardToSTM32(
    const char* topic,
    const char* payload,
    uint16_t length,
    STM32Communicator& stm32
) {
    // Create UART packet to forward MQTT message to STM32
    uart_packet_t packet;
    uart_init_packet(&packet, RSP_MQTT_RECEIVED, 0);  // Sequence 0 for async messages

    // Build combined payload: topic + data
    // Format: topic\0payload
    size_t topicLen = strlen(topic);
    size_t totalLen = topicLen + 1 + length;  // +1 for null terminator

    if (totalLen > UART_MAX_PAYLOAD) {
        LOG_ERROR("MQTTIn", "Message too large: %d bytes (max %d)", totalLen, UART_MAX_PAYLOAD);
        return;
    }

    // Copy topic
    memcpy(packet.payload, topic, topicLen);
    packet.payload[topicLen] = '\0';

    // Copy payload
    memcpy(packet.payload + topicLen + 1, payload, length);

    packet.length = totalLen;

    // Send to STM32
    UARTError result = stm32.sendPacket(packet);

    if (result == UARTError::SUCCESS) {
        LOG_DEBUG("MQTTIn", "Forwarded to STM32: %s", topic);
    } else {
        LOG_ERROR("MQTTIn", "Failed to forward to STM32");
    }
}
