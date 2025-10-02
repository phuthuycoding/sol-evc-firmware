/**
 * @file device_manager.cpp
 * @brief Device Manager implementation
 */

#include "core/device_manager.h"
#include <ArduinoJson.h>

DeviceManager* DeviceManager::instance = nullptr;

DeviceManager::DeviceManager()
    : wifiManager(nullptr),
      mqttClient(nullptr),
      stm32() {

    memset(&systemStatus, 0, sizeof(systemStatus));
    instance = this;
}

DeviceManager::~DeviceManager() {
    if (wifiManager) delete wifiManager;
    if (mqttClient) delete mqttClient;
    instance = nullptr;
}

bool DeviceManager::init() {
    LOG_INFO("DeviceManager", "Initializing system...");

    systemStatus.bootTime = millis();
    systemStatus.initialized = false;

    // Initialize components
    if (!initializeConfig()) {
        LOG_ERROR("DeviceManager", "Config initialization failed");
        return false;
    }

    if (!initializeCommunication()) {
        LOG_ERROR("DeviceManager", "Communication initialization failed");
        return false;
    }

    if (!initializeNetwork()) {
        LOG_WARN("DeviceManager", "Network initialization failed, will retry");
    }

    systemStatus.initialized = true;
    LOG_INFO("DeviceManager", "System initialized successfully");

    return true;
}

bool DeviceManager::initializeConfig() {
    LOG_INFO("Config", "Loading configuration...");

    if (!configManager.init()) {
        return false;
    }

    const DeviceConfig& config = configManager.get();

    // Set log level from config
    Logger::getInstance().setLevel((LogLevel)config.system.logLevel);

    LOG_INFO("Config", "Station: %s, Device: %s", config.stationId, config.deviceId);

    return true;
}

bool DeviceManager::initializeNetwork() {
    const DeviceConfig& config = configManager.get();

    // Initialize WiFi
    wifiManager = new WiFiManager(config);
    if (wifiManager->init() != WiFiError::SUCCESS) {
        LOG_ERROR("WiFi", "Initialization failed");
        return false;
    }

    // Try to connect
    WiFiError wifiErr = wifiManager->connect();
    if (wifiErr != WiFiError::SUCCESS) {
        LOG_WARN("WiFi", "Connection failed, starting AP mode");
        wifiManager->startConfigPortal();
        return false;
    }

    // Initialize MQTT
    mqttClient = new MQTTClient(config);
    mqttClient->setCallback(mqttMessageCallback);

    MQTTError mqttErr = mqttClient->connect();
    if (mqttErr != MQTTError::SUCCESS) {
        LOG_WARN("MQTT", "Connection failed, will retry");
        return false;
    }

    LOG_INFO("Network", "WiFi and MQTT connected");
    return true;
}

bool DeviceManager::initializeCommunication() {
    stm32.setCallback(stm32PacketCallback);

    if (stm32.init() != UARTError::SUCCESS) {
        return false;
    }

    LOG_INFO("STM32", "Communication initialized");
    return true;
}

void DeviceManager::run() {
    if (!systemStatus.initialized) return;

    // Handle STM32 communication (always)
    stm32.handle();

    // Handle WiFi
    if (wifiManager) {
        wifiManager->handle();
    }

    // Handle MQTT (only if WiFi connected)
    if (mqttClient && wifiManager && wifiManager->isConnected()) {
        mqttClient->handle();

        // Send heartbeat
        const DeviceConfig& config = configManager.get();
        if (millis() - systemStatus.lastHeartbeat > config.system.heartbeatInterval) {
            handleHeartbeat();
            systemStatus.lastHeartbeat = millis();
        }
    }

    // Request meter values periodically
    handleMeterValues();
}

void DeviceManager::handleHeartbeat() {
    if (!mqttClient || !mqttClient->isConnected()) return;

    const DeviceConfig& config = configManager.get();

    StaticJsonDocument<512> doc;
    doc["msgId"] = String(millis());
    doc["uptime"] = (millis() - systemStatus.bootTime) / 1000;
    doc["rssi"] = wifiManager ? wifiManager->getStatus().rssi : 0;
    doc["freeHeap"] = ESP.getFreeHeap();

    String payload;
    serializeJson(doc, payload);

    char topic[128];
    MQTTTopicBuilder::buildHeartbeat(topic, sizeof(topic), config);

    mqttClient->publish(topic, payload.c_str());
    LOG_DEBUG("Heartbeat", "Sent (heap: %u bytes)", ESP.getFreeHeap());
}

void DeviceManager::handleMeterValues() {
    static uint32_t lastRequest = 0;

    if (millis() - lastRequest > 5000) {
        uart_packet_t packet = STM32Commands::createMeterValuesRequest(0);
        stm32.sendPacket(packet);
        lastRequest = millis();
    }
}

void DeviceManager::mqttMessageCallback(const char* topic, const char* payload, uint16_t length) {
    if (!instance) return;

    LOG_INFO("MQTT", "RX: %s -> %s", topic, payload);
    // TODO: Handle MQTT commands
}

void DeviceManager::stm32PacketCallback(const uart_packet_t* packet) {
    if (!instance) return;

    LOG_DEBUG("STM32", "RX: CMD=0x%02X", packet->cmd_type);

    switch (packet->cmd_type) {
        case CMD_MQTT_PUBLISH:
            if (instance->mqttClient) {
                StaticJsonDocument<512> doc;
                if (deserializeJson(doc, (char*)packet->payload) == DeserializationError::Ok) {
                    const char* topic = doc["topic"];
                    const char* data = doc["data"];
                    instance->mqttClient->publish(topic, data);
                }
            }
            instance->stm32.sendAck(packet->sequence, STATUS_SUCCESS);
            break;

        default:
            LOG_WARN("STM32", "Unknown command: 0x%02X", packet->cmd_type);
            instance->stm32.sendAck(packet->sequence, STATUS_INVALID);
            break;
    }
}
