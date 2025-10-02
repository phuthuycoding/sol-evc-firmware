/**
 * @file device_config.h
 * @brief Common device configuration and constants
 * @version 1.0.0
 */

#ifndef __DEVICE_CONFIG_H
#define __DEVICE_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/* Device Information */
#define DEVICE_VENDOR           "PhuthuyCoding"
#define DEVICE_MODEL            "EVSE-STM32F103"
#define FIRMWARE_VERSION        "1.0.0"
#define HARDWARE_VERSION        "1.0"

/* Hardware Configuration */
#define MAX_CONNECTORS          10
#define MAX_RELAY_CHANNELS      10
#define MAX_METER_CHANNELS      10
#define MAX_RS485_SLAVES        8

/* Communication Settings */
#define UART_BAUD_RATE          115200
#define RS485_BAUD_RATE         9600
#define SPI_CLOCK_SPEED         1000000  // 1MHz

/* Power Specifications */
#define MAX_CURRENT_PER_CHANNEL 30       // 30A per SSR
#define MAX_VOLTAGE             240      // 240V AC
#define MAX_POWER_PER_CHANNEL   7200     // 7.2kW per channel

/* Timing Configuration */
#define HEARTBEAT_INTERVAL      30000    // 30 seconds
#define METER_READING_INTERVAL  1000     // 1 second
#define STATUS_CHECK_INTERVAL   100      // 100ms
#define SAFETY_CHECK_INTERVAL   50       // 50ms
#define WATCHDOG_TIMEOUT        5000     // 5 seconds

/* Memory Configuration */
#define FLASH_SIZE              65536    // 64KB
#define RAM_SIZE                20480    // 20KB
#define EEPROM_SIZE             1024     // 1KB for config
#define STACK_SIZE_MAIN         2048
#define STACK_SIZE_TASK         1024

/* GPIO Pin Definitions */
#define LED_STATUS_PIN          13       // PC13
#define RELAY_BASE_PIN          0        // PA0-PA9
#define CS5460A_CS_BASE_PIN     0        // PB0-PB9
#define ESP8266_RESET_PIN       10       // PA10
#define RS485_DE_PIN            11       // PA11

/* MQTT Topics */
#define MQTT_TOPIC_BASE         "ocpp"
#define MQTT_TOPIC_EVENT        "event/0"
#define MQTT_TOPIC_STATUS       "status"
#define MQTT_TOPIC_METER        "meter"
#define MQTT_TOPIC_TRANSACTION  "transaction"
#define MQTT_TOPIC_CMD          "cmd"

/* Safety Limits */
#define OVERCURRENT_THRESHOLD   35       // 35A (above 30A rating)
#define OVERVOLTAGE_THRESHOLD   260      // 260V
#define UNDERVOLTAGE_THRESHOLD  200      // 200V
#define OVERTEMP_THRESHOLD      80       // 80°C
#define MAX_TRANSACTION_TIME    28800    // 8 hours

/* Error Handling */
#define MAX_ERROR_COUNT         10
#define ERROR_RESET_TIME        300000   // 5 minutes
#define COMM_TIMEOUT            5000     // 5 seconds
#define RETRY_DELAY             1000     // 1 second

/* Device States */
typedef enum {
    DEVICE_STATE_BOOT = 0,
    DEVICE_STATE_PENDING,
    DEVICE_STATE_ACCEPTED,
    DEVICE_STATE_OPERATIONAL,
    DEVICE_STATE_FAULTED,
    DEVICE_STATE_OFFLINE
} device_state_t;

/* Connector Configuration */
typedef struct {
    uint8_t connector_id;
    uint8_t relay_channel;
    uint8_t meter_channel;
    uint16_t max_current;
    uint16_t max_power;
    bool enabled;
} connector_config_t;

/* Device Configuration */
typedef struct {
    char station_id[32];
    char device_id[32];
    char serial_number[32];
    uint8_t connector_count;
    connector_config_t connectors[MAX_CONNECTORS];
    uint32_t heartbeat_interval;
    uint32_t meter_interval;
    bool debug_enabled;
} device_config_t;

#ifdef __cplusplus
}
#endif

#endif /* __DEVICE_CONFIG_H */
