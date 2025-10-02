/**
 * @file stm32_command_handler.h
 * @brief STM32 Command Handler (Stateless)
 * @version 1.0.0
 *
 * Business logic: Handle commands from STM32
 */

#ifndef STM32_COMMAND_HANDLER_H
#define STM32_COMMAND_HANDLER_H

#include "drivers/communication/stm32_comm.h"
#include "drivers/mqtt/mqtt_client.h"
#include "drivers/config/unified_config.h"
#include "drivers/time/ntp_time.h"
#include "../../shared/uart_protocol.h"

/**
 * @brief STM32 Command handler (stateless)
 *
 * Routes incoming STM32 commands to appropriate actions:
 * - CMD_MQTT_PUBLISH -> Publish to MQTT
 * - CMD_GET_TIME -> Send time response
 * - CMD_WIFI_STATUS -> Send WiFi status
 */
class STM32CommandHandler {
public:
    /**
     * @brief Execute command handler
     * @param packet Received UART packet from STM32
     * @param stm32 STM32 communicator reference (for ACK)
     * @param mqtt MQTT client reference
     * @param ntpTime NTP time driver reference
     * @param configManager Config manager reference
     */
    static void execute(
        const uart_packet_t& packet,
        STM32Communicator& stm32,
        MQTTClient& mqtt,
        NTPTimeDriver& ntpTime,
        UnifiedConfigManager& configManager
    );

private:
    // Internal handlers
    static void handleMqttPublish(const uart_packet_t& packet, STM32Communicator& stm32, MQTTClient& mqtt);
    static void handleGetTime(const uart_packet_t& packet, STM32Communicator& stm32, NTPTimeDriver& ntpTime);
    static void handleWiFiStatus(const uart_packet_t& packet, STM32Communicator& stm32, MQTTClient& mqtt);
    static void handleConfigUpdate(const uart_packet_t& packet, STM32Communicator& stm32, UnifiedConfigManager& configManager);
    static void handleOTARequest(const uart_packet_t& packet, STM32Communicator& stm32);
    static void handlePublishMeterValues(const uart_packet_t& packet, STM32Communicator& stm32, MQTTClient& mqtt, const DeviceConfig& config);
};

#endif // STM32_COMMAND_HANDLER_H
