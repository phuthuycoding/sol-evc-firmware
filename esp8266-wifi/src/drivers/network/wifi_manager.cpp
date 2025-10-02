/**
 * @file wifi_manager.cpp
 * @brief WiFi Manager implementation (native - no WiFiManager library)
 * @version 3.0.0
 */

#include "drivers/network/wifi_manager.h"
#include "utils/logger.h"

CustomWiFiManager::CustomWiFiManager(const DeviceConfig& cfg)
    : config(cfg), lastReconnectAttempt(0) {
    memset(&status, 0, sizeof(WiFiStatus));
}

WiFiError CustomWiFiManager::init() {
    // Start in STA mode by default
    WiFi.mode(WIFI_STA);
    WiFi.persistent(false);  // Don't save to flash
    WiFi.setAutoConnect(false);
    WiFi.setAutoReconnect(true);

    LOG_INFO("WiFi", "Initialized in STA mode");
    return WiFiError::SUCCESS;
}

WiFiError CustomWiFiManager::connect() {
    if (WiFi.status() == WL_CONNECTED) {
        return WiFiError::ALREADY_CONNECTED;
    }

    if (strlen(config.wifi.ssid) == 0) {
        LOG_WARN("WiFi", "No SSID configured");
        return WiFiError::NOT_CONFIGURED;
    }

    return connectToNetwork(config.wifi.ssid, config.wifi.password);
}

WiFiError CustomWiFiManager::connectToNetwork(const char* ssid, const char* password) {
    LOG_INFO("WiFi", "Connecting to: %s", ssid);

    WiFi.begin(ssid, password);

    uint8_t attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
        delay(500);
        attempts++;
        ESP.wdtFeed();
    }

    if (WiFi.status() == WL_CONNECTED) {
        LOG_INFO("WiFi", "Connected! IP: %s", WiFi.localIP().toString().c_str());
        status.apMode = false;
        updateStatus();
        return WiFiError::SUCCESS;
    }

    LOG_ERROR("WiFi", "Connection failed");
    status.disconnectCount++;
    return WiFiError::CONNECTION_FAILED;
}

void CustomWiFiManager::disconnect() {
    WiFi.disconnect();
    status.connected = false;
    LOG_INFO("WiFi", "Disconnected");
}

WiFiError CustomWiFiManager::startAPMode() {
    char apName[32];
    snprintf(apName, sizeof(apName), "SolEVC-%06X", ESP.getChipId());

    LOG_INFO("WiFi", "Starting AP mode: %s", apName);

    WiFi.mode(WIFI_AP);

    if (WiFi.softAP(apName)) {
        status.apMode = true;
        status.connected = false;
        LOG_INFO("WiFi", "AP started. IP: %s", WiFi.softAPIP().toString().c_str());
        return WiFiError::SUCCESS;
    }

    LOG_ERROR("WiFi", "Failed to start AP");
    return WiFiError::CONNECTION_FAILED;
}

void CustomWiFiManager::handle() {
    // Skip reconnection if in AP mode
    if (status.apMode) {
        return;
    }

    if (WiFi.status() != WL_CONNECTED) {
        status.connected = false;

        if (config.wifi.autoConnect) {
            uint32_t now = millis();
            if (now - lastReconnectAttempt > RECONNECT_INTERVAL) {
                LOG_INFO("WiFi", "Auto-reconnecting...");
                connect();
                lastReconnectAttempt = now;
            }
        }
    } else {
        if (!status.connected) {
            updateStatus();
        }
    }
}

bool CustomWiFiManager::isConnected() const {
    return WiFi.status() == WL_CONNECTED && !status.apMode;
}

void CustomWiFiManager::updateStatus() {
    status.connected = (WiFi.status() == WL_CONNECTED);
    status.rssi = WiFi.RSSI();
    status.ipAddress = WiFi.localIP();
    status.gateway = WiFi.gatewayIP();

    if (status.connected) {
        status.connectTime = millis();
        strncpy(status.ssid, WiFi.SSID().c_str(), sizeof(status.ssid) - 1);
        status.ssid[sizeof(status.ssid) - 1] = '\0';
    }
}
