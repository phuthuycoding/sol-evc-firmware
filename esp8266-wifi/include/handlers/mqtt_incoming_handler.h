/**
 * @file mqtt_incoming_handler.h
 * @brief MQTT Incoming Message Handler (Stateless)
 * @version 1.0.0
 *
 * Handle incoming MQTT messages from cloud and forward to STM32
 */

#ifndef MQTT_INCOMING_HANDLER_H
#define MQTT_INCOMING_HANDLER_H

#include "drivers/communication/stm32_comm.h"
#include "drivers/config/unified_config.h"
#include <Arduino.h>

/**
 * @brief MQTT Incoming handler (stateless)
 *
 * Routes incoming MQTT commands to STM32:
 * - remote_start → CMD to STM32
 * - remote_stop → CMD to STM32
 * - reset → CMD to STM32
 */
class MQTTIncomingHandler {
public:
    /**
     * @brief Execute incoming message handler
     * @param topic MQTT topic
     * @param payload Message payload
     * @param length Payload length
     * @param stm32 STM32 communicator reference (to forward message)
     * @param config Device config reference
     */
    static void execute(
        const char* topic,
        const char* payload,
        uint16_t length,
        STM32Communicator& stm32,
        const DeviceConfig& config
    );

private:
    static bool isCommandTopic(const char* topic, const DeviceConfig& config);
    static void forwardToSTM32(const char* topic, const char* payload, uint16_t length, STM32Communicator& stm32);
};

#endif // MQTT_INCOMING_HANDLER_H
