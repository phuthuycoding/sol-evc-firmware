/**
 * @file uart_protocol.h
 * @brief UART communication protocol between STM32F103 and ESP8266
 * @version 1.0.0
 */

#ifndef __UART_PROTOCOL_H
#define __UART_PROTOCOL_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

/* Protocol Constants */
#define UART_START_BYTE     0xAA
#define UART_END_BYTE       0x55
#define UART_MAX_PAYLOAD    512
#define UART_TIMEOUT_MS     1000
#define UART_MAX_RETRIES    3

/* Command Types - STM32 to ESP8266 */
#define CMD_MQTT_PUBLISH    0x01
#define CMD_GET_TIME        0x02
#define CMD_WIFI_STATUS     0x03
#define CMD_CONFIG_UPDATE   0x04
#define CMD_OTA_REQUEST     0x05
#define CMD_GET_METER_VALUES 0x06

/* Response Types - ESP8266 to STM32 */
#define RSP_MQTT_ACK        0x81
#define RSP_TIME_DATA       0x82
#define RSP_WIFI_STATUS     0x83
#define RSP_CONFIG_ACK      0x84
#define RSP_MQTT_RECEIVED   0x85
#define RSP_OTA_STATUS      0x86

/* Status Codes */
#define STATUS_SUCCESS      0x00
#define STATUS_ERROR        0x01
#define STATUS_TIMEOUT      0x02
#define STATUS_INVALID      0x03

/* Packet Structure */
typedef struct __attribute__((packed)) {
    uint8_t start_byte;         // 0xAA
    uint8_t cmd_type;           // Command/Response type
    uint16_t length;            // Payload length (little-endian)
    uint8_t sequence;           // Sequence number for ACK
    uint8_t payload[UART_MAX_PAYLOAD];  // Variable length data
    uint8_t checksum;           // XOR checksum
    uint8_t end_byte;           // 0x55
} uart_packet_t;

/* MQTT Publish Command Payload */
typedef struct __attribute__((packed)) {
    char topic[128];            // MQTT topic
    uint8_t qos;               // QoS level (0, 1, 2)
    uint16_t data_length;      // JSON data length
    char data[];               // JSON payload (variable length)
} mqtt_publish_payload_t;

/* WiFi Status Response Payload */
typedef struct __attribute__((packed)) {
    uint8_t wifi_connected;     // 0=disconnected, 1=connected
    uint8_t mqtt_connected;     // 0=disconnected, 1=connected
    int8_t rssi;               // WiFi signal strength
    uint8_t ip_address[4];     // IP address
    uint32_t uptime;           // Uptime in seconds
} wifi_status_payload_t;

/* Time Data Response Payload */
typedef struct __attribute__((packed)) {
    uint32_t unix_timestamp;    // Unix timestamp
    int16_t timezone_offset;    // Timezone offset in minutes
    uint8_t ntp_synced;        // 0=not synced, 1=synced
} time_data_payload_t;

/* Function Prototypes */
uint8_t uart_calculate_checksum(const uart_packet_t* packet);
bool uart_validate_packet(const uart_packet_t* packet);
bool uart_send_packet(const uart_packet_t* packet);
bool uart_receive_packet(uart_packet_t* packet, uint32_t timeout_ms);
void uart_init_packet(uart_packet_t* packet, uint8_t cmd_type, uint8_t sequence);

#ifdef __cplusplus
}
#endif

#endif /* __UART_PROTOCOL_H */
