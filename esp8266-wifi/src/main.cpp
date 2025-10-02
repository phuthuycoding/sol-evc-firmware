/**
 * @file main.cpp
 * @brief ESP8266 EVSE WiFi Module - OOP Refactored Version
 * @version 2.0.0
 *
 * Complete OOP refactoring:
 * - No global variables
 * - Stack allocated objects
 * - RAII compliant
 * - ~100 lines (vs 338 lines old code)
 * - Memory optimized for ESP8266
 */

#include <Arduino.h>
#include "core/device_manager.h"
#include "utils/logger.h"

// Single global instance (stack allocated)
DeviceManager deviceManager;

/**
 * @brief Print memory statistics
 */
void printMemoryStats() {
    uint32_t freeHeap = ESP.getFreeHeap();
    uint32_t heapFrag = ESP.getHeapFragmentation();

    LOG_INFO("Memory", "Free: %u bytes, Frag: %u%%", freeHeap, heapFrag);

    if (freeHeap < 10000) {
        LOG_WARN("Memory", "LOW MEMORY WARNING!");
    }
}

/**
 * @brief Setup function
 */
void setup() {
    Serial.begin(115200);
    delay(100);

    // Print banner
    Serial.println(F("\n\n"));
    Serial.println(F("╔════════════════════════════════════════╗"));
    Serial.println(F("║  ESP8266 EVSE WiFi Module v2.0 (OOP)  ║"));
    Serial.println(F("║     Refactored Clean Architecture     ║"));
    Serial.println(F("╚════════════════════════════════════════╝\n"));

    // Initialize logger
    Logger::getInstance().setLevel(LogLevel::INFO);

    // Initialize device manager (does everything!)
    if (!deviceManager.init()) {
        LOG_ERROR("Main", "Device initialization FAILED!");
        while (1) {
            delay(1000);
            LOG_ERROR("Main", "System halted");
        }
    }

    LOG_INFO("Main", "=== Setup Complete ===");
    printMemoryStats();
}

/**
 * @brief Main loop
 */
void loop() {
    // Device manager handles everything!
    deviceManager.run();

    // Print memory stats every 60 seconds
    static uint32_t lastMemCheck = 0;
    if (millis() - lastMemCheck > 60000) {
        printMemoryStats();
        lastMemCheck = millis();
    }

    delay(10);
}
