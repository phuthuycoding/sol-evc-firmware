/**
 * @file config_update_handler.h
 * @brief Config Update Handler
 * @version 1.0.0
 *
 * Handle configuration updates from STM32 or MQTT
 */

#ifndef CONFIG_UPDATE_HANDLER_H
#define CONFIG_UPDATE_HANDLER_H

#include "drivers/config/unified_config.h"
#include "drivers/communication/stm32_comm.h"
#include <Arduino.h>

/**
 * @brief Config Update handler (stateless)
 */
class ConfigUpdateHandler {
public:
    /**
     * @brief Handle config update from STM32
     * @param packet UART packet with JSON config
     * @param stm32 STM32 communicator (for ACK)
     * @param configManager Config manager to update
     * @return true if update successful
     */
    static bool handleFromSTM32(
        const uart_packet_t& packet,
        STM32Communicator& stm32,
        UnifiedConfigManager& configManager
    );

    /**
     * @brief Handle config update from MQTT
     * @param jsonConfig JSON config string
     * @param configManager Config manager to update
     * @return true if update successful
     */
    static bool handleFromMQTT(
        const char* jsonConfig,
        UnifiedConfigManager& configManager
    );

private:
    static bool validateConfig(const char* jsonConfig);
    static bool saveConfig(const char* jsonConfig, UnifiedConfigManager& configManager);
};

#endif // CONFIG_UPDATE_HANDLER_H
