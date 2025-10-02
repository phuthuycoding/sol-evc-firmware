/**
 * @file test_ocpp_message_handler.cpp
 * @brief Unit tests for OCPPMessageHandler
 */

#include <unity.h>
#include "handlers/ocpp_message_handler.h"
#include "test_mocks/mock_mqtt_client.h"

void setUp(void) {}
void tearDown(void) {}

void test_publish_status_notification(void) {
    // Arrange
    MockMQTTClient mqtt;
    DeviceConfig config = {
        .deviceId = "TEST-001",
        .stationId = "STATION-01"
    };

    status_notification_t status;
    strcpy(status.msg_id, "MSG-001");
    status.connector_id = 1;
    status.status = 1;  // Available
    status.error_code = 0;  // NoError
    status.timestamp = 1234567890;

    // Act
    bool result = OCPPMessageHandler::publishStatusNotification(mqtt, config, status);

    // Assert
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_TRUE(mqtt.wasPublishCalled());

    const char* payload = mqtt.getLastPayload();
    TEST_ASSERT_NOT_NULL(strstr(payload, "\"msgId\":\"MSG-001\""));
    TEST_ASSERT_NOT_NULL(strstr(payload, "\"connectorId\":1"));
}

void test_publish_meter_values(void) {
    // Arrange
    MockMQTTClient mqtt;
    DeviceConfig config = {.deviceId = "TEST-001", .stationId = "STATION-01"};

    meter_values_t meter;
    strcpy(meter.msg_id, "METER-001");
    meter.connector_id = 1;
    meter.transaction_id = 100;
    meter.sample.energy_wh = 5000;
    meter.sample.voltage_mv = 230000;
    meter.sample.current_ma = 16000;
    meter.sample.power_w = 3680;
    meter.sample.temperature_c = 25;
    meter.sample.timestamp = 1234567890;

    // Act
    bool result = OCPPMessageHandler::publishMeterValues(mqtt, config, meter);

    // Assert
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_NOT_NULL(strstr(mqtt.getLastPayload(), "\"energy_wh\":5000"));
    TEST_ASSERT_NOT_NULL(strstr(mqtt.getLastPayload(), "\"voltage_mv\":230000"));
}

void test_publish_boot_notification(void) {
    // Arrange
    MockMQTTClient mqtt;
    DeviceConfig config = {.deviceId = "TEST-001", .stationId = "STATION-01"};

    boot_notification_t boot;
    strcpy(boot.firmware_version, "1.0.0");
    strcpy(boot.station_id, "STATION-01");
    boot.boot_reason = 1;  // PowerUp
    boot.timestamp = 1234567890;

    // Act
    bool result = OCPPMessageHandler::publishBootNotification(mqtt, config, boot);

    // Assert
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_NOT_NULL(strstr(mqtt.getLastPayload(), "\"firmwareVersion\":\"1.0.0\""));
    TEST_ASSERT_NOT_NULL(strstr(mqtt.getLastPayload(), "\"bootReason\":1"));
}

void process(void) {
    UNITY_BEGIN();

    RUN_TEST(test_publish_status_notification);
    RUN_TEST(test_publish_meter_values);
    RUN_TEST(test_publish_boot_notification);

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
