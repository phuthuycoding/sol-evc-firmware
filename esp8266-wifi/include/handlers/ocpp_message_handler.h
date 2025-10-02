/**
 * @file ocpp_message_handler.h
 * @brief OCPP Message Handler (Stateless)
 * @version 1.0.0
 *
 * Handle OCPP messages from STM32 and publish to MQTT
 */

#ifndef OCPP_MESSAGE_HANDLER_H
#define OCPP_MESSAGE_HANDLER_H

#include "drivers/mqtt/mqtt_client.h"
#include "drivers/config/unified_config.h"
#include "../../shared/ocpp_messages.h"
#include <Arduino.h>

/**
 * @brief OCPP Message handler (stateless)
 *
 * Handles:
 * - Status Notification
 * - Meter Values
 * - Start/Stop Transaction
 * - Boot Notification
 */
class OCPPMessageHandler {
public:
    /**
     * @brief Publish Status Notification
     */
    static bool publishStatusNotification(
        MQTTClient& mqtt,
        const DeviceConfig& config,
        const status_notification_t& status
    );

    /**
     * @brief Publish Meter Values
     */
    static bool publishMeterValues(
        MQTTClient& mqtt,
        const DeviceConfig& config,
        const meter_values_t& meter
    );

    /**
     * @brief Publish Start Transaction
     */
    static bool publishStartTransaction(
        MQTTClient& mqtt,
        const DeviceConfig& config,
        const start_transaction_t& txStart
    );

    /**
     * @brief Publish Stop Transaction
     */
    static bool publishStopTransaction(
        MQTTClient& mqtt,
        const DeviceConfig& config,
        const stop_transaction_t& txStop
    );

    /**
     * @brief Publish Boot Notification
     */
    static bool publishBootNotification(
        MQTTClient& mqtt,
        const DeviceConfig& config,
        const boot_notification_t& boot
    );

private:
    static void buildStatusTopic(char* buffer, size_t size, const DeviceConfig& config, uint8_t connectorId);
    static void buildMeterTopic(char* buffer, size_t size, const DeviceConfig& config, uint8_t connectorId);
    static void buildTransactionTopic(char* buffer, size_t size, const DeviceConfig& config, const char* type);
    static void buildBootTopic(char* buffer, size_t size, const DeviceConfig& config);
};

#endif // OCPP_MESSAGE_HANDLER_H
