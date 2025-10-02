# Architecture Documentation

**ESP8266 WiFi Module for SolEVC Charging Point Controller**
**Version:** 3.0.0
**Pattern:** Handlers + Drivers (Memory-Optimized)

---

## Table of Contents

1. [Overview](#overview)
2. [Design Principles](#design-principles)
3. [Layered Architecture](#layered-architecture)
4. [Component Details](#component-details)
5. [Data Flow](#data-flow)
6. [Memory Management](#memory-management)
7. [Error Handling](#error-handling)

---

## Overview

ESP8266 firmware được thiết kế theo kiến trúc phân tầng đơn giản, tối ưu cho embedded system với RAM giới hạn (~50KB heap).

### Key Characteristics

- ✅ **Stateless Handlers**: Không lưu state, chỉ xử lý logic
- ✅ **Thin Drivers**: Wrapper nhẹ cho hardware
- ✅ **Stack Allocation**: Tránh heap fragmentation
- ✅ **Fixed Buffers**: Không dynamic allocation
- ✅ **Single Orchestrator**: DeviceManager quản lý tất cả

---

## Design Principles

### 1. Memory First

**Constraint:** ESP8266 chỉ có ~50KB heap

**Solutions:**
- Stack allocation ưu tiên hơn heap
- Static buffers thay vì dynamic
- Stateless handlers (không state object)
- Fixed-size containers
- No STL containers

### 2. Separation of Concerns

**Layers:**
1. **Handlers** - Business logic (stateless)
2. **Drivers** - Hardware abstraction
3. **Platform** - Arduino/ESP8266 SDK

### 3. Testability

- Mock objects cho drivers
- Unit tests chạy trên PC (native)
- Integration tests trên hardware

### 4. Maintainability

- Clear responsibility per class
- Minimal dependencies
- Well-documented APIs

---

## Layered Architecture

```
┌──────────────────────────────────────────────────────┐
│                   Application Layer                  │
│                     main.cpp                         │
│  • System initialization                             │
│  • Main loop orchestration                           │
│  • Diagnostics & watchdog                            │
└─────────────────────┬────────────────────────────────┘
                      │
                      ▼
┌──────────────────────────────────────────────────────┐
│                 Orchestration Layer                  │
│                  DeviceManager                       │
│  • Initialize all components                         │
│  • Coordinate handlers & drivers                     │
│  • Manage callbacks                                  │
└─────────────────────┬────────────────────────────────┘
                      │
         ┌────────────┴────────────┐
         ▼                         ▼
┌──────────────────┐       ┌──────────────────┐
│  Handler Layer   │       │   Driver Layer   │
│   (Stateless)    │       │ (Thin Wrappers)  │
├──────────────────┤       ├──────────────────┤
│ • Heartbeat      │       │ • WiFi           │
│ • OCPP Messages  │       │ • MQTT           │
│ • STM32 Commands │       │ • STM32 UART     │
│ • MQTT Incoming  │       │ • NTP Time       │
│ • Config Update  │       │ • Config Manager │
│ • OTA            │       │ • Topic Builder  │
└──────────────────┘       └─────────┬────────┘
                                     │
                                     ▼
                      ┌──────────────────────────┐
                      │    Platform Layer        │
                      │ Arduino / ESP8266 SDK    │
                      │  • WiFi.h                │
                      │  • PubSubClient.h        │
                      │  • Serial                │
                      │  • NTPClient.h           │
                      └──────────────────────────┘
```

---

## Component Details

### Application Layer

#### main.cpp

**Responsibility:** Entry point và system lifecycle

**Functions:**
```cpp
void setup() {
    // 1. Initialize serial, logger
    // 2. Print banner & chip info
    // 3. Initialize DeviceManager
    // 4. Print diagnostics
}

void loop() {
    // 1. Feed watchdog
    // 2. Run DeviceManager
    // 3. Update diagnostics
    // 4. Yield & delay
}
```

**Key Features:**
- Watchdog feeding
- System diagnostics (heap, uptime, loop count)
- Memory warnings
- Detailed boot logs

---

### Orchestration Layer

#### DeviceManager

**Location:** `src/core/device_manager.cpp`

**Responsibility:** Quản lý vòng đời của tất cả components

**Components Managed:**
```cpp
class DeviceManager {
private:
    UnifiedConfigManager configManager;      // Config
    WiFiManager* wifiManager;                 // WiFi (heap)
    MQTTClient* mqttClient;                   // MQTT (heap)
    STM32Communicator stm32;                  // UART (stack)
    NTPTimeDriver ntpTime;                    // NTP (stack)
};
```

**Lifecycle:**
```cpp
bool init() {
    1. initializeConfig()      // Load config.json
    2. initializeCommunication() // UART to STM32
    3. initializeNetwork()      // WiFi + MQTT + NTP
}

void run() {
    1. stm32.handle()          // UART packets
    2. wifiManager->handle()   // WiFi reconnect
    3. mqttClient->handle()    // MQTT pub/sub
    4. ntpTime.update()        // Time sync
    5. handleBootNotification() // Once
    6. handleHeartbeat()       // Periodic
}
```

**Callbacks:**
```cpp
static void mqttMessageCallback(topic, payload, length)
    → MQTTIncomingHandler::execute()

static void stm32PacketCallback(packet)
    → STM32CommandHandler::execute()
```

---

### Handler Layer (Business Logic)

**Design Pattern:** Static methods only (stateless)

**Characteristics:**
- No instance variables
- No constructors/destructors
- Pure functions
- Stack-allocated buffers
- Pass dependencies as parameters

#### 1. HeartbeatHandler

**File:** `src/handlers/heartbeat_handler.cpp`

**API:**
```cpp
class HeartbeatHandler {
public:
    static bool execute(
        MQTTClient& mqtt,
        WiFiManager& wifi,
        const DeviceConfig& config,
        uint32_t bootTime
    );
};
```

**Logic:**
1. Check MQTT connected
2. Build JSON payload (stack)
3. Publish to heartbeat topic
4. Return success/failure

#### 2. OCPPMessageHandler

**File:** `src/handlers/ocpp_message_handler.cpp`

**API:**
```cpp
class OCPPMessageHandler {
public:
    static bool publishStatusNotification(...);
    static bool publishMeterValues(...);
    static bool publishStartTransaction(...);
    static bool publishStopTransaction(...);
    static bool publishBootNotification(...);
};
```

**Shared Logic:**
- Build MQTT topic via MQTTTopicBuilder
- Serialize JSON via ArduinoJson
- Publish via MQTTClient
- Return success/failure

#### 3. STM32CommandHandler

**File:** `src/handlers/stm32_command_handler.cpp`

**API:**
```cpp
class STM32CommandHandler {
public:
    static void execute(
        const uart_packet_t& packet,
        STM32Communicator& stm32,
        MQTTClient& mqtt,
        NTPTimeDriver& ntpTime,
        UnifiedConfigManager& configManager
    );

private:
    static void handleMqttPublish(...);
    static void handleGetTime(...);
    static void handleWiFiStatus(...);
    static void handleConfigUpdate(...);
    static void handleOTARequest(...);
    static void handlePublishMeterValues(...);
};
```

**Logic:** Switch-case router for 6 commands

#### 4. MQTTIncomingHandler

**File:** `src/handlers/mqtt_incoming_handler.cpp`

**API:**
```cpp
class MQTTIncomingHandler {
public:
    static void execute(
        const char* topic,
        const char* payload,
        uint16_t length,
        STM32Communicator& stm32,
        const DeviceConfig& config
    );
};
```

**Logic:**
1. Filter by device ID
2. Build UART packet
3. Forward to STM32

#### 5. ConfigUpdateHandler

**File:** `src/handlers/config_update_handler.cpp`

**API:**
```cpp
class ConfigUpdateHandler {
public:
    static bool handleFromSTM32(...);
    static bool handleFromMQTT(...);
};
```

#### 6. OTAHandler

**File:** `src/handlers/ota_handler.cpp`

**API:**
```cpp
class OTAHandler {
public:
    static bool checkUpdate(...);
    static OTAResult performUpdate(const char* url);
    static void handleFromSTM32(...);
    static const char* getCurrentVersion();
};
```

**Logic:**
1. Check free sketch space
2. Download firmware via ESP8266HTTPClient
3. Flash to OTA partition
4. Reboot

---

### Driver Layer (Hardware Abstraction)

**Design Pattern:** Thin wrappers with minimal logic

#### 1. WiFiManager

**File:** `src/drivers/network/wifi_manager.cpp`

**Responsibility:** WiFi connection + Captive Portal

**API:**
```cpp
class WiFiManager {
public:
    WiFiError init();
    WiFiError connect();
    WiFiError startConfigPortal();
    void handle();                    // Auto-reconnect
    bool isConnected() const;
    const WiFiStatus& getStatus() const;
};
```

**Features:**
- Auto-connect on boot
- Captive portal for provisioning
- Auto-reconnect every 30s
- RSSI monitoring

#### 2. MQTTClient

**File:** `src/drivers/mqtt/mqtt_client.cpp`

**Responsibility:** MQTT pub/sub with reconnection

**API:**
```cpp
class MQTTClient {
public:
    MQTTError connect();
    void disconnect();
    MQTTError publish(topic, payload, qos);
    MQTTError subscribe(topic);
    void handle();                    // Process messages
    bool isConnected() const;
    void setCallback(MQTTMessageCallback);
};
```

**Features:**
- PubSub with QoS 0/1
- Message queue (10 messages)
- Auto-reconnect with backoff
- TLS support (insecure mode)

#### 3. STM32Communicator

**File:** `src/drivers/communication/stm32_comm.cpp`

**Responsibility:** UART protocol with STM32

**API:**
```cpp
class STM32Communicator {
public:
    UARTError init();
    void handle();                    // Parse packets
    void sendPacket(const uart_packet_t& packet);
    void sendAck(uint8_t sequence, uint8_t status);
    void setCallback(UARTPacketCallback);
};
```

**Protocol:**
- Baud rate: 115200
- Packet format: Header + Payload + Checksum
- Max payload: 256 bytes
- XOR checksum validation

#### 4. NTPTimeDriver

**File:** `src/drivers/time/ntp_time.cpp`

**Responsibility:** NTP time synchronization

**API:**
```cpp
class NTPTimeDriver {
public:
    void init(const char* server, int16_t tzOffset);
    void update();                    // Call in loop
    bool forceSync();
    uint32_t getUnixTime();
    String getFormattedTime();
    bool isSynced() const;
    int16_t getTimezoneOffset() const;
};
```

**Features:**
- Auto-sync every 1 hour
- Timezone offset support
- Fallback to millis() if not synced

#### 5. UnifiedConfigManager

**File:** `src/drivers/config/unified_config.cpp`

**Responsibility:** JSON configuration management

**API:**
```cpp
class UnifiedConfigManager {
public:
    bool init();
    bool load();
    bool save();
    const DeviceConfig& get() const;
    bool update(const char* json);
};
```

**Storage:** LittleFS `/config.json`

#### 6. MQTTTopicBuilder

**File:** `src/drivers/mqtt/mqtt_topic_builder.cpp`

**Responsibility:** Centralized MQTT topic construction

**API:**
```cpp
namespace MQTTTopicBuilder {
    void buildHeartbeat(buffer, size, config);
    void buildStatus(buffer, size, config, connectorId);
    void buildMeter(buffer, size, config, connectorId);
    void buildTransaction(buffer, size, config, type);
    void buildBoot(buffer, size, config);
    void buildCommand(buffer, size, config);
}
```

**Topic Format:**
```
ocpp/{stationId}/{deviceId}/{category}/{connector}/{message}
```

---

## Data Flow

### Example 1: STM32 Publishes Meter Values

```
┌─────────┐  CMD_PUBLISH_METER_VALUES   ┌──────────┐
│  STM32  │ ──────────────────────────> │  ESP8266 │
└─────────┘                              └────┬─────┘
                                              │
                                              ▼
                                    ┌──────────────────┐
                                    │ UART RX Buffer   │
                                    └────────┬─────────┘
                                             │
                                             ▼
                                  ┌─────────────────────┐
                                  │ STM32Communicator   │
                                  │   .handle()         │
                                  └──────────┬──────────┘
                                             │ Callback
                                             ▼
                                  ┌──────────────────────┐
                                  │ DeviceManager        │
                                  │  ::stm32PacketCallback│
                                  └──────────┬───────────┘
                                             │
                                             ▼
                                  ┌──────────────────────┐
                                  │ STM32CommandHandler  │
                                  │  ::execute()         │
                                  └──────────┬───────────┘
                                             │
                                             ▼
                                  ┌──────────────────────┐
                                  │ handlePublishMeterValues()│
                                  └──────────┬───────────┘
                                             │
                                             ▼
                                  ┌──────────────────────┐
                                  │ OCPPMessageHandler   │
                                  │  ::publishMeterValues()│
                                  └──────────┬───────────┘
                                             │
                                             ▼
                                  ┌──────────────────────┐
                                  │ MQTTTopicBuilder     │
                                  │  ::buildMeter()      │
                                  └──────────┬───────────┘
                                             │
                                             ▼
                                  ┌──────────────────────┐
                                  │ MQTTClient           │
                                  │  .publish()          │
                                  └──────────┬───────────┘
                                             │
                                             ▼
                                  ┌──────────────────────┐
                                  │   MQTT Broker        │
                                  └──────────────────────┘
```

### Example 2: Cloud Sends Remote Command

```
┌──────────┐  MQTT Publish           ┌──────────┐
│  Cloud   │ ──────────────────────> │  Broker  │
└──────────┘                          └────┬─────┘
                                           │
                                           ▼
                                ┌──────────────────┐
                                │  MQTTClient      │
                                │   .handle()      │
                                └────────┬─────────┘
                                         │ Callback
                                         ▼
                              ┌──────────────────────┐
                              │ DeviceManager        │
                              │  ::mqttMessageCallback│
                              └──────────┬───────────┘
                                         │
                                         ▼
                              ┌──────────────────────┐
                              │ MQTTIncomingHandler  │
                              │  ::execute()         │
                              └──────────┬───────────┘
                                         │
                                         ▼
                              ┌──────────────────────┐
                              │ STM32Communicator    │
                              │  .sendPacket()       │
                              └──────────┬───────────┘
                                         │
                                         ▼
                                    ┌─────────┐
                                    │  STM32  │
                                    └─────────┘
```

---

## Memory Management

### Allocation Strategy

```cpp
// ✅ GOOD: Stack allocation
void handleCommand() {
    StaticJsonDocument<256> doc;        // Stack
    char buffer[128];                   // Stack
    // ... use and discard
}

// ✅ GOOD: Static/Global (careful!)
static DeviceManager manager;           // BSS section

// ❌ BAD: Heap allocation
void badExample() {
    DynamicJsonDocument doc(256);       // Heap
    String str = "hello";               // Heap
    char* buf = new char[128];          // Heap
}
```

### Memory Budget

| Component | Stack | Heap | Total |
|-----------|-------|------|-------|
| DeviceManager | 200B | 500B | 700B |
| WiFiManager | - | 1.5KB | 1.5KB |
| MQTTClient | - | 2KB | 2KB |
| STM32Communicator | 300B | - | 300B |
| NTPTimeDriver | 200B | - | 200B |
| **Available** | - | ~46KB | ~46KB |
| **Total** | ~1KB | ~4KB | **~5KB used** |

### Fragmentation Prevention

1. **Avoid dynamic allocation in loops**
2. **Reuse buffers**
3. **Fixed-size containers**
4. **Stack for temporary data**

---

## Error Handling

### Error Codes

```cpp
enum class WiFiError {
    SUCCESS, NOT_CONFIGURED, CONNECTION_FAILED,
    TIMEOUT, ALREADY_CONNECTED
};

enum class MQTTError {
    SUCCESS, NOT_CONNECTED, PUBLISH_FAILED,
    SUBSCRIBE_FAILED, QUEUE_FULL, INVALID_PARAM
};

enum class UARTError {
    SUCCESS, INIT_FAILED, TIMEOUT,
    CHECKSUM_ERROR, BUFFER_OVERFLOW
};
```

### Error Handling Pattern

```cpp
// Check and return early
if (!mqtt.isConnected()) {
    LOG_WARN("Handler", "MQTT not connected");
    return false;
}

// Log and continue
if (result != SUCCESS) {
    LOG_ERROR("Handler", "Operation failed");
    stm32.sendAck(seq, STATUS_ERROR);
    return;
}
```

---

## Summary

### Architecture Benefits

✅ **Memory Efficient:** ~4.5KB / 50KB heap used
✅ **Maintainable:** Clear separation of concerns
✅ **Testable:** Mock-able drivers, stateless handlers
✅ **Extensible:** Easy to add new handlers/drivers
✅ **Robust:** Comprehensive error handling

### Design Trade-offs

**Chosen:**
- Simplicity over flexibility
- Memory over features
- Stack over heap
- Static over dynamic

**Result:** Production-ready firmware với memory footprint nhỏ!

---

**Version:** 3.0.0
**Last Updated:** 2025-10-02
