/**
 * @file mqtt_client.cpp
 * @brief MQTT Client implementation
 * @version 2.0.0
 */

#include "mqtt/mqtt_client.h"

// Static instance for callback
MQTTClient* MQTTClient::instance = nullptr;

/**
 * @brief Constructor
 */
MQTTClient::MQTTClient(const DeviceConfig& cfg)
    : config(cfg),
      client(cfg.mqtt.tlsEnabled ? (Client&)wifiSecureClient : (Client&)wifiClient),
      userCallback(nullptr) {

    // Initialize status
    memset(&status, 0, sizeof(MQTTStatus));

    // Build client ID from config
    ConfigHelper::buildMqttClientId(clientId, sizeof(clientId), config);

    // Configure MQTT client
    client.setServer(config.mqtt.broker, config.mqtt.port);
    client.setCallback(staticCallback);
    client.setBufferSize(512); // Reduced from 1024 for ESP8266
    client.setKeepAlive(config.mqtt.keepAlive);

    // Set TLS to insecure for now (TODO: proper certificates)
    if (config.mqtt.tlsEnabled) {
        wifiSecureClient.setInsecure();
    }

    // Set static instance for callback
    instance = this;

    Serial.printf("[MQTT] Initialized - Broker: %s:%d, ClientID: %s\n",
                  config.mqtt.broker, config.mqtt.port, clientId);
}

/**
 * @brief Destructor
 */
MQTTClient::~MQTTClient() {
    disconnect();
    instance = nullptr;
}

/**
 * @brief Connect to MQTT broker
 */
MQTTError MQTTClient::connect() {
    if (client.connected()) {
        return MQTTError::SUCCESS;
    }

    Serial.println(F("[MQTT] Attempting connection..."));

    bool connected = false;

    // Connect with username/password if provided
    if (strlen(config.mqtt.username) > 0) {
        connected = client.connect(clientId,
                                   config.mqtt.username,
                                   config.mqtt.password);
    } else {
        connected = client.connect(clientId);
    }

    if (connected) {
        Serial.println(F("[MQTT] Connected successfully"));
        status.connected = true;
        status.connectTime = millis();

        // Subscribe to command topic
        char cmdTopic[128];
        MQTTTopicBuilder::buildCommand(cmdTopic, sizeof(cmdTopic), config);
        subscribe(cmdTopic, 1);

        return MQTTError::SUCCESS;
    } else {
        int rc = client.state();
        Serial.printf("[MQTT] Connection failed, rc=%d\n", rc);
        status.connected = false;
        status.reconnectCount++;
        status.lastError = rc;

        return MQTTError::CONNECTION_FAILED;
    }
}

/**
 * @brief Disconnect from broker
 */
void MQTTClient::disconnect() {
    if (client.connected()) {
        client.disconnect();
        status.connected = false;
        Serial.println(F("[MQTT] Disconnected"));
    }
}

/**
 * @brief Check if connected
 */
bool MQTTClient::isConnected() const {
    return client.connected();
}

/**
 * @brief Publish message
 */
MQTTError MQTTClient::publish(const char* topic, const char* payload, uint8_t qos) {
    if (!topic || !payload) {
        return MQTTError::INVALID_PARAM;
    }

    // If not connected, queue the message
    if (!client.connected()) {
        if (messageQueue.isFull()) {
            Serial.println(F("[MQTT] Queue full, dropping oldest message"));
            MQTTMessage dummy;
            messageQueue.pop(dummy); // Remove oldest
        }

        MQTTMessage msg;
        strncpy(msg.topic, topic, sizeof(msg.topic) - 1);
        msg.topic[sizeof(msg.topic) - 1] = '\0';
        strncpy(msg.payload, payload, sizeof(msg.payload) - 1);
        msg.payload[sizeof(msg.payload) - 1] = '\0';
        msg.qos = qos;
        msg.timestamp = millis();

        if (messageQueue.push(msg)) {
            Serial.printf("[MQTT] Message queued (%u in queue): %s\n",
                         messageQueue.size(), topic);
            return MQTTError::SUCCESS;
        } else {
            return MQTTError::QUEUE_FULL;
        }
    }

    // Publish immediately if connected
    bool result = client.publish(topic, payload, qos == 1);

    if (result) {
        status.messageTxCount++;
        status.lastMessageTime = millis();
        Serial.printf("[MQTT] Published: %s\n", topic);
        return MQTTError::SUCCESS;
    } else {
        Serial.printf("[MQTT] Publish failed: %s\n", topic);
        return MQTTError::PUBLISH_FAILED;
    }
}

/**
 * @brief Subscribe to topic
 */
MQTTError MQTTClient::subscribe(const char* topic, uint8_t qos) {
    if (!topic) {
        return MQTTError::INVALID_PARAM;
    }

    if (!client.connected()) {
        return MQTTError::NOT_CONNECTED;
    }

    bool result = client.subscribe(topic, qos);

    if (result) {
        Serial.printf("[MQTT] Subscribed: %s\n", topic);
        return MQTTError::SUCCESS;
    } else {
        Serial.printf("[MQTT] Subscribe failed: %s\n", topic);
        return MQTTError::SUBSCRIBE_FAILED;
    }
}

/**
 * @brief Unsubscribe from topic
 */
MQTTError MQTTClient::unsubscribe(const char* topic) {
    if (!topic) {
        return MQTTError::INVALID_PARAM;
    }

    if (!client.connected()) {
        return MQTTError::NOT_CONNECTED;
    }

    bool result = client.unsubscribe(topic);

    if (result) {
        Serial.printf("[MQTT] Unsubscribed: %s\n", topic);
        return MQTTError::SUCCESS;
    } else {
        return MQTTError::SUBSCRIBE_FAILED;
    }
}

/**
 * @brief Set message callback
 */
void MQTTClient::setCallback(MQTTMessageCallback callback) {
    userCallback = callback;
}

/**
 * @brief Handle MQTT loop and reconnection
 */
void MQTTClient::handle() {
    if (client.connected()) {
        // Process incoming messages
        client.loop();

        // Process queued messages
        MQTTMessage msg;
        while (!messageQueue.isEmpty()) {
            if (messageQueue.pop(msg)) {
                MQTTError err = publish(msg.topic, msg.payload, msg.qos);
                if (err != MQTTError::SUCCESS) {
                    // Re-queue if failed
                    messageQueue.push(msg);
                    break;
                }
                delay(10); // Small delay between messages
            }
        }
    } else {
        // Attempt reconnection
        static uint32_t lastReconnectAttempt = 0;
        uint32_t now = millis();

        if (now - lastReconnectAttempt > 5000) { // Try every 5 seconds
            if (connect() == MQTTError::SUCCESS) {
                lastReconnectAttempt = 0;
            } else {
                lastReconnectAttempt = now;
            }
        }
    }

    updateStatus();
}

/**
 * @brief Update status
 */
void MQTTClient::updateStatus() {
    status.connected = client.connected();
}

/**
 * @brief Static callback for PubSubClient
 */
void MQTTClient::staticCallback(char* topic, byte* payload, unsigned int length) {
    if (!instance) return;

    // Null-terminate payload
    char payloadStr[length + 1];
    memcpy(payloadStr, payload, length);
    payloadStr[length] = '\0';

    Serial.printf("[MQTT] Received: %s -> %s\n", topic, payloadStr);

    instance->status.messageRxCount++;
    instance->status.lastMessageTime = millis();

    // Call user callback
    if (instance->userCallback) {
        instance->userCallback(topic, payloadStr, length);
    }
}

/* ============================================================================
 * MQTT Topic Builder Functions
 * ============================================================================ */

namespace MQTTTopicBuilder {

void buildHeartbeat(char* buffer, size_t size, const DeviceConfig& config) {
    snprintf(buffer, size, "ocpp/%s/%s/heartbeat",
             config.stationId, config.deviceId);
}

void buildStatus(char* buffer, size_t size, const DeviceConfig& config, uint8_t connectorId) {
    snprintf(buffer, size, "ocpp/%s/%s/status/%u/status_notification",
             config.stationId, config.deviceId, connectorId);
}

void buildMeter(char* buffer, size_t size, const DeviceConfig& config, uint8_t connectorId) {
    snprintf(buffer, size, "ocpp/%s/%s/meter/%u/meter_values",
             config.stationId, config.deviceId, connectorId);
}

void buildCommand(char* buffer, size_t size, const DeviceConfig& config) {
    snprintf(buffer, size, "ocpp/%s/%s/cmd/+",
             config.stationId, config.deviceId);
}

} // namespace MQTTTopicBuilder
