/**
 * @file ntp_time.cpp
 * @brief NTP Time Synchronization Driver Implementation
 */

#include "drivers/time/ntp_time.h"
#include "utils/logger.h"

NTPTimeDriver::NTPTimeDriver()
    : ntpClient(nullptr), synced(false), lastSync(0), timezoneOffset(0) {
}

NTPTimeDriver::~NTPTimeDriver() {
    if (ntpClient) {
        delete ntpClient;
        ntpClient = nullptr;
    }
}

void NTPTimeDriver::init(const char* server, int16_t tzOffset) {
    timezoneOffset = tzOffset;

    // Create NTP client (offset in seconds)
    ntpClient = new NTPClient(udp, server, tzOffset * 60, SYNC_INTERVAL);

    ntpClient->begin();
    LOG_INFO("NTP", "Initialized: server=%s, tz=%d min", server, tzOffset);

    // Try initial sync
    forceSync();
}

void NTPTimeDriver::update() {
    if (!ntpClient) return;

    ntpClient->update();

    // Check if we need to sync
    if (millis() - lastSync > SYNC_INTERVAL) {
        forceSync();
    }
}

bool NTPTimeDriver::forceSync() {
    if (!ntpClient) return false;

    LOG_INFO("NTP", "Forcing sync...");

    bool success = ntpClient->forceUpdate();

    if (success) {
        synced = true;
        lastSync = millis();
        LOG_INFO("NTP", "Sync OK: %s", ntpClient->getFormattedTime().c_str());
    } else {
        LOG_ERROR("NTP", "Sync failed");
    }

    return success;
}

uint32_t NTPTimeDriver::getUnixTime() {
    if (!ntpClient) return millis() / 1000;  // Fallback to uptime

    return ntpClient->getEpochTime();
}

String NTPTimeDriver::getFormattedTime() {
    if (!ntpClient) return "00:00:00";

    return ntpClient->getFormattedTime();
}
