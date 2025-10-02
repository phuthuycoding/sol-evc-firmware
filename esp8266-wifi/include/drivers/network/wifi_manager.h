/**
 * @file wifi_manager.h
 * @brief WiFi Manager (ESP8266 optimized)
 * @version 2.0.0
 */

#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <WiFiManager.h>
#include "../config/unified_config.h"

/**
 * @brief WiFi error codes
 */
enum class WiFiError : int8_t {
    SUCCESS = 0,
    NOT_CONFIGURED = -1,
    CONNECTION_FAILED = -2,
    TIMEOUT = -3,
    ALREADY_CONNECTED = -4
};

/**
 * @brief WiFi status
 */
struct WiFiStatus {
    bool connected;
    bool apMode;
    int8_t rssi;
    IPAddress ipAddress;
    IPAddress gateway;
    uint32_t connectTime;
    uint32_t disconnectCount;
    char ssid[32];
};

/**
 * @brief WiFi Manager
 */
class WiFiManager {
private:
    const DeviceConfig& config;
    ::WiFiManager wifiManagerLib;
    WiFiStatus status;
    uint32_t lastReconnectAttempt;

    static constexpr uint32_t RECONNECT_INTERVAL = 30000;

    void updateStatus();

public:
    explicit WiFiManagerOOP(const DeviceConfig& cfg);

    WiFiError init();
    WiFiError connect();
    void disconnect();
    WiFiError startConfigPortal();
    void handle();

    bool isConnected() const;
    const WiFiStatus& getStatus() const { return status; }

    // Prevent copying
    WiFiManager(const WiFiManager&) = delete;
    WiFiManager& operator=(const WiFiManager&) = delete;
};

#endif // WIFI_MANAGER_H
