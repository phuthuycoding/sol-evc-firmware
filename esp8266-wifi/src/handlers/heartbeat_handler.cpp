/**
 * @file heartbeat_handler.cpp
 * @brief Heartbeat Handler Implementation
 */

#include "handlers/heartbeat_handler.h"
#include "utils/logger.h"
#include <ArduinoJson.h>

bool HeartbeatHandler::execute(
    MQTTClient& mqtt,
    CustomWiFiManager& wifi,
    const DeviceConfig& config,
    uint32_t bootTime
) {
    // Check if MQTT is connected
    if (!mqtt.isConnected()) {
        LOG_WARN("Heartbeat", "MQTT not connected, skipping");
        return false;
    }

    // Build heartbeat payload
    StaticJsonDocument<256> doc;  // Stack allocated
    doc["msgId"] = String(millis());
    doc["uptime"] = (millis() - bootTime) / 1000;

    const WiFiStatus& wifiStatus = wifi.getStatus();
    doc["rssi"] = wifiStatus.rssi;
    doc["freeHeap"] = ESP.getFreeHeap();
    doc["heapFrag"] = ESP.getHeapFragmentation();

    // Serialize to string
    char payload[256];
    serializeJson(doc, payload, sizeof(payload));

    // Build topic
    char topic[128];
    MQTTTopicBuilder::buildHeartbeat(topic, sizeof(topic), config);

    // Publish
    MQTTError result = mqtt.publish(topic, payload, 1);

    if (result == MQTTError::SUCCESS) {
        LOG_DEBUG("Heartbeat", "Sent (heap: %u bytes)", ESP.getFreeHeap());
        return true;
    } else {
        LOG_ERROR("Heartbeat", "Failed to send");
        return false;
    }
}
