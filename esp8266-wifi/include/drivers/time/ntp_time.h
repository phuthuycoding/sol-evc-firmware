/**
 * @file ntp_time.h
 * @brief NTP Time Synchronization Driver
 * @version 1.0.0
 */

#ifndef NTP_TIME_H
#define NTP_TIME_H

#include <Arduino.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

/**
 * @brief NTP Time Driver
 *
 * Handles NTP time synchronization
 */
class NTPTimeDriver {
private:
    WiFiUDP udp;
    NTPClient* ntpClient;
    bool synced;
    uint32_t lastSync;
    int16_t timezoneOffset;  // Offset in minutes

    static constexpr uint32_t SYNC_INTERVAL = 3600000;  // 1 hour

public:
    NTPTimeDriver();
    ~NTPTimeDriver();

    /**
     * @brief Initialize NTP client
     * @param server NTP server (default: pool.ntp.org)
     * @param tzOffset Timezone offset in minutes (default: 0 UTC)
     */
    void init(const char* server = "pool.ntp.org", int16_t tzOffset = 0);

    /**
     * @brief Update NTP time (call in loop)
     */
    void update();

    /**
     * @brief Force sync now
     * @return true if sync successful
     */
    bool forceSync();

    /**
     * @brief Get current unix timestamp
     */
    uint32_t getUnixTime();

    /**
     * @brief Check if time is synced
     */
    bool isSynced() const { return synced; }

    /**
     * @brief Get timezone offset in minutes
     */
    int16_t getTimezoneOffset() const { return timezoneOffset; }

    /**
     * @brief Get formatted time string
     */
    String getFormattedTime();
};

#endif // NTP_TIME_H
