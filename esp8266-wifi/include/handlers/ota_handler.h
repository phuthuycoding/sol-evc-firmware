/**
 * @file ota_handler.h
 * @brief OTA Update Handler
 * @version 2.0.0
 *
 * Handle OTA firmware updates via HTTP
 */

#ifndef OTA_HANDLER_H
#define OTA_HANDLER_H

#include "drivers/communication/stm32_comm.h"
#include <Arduino.h>

/**
 * @brief OTA Update Result
 */
enum class OTAResult {
    SUCCESS = 0,
    FAILED_HTTP,
    FAILED_NO_SPACE,
    FAILED_FLASH,
    FAILED_VERIFY,
    FAILED_INVALID_URL
};

/**
 * @brief OTA handler (stateless)
 */
class OTAHandler {
public:
    /**
     * @brief Check if OTA update is available
     * @param url Firmware version check URL
     * @param currentVersion Current version
     * @param newVersion Output: New version if available
     * @return true if update available
     */
    static bool checkUpdate(
        const char* url,
        const char* currentVersion,
        char* newVersion,
        size_t newVersionSize
    );

    /**
     * @brief Perform OTA update from HTTP
     * @param url Firmware binary URL
     * @return OTAResult code
     */
    static OTAResult performUpdate(const char* url);

    /**
     * @brief Handle OTA request from STM32
     * @param packet UART packet with OTA URL
     * @param stm32 STM32 communicator (for status response)
     */
    static void handleFromSTM32(
        const uart_packet_t& packet,
        STM32Communicator& stm32
    );

    /**
     * @brief Get current firmware version
     */
    static const char* getCurrentVersion();

private:
    static bool verifyUpdate();
    static void sendOTAStatus(STM32Communicator& stm32, uint8_t sequence, OTAResult result);
};

#endif // OTA_HANDLER_H
