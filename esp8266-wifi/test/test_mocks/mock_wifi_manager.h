/**
 * @file mock_wifi_manager.h
 * @brief Mock WiFi Manager for testing
 */

#ifndef MOCK_WIFI_MANAGER_H
#define MOCK_WIFI_MANAGER_H

#include <Arduino.h>

/**
 * @brief Mock WiFi Manager
 */
class MockWiFiManager {
private:
    bool connected;
    int8_t rssi;
    IPAddress ipAddress;

public:
    MockWiFiManager() : connected(true), rssi(-50), ipAddress(192, 168, 1, 100) {}

    void setConnected(bool state) { connected = state; }
    bool isConnected() const { return connected; }

    void setRSSI(int8_t value) { rssi = value; }
    int8_t getRSSI() const { return rssi; }

    void setIPAddress(IPAddress ip) { ipAddress = ip; }
    IPAddress getIPAddress() const { return ipAddress; }

    // WiFi status structure
    struct {
        bool connected;
        int8_t rssi;
        IPAddress ip;
        uint32_t uptime;
    } getStatus() const {
        return {connected, rssi, ipAddress, millis() / 1000};
    }
};

#endif // MOCK_WIFI_MANAGER_H
