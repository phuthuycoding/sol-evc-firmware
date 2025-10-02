/**
 * @file unified_config.cpp
 * @brief Unified configuration implementation
 * @version 1.0.0
 */

#include "config/unified_config.h"
#include <LittleFS.h>
#include <ArduinoJson.h>
#include <ESP8266WiFi.h>

/* Configuration file paths */
static constexpr const char* CONFIG_FILE = "/unified_config.json";
static constexpr const char* BACKUP_FILE = "/unified_config.bak";
static constexpr uint8_t CONFIG_VERSION = 1;

/**
 * @brief Constructor
 */
UnifiedConfigManager::UnifiedConfigManager()
    : initialized(false) {
    memset(&config, 0, sizeof(DeviceConfig));
    loadFactoryDefaults();
}

/**
 * @brief Initialize config system
 */
bool UnifiedConfigManager::init() {
    if (initialized) return true;

    Serial.println(F("[Config] Initializing unified config system..."));

    // Initialize filesystem
    if (!LittleFS.begin()) {
        Serial.println(F("[Config] Failed to mount LittleFS"));
        return false;
    }

    // Load configuration
    if (!load()) {
        Serial.println(F("[Config] No saved config, using factory defaults"));
        loadFactoryDefaults();
        save(); // Save defaults
    }

    initialized = true;
    Serial.println(F("[Config] Config system initialized"));
    printConfig();

    return true;
}

/**
 * @brief Load factory defaults
 */
void UnifiedConfigManager::loadFactoryDefaults() {
    Serial.println(F("[Config] Loading factory defaults..."));

    memset(&config, 0, sizeof(DeviceConfig));

    // Device identity (will be overwritten by MAC-based values)
    strncpy(config.stationId, "station001", sizeof(config.stationId));
    strncpy(config.deviceId, "device001", sizeof(config.deviceId));
    ConfigHelper::generateSerialFromMac(config.serialNumber, sizeof(config.serialNumber));

    // WiFi defaults
    config.wifi.ssid[0] = '\0'; // Empty - user must configure
    config.wifi.password[0] = '\0';
    config.wifi.autoConnect = true;
    strncpy(config.wifi.apNamePrefix, "SolEVC-Provision", sizeof(config.wifi.apNamePrefix));
    config.wifi.configPortalTimeout = 300; // 5 minutes

    // MQTT defaults
    strncpy(config.mqtt.broker, "localhost", sizeof(config.mqtt.broker));
    config.mqtt.port = 1883;
    config.mqtt.username[0] = '\0';
    config.mqtt.password[0] = '\0';
    strncpy(config.mqtt.clientIdPrefix, "evse-", sizeof(config.mqtt.clientIdPrefix));
    config.mqtt.tlsEnabled = false;
    config.mqtt.keepAlive = 60;

    // Provisioning defaults
    #ifdef ENV_PROD
    strncpy(config.provisioning.serverUrl, "api.evse-cloud.com", sizeof(config.provisioning.serverUrl));
    #elif defined(ENV_STAGING)
    strncpy(config.provisioning.serverUrl, "staging-api.evse.cloud", sizeof(config.provisioning.serverUrl));
    #else
    strncpy(config.provisioning.serverUrl, "dev-api.evse.local", sizeof(config.provisioning.serverUrl));
    #endif
    config.provisioning.serverPort = 443;
    config.provisioning.timeoutMs = 300000; // 5 minutes
    config.provisioning.maxRetries = 5;
    config.provisioning.retryIntervalMs = 30000; // 30 seconds

    // System defaults
    config.system.otaEnabled = true;
    config.system.otaPassword[0] = '\0'; // Must be set by user!
    config.system.heartbeatInterval = 30000; // 30 seconds
    config.system.debugEnabled = true;
    config.system.logLevel = 2; // INFO

    // Web server defaults
    config.web.enabled = true;
    config.web.port = 80;
    strncpy(config.web.username, "admin", sizeof(config.web.username));
    config.web.password[0] = '\0'; // Must be set by user!
    config.web.authRequired = true;

    config.version = CONFIG_VERSION;
    config.isValid = validateConfig();
}

/**
 * @brief Load configuration from file
 */
bool UnifiedConfigManager::load() {
    if (!LittleFS.exists(CONFIG_FILE)) {
        Serial.println(F("[Config] Config file not found"));
        return false;
    }

    File file = LittleFS.open(CONFIG_FILE, "r");
    if (!file) {
        Serial.println(F("[Config] Failed to open config file"));
        return false;
    }

    // Parse JSON (use static allocation)
    StaticJsonDocument<2048> doc;
    DeserializationError error = deserializeJson(doc, file);
    file.close();

    if (error) {
        Serial.printf("[Config] JSON parse error: %s\n", error.c_str());
        return false;
    }

    // Check version
    uint8_t fileVersion = doc["version"] | 0;
    if (fileVersion != CONFIG_VERSION) {
        Serial.printf("[Config] Config version mismatch: %d vs %d\n", fileVersion, CONFIG_VERSION);
        // TODO: Implement migration logic here
        return false;
    }

    // Load device identity
    strncpy(config.stationId, doc["device"]["stationId"] | "station001", sizeof(config.stationId));
    strncpy(config.deviceId, doc["device"]["deviceId"] | "device001", sizeof(config.deviceId));
    strncpy(config.serialNumber, doc["device"]["serialNumber"] | "", sizeof(config.serialNumber));

    // Load WiFi config
    strncpy(config.wifi.ssid, doc["wifi"]["ssid"] | "", sizeof(config.wifi.ssid));
    strncpy(config.wifi.password, doc["wifi"]["password"] | "", sizeof(config.wifi.password));
    config.wifi.autoConnect = doc["wifi"]["autoConnect"] | true;
    strncpy(config.wifi.apNamePrefix, doc["wifi"]["apNamePrefix"] | "SolEVC-Provision", sizeof(config.wifi.apNamePrefix));
    config.wifi.configPortalTimeout = doc["wifi"]["configPortalTimeout"] | 300;

    // Load MQTT config
    strncpy(config.mqtt.broker, doc["mqtt"]["broker"] | "localhost", sizeof(config.mqtt.broker));
    config.mqtt.port = doc["mqtt"]["port"] | 1883;
    strncpy(config.mqtt.username, doc["mqtt"]["username"] | "", sizeof(config.mqtt.username));
    strncpy(config.mqtt.password, doc["mqtt"]["password"] | "", sizeof(config.mqtt.password));
    strncpy(config.mqtt.clientIdPrefix, doc["mqtt"]["clientIdPrefix"] | "evse-", sizeof(config.mqtt.clientIdPrefix));
    config.mqtt.tlsEnabled = doc["mqtt"]["tlsEnabled"] | false;
    config.mqtt.keepAlive = doc["mqtt"]["keepAlive"] | 60;

    // Load provisioning config
    strncpy(config.provisioning.serverUrl, doc["provisioning"]["serverUrl"] | "", sizeof(config.provisioning.serverUrl));
    config.provisioning.serverPort = doc["provisioning"]["serverPort"] | 443;
    config.provisioning.timeoutMs = doc["provisioning"]["timeoutMs"] | 300000;
    config.provisioning.maxRetries = doc["provisioning"]["maxRetries"] | 5;
    config.provisioning.retryIntervalMs = doc["provisioning"]["retryIntervalMs"] | 30000;

    // Load system config
    config.system.otaEnabled = doc["system"]["otaEnabled"] | true;
    strncpy(config.system.otaPassword, doc["system"]["otaPassword"] | "", sizeof(config.system.otaPassword));
    config.system.heartbeatInterval = doc["system"]["heartbeatInterval"] | 30000;
    config.system.debugEnabled = doc["system"]["debugEnabled"] | true;
    config.system.logLevel = doc["system"]["logLevel"] | 2;

    // Load web config
    config.web.enabled = doc["web"]["enabled"] | true;
    config.web.port = doc["web"]["port"] | 80;
    strncpy(config.web.username, doc["web"]["username"] | "admin", sizeof(config.web.username));
    strncpy(config.web.password, doc["web"]["password"] | "", sizeof(config.web.password));
    config.web.authRequired = doc["web"]["authRequired"] | true;

    config.version = CONFIG_VERSION;
    sanitizeConfig();
    config.isValid = validateConfig();

    Serial.println(F("[Config] Configuration loaded successfully"));
    return config.isValid;
}

/**
 * @brief Save configuration to file
 */
bool UnifiedConfigManager::save() {
    if (!validateConfig()) {
        Serial.println(F("[Config] Cannot save invalid config"));
        return false;
    }

    // Create JSON document (static allocation)
    StaticJsonDocument<2048> doc;

    doc["version"] = CONFIG_VERSION;

    // Device identity
    doc["device"]["stationId"] = config.stationId;
    doc["device"]["deviceId"] = config.deviceId;
    doc["device"]["serialNumber"] = config.serialNumber;

    // WiFi config
    doc["wifi"]["ssid"] = config.wifi.ssid;
    doc["wifi"]["password"] = config.wifi.password;
    doc["wifi"]["autoConnect"] = config.wifi.autoConnect;
    doc["wifi"]["apNamePrefix"] = config.wifi.apNamePrefix;
    doc["wifi"]["configPortalTimeout"] = config.wifi.configPortalTimeout;

    // MQTT config
    doc["mqtt"]["broker"] = config.mqtt.broker;
    doc["mqtt"]["port"] = config.mqtt.port;
    doc["mqtt"]["username"] = config.mqtt.username;
    doc["mqtt"]["password"] = config.mqtt.password;
    doc["mqtt"]["clientIdPrefix"] = config.mqtt.clientIdPrefix;
    doc["mqtt"]["tlsEnabled"] = config.mqtt.tlsEnabled;
    doc["mqtt"]["keepAlive"] = config.mqtt.keepAlive;

    // Provisioning config
    doc["provisioning"]["serverUrl"] = config.provisioning.serverUrl;
    doc["provisioning"]["serverPort"] = config.provisioning.serverPort;
    doc["provisioning"]["timeoutMs"] = config.provisioning.timeoutMs;
    doc["provisioning"]["maxRetries"] = config.provisioning.maxRetries;
    doc["provisioning"]["retryIntervalMs"] = config.provisioning.retryIntervalMs;

    // System config
    doc["system"]["otaEnabled"] = config.system.otaEnabled;
    doc["system"]["otaPassword"] = config.system.otaPassword;
    doc["system"]["heartbeatInterval"] = config.system.heartbeatInterval;
    doc["system"]["debugEnabled"] = config.system.debugEnabled;
    doc["system"]["logLevel"] = config.system.logLevel;

    // Web config
    doc["web"]["enabled"] = config.web.enabled;
    doc["web"]["port"] = config.web.port;
    doc["web"]["username"] = config.web.username;
    doc["web"]["password"] = config.web.password;
    doc["web"]["authRequired"] = config.web.authRequired;

    // Backup existing config
    if (LittleFS.exists(CONFIG_FILE)) {
        if (LittleFS.exists(BACKUP_FILE)) {
            LittleFS.remove(BACKUP_FILE);
        }
        LittleFS.rename(CONFIG_FILE, BACKUP_FILE);
    }

    // Save to file
    File file = LittleFS.open(CONFIG_FILE, "w");
    if (!file) {
        Serial.println(F("[Config] Failed to open config file for writing"));
        return false;
    }

    size_t bytesWritten = serializeJson(doc, file);
    file.close();

    if (bytesWritten == 0) {
        Serial.println(F("[Config] Failed to write config"));
        // Restore backup
        if (LittleFS.exists(BACKUP_FILE)) {
            LittleFS.rename(BACKUP_FILE, CONFIG_FILE);
        }
        return false;
    }

    Serial.printf("[Config] Configuration saved (%u bytes)\n", bytesWritten);
    return true;
}

/**
 * @brief Reset to factory defaults
 */
bool UnifiedConfigManager::resetToDefaults() {
    Serial.println(F("[Config] Resetting to factory defaults..."));

    // Delete config files
    if (LittleFS.exists(CONFIG_FILE)) {
        LittleFS.remove(CONFIG_FILE);
    }
    if (LittleFS.exists(BACKUP_FILE)) {
        LittleFS.remove(BACKUP_FILE);
    }

    // Load defaults
    loadFactoryDefaults();

    // Save defaults
    return save();
}

/**
 * @brief Validate configuration
 */
bool UnifiedConfigManager::validateConfig() const {
    // Check required fields
    if (strlen(config.stationId) == 0) {
        Serial.println(F("[Config] Validation failed: stationId required"));
        return false;
    }

    if (strlen(config.deviceId) == 0) {
        Serial.println(F("[Config] Validation failed: deviceId required"));
        return false;
    }

    if (strlen(config.mqtt.broker) == 0) {
        Serial.println(F("[Config] Validation failed: MQTT broker required"));
        return false;
    }

    if (config.mqtt.port == 0 || config.mqtt.port > 65535) {
        Serial.println(F("[Config] Validation failed: Invalid MQTT port"));
        return false;
    }

    if (config.system.heartbeatInterval < 1000 || config.system.heartbeatInterval > 300000) {
        Serial.println(F("[Config] Validation failed: Invalid heartbeat interval"));
        return false;
    }

    return true;
}

/**
 * @brief Sanitize config values
 */
void UnifiedConfigManager::sanitizeConfig() {
    // Ensure null-termination
    config.stationId[sizeof(config.stationId) - 1] = '\0';
    config.deviceId[sizeof(config.deviceId) - 1] = '\0';
    config.serialNumber[sizeof(config.serialNumber) - 1] = '\0';

    // Clamp values
    if (config.mqtt.port == 0) config.mqtt.port = 1883;
    if (config.system.heartbeatInterval < 1000) config.system.heartbeatInterval = 30000;
    if (config.system.logLevel > 3) config.system.logLevel = 2;
}

/**
 * @brief Print configuration
 */
void UnifiedConfigManager::printConfig() const {
    Serial.println(F("\n=== Device Configuration ==="));
    Serial.printf("Station ID: %s\n", config.stationId);
    Serial.printf("Device ID: %s\n", config.deviceId);
    Serial.printf("Serial: %s\n", config.serialNumber);

    Serial.println(F("\n--- WiFi ---"));
    Serial.printf("SSID: %s\n", strlen(config.wifi.ssid) > 0 ? config.wifi.ssid : "(not configured)");
    Serial.printf("Auto-connect: %s\n", config.wifi.autoConnect ? "Yes" : "No");
    Serial.printf("AP Prefix: %s\n", config.wifi.apNamePrefix);

    Serial.println(F("\n--- MQTT ---"));
    Serial.printf("Broker: %s:%d\n", config.mqtt.broker, config.mqtt.port);
    Serial.printf("Username: %s\n", strlen(config.mqtt.username) > 0 ? config.mqtt.username : "(none)");
    Serial.printf("TLS: %s\n", config.mqtt.tlsEnabled ? "Enabled" : "Disabled");

    Serial.println(F("\n--- System ---"));
    Serial.printf("OTA: %s\n", config.system.otaEnabled ? "Enabled" : "Disabled");
    Serial.printf("Heartbeat: %u ms\n", config.system.heartbeatInterval);
    Serial.printf("Debug: %s\n", config.system.debugEnabled ? "Yes" : "No");

    Serial.printf("\nConfig valid: %s\n", config.isValid ? "Yes" : "No");
    Serial.println(F("============================\n"));
}

/**
 * @brief Update config from JSON
 */
bool UnifiedConfigManager::updateFromJson(const char* jsonStr) {
    if (!jsonStr) return false;

    StaticJsonDocument<1024> doc;
    DeserializationError error = deserializeJson(doc, jsonStr);

    if (error) {
        Serial.printf("[Config] JSON parse error: %s\n", error.c_str());
        return false;
    }

    // Update only provided fields (partial update)
    bool changed = false;

    if (doc.containsKey("stationId")) {
        strncpy(config.stationId, doc["stationId"], sizeof(config.stationId));
        changed = true;
    }

    if (doc.containsKey("deviceId")) {
        strncpy(config.deviceId, doc["deviceId"], sizeof(config.deviceId));
        changed = true;
    }

    // Add more fields as needed...

    if (changed) {
        sanitizeConfig();
        config.isValid = validateConfig();
        return save();
    }

    return false;
}

/**
 * @brief Export config to JSON
 */
bool UnifiedConfigManager::exportToJson(char* buffer, size_t size, bool includeSecrets) {
    if (!buffer || size == 0) return false;

    StaticJsonDocument<1024> doc;

    doc["stationId"] = config.stationId;
    doc["deviceId"] = config.deviceId;
    doc["mqtt"]["broker"] = config.mqtt.broker;
    doc["mqtt"]["port"] = config.mqtt.port;

    if (includeSecrets) {
        doc["wifi"]["password"] = config.wifi.password;
        doc["mqtt"]["password"] = config.mqtt.password;
    } else {
        doc["wifi"]["password"] = "***";
        doc["mqtt"]["password"] = "***";
    }

    return serializeJson(doc, buffer, size) > 0;
}

/* ============================================================================
 * Config Helper Functions
 * ============================================================================ */

namespace ConfigHelper {

void buildMqttClientId(char* buffer, size_t size, const DeviceConfig& config) {
    snprintf(buffer, size, "%s%s-%s",
             config.mqtt.clientIdPrefix,
             config.stationId,
             config.deviceId);
}

void buildApName(char* buffer, size_t size, const DeviceConfig& config) {
    String mac = WiFi.macAddress();
    mac.replace(":", "");
    String suffix = mac.substring(mac.length() - 6);

    snprintf(buffer, size, "%s%s", config.wifi.apNamePrefix, suffix.c_str());
}

void generateSerialFromMac(char* buffer, size_t size) {
    String mac = WiFi.macAddress();
    mac.replace(":", "");
    snprintf(buffer, size, "SolEVC-Provision");
}

} // namespace ConfigHelper
