# ESP8266 Architecture - Simplified & Memory-Optimized

**Target**: ESP8266 với ~50KB heap available
**Philosophy**: Đơn giản, stack-based, minimal abstraction

---

## 🏗️ Kiến Trúc 3 Lớp

```
┌─────────────────────────────────────┐
│   Application (handlers/)           │  Business logic
│   - Stateless functions             │  (optional layer)
│   - Static methods                  │
└──────────────┬──────────────────────┘
               ↓ uses
┌──────────────▼──────────────────────┐
│   Drivers (drivers/)                │  Thin wrappers
│   - WiFiDriver                      │
│   - MQTTDriver                      │
│   - UARTDriver                      │
│   - ConfigDriver                    │
└──────────────┬──────────────────────┘
               ↓ wraps
┌──────────────▼──────────────────────┐
│   Platform (Arduino/ESP8266)        │  Platform libs
│   - ESP8266WiFi                     │
│   - PubSubClient                    │
│   - LittleFS                        │
└─────────────────────────────────────┘
```

---

## 📂 Cấu Trúc Thư Mục

```
esp8266-wifi/
├── src/
│   ├── main.cpp                    # Entry, stack objects only (~80 lines)
│   │
│   ├── core/
│   │   └── device_manager.cpp      # Orchestrator (Facade pattern)
│   │
│   ├── drivers/                    # Platform wrappers (no virtuals)
│   │   ├── wifi_driver.cpp
│   │   ├── mqtt_driver.cpp
│   │   ├── uart_driver.cpp
│   │   └── config_driver.cpp
│   │
│   ├── handlers/                   # Business logic (optional, stateless)
│   │   ├── mqtt_publish.cpp        # Static methods only
│   │   ├── stm32_command.cpp
│   │   └── heartbeat.cpp
│   │
│   └── utils/
│       ├── logger.cpp
│       └── ring_buffer.h
│
└── include/ (mirror structure)
```

---

## 💾 Memory Rules

### ✅ DO:
- **Stack allocation**: `MyClass obj;` không phải `new MyClass()`
- **Fixed arrays**: `uint8_t buf[256];` không phải `std::vector`
- **Static methods**: Cho handlers, không cần state
- **Const strings in PROGMEM**: `const char* text PROGMEM = "...";`
- **Monitor heap**: `ESP.getFreeHeap()` mỗi 60s

### ❌ DON'T:
- **Virtual functions**: Vtables tốn RAM
- **Dynamic allocation**: `new`, `malloc`, `std::vector`, `std::map`
- **String concatenation**: `String a = "hello" + "world";` → tạo temp objects
- **Deep inheritance**: Overhead không cần thiết
- **Clean Architecture patterns**: Interfaces, use-cases → too heavy

---

## 📝 Code Patterns

### Pattern 1: Driver (Thin Wrapper)

```cpp
// drivers/mqtt_driver.h
#ifndef MQTT_DRIVER_H
#define MQTT_DRIVER_H

#include <PubSubClient.h>

class MQTTDriver {
private:
    PubSubClient client;        // Stack
    WiFiClient wifiClient;      // Stack

public:
    MQTTDriver() : client(wifiClient) {}

    void init(const char* broker, uint16_t port) {
        client.setServer(broker, port);
    }

    bool connect(const char* clientId) {
        return client.connect(clientId);
    }

    bool publish(const char* topic, const char* payload) {
        return client.publish(topic, payload);
    }

    void loop() { client.loop(); }
};

#endif
```

### Pattern 2: Handler (Stateless)

```cpp
// handlers/mqtt_publish.h
#ifndef MQTT_PUBLISH_H
#define MQTT_PUBLISH_H

#include "drivers/mqtt_driver.h"

class MQTTPublishHandler {
public:
    // Static method - no object needed
    static bool execute(
        MQTTDriver& mqtt,
        const char* topic,
        const char* payload
    ) {
        if (!mqtt.isConnected()) {
            return mqtt.queueMessage(topic, payload);
        }
        return mqtt.publish(topic, payload);
    }
};

#endif
```

### Pattern 3: Main (DI via Constructor)

```cpp
// main.cpp
#include "drivers/wifi_driver.h"
#include "drivers/mqtt_driver.h"
#include "handlers/mqtt_publish.h"

// Stack-allocated drivers
WiFiDriver wifi;
MQTTDriver mqtt;

void setup() {
    Serial.begin(115200);

    wifi.init("SSID", "password");
    mqtt.init("broker", 1883);

    wifi.connect();
    mqtt.connect("esp8266");
}

void loop() {
    mqtt.loop();

    // Use handler
    static uint32_t last = 0;
    if (millis() - last > 30000) {
        MQTTPublishHandler::execute(mqtt, "topic", "payload");
        last = millis();
    }
}
```

---

## 🔧 Current Architecture (DeviceManager)

**File**: `src/core/device_manager.cpp`

```cpp
class DeviceManager {
private:
    UnifiedConfigManager configManager;  // Stack
    WiFiManager* wifiManager;            // Pointer (managed)
    MQTTClient* mqttClient;              // Pointer (managed)
    STM32Communicator stm32;             // Stack

public:
    bool init() {
        initializeConfig();
        initializeCommunication();
        initializeNetwork();
    }

    void run() {
        stm32.handle();
        if (wifiManager) wifiManager->handle();
        if (mqttClient) mqttClient->handle();

        // Periodic tasks
        handleHeartbeat();
        handleMeterValues();
    }
};
```

**Why pointers?**
- WiFiManager/MQTTClient có thể NULL khi offline
- Lazy initialization để tiết kiệm memory khi không cần

---

## 📊 Memory Budget

| Component | RAM Usage | Notes |
|-----------|-----------|-------|
| WiFi stack | ~10-15KB | ESP8266WiFi lib |
| MQTT buffer | ~1-2KB | PubSubClient |
| UART buffer | ~512B | Ring buffer |
| Config | ~2-3KB | JSON config |
| Logger | ~1KB | Log buffer |
| **Available** | **~30-35KB** | For app logic |

**Target**: Giữ free heap > 20KB để tránh fragmentation

---

## 🎯 Khi Nào Refactor?

### Giữ nguyên (Current) nếu:
- ✅ Code < 2000 lines
- ✅ Free heap > 25KB
- ✅ Không có bug memory leak
- ✅ Team quen với code hiện tại

### Refactor sang 3-layer nếu:
- ⚠️ Code > 3000 lines, khó maintain
- ⚠️ Nhiều duplicate logic
- ⚠️ Cần test coverage cao hơn
- ⚠️ Team cần structure rõ ràng hơn

### KHÔNG BAO GIỜ:
- ❌ Dùng Clean Architecture đầy đủ
- ❌ Thêm interfaces/polymorphism không cần thiết
- ❌ Copy design patterns từ server-side code

---

## 🔍 Debugging Memory

```cpp
void printMemory() {
    uint32_t free = ESP.getFreeHeap();
    uint32_t frag = ESP.getHeapFragmentation();

    Serial.printf("Heap: %u bytes, Frag: %u%%\n", free, frag);

    if (free < 15000) {
        Serial.println("WARNING: Low memory!");
    }
    if (frag > 50) {
        Serial.println("WARNING: High fragmentation!");
    }
}

void loop() {
    // Print every 60s
    static uint32_t last = 0;
    if (millis() - last > 60000) {
        printMemory();
        last = millis();
    }
}
```

---

## 📚 References

- **ESP8266 Memory**: https://arduino-esp8266.readthedocs.io/en/latest/faq/readme.html#stack-and-heap
- **PROGMEM**: https://arduino-esp8266.readthedocs.io/en/latest/PROGMEM.html
- **Embedded OOP**: Keep it simple, avoid over-engineering

---

**Last Updated**: 2025-10-02
**Memory Target**: Keep free heap > 20KB
**Philosophy**: Simple > Complex, Stack > Heap
