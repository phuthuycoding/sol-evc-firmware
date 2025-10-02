/**
 * @file unified_config.h
 * @brief Unified lightweight configuration system for ESP8266
 * @version 1.0.0
 *
 * Design: Single struct with all configs, no dynamic allocation
 */

#ifndef UNIFIED_CONFIG_H
#define UNIFIED_CONFIG_H

#include <Arduino.h>

/**
 * @brief Complete device configuration (stack allocated)
 * Total size: ~600 bytes
 */
struct DeviceConfig {
    /* Device Identity */
    char stationId[32];
    char deviceId[32];
    char serialNumber[32];

    /* WiFi Configuration */
    struct {
        char ssid[32];
        char password[64];
        bool autoConnect;
        char apNamePrefix[16];      // "EVSE-" + MAC suffix
        uint32_t configPortalTimeout;
    } wifi;

    /* MQTT Configuration */
    struct {
        char broker[64];
        uint16_t port;
        char username[32];
        char password[64];
        char clientIdPrefix[16];    // Will append device ID
        bool tlsEnabled;
        uint16_t keepAlive;
    } mqtt;

    /* Provisioning Configuration */
    struct {
        char serverUrl[64];
        uint16_t serverPort;
        uint32_t timeoutMs;
        uint8_t maxRetries;
        uint32_t retryIntervalMs;
    } provisioning;

    /* System Configuration */
    struct {
        bool otaEnabled;
        char otaPassword[32];
        uint32_t heartbeatInterval;
        bool debugEnabled;
        uint8_t logLevel;           // 0=ERROR, 1=WARN, 2=INFO, 3=DEBUG
    } system;

    /* Web Server Configuration */
    struct {
        bool enabled;
        uint16_t port;
        char username[32];
        char password[32];
        bool authRequired;
    } web;

    /* Validation flags */
    bool isValid;
    uint8_t version;                // Config schema version
};

/**
 * @brief Lightweight config manager (stack allocated)
 * No STL, no dynamic allocation, ESP8266 optimized
 */
class UnifiedConfigManager {
private:
    DeviceConfig config;
    bool initialized;

    // Helper methods
    void loadFactoryDefaults();
    bool validateConfig() const;
    void sanitizeConfig();

public:
    UnifiedConfigManager();

    /**
     * @brief Initialize config system
     * @return true if success
     */
    bool init();

    /**
     * @brief Load configuration from LittleFS
     * Priority: Runtime config -> Bootstrap config -> Factory defaults
     * @return true if loaded successfully
     */
    bool load();

    /**
     * @brief Save current config to LittleFS
     * @return true if saved successfully
     */
    bool save();

    /**
     * @brief Reset to factory defaults
     * @return true if reset successfully
     */
    bool resetToDefaults();

    /**
     * @brief Get read-only reference to config
     */
    const DeviceConfig& get() const { return config; }

    /**
     * @brief Get writable reference (use with care!)
     */
    DeviceConfig& getMutable() { return config; }

    /**
     * @brief Update config from JSON string
     * @param jsonStr JSON configuration string
     * @return true if updated successfully
     */
    bool updateFromJson(const char* jsonStr);

    /**
     * @brief Export config to JSON string
     * @param buffer Output buffer
     * @param size Buffer size
     * @param includeSecrets Include passwords in export
     * @return true if exported successfully
     */
    bool exportToJson(char* buffer, size_t size, bool includeSecrets = false);

    /**
     * @brief Print current config to Serial (debug)
     */
    void printConfig() const;

    /**
     * @brief Validate current configuration
     * @return true if valid
     */
    bool isValid() const { return config.isValid && validateConfig(); }
};

/**
 * @brief Helper functions for config building
 */
namespace ConfigHelper {
    /**
     * @brief Build MQTT client ID
     * @param buffer Output buffer
     * @param size Buffer size
     * @param config Device config
     */
    void buildMqttClientId(char* buffer, size_t size, const DeviceConfig& config);

    /**
     * @brief Build WiFi AP name
     * @param buffer Output buffer
     * @param size Buffer size
     * @param config Device config
     */
    void buildApName(char* buffer, size_t size, const DeviceConfig& config);

    /**
     * @brief Generate device serial number from MAC
     * @param buffer Output buffer
     * @param size Buffer size
     */
    void generateSerialFromMac(char* buffer, size_t size);
}

#endif // UNIFIED_CONFIG_H
