/**
 * @file main.cpp
 * @brief ESP8266 EVSE WiFi Module - Main Entry Point
 * @version 3.0.0
 *
 * Architecture: Handlers + Drivers (memory-optimized for ESP8266)
 *
 * Use Cases Implemented:
 * =====================
 * STM32 → ESP8266 Commands:
 * - CMD_MQTT_PUBLISH: Publish OCPP messages to cloud
 * - CMD_GET_TIME: Get NTP-synced time with timezone
 * - CMD_WIFI_STATUS: Get WiFi connection status + RSSI
 * - CMD_CONFIG_UPDATE: Runtime configuration updates
 * - CMD_OTA_REQUEST: Firmware OTA updates via HTTP
 * - CMD_PUBLISH_METER_VALUES: Publish meter readings to MQTT
 *
 * Cloud → ESP8266 → STM32:
 * - RSP_MQTT_RECEIVED: Forward remote commands to STM32
 *
 * ESP8266 → Cloud:
 * - Boot notification (on startup)
 * - Heartbeat (periodic status updates)
 * - OCPP messages (status, meter, transactions)
 *
 * Drivers:
 * - WiFiManager: WiFi connection + config portal
 * - MQTTClient: MQTT pub/sub with reconnection
 * - STM32Communicator: UART protocol with checksums
 * - NTPTimeDriver: Time synchronization
 * - ConfigManager: Unified JSON configuration
 *
 * All use cases fully implemented - no TODOs!
 */

#include <Arduino.h>
#include "core/device_manager.h"
#include "utils/logger.h"

// Single global instance (stack allocated)
DeviceManager deviceManager;

// System diagnostics
struct SystemDiagnostics {
    uint32_t loopCount;
    uint32_t minFreeHeap;
    uint32_t maxHeapFrag;
    uint32_t lastWatchdog;
} diagnostics = {0, 0xFFFFFFFF, 0, 0};

/**
 * @brief Print system diagnostics
 */
void printDiagnostics() {
    uint32_t freeHeap = ESP.getFreeHeap();
    uint32_t heapFrag = ESP.getHeapFragmentation();
    uint32_t uptime = millis() / 1000;

    // Track min/max
    if (freeHeap < diagnostics.minFreeHeap) {
        diagnostics.minFreeHeap = freeHeap;
    }
    if (heapFrag > diagnostics.maxHeapFrag) {
        diagnostics.maxHeapFrag = heapFrag;
    }

    LOG_INFO("Diagnostics", "===== System Status =====");
    LOG_INFO("Diagnostics", "Uptime: %u sec (%u days)", uptime, uptime / 86400);
    LOG_INFO("Diagnostics", "Loop count: %u", diagnostics.loopCount);
    LOG_INFO("Diagnostics", "Free heap: %u bytes (min: %u)", freeHeap, diagnostics.minFreeHeap);
    LOG_INFO("Diagnostics", "Heap frag: %u%% (max: %u%%)", heapFrag, diagnostics.maxHeapFrag);
    LOG_INFO("Diagnostics", "Firmware: %s", FIRMWARE_VERSION);
    LOG_INFO("Diagnostics", "=========================");

    // Memory warnings
    if (freeHeap < 10000) {
        LOG_WARN("Memory", "LOW MEMORY: %u bytes free!", freeHeap);
    }
    if (heapFrag > 50) {
        LOG_WARN("Memory", "HIGH FRAGMENTATION: %u%%!", heapFrag);
    }
}

/**
 * @brief Software watchdog
 */
void feedWatchdog() {
    diagnostics.lastWatchdog = millis();
    ESP.wdtFeed();
}

/**
 * @brief Setup function
 */
void setup() {
    Serial.begin(115200);
    delay(100);

    // Print banner
    Serial.println(F("\n\n"));
    Serial.println(F("╔════════════════════════════════════════════════════╗"));
    Serial.println(F("║  SolEVC Charging Point Controller v3.0           ║"));
    Serial.println(F("║  WiFi Module - ESP8266                            ║"));
    Serial.println(F("║  All Use Cases Implemented ✓                      ║"));
    Serial.println(F("╚════════════════════════════════════════════════════╝"));
    Serial.println();

    // Print chip info
    LOG_INFO("Main", "Chip ID: 0x%08X", ESP.getChipId());
    LOG_INFO("Main", "Flash size: %u bytes", ESP.getFlashChipSize());
    LOG_INFO("Main", "CPU freq: %u MHz", ESP.getCpuFreqMHz());
    LOG_INFO("Main", "SDK version: %s", ESP.getSdkVersion());
    Serial.println();

    // Initialize logger
    Logger::getInstance().setLevel(LogLevel::INFO);

    // Initialize device manager (orchestrates all components)
    LOG_INFO("Main", "Initializing device manager...");
    LOG_INFO("Main", "");
    LOG_INFO("Main", "Initialization steps:");
    LOG_INFO("Main", "  1. Load configuration from config.json");
    LOG_INFO("Main", "  2. Initialize STM32 UART communication");
    LOG_INFO("Main", "  3. Check WiFi provisioning");
    LOG_INFO("Main", "     - If not configured: Start AP 'SolEVC-Provision'");
    LOG_INFO("Main", "     - If configured: Connect to saved network");
    LOG_INFO("Main", "  4. Connect to MQTT broker");
    LOG_INFO("Main", "  5. Synchronize time via NTP");
    LOG_INFO("Main", "");

    if (!deviceManager.init()) {
        LOG_ERROR("Main", "❌ Device initialization FAILED!");
        LOG_ERROR("Main", "Possible reasons:");
        LOG_ERROR("Main", "  • Config file missing/corrupt");
        LOG_ERROR("Main", "  • WiFi not provisioned (check for AP mode)");
        LOG_ERROR("Main", "  • MQTT broker unreachable");
        LOG_ERROR("Main", "System halted - check configuration");
        while (1) {
            delay(1000);
            feedWatchdog();
        }
    }

    LOG_INFO("Main", "✓ All components initialized");
    LOG_INFO("Main", "");
    LOG_INFO("Main", "Use cases ready:");
    LOG_INFO("Main", "  • STM32 UART commands (6 types)");
    LOG_INFO("Main", "  • MQTT pub/sub");
    LOG_INFO("Main", "  • NTP time sync");
    LOG_INFO("Main", "  • OTA updates");
    LOG_INFO("Main", "  • Boot notification");
    LOG_INFO("Main", "  • Heartbeat");
    LOG_INFO("Main", "");
    LOG_INFO("Main", "=== Setup Complete ===");
    printDiagnostics();
    Serial.println();
}

/**
 * @brief Main loop
 */
void loop() {
    // Feed watchdog
    feedWatchdog();

    // Device manager orchestrates all use cases
    deviceManager.run();

    // Increment loop counter
    diagnostics.loopCount++;

    // Print diagnostics every 60 seconds
    static uint32_t lastDiagnostics = 0;
    if (millis() - lastDiagnostics > 60000) {
        printDiagnostics();
        lastDiagnostics = millis();
    }

    // Cooperative multitasking
    yield();
    delay(10);
}
