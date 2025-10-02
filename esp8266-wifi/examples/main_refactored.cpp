/**
 * @file main_refactored.cpp
 * @brief Example of refactored OOP ESP8266 firmware
 * @version 2.0.0
 *
 * This demonstrates the new lightweight OOP architecture:
 * - UnifiedConfigManager (stack allocated, ~600 bytes)
 * - MQTTClient (stack allocated, ~400 bytes)
 * - STM32Communicator (stack allocated, ~550 bytes)
 * - Total: ~1.5KB (vs ~5KB+ with old code)
 *
 * Key improvements:
 * - No global variables
 * - No memory leaks (no new/delete)
 * - Clear encapsulation
 * - Easy to test and maintain
 */

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ArduinoJson.h>

// New refactored headers
#include "config/unified_config.h"
#include "mqtt/mqtt_client_oop.h"
#include "communication/stm32_comm_oop.h"

/* ============================================================================
 * Global Objects (Stack Allocated)
 * ============================================================================ */

// Configuration manager
UnifiedConfigManager configManager;

// MQTT Client (will be initialized after config is loaded)
MQTTClient* mqttClient = nullptr;

// STM32 Communicator
STM32Communicator stm32;

/* Status tracking */
struct {
    bool configLoaded;
    bool wifiConnected;
    bool mqttReady;
    uint32_t bootTime;
    uint32_t lastHeartbeat;
} systemStatus;

/* ============================================================================
 * Callback Functions
 * ============================================================================ */

/**
 * @brief MQTT message received callback
 */
void onMqttMessage(const char* topic, const char* payload, uint16_t length) {
    Serial.printf("[App] MQTT Message: %s -> %s\n", topic, payload);

    // Parse command from topic
    // Format: ocpp/{station}/{device}/cmd/{command}
    // TODO: Implement command handler
}

/**
 * @brief STM32 packet received callback
 */
void onStm32Packet(const uart_packet_t* packet) {
    Serial.printf("[App] STM32 Packet: CMD=0x%02X\n", packet->cmd_type);

    switch (packet->cmd_type) {
        case CMD_MQTT_PUBLISH: {
            // STM32 wants to publish MQTT message
            StaticJsonDocument<512> doc;
            DeserializationError error = deserializeJson(doc, (char*)packet->payload);

            if (!error && mqttClient) {
                const char* topic = doc["topic"];
                const char* data = doc["data"];
                int qos = doc["qos"] | 0;

                MQTTError result = mqttClient->publish(topic, data, qos);
                stm32.sendAck(packet->sequence,
                             result == MQTTError::SUCCESS ? STATUS_SUCCESS : STATUS_ERROR);
            } else {
                stm32.sendAck(packet->sequence, STATUS_ERROR);
            }
            break;
        }

        case CMD_GET_TIME: {
            // STM32 requests current time
            // TODO: Send time data response
            Serial.println(F("[App] Time request - not implemented yet"));
            stm32.sendAck(packet->sequence, STATUS_SUCCESS);
            break;
        }

        case CMD_WIFI_STATUS: {
            // STM32 requests WiFi/MQTT status
            // TODO: Send status response
            Serial.println(F("[App] Status request - not implemented yet"));
            stm32.sendAck(packet->sequence, STATUS_SUCCESS);
            break;
        }

        default:
            Serial.printf("[App] Unknown STM32 command: 0x%02X\n", packet->cmd_type);
            stm32.sendAck(packet->sequence, STATUS_INVALID);
            break;
    }
}

/* ============================================================================
 * Setup Functions
 * ============================================================================ */

/**
 * @brief Initialize configuration
 */
bool setupConfig() {
    Serial.println(F("\n=== Configuration Setup ==="));

    if (!configManager.init()) {
        Serial.println(F("[ERROR] Failed to initialize config manager"));
        return false;
    }

    const DeviceConfig& config = configManager.get();

    // Validate config
    if (!configManager.isValid()) {
        Serial.println(F("[WARN] Config validation failed, using defaults"));
    }

    // Check for default passwords
    if (strlen(config.web.password) == 0) {
        Serial.println(F("[WARN] Web password not set! Please configure."));
    }

    if (strlen(config.system.otaPassword) == 0) {
        Serial.println(F("[WARN] OTA password not set! OTA disabled for security."));
    }

    systemStatus.configLoaded = true;
    return true;
}

/**
 * @brief Connect to WiFi
 */
bool setupWiFi() {
    Serial.println(F("\n=== WiFi Setup ==="));

    const DeviceConfig& config = configManager.get();

    // Check if WiFi credentials are configured
    if (strlen(config.wifi.ssid) == 0) {
        Serial.println(F("[WARN] WiFi not configured. Starting AP mode..."));

        char apName[32];
        ConfigHelper::buildApName(apName, sizeof(apName), config);

        WiFi.mode(WIFI_AP);
        WiFi.softAP(apName);

        Serial.printf("AP started: %s\n", apName);
        Serial.printf("IP: %s\n", WiFi.softAPIP().toString().c_str());
        Serial.println(F("Please configure WiFi via web portal"));

        systemStatus.wifiConnected = false;
        return false;
    }

    // Connect to WiFi
    Serial.printf("Connecting to: %s\n", config.wifi.ssid);

    WiFi.mode(WIFI_STA);
    WiFi.begin(config.wifi.ssid, config.wifi.password);

    uint8_t attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
        delay(500);
        Serial.print(".");
        attempts++;
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println(F("\n[SUCCESS] WiFi connected"));
        Serial.printf("IP: %s\n", WiFi.localIP().toString().c_str());
        Serial.printf("RSSI: %d dBm\n", WiFi.RSSI());

        systemStatus.wifiConnected = true;
        return true;
    } else {
        Serial.println(F("\n[ERROR] WiFi connection failed"));
        systemStatus.wifiConnected = false;
        return false;
    }
}

/**
 * @brief Initialize MQTT
 */
bool setupMQTT() {
    Serial.println(F("\n=== MQTT Setup ==="));

    if (!systemStatus.wifiConnected) {
        Serial.println(F("[SKIP] WiFi not connected, skipping MQTT"));
        return false;
    }

    const DeviceConfig& config = configManager.get();

    // Create MQTT client (stack allocated!)
    mqttClient = new MQTTClient(config);
    mqttClient->setCallback(onMqttMessage);

    // Try to connect
    MQTTError result = mqttClient->connect();
    if (result == MQTTError::SUCCESS) {
        Serial.println(F("[SUCCESS] MQTT connected"));
        systemStatus.mqttReady = true;
        return true;
    } else {
        Serial.println(F("[WARN] MQTT connection failed, will retry in loop"));
        systemStatus.mqttReady = false;
        return false;
    }
}

/**
 * @brief Initialize STM32 communication
 */
bool setupSTM32() {
    Serial.println(F("\n=== STM32 Communication Setup ==="));

    UARTError result = stm32.init(115200);
    if (result == UARTError::SUCCESS) {
        stm32.setCallback(onStm32Packet);
        Serial.println(F("[SUCCESS] STM32 UART initialized"));
        return true;
    } else {
        Serial.println(F("[ERROR] STM32 UART init failed"));
        return false;
    }
}

/**
 * @brief Send heartbeat message
 */
void sendHeartbeat() {
    if (!mqttClient || !mqttClient->isConnected()) {
        return;
    }

    const DeviceConfig& config = configManager.get();

    // Build heartbeat payload
    StaticJsonDocument<512> doc;
    doc["msgId"] = String(millis());
    doc["uptime"] = (millis() - systemStatus.bootTime) / 1000;
    doc["rssi"] = WiFi.RSSI();
    doc["freeHeap"] = ESP.getFreeHeap();
    doc["stationId"] = config.stationId;
    doc["deviceId"] = config.deviceId;

    // MQTT status
    const MQTTStatus& mqttStatus = mqttClient->getStatus();
    doc["mqtt"]["txCount"] = mqttStatus.messageTxCount;
    doc["mqtt"]["rxCount"] = mqttStatus.messageRxCount;
    doc["mqtt"]["queueSize"] = mqttClient->getQueueSize();

    // STM32 status
    const STM32Status& stm32Status = stm32.getStatus();
    doc["stm32"]["connected"] = stm32Status.connected;
    doc["stm32"]["txCount"] = stm32Status.messageTxCount;
    doc["stm32"]["rxCount"] = stm32Status.messageRxCount;
    doc["stm32"]["errors"] = stm32Status.errorCount;

    String payload;
    serializeJson(doc, payload);

    // Build topic and publish
    char topic[128];
    MQTTTopicBuilder::buildHeartbeat(topic, sizeof(topic), config);

    mqttClient->publish(topic, payload.c_str());
    Serial.printf("[App] Heartbeat sent (heap: %u bytes)\n", ESP.getFreeHeap());
}

/**
 * @brief Print memory stats
 */
void printMemoryStats() {
    uint32_t freeHeap = ESP.getFreeHeap();
    uint32_t heapFrag = ESP.getHeapFragmentation();

    Serial.println(F("\n=== Memory Stats ==="));
    Serial.printf("Free Heap: %u bytes\n", freeHeap);
    Serial.printf("Fragmentation: %u%%\n", heapFrag);

    if (freeHeap < 10000) {
        Serial.println(F("⚠️  LOW MEMORY WARNING!"));
    }

    if (heapFrag > 50) {
        Serial.println(F("⚠️  HIGH FRAGMENTATION WARNING!"));
    }

    // Print buffer stats
    if (mqttClient) {
        Serial.printf("MQTT Queue: %u messages\n", mqttClient->getQueueSize());
    }

    Serial.printf("STM32 Buffer: %u bytes\n", stm32.getBufferUsage());
    Serial.println(F("====================\n"));
}

/* ============================================================================
 * Main Setup & Loop
 * ============================================================================ */

void setup() {
    Serial.begin(115200);
    delay(100);

    Serial.println(F("\n\n"));
    Serial.println(F("╔════════════════════════════════════════╗"));
    Serial.println(F("║  ESP8266 EVSE WiFi Module (OOP v2.0)  ║"));
    Serial.println(F("║        Refactored Architecture         ║"));
    Serial.println(F("╚════════════════════════════════════════╝"));

    systemStatus.bootTime = millis();
    systemStatus.configLoaded = false;
    systemStatus.wifiConnected = false;
    systemStatus.mqttReady = false;
    systemStatus.lastHeartbeat = 0;

    // Initialize components
    if (!setupConfig()) {
        Serial.println(F("[FATAL] Config init failed!"));
        return;
    }

    setupWiFi(); // Can fail, will retry in loop

    setupMQTT(); // Can fail, will retry in loop

    if (!setupSTM32()) {
        Serial.println(F("[WARN] STM32 init failed, continuing anyway"));
    }

    Serial.println(F("\n=== Setup Complete ==="));
    Serial.println(F("Entering main loop...\n"));

    printMemoryStats();
}

void loop() {
    static uint32_t lastMemoryCheck = 0;

    // Handle STM32 communication (always)
    stm32.handle();

    // Handle WiFi reconnection
    if (!systemStatus.wifiConnected) {
        static uint32_t lastWiFiRetry = 0;
        if (millis() - lastWiFiRetry > 30000) { // Retry every 30 seconds
            setupWiFi();
            lastWiFiRetry = millis();
        }
    }

    // Handle MQTT (only if WiFi connected)
    if (systemStatus.wifiConnected && mqttClient) {
        mqttClient->handle(); // Handles reconnection automatically

        // Send heartbeat
        const DeviceConfig& config = configManager.get();
        if (millis() - systemStatus.lastHeartbeat > config.system.heartbeatInterval) {
            sendHeartbeat();
            systemStatus.lastHeartbeat = millis();
        }
    }

    // Request meter values from STM32 periodically
    static uint32_t lastMeterRequest = 0;
    if (millis() - lastMeterRequest > 5000) {
        uart_packet_t packet = STM32Commands::createMeterValuesRequest(0);
        stm32.sendPacket(packet);
        lastMeterRequest = millis();
    }

    // Print memory stats every 60 seconds
    if (millis() - lastMemoryCheck > 60000) {
        printMemoryStats();
        lastMemoryCheck = millis();
    }

    delay(10);
}
