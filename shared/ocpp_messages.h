/**
 * @file ocpp_messages.h
 * @brief OCPP message structures and definitions
 * @version 1.0.0
 */

#ifndef __OCPP_MESSAGES_H
#define __OCPP_MESSAGES_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

/* OCPP Message Types */
#define OCPP_BOOT_NOTIFICATION      "boot_notification"
#define OCPP_HEARTBEAT              "heartbeat"
#define OCPP_STATUS_NOTIFICATION    "status_notification"
#define OCPP_START_TRANSACTION      "start_transaction"
#define OCPP_STOP_TRANSACTION       "stop_transaction"
#define OCPP_METER_VALUES           "meter_values"
#define OCPP_REMOTE_START           "remote_start"
#define OCPP_REMOTE_STOP            "remote_stop"

/* Connector Status */
typedef enum {
    CONNECTOR_AVAILABLE = 0,
    CONNECTOR_PREPARING,
    CONNECTOR_CHARGING,
    CONNECTOR_SUSPENDED_EV,
    CONNECTOR_SUSPENDED_EVSE,
    CONNECTOR_FINISHING,
    CONNECTOR_RESERVED,
    CONNECTOR_UNAVAILABLE,
    CONNECTOR_FAULTED
} connector_status_t;

/* Error Codes */
typedef enum {
    ERROR_NO_ERROR = 0,
    ERROR_CONNECTOR_LOCK_FAILURE,
    ERROR_EV_COMMUNICATION_ERROR,
    ERROR_GROUND_FAILURE,
    ERROR_HIGH_TEMPERATURE,
    ERROR_INTERNAL_ERROR,
    ERROR_LOCAL_LIST_CONFLICT,
    ERROR_OTHER_ERROR,
    ERROR_OVER_CURRENT_FAILURE,
    ERROR_OVER_VOLTAGE,
    ERROR_POWER_METER_FAILURE,
    ERROR_POWER_SWITCH_FAILURE,
    ERROR_READER_FAILURE,
    ERROR_RESET_FAILURE,
    ERROR_UNDER_VOLTAGE,
    ERROR_WEAK_SIGNAL
} error_code_t;

/* Transaction Status */
typedef enum {
    TX_STATUS_IDLE = 0,
    TX_STATUS_PREPARING,
    TX_STATUS_CHARGING,
    TX_STATUS_SUSPENDED,
    TX_STATUS_FINISHING,
    TX_STATUS_COMPLETED,
    TX_STATUS_FAULTED
} transaction_status_t;

/* Boot Notification Message */
typedef struct {
    char msg_id[32];
    char timestamp[32];
    char charge_point_model[50];
    char charge_point_vendor[50];
    char firmware_version[50];
    char charge_point_serial_number[50];
} boot_notification_t;

/* Heartbeat Message */
typedef struct {
    char msg_id[32];
    char timestamp[32];
} heartbeat_t;

/* Status Notification Message */
typedef struct {
    char msg_id[32];
    char timestamp[32];
    uint8_t connector_id;
    connector_status_t status;
    error_code_t error_code;
    char info[128];
    char vendor_id[32];
} status_notification_t;

/* Meter Sample */
typedef struct {
    uint32_t energy_wh;
    uint16_t power_w;
    uint16_t voltage_v;
    uint16_t current_a;
    uint16_t frequency_hz;
    int16_t temperature_c;
    uint8_t power_factor_pct;
    uint32_t energy_kvarh;
} meter_sample_t;

/* Meter Values Message */
typedef struct {
    char msg_id[32];
    char timestamp[32];
    uint8_t connector_id;
    uint32_t transaction_id;
    meter_sample_t sample;
} meter_values_t;

/* Start Transaction Message */
typedef struct {
    char msg_id[32];
    char timestamp[32];
    uint8_t connector_id;
    char id_tag[20];
    uint32_t meter_start;
    uint32_t reservation_id;
} start_transaction_t;

/* Stop Transaction Message */
typedef struct {
    char msg_id[32];
    char timestamp[32];
    uint32_t transaction_id;
    char id_tag[20];
    uint32_t meter_stop;
    char reason[32];
} stop_transaction_t;

/* Remote Start Command */
typedef struct {
    char msg_id[32];
    uint8_t connector_id;
    char id_tag[20];
    uint32_t charging_profile_id;
} remote_start_cmd_t;

/* Remote Stop Command */
typedef struct {
    char msg_id[32];
    uint32_t transaction_id;
} remote_stop_cmd_t;

/* OCPP Response */
typedef struct {
    char msg_id[32];
    char status[16];        // Accepted/Rejected
    char error_code[32];
    char error_description[128];
} ocpp_response_t;

#ifdef __cplusplus
}
#endif

#endif /* __OCPP_MESSAGES_H */
