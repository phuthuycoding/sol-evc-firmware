# UART Communication Protocol Specification

## Overview

The UART protocol enables communication between the STM32F103 Master Controller and ESP8266 WiFi Module in the EVSE system. This document defines the packet structure, command types, and communication flow.

## Physical Layer

### Connection

- **Interface**: UART (Universal Asynchronous Receiver-Transmitter)
- **Baud Rate**: 115200 bps
- **Data Format**: 8N1 (8 data bits, no parity, 1 stop bit)
- **Flow Control**: None (software flow control via protocol)
- **Voltage Level**: 3.3V TTL

### Pin Mapping

```
STM32F103    ESP8266
---------    -------
PA9 (TX1) -> GPIO3 (RX)
PA10 (RX1) <- GPIO1 (TX)
GND       -- GND
```

## Protocol Layer

### Packet Structure

```c
typedef struct __attribute__((packed)) {
    uint8_t start_byte;         // 0xAA - Start delimiter
    uint8_t cmd_type;           // Command/Response type
    uint16_t length;            // Payload length (little-endian)
    uint8_t sequence;           // Sequence number for ACK
    uint8_t payload[512];       // Variable length data
    uint8_t checksum;           // XOR of all bytes except start/end
    uint8_t end_byte;           // 0x55 - End delimiter
} uart_packet_t;
```

### Field Descriptions

| Field      | Size  | Description                                              |
| ---------- | ----- | -------------------------------------------------------- |
| start_byte | 1     | Fixed value 0xAA indicating packet start                 |
| cmd_type   | 1     | Command or response type identifier                      |
| length     | 2     | Payload length in bytes (0-512)                          |
| sequence   | 1     | Sequence number for packet ordering and ACK              |
| payload    | 0-512 | Command-specific data                                    |
| checksum   | 1     | XOR checksum of all bytes except start_byte and end_byte |
| end_byte   | 1     | Fixed value 0x55 indicating packet end                   |

### Checksum Calculation

```c
uint8_t uart_calculate_checksum(const uart_packet_t* packet) {
    uint8_t checksum = 0;
    checksum ^= packet->cmd_type;
    checksum ^= (packet->length & 0xFF);
    checksum ^= (packet->length >> 8);
    checksum ^= packet->sequence;

    for (uint16_t i = 0; i < packet->length; i++) {
        checksum ^= packet->payload[i];
    }

    return checksum;
}
```

## Command Types

### STM32 → ESP8266 Commands

| Command           | Value | Description          | Payload                |
| ----------------- | ----- | -------------------- | ---------------------- |
| CMD_MQTT_PUBLISH  | 0x01  | Publish MQTT message | mqtt_publish_payload_t |
| CMD_GET_TIME      | 0x02  | Request current time | None                   |
| CMD_WIFI_STATUS   | 0x03  | Request WiFi status  | None                   |
| CMD_CONFIG_UPDATE | 0x04  | Update configuration | JSON string            |
| CMD_OTA_REQUEST   | 0x05  | Request OTA update   | ota_request_payload_t  |

### ESP8266 → STM32 Responses

| Response          | Value | Description                 | Payload                |
| ----------------- | ----- | --------------------------- | ---------------------- |
| RSP_MQTT_ACK      | 0x81  | MQTT publish acknowledgment | status_code            |
| RSP_TIME_DATA     | 0x82  | Current time data           | time_data_payload_t    |
| RSP_WIFI_STATUS   | 0x83  | WiFi connection status      | wifi_status_payload_t  |
| RSP_CONFIG_ACK    | 0x84  | Configuration update ACK    | status_code            |
| RSP_MQTT_RECEIVED | 0x85  | Incoming MQTT message       | mqtt_message_payload_t |
| RSP_OTA_STATUS    | 0x86  | OTA update status           | ota_status_payload_t   |

## Payload Structures

### MQTT Publish Payload

```c
typedef struct __attribute__((packed)) {
    char topic[128];            // MQTT topic
    uint8_t qos;               // QoS level (0, 1, 2)
    uint16_t data_length;      // JSON data length
    char data[];               // JSON payload (variable length)
} mqtt_publish_payload_t;
```

### WiFi Status Payload

```c
typedef struct __attribute__((packed)) {
    uint8_t wifi_connected;     // 0=disconnected, 1=connected
    uint8_t mqtt_connected;     // 0=disconnected, 1=connected
    int8_t rssi;               // WiFi signal strength
    uint8_t ip_address[4];     // IP address
    uint32_t uptime;           // Uptime in seconds
} wifi_status_payload_t;
```

### Time Data Payload

```c
typedef struct __attribute__((packed)) {
    uint32_t unix_timestamp;    // Unix timestamp
    int16_t timezone_offset;    // Timezone offset in minutes
    uint8_t ntp_synced;        // 0=not synced, 1=synced
} time_data_payload_t;
```

### MQTT Message Payload

```c
typedef struct __attribute__((packed)) {
    char topic[128];           // MQTT topic
    uint16_t data_length;      // Message data length
    char data[];               // Message data (variable length)
} mqtt_message_payload_t;
```

## Communication Flow

### 1. MQTT Publish Flow

```
STM32F103                    ESP8266
    |                           |
    |-- CMD_MQTT_PUBLISH ------>|
    |   (topic, qos, data)      |
    |                           |-- MQTT Publish
    |                           |   to Broker
    |<----- RSP_MQTT_ACK -------|
    |   (status_code)           |
```

### 2. Incoming MQTT Message Flow

```
STM32F103                    ESP8266
    |                           |
    |                           |<-- MQTT Message
    |                           |   from Broker
    |<-- RSP_MQTT_RECEIVED -----|
    |   (topic, data)           |
    |                           |
    |-- ACK (sequence) -------->|
```

### 3. Time Synchronization Flow

```
STM32F103                    ESP8266
    |                           |
    |-- CMD_GET_TIME ---------->|
    |                           |-- NTP Query
    |                           |   (if needed)
    |<----- RSP_TIME_DATA ------|
    |   (timestamp, tz, sync)   |
```

## Error Handling

### Status Codes

```c
#define STATUS_SUCCESS      0x00
#define STATUS_ERROR        0x01
#define STATUS_TIMEOUT      0x02
#define STATUS_INVALID      0x03
#define STATUS_BUSY         0x04
#define STATUS_NOT_READY    0x05
```

### Timeout and Retry Logic

```c
#define UART_TIMEOUT_MS     1000
#define UART_MAX_RETRIES    3

bool uart_send_with_retry(const uart_packet_t* packet) {
    for (int retry = 0; retry < UART_MAX_RETRIES; retry++) {
        if (uart_send_packet(packet)) {
            uart_packet_t ack;
            if (uart_receive_packet(&ack, UART_TIMEOUT_MS)) {
                if (ack.sequence == packet->sequence) {
                    return true;  // Success
                }
            }
        }
        delay(100 * (retry + 1));  // Exponential backoff
    }
    return false;  // Failed after retries
}
```

### Packet Validation

```c
bool uart_validate_packet(const uart_packet_t* packet) {
    // Check start and end bytes
    if (packet->start_byte != UART_START_BYTE ||
        packet->end_byte != UART_END_BYTE) {
        return false;
    }

    // Check payload length
    if (packet->length > UART_MAX_PAYLOAD) {
        return false;
    }

    // Verify checksum
    uint8_t calculated_checksum = uart_calculate_checksum(packet);
    if (calculated_checksum != packet->checksum) {
        return false;
    }

    return true;
}
```

## Implementation Guidelines

### STM32F103 Implementation

```c
// Initialize UART for ESP8266 communication
void esp8266_comm_init(void) {
    // Configure UART1 at 115200 baud
    // Enable RX interrupt
    // Initialize message queues
}

// Send command to ESP8266
bool esp8266_send_command(uint8_t cmd_type, const void* payload, uint16_t length) {
    uart_packet_t packet;
    uart_init_packet(&packet, cmd_type, get_next_sequence());

    if (length > 0 && payload != NULL) {
        memcpy(packet.payload, payload, length);
        packet.length = length;
    }

    return uart_send_with_retry(&packet);
}
```

### ESP8266 Implementation

```cpp
// Handle incoming packets from STM32
void handle_stm32_packet(const uart_packet_t* packet) {
    switch (packet->cmd_type) {
        case CMD_MQTT_PUBLISH:
            handle_mqtt_publish_cmd(packet);
            break;

        case CMD_GET_TIME:
            handle_time_request_cmd(packet);
            break;

        case CMD_WIFI_STATUS:
            handle_wifi_status_cmd(packet);
            break;

        default:
            send_error_response(packet->sequence, STATUS_INVALID);
            break;
    }
}
```

## Testing and Debugging

### Protocol Analyzer

```c
void uart_debug_packet(const uart_packet_t* packet, bool is_tx) {
    printf("%s: START=0x%02X CMD=0x%02X LEN=%d SEQ=%d CHKSUM=0x%02X END=0x%02X\n",
           is_tx ? "TX" : "RX",
           packet->start_byte,
           packet->cmd_type,
           packet->length,
           packet->sequence,
           packet->checksum,
           packet->end_byte);
}
```

### Loopback Test

```c
// Test UART communication with loopback
void uart_loopback_test(void) {
    uart_packet_t tx_packet, rx_packet;

    uart_init_packet(&tx_packet, CMD_GET_TIME, 1);
    uart_send_packet(&tx_packet);

    if (uart_receive_packet(&rx_packet, 1000)) {
        if (uart_validate_packet(&rx_packet)) {
            printf("Loopback test PASSED\n");
        } else {
            printf("Loopback test FAILED - Invalid packet\n");
        }
    } else {
        printf("Loopback test FAILED - Timeout\n");
    }
}
```

## Performance Characteristics

### Throughput

- **Maximum baud rate**: 115200 bps
- **Effective throughput**: ~10KB/s (accounting for protocol overhead)
- **Packet overhead**: 8 bytes per packet
- **Maximum packet size**: 520 bytes (8 + 512 payload)

### Latency

- **Typical command response**: <10ms
- **MQTT publish latency**: <50ms
- **Time sync latency**: <100ms (including NTP query)

### Reliability

- **Error detection**: XOR checksum
- **Error recovery**: Timeout and retry mechanism
- **Sequence numbering**: Prevents duplicate processing
- **Flow control**: Software-based via ACK/NACK
