/**
 * @file mqtt_client.h
 * @brief MQTT Client (stack allocated, ESP8266 optimized)
 * @version 2.0.0
 *
 * Changes from v1:
 * - Stack allocated (no new/delete)
 * - Uses DeviceConfig instead of mqtt_config_t
 * - Fixed-size message queue (no std::queue)
 * - RAII compliant
 */

#ifndef MQTT_CLIENT_H
#define MQTT_CLIENT_H

#include <Arduino.h>
#include <PubSubClient.h>
#include <WiFiClient.h>
#include <WiFiClientSecure.h>
#include "config/unified_config.h"

/**
 * @brief Error codes for MQTT operations
 */
enum class MQTTError : int8_t {
    SUCCESS = 0,
    NOT_CONNECTED = -1,
    PUBLISH_FAILED = -2,
    SUBSCRIBE_FAILED = -3,
    QUEUE_FULL = -4,
    INVALID_PARAM = -5,
    CONNECTION_FAILED = -6
};

/**
 * @brief MQTT connection status
 */
struct MQTTStatus {
    bool connected;
    uint32_t connectTime;
    uint32_t reconnectCount;
    uint32_t messageTxCount;
    uint32_t messageRxCount;
    uint32_t lastMessageTime;
    int8_t lastError;
};

/**
 * @brief MQTT message (fixed size for queue)
 */
struct MQTTMessage {
    char topic[128];
    char payload[256];
    uint8_t qos;
    uint32_t timestamp;
};

/**
 * @brief Message callback function type
 */
typedef void (*MQTTMessageCallback)(const char* topic, const char* payload, uint16_t length);

/**
 * @brief Fixed-size message queue (no STL)
 */
template<size_t N>
class FixedMessageQueue {
private:
    MQTTMessage messages[N];
    size_t head;
    size_t tail;
    size_t count;

public:
    FixedMessageQueue() : head(0), tail(0), count(0) {}

    bool push(const MQTTMessage& msg) {
        if (count >= N) return false;
        messages[head] = msg;
        head = (head + 1) % N;
        count++;
        return true;
    }

    bool pop(MQTTMessage& msg) {
        if (count == 0) return false;
        msg = messages[tail];
        tail = (tail + 1) % N;
        count--;
        return true;
    }

    size_t size() const { return count; }
    bool isEmpty() const { return count == 0; }
    bool isFull() const { return count >= N; }

    void clear() {
        head = tail = count = 0;
    }
};

/**
 * @brief OOP MQTT Client (stack allocated)
 *
 * Usage:
 *   MQTTClient mqtt(config);
 *   mqtt.connect();
 *   mqtt.publish("topic", "payload");
 *   mqtt.handle(); // Call in loop()
 */
class MQTTClient {
private:
    // Configuration
    const DeviceConfig& config;

    // WiFi clients (stack allocated)
    WiFiClient wifiClient;
    WiFiClientSecure wifiSecureClient;

    // MQTT client (stack allocated)
    PubSubClient client;

    // Client ID (built from config)
    char clientId[64];

    // Status
    MQTTStatus status;

    // Message queue (max 10 messages)
    FixedMessageQueue<10> messageQueue;

    // User callback
    MQTTMessageCallback userCallback;

    // Private methods
    bool connectInternal();
    void updateStatus();
    static void staticCallback(char* topic, byte* payload, unsigned int length);

    // Static instance pointer for callback
    static MQTTClient* instance;

public:
    /**
     * @brief Constructor
     * @param cfg Reference to device configuration
     */
    explicit MQTTClient(const DeviceConfig& cfg);

    /**
     * @brief Destructor (RAII cleanup)
     */
    ~MQTTClient();

    // Prevent copying (ESP8266 memory constraints)
    MQTTClient(const MQTTClient&) = delete;
    MQTTClient& operator=(const MQTTClient&) = delete;

    /**
     * @brief Connect to MQTT broker
     * @return MQTTError::SUCCESS if connected
     */
    MQTTError connect();

    /**
     * @brief Disconnect from broker
     */
    void disconnect();

    /**
     * @brief Check if connected
     */
    bool isConnected() const;

    /**
     * @brief Publish message
     * @param topic Topic string (will be copied)
     * @param payload Payload string (will be copied)
     * @param qos QoS level (0 or 1)
     * @return MQTTError code
     */
    MQTTError publish(const char* topic, const char* payload, uint8_t qos = 0);

    /**
     * @brief Subscribe to topic
     * @param topic Topic pattern
     * @param qos QoS level
     * @return MQTTError code
     */
    MQTTError subscribe(const char* topic, uint8_t qos = 0);

    /**
     * @brief Unsubscribe from topic
     */
    MQTTError unsubscribe(const char* topic);

    /**
     * @brief Set message received callback
     */
    void setCallback(MQTTMessageCallback callback);

    /**
     * @brief Handle MQTT loop and reconnection
     * Must be called frequently in main loop()
     */
    void handle();

    /**
     * @brief Get current status
     */
    const MQTTStatus& getStatus() const { return status; }

    /**
     * @brief Get queue size
     */
    size_t getQueueSize() const { return messageQueue.size(); }

    /**
     * @brief Clear message queue
     */
    void clearQueue() { messageQueue.clear(); }
};

/**
 * @brief Helper functions for MQTT topic building
 */
namespace MQTTTopicBuilder {
    /**
     * @brief Build heartbeat topic
     * Format: ocpp/{stationId}/{deviceId}/heartbeat
     */
    void buildHeartbeat(char* buffer, size_t size, const DeviceConfig& config);

    /**
     * @brief Build status notification topic
     * Format: ocpp/{stationId}/{deviceId}/status/{connectorId}/status_notification
     */
    void buildStatus(char* buffer, size_t size, const DeviceConfig& config, uint8_t connectorId);

    /**
     * @brief Build meter values topic
     * Format: ocpp/{stationId}/{deviceId}/meter/{connectorId}/meter_values
     */
    void buildMeter(char* buffer, size_t size, const DeviceConfig& config, uint8_t connectorId);

    /**
     * @brief Build command topic (subscribe)
     * Format: ocpp/{stationId}/{deviceId}/cmd/+
     */
    void buildCommand(char* buffer, size_t size, const DeviceConfig& config);
}

#endif // MQTT_CLIENT_H
