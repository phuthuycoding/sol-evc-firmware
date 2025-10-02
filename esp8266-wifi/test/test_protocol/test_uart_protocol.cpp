/**
 * @file test_uart_protocol.cpp
 * @brief Unit tests for UART protocol functions
 */

#include <unity.h>
#include "../../shared/uart_protocol.h"
#include <string.h>

void setUp(void) {}
void tearDown(void) {}

void test_uart_init_packet(void) {
    // Arrange
    uart_packet_t packet;

    // Act
    uart_init_packet(&packet, CMD_MQTT_PUBLISH, 42);

    // Assert
    TEST_ASSERT_EQUAL_UINT8(UART_START_BYTE, packet.start);
    TEST_ASSERT_EQUAL_UINT8(CMD_MQTT_PUBLISH, packet.cmd_type);
    TEST_ASSERT_EQUAL_UINT8(42, packet.sequence);
    TEST_ASSERT_EQUAL_UINT16(0, packet.length);
}

void test_uart_calculate_checksum(void) {
    // Arrange
    uart_packet_t packet;
    uart_init_packet(&packet, CMD_GET_TIME, 1);
    packet.length = 0;

    // Act
    uint8_t checksum = uart_calculate_checksum(&packet);

    // Assert
    // Checksum = cmd_type XOR sequence XOR length_low XOR length_high
    uint8_t expected = CMD_GET_TIME ^ 1 ^ 0 ^ 0;
    TEST_ASSERT_EQUAL_UINT8(expected, checksum);
}

void test_uart_calculate_checksum_with_payload(void) {
    // Arrange
    uart_packet_t packet;
    uart_init_packet(&packet, CMD_MQTT_PUBLISH, 5);

    const char* testData = "Hello";
    memcpy(packet.payload, testData, 5);
    packet.length = 5;

    // Act
    uint8_t checksum = uart_calculate_checksum(&packet);

    // Assert
    uint8_t expected = CMD_MQTT_PUBLISH ^ 5 ^ 5 ^ 0;  // cmd ^ seq ^ len_low ^ len_high
    for (int i = 0; i < 5; i++) {
        expected ^= testData[i];
    }
    TEST_ASSERT_EQUAL_UINT8(expected, checksum);
}

void test_uart_verify_checksum_valid(void) {
    // Arrange
    uart_packet_t packet;
    uart_init_packet(&packet, CMD_WIFI_STATUS, 10);
    packet.length = 0;
    packet.checksum = uart_calculate_checksum(&packet);

    // Act
    bool valid = uart_verify_checksum(&packet);

    // Assert
    TEST_ASSERT_TRUE(valid);
}

void test_uart_verify_checksum_invalid(void) {
    // Arrange
    uart_packet_t packet;
    uart_init_packet(&packet, CMD_WIFI_STATUS, 10);
    packet.length = 0;
    packet.checksum = 0xFF;  // Wrong checksum

    // Act
    bool valid = uart_verify_checksum(&packet);

    // Assert
    TEST_ASSERT_FALSE(valid);
}

void test_uart_packet_max_payload(void) {
    // Arrange
    uart_packet_t packet;
    uart_init_packet(&packet, CMD_MQTT_PUBLISH, 1);

    // Act
    packet.length = UART_MAX_PAYLOAD;
    memset(packet.payload, 0xAA, UART_MAX_PAYLOAD);
    packet.checksum = uart_calculate_checksum(&packet);

    // Assert
    TEST_ASSERT_TRUE(uart_verify_checksum(&packet));
    TEST_ASSERT_EQUAL_UINT16(UART_MAX_PAYLOAD, packet.length);
}

void process(void) {
    UNITY_BEGIN();

    RUN_TEST(test_uart_init_packet);
    RUN_TEST(test_uart_calculate_checksum);
    RUN_TEST(test_uart_calculate_checksum_with_payload);
    RUN_TEST(test_uart_verify_checksum_valid);
    RUN_TEST(test_uart_verify_checksum_invalid);
    RUN_TEST(test_uart_packet_max_payload);

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
