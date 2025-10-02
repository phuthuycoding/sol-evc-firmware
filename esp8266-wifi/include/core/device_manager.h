/**
 * @file device_manager.h
 * @brief Main device orchestrator
 * @version 2.0.0
 */

#ifndef DEVICE_MANAGER_H
#define DEVICE_MANAGER_H

#include "drivers/config/unified_config.h"
#include "drivers/network/wifi_manager.h"
#include "drivers/network/web_server.h"
#include "drivers/mqtt/mqtt_client.h"
#include "drivers/communication/stm32_comm.h"
#include "drivers/time/ntp_time.h"
#include "handlers/web_api_handler.h"
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

    // Network components (heap allocated)
    CustomWiFiManager* wifiManager;
    MQTTClient* mqttClient;
    WebServerDriver* webServer;
    WebAPIHandler* webAPIHandler;

    // Communication
    STM32Communicator stm32;

    // Time synchronization
    NTPTimeDriver ntpTime;

    // Status
    struct {
        bool initialized;
        uint32_t bootTime;
        uint32_t lastHeartbeat;
        bool bootNotificationSent;
        bool provisioningMode;
    } systemStatus;

    // Private methods
    bool initializeConfig();
    bool initializeNetwork();
    bool initializeWebServer();
    bool initializeCommunication();

    void handleHeartbeat();
    void handleMeterValues();
    void handleBootNotification();

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
