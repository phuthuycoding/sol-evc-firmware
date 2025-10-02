/**
 * @file web_api_handler.h
 * @brief Web API Handler (stateless business logic)
 * @version 1.0.0
 */

#ifndef WEB_API_HANDLER_H
#define WEB_API_HANDLER_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <ESPAsyncWebServer.h>
#include "drivers/network/wifi_manager.h"
#include "drivers/mqtt/mqtt_client.h"
#include "utils/logger.h"

/**
 * @brief Provisioning state
 */
struct ProvisioningState {
    bool subscribed;
    bool provisioned;
    char mqttUsername[64];
    char mqttPassword[64];
    char mqttBroker[128];
    uint32_t subscribeTime;
};

/**
 * @brief Web API Handler (stateless)
 *
 * Handles all web API requests for WiFi and provisioning
 */
class WebAPIHandler {
private:
    CustomWiFiManager* wifiManager;
    MQTTClient* mqttClient;
    UnifiedConfigManager* configManager;
    ProvisioningState provisionState;
    char deviceId[32];

    // Helper methods
    void sendJsonResponse(AsyncWebServerRequest* request, int code, JsonDocument& doc);
    void sendErrorResponse(AsyncWebServerRequest* request, int code, const char* message);
    void saveProvisioningConfig();

public:
    WebAPIHandler(CustomWiFiManager* wifi, MQTTClient* mqtt, UnifiedConfigManager* config, const char* devId);

    /**
     * @brief Register all API routes
     */
    void registerRoutes(AsyncWebServer& server);

    /**
     * @brief Handle WiFi scan request
     * GET /api/wifi/scan
     */
    void handleWiFiScan(AsyncWebServerRequest* request);

    /**
     * @brief Handle WiFi connect request
     * POST /api/wifi/connect
     * Body: {"ssid": "...", "password": "..."}
     */
    void handleWiFiConnect(AsyncWebServerRequest* request, uint8_t* data, size_t len);

    /**
     * @brief Handle WiFi status request
     * GET /api/wifi/status
     */
    void handleWiFiStatus(AsyncWebServerRequest* request);

    /**
     * @brief Handle provisioning subscribe request
     * POST /api/provision/subscribe
     */
    void handleProvisionSubscribe(AsyncWebServerRequest* request);

    /**
     * @brief Handle provisioning status request
     * GET /api/provision/status
     */
    void handleProvisionStatus(AsyncWebServerRequest* request);

    /**
     * @brief MQTT callback for provisioning messages
     */
    void onProvisioningMessage(const char* payload, uint16_t length);

    /**
     * @brief Check if provisioning is complete
     */
    bool isProvisioned() const { return provisionState.provisioned; }
};

#endif // WEB_API_HANDLER_H
