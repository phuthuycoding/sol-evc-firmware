# ESP8266 Handlers Guide

**Version**: 2.0.0
**Architecture**: Stateless Handlers Pattern

---

## 📋 Handler Overview

All handlers are **stateless** (static methods only) để tiết kiệm memory. Business logic được tách riêng khỏi drivers và core orchestrator.

```
handlers/
├── heartbeat_handler.cpp          # MQTT heartbeat
├── stm32_command_handler.cpp      # STM32 UART commands
├── mqtt_incoming_handler.cpp      # MQTT incoming messages
├── ocpp_message_handler.cpp       # OCPP messages (status, meter, tx)
└── ota_handler.cpp                # OTA updates (placeholder)
```

---

## 🔄 Communication Flow

### 1. **STM32 → ESP8266 → Cloud (Outgoing)**

```
STM32 (UART) → ESP8266 → MQTT Broker → Cloud

Handlers used:
- stm32_command_handler: Process UART packets
- ocpp_message_handler: Publish OCPP messages
- heartbeat_handler: Send periodic heartbeats
```

**Example**:
```cpp
// STM32 sends meter values via UART
uart_packet_t packet;
packet.cmd_type = CMD_MQTT_PUBLISH;
// ... payload with meter data

// ESP8266 receives and processes
STM32CommandHandler::execute(packet, stm32, mqtt);

// Which calls:
OCPPMessageHandler::publishMeterValues(mqtt, config, meterData);
```

### 2. **Cloud → ESP8266 → STM32 (Incoming)**

```
Cloud → MQTT Broker → ESP8266 (MQTT) → STM32 (UART)

Handlers used:
- mqtt_incoming_handler: Process MQTT messages
- Forward to STM32 via UART
```

**Example**:
```cpp
// MQTT message received
void mqttCallback(const char* topic, const char* payload, uint16_t length) {
    // Use handler
    MQTTIncomingHandler::execute(topic, payload, length, stm32, config);
}

// Handler forwards to STM32 via UART
// STM32 processes command (remote_start, remote_stop, etc.)
```

---

## 📝 Handler Details

### 1. HeartbeatHandler

**Purpose**: Send periodic heartbeat to MQTT broker

**Usage**:
```cpp
HeartbeatHandler::execute(
    mqtt,        // MQTTClient reference
    wifi,        // WiFiManager reference
    config,      // DeviceConfig reference
    bootTime     // uint32_t boot timestamp
);
```

**Published Data**:
- `msgId`: Timestamp
- `uptime`: Uptime in seconds
- `rssi`: WiFi signal strength
- `freeHeap`: Available heap memory
- `heapFrag`: Heap fragmentation percentage

**Topic**: `ocpp/{stationId}/{deviceId}/heartbeat`

---

### 2. STM32CommandHandler

**Purpose**: Handle commands from STM32 via UART

**Usage**:
```cpp
STM32CommandHandler::execute(
    packet,      // uart_packet_t from STM32
    stm32,       // STM32Communicator reference
    mqtt         // MQTTClient reference
);
```

**Supported Commands**:
- `CMD_MQTT_PUBLISH (0x01)`: Publish MQTT message
- `CMD_GET_TIME (0x02)`: Send time to STM32
- `CMD_WIFI_STATUS (0x03)`: Send WiFi status
- `CMD_CONFIG_UPDATE (0x04)`: Update config (TODO)
- `CMD_OTA_REQUEST (0x05)`: OTA request (TODO)

**Responses**:
- `RSP_MQTT_ACK`: ACK for MQTT publish
- `RSP_TIME_DATA`: Time data (unix timestamp, timezone, NTP sync)
- `RSP_WIFI_STATUS`: WiFi + MQTT status, RSSI, IP, uptime

---

### 3. MQTTIncomingHandler

**Purpose**: Handle incoming MQTT messages from cloud and forward to STM32

**Usage**:
```cpp
MQTTIncomingHandler::execute(
    topic,       // const char* MQTT topic
    payload,     // const char* message payload
    length,      // uint16_t payload length
    stm32,       // STM32Communicator reference
    config       // DeviceConfig reference
);
```

**Flow**:
1. Check if topic matches this device (`ocpp/{stationId}/{deviceId}/cmd/...`)
2. Forward to STM32 via UART packet (`RSP_MQTT_RECEIVED`)
3. STM32 processes command

**Example Topics**:
- `ocpp/station001/device001/cmd/remote_start`
- `ocpp/station001/device001/cmd/remote_stop`
- `ocpp/station001/device001/cmd/reset`

---

### 4. OCPPMessageHandler

**Purpose**: Publish OCPP messages to MQTT

**Methods**:

#### publishStatusNotification
```cpp
OCPPMessageHandler::publishStatusNotification(mqtt, config, status);
```
**Topic**: `ocpp/{stationId}/{deviceId}/status/{connectorId}/status_notification`

**Payload**:
- `msgId`, `timestamp`, `connectorId`
- `status`: AVAILABLE, CHARGING, FAULTED, etc.
- `errorCode`: NO_ERROR, OVER_CURRENT, etc.
- `info`, `vendorId`

#### publishMeterValues
```cpp
OCPPMessageHandler::publishMeterValues(mqtt, config, meter);
```
**Topic**: `ocpp/{stationId}/{deviceId}/meter/{connectorId}/meter_values`

**Payload**:
- `msgId`, `timestamp`, `connectorId`, `transactionId`
- `sample`: energy, power, voltage, current, frequency, temperature, power_factor

#### publishStartTransaction
```cpp
OCPPMessageHandler::publishStartTransaction(mqtt, config, txStart);
```
**Topic**: `ocpp/{stationId}/{deviceId}/transaction/start`

**Payload**:
- `msgId`, `timestamp`, `connectorId`
- `idTag`, `meterStart`, `reservationId`

#### publishStopTransaction
```cpp
OCPPMessageHandler::publishStopTransaction(mqtt, config, txStop);
```
**Topic**: `ocpp/{stationId}/{deviceId}/transaction/stop`

**Payload**:
- `msgId`, `timestamp`, `transactionId`
- `idTag`, `meterStop`, `reason`

#### publishBootNotification
```cpp
OCPPMessageHandler::publishBootNotification(mqtt, config, boot);
```
**Topic**: `ocpp/{stationId}/{deviceId}/event/0/boot_notification`

**Payload**:
- `msgId`, `timestamp`
- `chargePointModel`, `chargePointVendor`
- `firmwareVersion`, `chargePointSerialNumber`

---

### 5. OTAHandler (Placeholder)

**Purpose**: Handle OTA firmware updates

**Usage**:
```cpp
// Check for update
bool available = OTAHandler::checkUpdate(url);

// Perform update
bool success = OTAHandler::performUpdate(url);

// Get version
const char* version = OTAHandler::getCurrentVersion();
```

**Note**: Hiện tại chỉ là placeholder. Cần implement:
- HTTP client để fetch firmware
- Flash writing logic
- Checksum verification
- Reboot after update

---

## 🎯 Adding New Handler

### Step 1: Create Handler Files

```cpp
// include/handlers/my_handler.h
#ifndef MY_HANDLER_H
#define MY_HANDLER_H

class MyHandler {
public:
    static bool execute(...);  // Stateless, static method
};

#endif
```

```cpp
// src/handlers/my_handler.cpp
#include "handlers/my_handler.h"
#include "utils/logger.h"

bool MyHandler::execute(...) {
    // Business logic here
    LOG_INFO("MyHandler", "Executing...");
    return true;
}
```

### Step 2: Use in DeviceManager

```cpp
// In device_manager.cpp
#include "handlers/my_handler.h"

void DeviceManager::someFunction() {
    MyHandler::execute(mqtt, config, ...);
}
```

### Step 3: Test

- Build and flash
- Monitor serial output
- Verify handler is called
- Check memory usage

---

## 💾 Memory Guidelines

### ✅ DO:
- **Static methods only** (no object state)
- **Stack-allocated buffers** (`char buf[256]`)
- **StaticJsonDocument** (not DynamicJsonDocument)
- **Pass by reference** (avoid copies)

### ❌ DON'T:
- **No member variables** (handlers are stateless)
- **No heap allocation** (new/malloc)
- **No DynamicJsonDocument** (use Static version)
- **No String concatenation** (creates temp objects)

---

## 🔍 Debugging Handlers

### Enable Debug Logging

```cpp
// In setup()
Logger::getInstance().setLevel(LogLevel::DEBUG);
```

### Check Handler Execution

```cpp
// Each handler logs its activity
LOG_DEBUG("HandlerName", "Action: %s", param);
```

### Monitor Memory

```cpp
void printMemory() {
    Serial.printf("Heap: %u, Frag: %u%%\n",
                  ESP.getFreeHeap(),
                  ESP.getHeapFragmentation());
}
```

---

## 📊 Handler Summary

| Handler | Purpose | Memory | Complexity |
|---------|---------|--------|------------|
| HeartbeatHandler | Send heartbeat | ~500B stack | Low |
| STM32CommandHandler | Process STM32 commands | ~800B stack | Medium |
| MQTTIncomingHandler | Forward MQTT to STM32 | ~600B stack | Low |
| OCPPMessageHandler | Publish OCPP messages | ~600B stack | Medium |
| OTAHandler | OTA updates | ~200B stack | Low (placeholder) |

**Total handler overhead**: < 3KB stack (when executing)

---

**Last Updated**: 2025-10-02
**Philosophy**: Stateless > Stateful | Stack > Heap | Simple > Complex
