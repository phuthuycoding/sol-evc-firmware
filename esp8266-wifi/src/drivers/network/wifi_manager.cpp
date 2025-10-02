/**
 * @file wifi_manager.cpp
 * @brief WiFi Manager implementation
 */

#include "network/wifi_manager.h"

WiFiManager::WiFiManager(const DeviceConfig& cfg)
    : config(cfg), lastReconnectAttempt(0) {
    memset(&status, 0, sizeof(WiFiStatus));
}

WiFiError WiFiManager::init() {
    WiFi.mode(WIFI_STA);
    wifiManagerLib.setConfigPortalTimeout(config.wifi.configPortalTimeout);

    Serial.println(F("[WiFi] Initialized"));
    return WiFiError::SUCCESS;
}

WiFiError WiFiManager::connect() {
    if (WiFi.status() == WL_CONNECTED) {
        return WiFiError::ALREADY_CONNECTED;
    }

    if (strlen(config.wifi.ssid) == 0) {
        return WiFiError::NOT_CONFIGURED;
    }

    Serial.printf("[WiFi] Connecting to: %s\n", config.wifi.ssid);
    WiFi.begin(config.wifi.ssid, config.wifi.password);

    uint8_t attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
        delay(500);
        Serial.print(".");
        attempts++;
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println(F("\n[WiFi] Connected"));
        Serial.printf("IP: %s\n", WiFi.localIP().toString().c_str());
        updateStatus();
        return WiFiError::SUCCESS;
    }

    status.disconnectCount++;
    return WiFiError::CONNECTION_FAILED;
}

void WiFiManager::disconnect() {
    WiFi.disconnect();
    status.connected = false;
}

WiFiError WiFiManager::startConfigPortal() {
    char apName[32];
    ConfigHelper::buildApName(apName, sizeof(apName), config);

    Serial.printf("[WiFi] Starting AP: %s\n", apName);

    if (wifiManagerLib.startConfigPortal(apName)) {
        status.apMode = false;
        updateStatus();
        return WiFiError::SUCCESS;
    }

    return WiFiError::TIMEOUT;
}

void WiFiManager::handle() {
    if (WiFi.status() != WL_CONNECTED) {
        status.connected = false;

        if (config.wifi.autoConnect) {
            uint32_t now = millis();
            if (now - lastReconnectAttempt > RECONNECT_INTERVAL) {
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

bool WiFiManager::isConnected() const {
    return WiFi.status() == WL_CONNECTED;
}

void WiFiManager::updateStatus() {
    status.connected = (WiFi.status() == WL_CONNECTED);
    status.rssi = WiFi.RSSI();
    status.ipAddress = WiFi.localIP();
    status.gateway = WiFi.gatewayIP();

    if (status.connected) {
        status.connectTime = millis();
        strncpy(status.ssid, WiFi.SSID().c_str(), sizeof(status.ssid));
    }
}
