/**
 * @file ota_handler.cpp
 * @brief OTA Update Handler Implementation
 */

#include "handlers/ota_handler.h"
#include "utils/logger.h"
#include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>
#include <WiFiClient.h>

bool OTAHandler::checkUpdate(
    const char* url,
    const char* currentVersion,
    char* newVersion,
    size_t newVersionSize
) {
    LOG_INFO("OTA", "Check update: %s (current: %s)", url, currentVersion);

    WiFiClient client;
    HTTPClient http;

    http.begin(client, url);
    int httpCode = http.GET();

    if (httpCode == HTTP_CODE_OK) {
        String version = http.getString();
        version.trim();

        // Copy to output
        strncpy(newVersion, version.c_str(), newVersionSize - 1);
        newVersion[newVersionSize - 1] = '\0';

        http.end();

        // Compare versions
        if (strcmp(currentVersion, newVersion) != 0) {
            LOG_INFO("OTA", "Update available: %s -> %s", currentVersion, newVersion);
            return true;
        } else {
            LOG_INFO("OTA", "Already up to date");
            return false;
        }
    } else {
        LOG_ERROR("OTA", "HTTP error: %d", httpCode);
        http.end();
        return false;
    }
}

OTAResult OTAHandler::performUpdate(const char* url) {
    LOG_INFO("OTA", "Starting update: %s", url);

    // Check free space
    uint32_t freeSpace = ESP.getFreeSketchSpace();
    LOG_INFO("OTA", "Free sketch space: %u bytes", freeSpace);

    if (freeSpace < 100000) {  // Need at least 100KB
        LOG_ERROR("OTA", "Not enough space");
        return OTAResult::FAILED_NO_SPACE;
    }

    // Perform update
    WiFiClient client;
    ESPhttpUpdate.setLedPin(LED_BUILTIN, LOW);  // Blink LED during update

    t_httpUpdate_return ret = ESPhttpUpdate.update(client, url);

    switch (ret) {
        case HTTP_UPDATE_FAILED:
            LOG_ERROR("OTA", "Update failed: %s",
                     ESPhttpUpdate.getLastErrorString().c_str());
            return OTAResult::FAILED_HTTP;

        case HTTP_UPDATE_NO_UPDATES:
            LOG_INFO("OTA", "No updates available");
            return OTAResult::SUCCESS;

        case HTTP_UPDATE_OK:
            LOG_INFO("OTA", "Update successful! Rebooting...");
            delay(1000);
            ESP.restart();
            return OTAResult::SUCCESS;

        default:
            return OTAResult::FAILED_FLASH;
    }
}

void OTAHandler::handleFromSTM32(
    const uart_packet_t& packet,
    STM32Communicator& stm32
) {
    // Extract URL from packet
    const char* url = (const char*)packet.payload;

    LOG_INFO("OTA", "Request from STM32: %s", url);

    // Validate URL
    if (strlen(url) == 0 || strlen(url) > 256) {
        sendOTAStatus(stm32, packet.sequence, OTAResult::FAILED_INVALID_URL);
        return;
    }

    // Perform update
    OTAResult result = performUpdate(url);

    // Send status (only if update failed, success will reboot)
    if (result != OTAResult::SUCCESS) {
        sendOTAStatus(stm32, packet.sequence, result);
    }
}

const char* OTAHandler::getCurrentVersion() {
    return FIRMWARE_VERSION;  // From shared/device_config.h
}

bool OTAHandler::verifyUpdate() {
    // TODO: Implement checksum verification
    return true;
}

void OTAHandler::sendOTAStatus(
    STM32Communicator& stm32,
    uint8_t sequence,
    OTAResult result
) {
    uart_packet_t response;
    uart_init_packet(&response, RSP_OTA_STATUS, sequence);

    struct {
        uint8_t status;
        char message[64];
    } payload;

    payload.status = (uint8_t)result;

    switch (result) {
        case OTAResult::SUCCESS:
            strcpy(payload.message, "Update successful");
            break;
        case OTAResult::FAILED_HTTP:
            strcpy(payload.message, "HTTP fetch failed");
            break;
        case OTAResult::FAILED_NO_SPACE:
            strcpy(payload.message, "Insufficient space");
            break;
        case OTAResult::FAILED_FLASH:
            strcpy(payload.message, "Flash write failed");
            break;
        case OTAResult::FAILED_VERIFY:
            strcpy(payload.message, "Verification failed");
            break;
        case OTAResult::FAILED_INVALID_URL:
            strcpy(payload.message, "Invalid URL");
            break;
    }

    memcpy(response.payload, &payload, sizeof(payload));
    response.length = sizeof(payload);

    stm32.sendPacket(response);
    LOG_INFO("OTA", "Status sent: %s", payload.message);
}

