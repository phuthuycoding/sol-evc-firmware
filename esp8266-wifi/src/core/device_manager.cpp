/**
 * @file device_manager.cpp
 * @brief Device Manager implementation
 */

#include "core/device_manager.h"
#include "handlers/heartbeat_handler.h"
#include "handlers/stm32_command_handler.h"
#include "handlers/mqtt_incoming_handler.h"
#include "handlers/ocpp_message_handler.h"
#include <ArduinoJson.h>

// Note: All driver headers now in drivers/ subdirectory

DeviceManager* DeviceManager::instance = nullptr;

DeviceManager::DeviceManager()
    : wifiManager(nullptr),
      mqttClient(nullptr),
      stm32(),
      ntpTime() {

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
    wifiManager = new CustomWiFiManager(config);
    if (wifiManager->init() != WiFiError::SUCCESS) {
        LOG_ERROR("WiFi", "Initialization failed");
        return false;
    }

    // Check if WiFi is configured
    if (strlen(config.wifi.ssid) == 0) {
        LOG_WARN("WiFi", "Not configured - starting config portal");
        wifiManager->startConfigPortal();

        // After config portal, try to connect
        WiFiError wifiErr = wifiManager->connect();
        if (wifiErr != WiFiError::SUCCESS) {
            LOG_ERROR("WiFi", "Connection failed after provisioning");
            return false;
        }
    } else {
        // Try to connect with saved credentials
        LOG_INFO("WiFi", "Connecting to saved network: %s", config.wifi.ssid);
        WiFiError wifiErr = wifiManager->connect();

        if (wifiErr != WiFiError::SUCCESS) {
            LOG_WARN("WiFi", "Connection failed, starting config portal");
            wifiManager->startConfigPortal();
            return false;
        }
    }

    // Initialize MQTT
    mqttClient = new MQTTClient(config);
    mqttClient->setCallback(mqttMessageCallback);

    MQTTError mqttErr = mqttClient->connect();
    if (mqttErr != MQTTError::SUCCESS) {
        LOG_WARN("MQTT", "Connection failed, will retry");
        return false;
    }

    // Initialize NTP time
    ntpTime.init("pool.ntp.org", 0);  // UTC, can be configured from config later

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

        // Update NTP time
        ntpTime.update();

        // Send boot notification (once after network ready)
        if (!systemStatus.bootNotificationSent) {
            handleBootNotification();
            systemStatus.bootNotificationSent = true;
        }

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
    if (!mqttClient || !wifiManager) return;

    // Use handler (stateless, stack-based)
    HeartbeatHandler::execute(
        *mqttClient,
        *wifiManager,
        configManager.get(),
        systemStatus.bootTime
    );
}

void DeviceManager::handleMeterValues() {
    // Note: Meter values are now pushed from STM32 via CMD_PUBLISH_METER_VALUES
    // No need to request periodically
    // STM32 sends meter data when available
}

void DeviceManager::handleBootNotification() {
    if (!mqttClient) return;

    boot_notification_t bootData;
    strncpy(bootData.firmware_version, FIRMWARE_VERSION, sizeof(bootData.firmware_version) - 1);
    strncpy(bootData.charge_point_vendor, DEVICE_VENDOR, sizeof(bootData.charge_point_vendor) - 1);
    strncpy(bootData.charge_point_model, DEVICE_MODEL, sizeof(bootData.charge_point_model) - 1);
    strncpy(bootData.charge_point_serial_number, configManager.get().deviceId, sizeof(bootData.charge_point_serial_number) - 1);
    snprintf(bootData.timestamp, sizeof(bootData.timestamp), "%u", ntpTime.getUnixTime());

    OCPPMessageHandler::publishBootNotification(
        *mqttClient,
        configManager.get(),
        bootData
    );

    LOG_INFO("DeviceManager", "Boot notification sent");
}

void DeviceManager::mqttMessageCallback(const char* topic, const char* payload, uint16_t length) {
    if (!instance) return;

    // Use handler to process and forward to STM32
    MQTTIncomingHandler::execute(
        topic,
        payload,
        length,
        instance->stm32,
        instance->configManager.get()
    );
}

void DeviceManager::stm32PacketCallback(const uart_packet_t* packet) {
    if (!instance || !packet) return;

    // Use handler (stateless, all logic in handler)
    if (instance->mqttClient) {
        STM32CommandHandler::execute(
            *packet,
            instance->stm32,
            *instance->mqttClient,
            instance->ntpTime,
            instance->configManager
        );
    } else {
        LOG_WARN("STM32", "MQTT not available");
        instance->stm32.sendAck(packet->sequence, STATUS_ERROR);
    }
}
