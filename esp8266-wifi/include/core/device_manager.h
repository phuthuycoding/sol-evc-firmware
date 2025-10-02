/**
 * @file device_manager.h
 * @brief Main device orchestrator
 * @version 1.0.0
 */

#ifndef DEVICE_MANAGER_H
#define DEVICE_MANAGER_H

#include "config/unified_config.h"
#include "network/wifi_manager.h"
#include "mqtt/mqtt_client.h"
#include "communication/stm32_comm.h"
#include "utils/logger.h"

/**
 * @brief Main device manager (Facade pattern)
 *
 * Orchestrates all system components
 */
class DeviceManager {
private:
    // Config manager
    UnifiedConfigManager configManager;

    // Network components (stack allocated)
    WiFiManager* wifiManager;
    MQTTClient* mqttClient;

    // Communication
    STM32Communicator stm32;

    // Status
    struct {
        bool initialized;
        uint32_t bootTime;
        uint32_t lastHeartbeat;
    } systemStatus;

    // Private methods
    bool initializeConfig();
    bool initializeNetwork();
    bool initializeCommunication();

    void handleHeartbeat();
    void handleMeterValues();

    // Callbacks
    static void mqttMessageCallback(const char* topic, const char* payload, uint16_t length);
    static void stm32PacketCallback(const uart_packet_t* packet);

    // Static instance for callbacks
    static DeviceManager* instance;

public:
    DeviceManager();
    ~DeviceManager();

    /**
     * @brief Initialize all components
     */
    bool init();

    /**
     * @brief Main loop - must be called frequently
     */
    void run();

    /**
     * @brief Get config reference
     */
    const DeviceConfig& getConfig() const { return configManager.get(); }

    // Prevent copying
    DeviceManager(const DeviceManager&) = delete;
    DeviceManager& operator=(const DeviceManager&) = delete;
};

#endif // DEVICE_MANAGER_H
