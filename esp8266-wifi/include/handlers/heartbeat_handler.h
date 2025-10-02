/**
 * @file heartbeat_handler.h
 * @brief Heartbeat Handler (Stateless, stack-based)
 * @version 1.0.0
 *
 * Business logic: Send heartbeat to MQTT broker
 */

#ifndef HEARTBEAT_HANDLER_H
#define HEARTBEAT_HANDLER_H

#include "drivers/mqtt/mqtt_client.h"
#include "drivers/network/wifi_manager.h"
#include "drivers/config/unified_config.h"
#include <Arduino.h>

/**
 * @brief Heartbeat handler (stateless)
 *
 * Usage:
 *   HeartbeatHandler::execute(mqtt, wifi, config, bootTime);
 */
class HeartbeatHandler {
public:
    /**
     * @brief Execute heartbeat
     * @param mqtt MQTT client reference
     * @param wifi WiFi manager reference
     * @param config Device config reference
     * @param bootTime Boot timestamp (for uptime calculation)
     * @return true if heartbeat sent successfully
     */
    static bool execute(
        MQTTClient& mqtt,
        CustomWiFiManager& wifi,
        const DeviceConfig& config,
        uint32_t bootTime
    );
};

#endif // HEARTBEAT_HANDLER_H
