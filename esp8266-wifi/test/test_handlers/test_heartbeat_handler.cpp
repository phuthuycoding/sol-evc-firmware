/**
 * @file test_heartbeat_handler.cpp
 * @brief Unit tests for HeartbeatHandler
 */

#include <unity.h>
#include "handlers/heartbeat_handler.h"
#include "test_mocks/mock_mqtt_client.h"
#include "test_mocks/mock_wifi_manager.h"

void setUp(void) {
    // Called before each test
}

void tearDown(void) {
    // Called after each test
}

void test_heartbeat_publishes_correct_format(void) {
    // Arrange
    MockMQTTClient mqtt;
    MockWiFiManager wifi;
    DeviceConfig config = {
        .deviceId = "TEST-001",
        .stationId = "STATION-01"
    };
    uint32_t bootTime = 0;

    // Act
    bool result = HeartbeatHandler::execute(mqtt, wifi, config, bootTime);

    // Assert
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_TRUE(mqtt.wasPublishCalled());
    TEST_ASSERT_EQUAL_STRING("evse/STATION-01/TEST-001/heartbeat", mqtt.getLastTopic());
}

void test_heartbeat_includes_uptime(void) {
    // Arrange
    MockMQTTClient mqtt;
    MockWiFiManager wifi;
    DeviceConfig config = {.deviceId = "TEST-001", .stationId = "STATION-01"};
    uint32_t bootTime = 1000;

    // Act
    HeartbeatHandler::execute(mqtt, wifi, config, bootTime);

    // Assert
    const char* payload = mqtt.getLastPayload();
    TEST_ASSERT_NOT_NULL(strstr(payload, "\"uptime\""));
}

void test_heartbeat_fails_when_mqtt_disconnected(void) {
    // Arrange
    MockMQTTClient mqtt;
    mqtt.setConnected(false);
    MockWiFiManager wifi;
    DeviceConfig config = {.deviceId = "TEST-001", .stationId = "STATION-01"};

    // Act
    bool result = HeartbeatHandler::execute(mqtt, wifi, config, 0);

    // Assert
    TEST_ASSERT_FALSE(result);
}

void process(void) {
    UNITY_BEGIN();

    RUN_TEST(test_heartbeat_publishes_correct_format);
    RUN_TEST(test_heartbeat_includes_uptime);
    RUN_TEST(test_heartbeat_fails_when_mqtt_disconnected);

    UNITY_END();
}

#ifdef ARDUINO
#include <Arduino.h>
void setup() {
    delay(2000);
    process();
}
void loop() {}
#else
int main(int argc, char **argv) {
    process();
    return 0;
}
#endif
