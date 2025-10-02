/**
 * @file stm32_command_handler.cpp
 * @brief STM32 Command Handler Implementation
 */

#include "handlers/stm32_command_handler.h"
#include "utils/logger.h"
#include <ArduinoJson.h>

void STM32CommandHandler::execute(
    const uart_packet_t& packet,
    STM32Communicator& stm32,
    MQTTClient& mqtt
) {
    LOG_DEBUG("STM32Cmd", "RX: CMD=0x%02X, SEQ=%d", packet.cmd_type, packet.sequence);

    switch (packet.cmd_type) {
        case CMD_MQTT_PUBLISH:
            handleMqttPublish(packet, stm32, mqtt);
            break;

        case CMD_GET_TIME:
            handleGetTime(packet, stm32);
            break;

        case CMD_WIFI_STATUS:
            handleWiFiStatus(packet, stm32);
            break;

        default:
            LOG_WARN("STM32Cmd", "Unknown command: 0x%02X", packet.cmd_type);
            stm32.sendAck(packet.sequence, STATUS_INVALID);
            break;
    }
}

void STM32CommandHandler::handleMqttPublish(
    const uart_packet_t& packet,
    STM32Communicator& stm32,
    MQTTClient& mqtt
) {
    // Parse JSON payload from STM32
    StaticJsonDocument<512> doc;
    DeserializationError error = deserializeJson(doc, (char*)packet.payload, packet.length);

    if (error) {
        LOG_ERROR("STM32Cmd", "JSON parse error: %s", error.c_str());
        stm32.sendAck(packet.sequence, STATUS_INVALID);
        return;
    }

    // Extract topic and data
    const char* topic = doc["topic"];
    const char* data = doc["data"];

    if (!topic || !data) {
        LOG_ERROR("STM32Cmd", "Missing topic or data");
        stm32.sendAck(packet.sequence, STATUS_INVALID);
        return;
    }

    // Publish to MQTT
    MQTTError result = mqtt.publish(topic, data, 1);

    if (result == MQTTError::SUCCESS) {
        LOG_DEBUG("STM32Cmd", "MQTT published: %s", topic);
        stm32.sendAck(packet.sequence, STATUS_SUCCESS);
    } else {
        LOG_ERROR("STM32Cmd", "MQTT publish failed");
        stm32.sendAck(packet.sequence, STATUS_ERROR);
    }
}

void STM32CommandHandler::handleGetTime(
    const uart_packet_t& packet,
    STM32Communicator& stm32
) {
    // Create time response packet
    uart_packet_t response;
    uart_init_packet(&response, RSP_TIME_DATA, packet.sequence);

    // Build time payload
    time_data_payload_t timeData;
    timeData.unix_timestamp = millis() / 1000;  // TODO: Use NTP time when available
    timeData.timezone_offset = 0;  // UTC (TODO: Get from config)
    timeData.ntp_synced = 0;       // Not synced yet (TODO: Check NTP status)

    memcpy(response.payload, &timeData, sizeof(timeData));
    response.length = sizeof(timeData);

    // Send response
    stm32.sendPacket(response);
    LOG_DEBUG("STM32Cmd", "Time sent: %u (synced: %d)", timeData.unix_timestamp, timeData.ntp_synced);
}

void STM32CommandHandler::handleWiFiStatus(
    const uart_packet_t& packet,
    STM32Communicator& stm32
) {
    // Create WiFi status response
    uart_packet_t response;
    uart_init_packet(&response, RSP_WIFI_STATUS, packet.sequence);

    // Use proper payload structure
    wifi_status_payload_t wifiData;
    wifiData.wifi_connected = WiFi.isConnected() ? 1 : 0;
    wifiData.mqtt_connected = 0;  // TODO: Get from MQTTClient
    wifiData.rssi = WiFi.RSSI();
    wifiData.uptime = millis() / 1000;

    if (wifiData.wifi_connected) {
        IPAddress ip = WiFi.localIP();
        wifiData.ip_address[0] = ip[0];
        wifiData.ip_address[1] = ip[1];
        wifiData.ip_address[2] = ip[2];
        wifiData.ip_address[3] = ip[3];
    } else {
        memset(wifiData.ip_address, 0, 4);
    }

    memcpy(response.payload, &wifiData, sizeof(wifiData));
    response.length = sizeof(wifiData);

    // Send response
    stm32.sendPacket(response);
    LOG_DEBUG("STM32Cmd", "WiFi status: connected=%d, RSSI=%d", wifiData.wifi_connected, wifiData.rssi);
}
