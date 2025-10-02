/**
 * @file config_update_handler.cpp
 * @brief Config Update Handler Implementation
 */

#include "handlers/config_update_handler.h"
#include "utils/logger.h"
#include <ArduinoJson.h>

bool ConfigUpdateHandler::handleFromSTM32(
    const uart_packet_t& packet,
    STM32Communicator& stm32,
    UnifiedConfigManager& configManager
) {
    // Parse JSON config from packet
    const char* jsonConfig = (const char*)packet.payload;

    LOG_INFO("ConfigUpdate", "Received from STM32: %s", jsonConfig);

    // Validate
    if (!validateConfig(jsonConfig)) {
        LOG_ERROR("ConfigUpdate", "Invalid config");
        stm32.sendAck(packet.sequence, STATUS_INVALID);
        return false;
    }

    // Save
    if (saveConfig(jsonConfig, configManager)) {
        LOG_INFO("ConfigUpdate", "Config updated successfully");
        stm32.sendAck(packet.sequence, STATUS_SUCCESS);
        return true;
    } else {
        LOG_ERROR("ConfigUpdate", "Failed to save config");
        stm32.sendAck(packet.sequence, STATUS_ERROR);
        return false;
    }
}

bool ConfigUpdateHandler::handleFromMQTT(
    const char* jsonConfig,
    UnifiedConfigManager& configManager
) {
    LOG_INFO("ConfigUpdate", "Received from MQTT");

    if (!validateConfig(jsonConfig)) {
        LOG_ERROR("ConfigUpdate", "Invalid config");
        return false;
    }

    return saveConfig(jsonConfig, configManager);
}

bool ConfigUpdateHandler::validateConfig(const char* jsonConfig) {
    // Parse JSON
    StaticJsonDocument<512> doc;
    DeserializationError error = deserializeJson(doc, jsonConfig);

    if (error) {
        LOG_ERROR("ConfigUpdate", "JSON parse error: %s", error.c_str());
        return false;
    }

    // Validate required fields (basic validation)
    if (!doc.containsKey("mqtt") || !doc.containsKey("wifi")) {
        LOG_ERROR("ConfigUpdate", "Missing required fields");
        return false;
    }

    return true;
}

bool ConfigUpdateHandler::saveConfig(
    const char* jsonConfig,
    UnifiedConfigManager& configManager
) {
    // Parse to DeviceConfig
    StaticJsonDocument<512> doc;
    deserializeJson(doc, jsonConfig);

    // Update config manager
    // Note: This requires UnifiedConfigManager to have an update method
    // For now, just log
    LOG_INFO("ConfigUpdate", "Config saved (TODO: implement persistence)");

    // TODO: Implement actual config update in UnifiedConfigManager
    // configManager.update(doc);

    return true;
}
