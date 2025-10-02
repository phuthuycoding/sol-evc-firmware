/**
 * @file mock_mqtt_client.h
 * @brief Mock MQTT Client for testing
 */

#ifndef MOCK_MQTT_CLIENT_H
#define MOCK_MQTT_CLIENT_H

#include <Arduino.h>
#include <string.h>

/**
 * @brief Mock MQTT Client
 */
class MockMQTTClient {
private:
    bool connected;
    bool publishCalled;
    char lastTopic[128];
    char lastPayload[512];
    uint8_t lastQos;

public:
    MockMQTTClient() : connected(true), publishCalled(false), lastQos(0) {
        memset(lastTopic, 0, sizeof(lastTopic));
        memset(lastPayload, 0, sizeof(lastPayload));
    }

    void setConnected(bool state) { connected = state; }
    bool isConnected() const { return connected; }

    bool publish(const char* topic, const char* payload, uint8_t qos = 0) {
        if (!connected) return false;

        publishCalled = true;
        strncpy(lastTopic, topic, sizeof(lastTopic) - 1);
        strncpy(lastPayload, payload, sizeof(lastPayload) - 1);
        lastQos = qos;

        return true;
    }

    bool wasPublishCalled() const { return publishCalled; }
    const char* getLastTopic() const { return lastTopic; }
    const char* getLastPayload() const { return lastPayload; }
    uint8_t getLastQos() const { return lastQos; }

    void reset() {
        publishCalled = false;
        memset(lastTopic, 0, sizeof(lastTopic));
        memset(lastPayload, 0, sizeof(lastPayload));
        lastQos = 0;
    }
};

#endif // MOCK_MQTT_CLIENT_H
