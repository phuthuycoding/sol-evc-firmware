/**
 * @file web_api_handler.cpp
 * @brief Web API Handler Implementation
 * @version 1.0.0
 */

#include "handlers/web_api_handler.h"

WebAPIHandler::WebAPIHandler(CustomWiFiManager* wifi, MQTTClient* mqtt, UnifiedConfigManager* config, const char* devId)
    : wifiManager(wifi), mqttClient(mqtt), configManager(config) {
    provisionState.subscribed = false;
    provisionState.provisioned = false;
    provisionState.mqttUsername[0] = '\0';
    provisionState.mqttPassword[0] = '\0';
    provisionState.mqttBroker[0] = '\0';
    provisionState.subscribeTime = 0;

    strncpy(deviceId, devId, sizeof(deviceId) - 1);
    deviceId[sizeof(deviceId) - 1] = '\0';
}

void WebAPIHandler::sendJsonResponse(AsyncWebServerRequest* request, int code, JsonDocument& doc) {
    String response;
    serializeJson(doc, response);
    request->send(code, "application/json", response);
}

void WebAPIHandler::sendErrorResponse(AsyncWebServerRequest* request, int code, const char* message) {
    StaticJsonDocument<128> doc;
    doc["error"] = message;
    sendJsonResponse(request, code, doc);
}

void WebAPIHandler::registerRoutes(AsyncWebServer& server) {
    // WiFi routes
    server.on("/api/wifi/scan", HTTP_GET, [this](AsyncWebServerRequest* request) {
        handleWiFiScan(request);
    });

    server.on("/api/wifi/status", HTTP_GET, [this](AsyncWebServerRequest* request) {
        handleWiFiStatus(request);
    });

    server.on(
        "/api/wifi/connect", HTTP_POST,
        [this](AsyncWebServerRequest* request) {
            // Response sent in body handler
        },
        NULL,
        [this](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
            if (index == 0) {
                // First chunk
                handleWiFiConnect(request, data, len);
            }
        }
    );

    // Provisioning routes
    server.on("/api/provision/subscribe", HTTP_POST, [this](AsyncWebServerRequest* request) {
        handleProvisionSubscribe(request);
    });

    server.on("/api/provision/status", HTTP_GET, [this](AsyncWebServerRequest* request) {
        handleProvisionStatus(request);
    });

    LOG_INFO("WebAPI", "API routes registered");
}

void WebAPIHandler::handleWiFiScan(AsyncWebServerRequest* request) {
    LOG_INFO("WebAPI", "Scanning WiFi networks...");

    int n = WiFi.scanNetworks();

    DynamicJsonDocument doc(2048);
    JsonArray networks = doc.to<JsonArray>();

    for (int i = 0; i < n && i < 20; i++) {  // Limit to 20 networks
        JsonObject network = networks.createNestedObject();
        network["ssid"] = WiFi.SSID(i);
        network["rssi"] = WiFi.RSSI(i);
        network["encryption"] = WiFi.encryptionType(i);
        network["bssid"] = WiFi.BSSIDstr(i);
    }

    LOG_INFO("WebAPI", "Found %d networks", n);
    sendJsonResponse(request, 200, doc);

    WiFi.scanDelete();
}

void WebAPIHandler::handleWiFiConnect(AsyncWebServerRequest* request, uint8_t* data, size_t len) {
    StaticJsonDocument<256> doc;
    DeserializationError error = deserializeJson(doc, data, len);

    if (error) {
        LOG_ERROR("WebAPI", "JSON parse error: %s", error.c_str());
        sendErrorResponse(request, 400, "Invalid JSON");
        return;
    }

    const char* ssid = doc["ssid"];
    const char* password = doc["password"];

    if (!ssid) {
        sendErrorResponse(request, 400, "Missing ssid");
        return;
    }

    LOG_INFO("WebAPI", "Connecting to WiFi: %s", ssid);

    // Attempt connection
    WiFi.begin(ssid, password ? password : "");

    // Wait for connection (max 10 seconds)
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
        delay(500);
        attempts++;
        ESP.wdtFeed();
    }

    StaticJsonDocument<128> response;
    if (WiFi.status() == WL_CONNECTED) {
        response["success"] = true;
        response["ip"] = WiFi.localIP().toString();
        LOG_INFO("WebAPI", "Connected! IP: %s", WiFi.localIP().toString().c_str());
        sendJsonResponse(request, 200, response);
    } else {
        response["success"] = false;
        response["error"] = "Connection failed";
        LOG_ERROR("WebAPI", "Connection failed");
        sendJsonResponse(request, 500, response);
    }
}

void WebAPIHandler::handleWiFiStatus(AsyncWebServerRequest* request) {
    StaticJsonDocument<256> doc;

    doc["connected"] = WiFi.status() == WL_CONNECTED;

    if (WiFi.status() == WL_CONNECTED) {
        doc["ssid"] = WiFi.SSID();
        doc["ip"] = WiFi.localIP().toString();
        doc["rssi"] = WiFi.RSSI();
    }

    sendJsonResponse(request, 200, doc);
}

void WebAPIHandler::handleProvisionSubscribe(AsyncWebServerRequest* request) {
    LOG_INFO("WebAPI", "Provisioning subscribe request");

    if (!wifiManager || !wifiManager->isConnected()) {
        sendErrorResponse(request, 400, "WiFi not connected");
        return;
    }

    if (!mqttClient || !mqttClient->isConnected()) {
        sendErrorResponse(request, 500, "MQTT not connected");
        return;
    }

    // Subscribe to provision topic with device ID
    char provisionTopic[64];
    snprintf(provisionTopic, sizeof(provisionTopic), "provision/%s", deviceId);
    MQTTError result = mqttClient->subscribe(provisionTopic);

    if (result == MQTTError::SUCCESS) {
        provisionState.subscribed = true;
        provisionState.subscribeTime = millis();

        StaticJsonDocument<128> doc;
        doc["success"] = true;
        doc["topic"] = provisionTopic;

        LOG_INFO("WebAPI", "Subscribed to %s", provisionTopic);
        sendJsonResponse(request, 200, doc);
    } else {
        sendErrorResponse(request, 500, "Failed to subscribe");
    }
}

void WebAPIHandler::handleProvisionStatus(AsyncWebServerRequest* request) {
    StaticJsonDocument<512> doc;

    doc["provisioned"] = provisionState.provisioned;

    if (provisionState.provisioned) {
        doc["mqttBroker"] = provisionState.mqttBroker;
        doc["mqttUsername"] = provisionState.mqttUsername;
        doc["mqttPassword"] = provisionState.mqttPassword;
    }

    sendJsonResponse(request, 200, doc);
}

void WebAPIHandler::onProvisioningMessage(const char* payload, uint16_t length) {
    LOG_INFO("WebAPI", "Provisioning message received: %.*s", length, payload);

    StaticJsonDocument<512> doc;
    DeserializationError error = deserializeJson(doc, payload, length);

    if (error) {
        LOG_ERROR("WebAPI", "Failed to parse provisioning message");
        return;
    }

    // Extract credentials
    const char* broker = doc["broker"];
    const char* username = doc["username"];
    const char* password = doc["password"];

    if (broker && username && password) {
        strncpy(provisionState.mqttBroker, broker, sizeof(provisionState.mqttBroker) - 1);
        strncpy(provisionState.mqttUsername, username, sizeof(provisionState.mqttUsername) - 1);
        strncpy(provisionState.mqttPassword, password, sizeof(provisionState.mqttPassword) - 1);
        provisionState.provisioned = true;

        LOG_INFO("WebAPI", "Provisioning complete!");
        LOG_INFO("WebAPI", "  Broker: %s", broker);
        LOG_INFO("WebAPI", "  Username: %s", username);

        // Save to config and schedule restart
        saveProvisioningConfig();

        LOG_INFO("WebAPI", "Config saved. Restarting in 3 seconds...");
        delay(3000);
        ESP.restart();
    } else {
        LOG_ERROR("WebAPI", "Invalid provisioning data");
    }
}

void WebAPIHandler::saveProvisioningConfig() {
    if (!configManager) {
        LOG_ERROR("WebAPI", "Config manager not available");
        return;
    }

    // Update MQTT config in memory
    DeviceConfig& config = const_cast<DeviceConfig&>(configManager->get());
    strncpy(config.mqtt.broker, provisionState.mqttBroker, sizeof(config.mqtt.broker) - 1);
    strncpy(config.mqtt.username, provisionState.mqttUsername, sizeof(config.mqtt.username) - 1);
    strncpy(config.mqtt.password, provisionState.mqttPassword, sizeof(config.mqtt.password) - 1);

    // Save to file
    if (configManager->save()) {
        LOG_INFO("WebAPI", "Provisioning config saved successfully");
    } else {
        LOG_ERROR("WebAPI", "Failed to save provisioning config");
    }
}
