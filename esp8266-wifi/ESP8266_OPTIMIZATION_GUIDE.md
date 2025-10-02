# ESP8266 OOP Optimization Guide

**Target:** ESP8266 ESP-07S (80MHz, ~50KB RAM, 4MB Flash)

---

## 🎯 Core Principles

1. **OOP for maintainability** - Yes!
2. **But avoid heavy C++ features** - Smart choices
3. **Prefer stack allocation** over heap
4. **Minimize virtual functions** - Only where needed
5. **Avoid STL containers** - Use lightweight alternatives

---

## ✅ RECOMMENDED OOP Patterns for ESP8266

### 1. **Classes with Stack Allocation (BEST)**

```cpp
// ✅ GOOD - Stack allocated, no overhead
class MQTTClient {
private:
    char broker[64];
    uint16_t port;
    PubSubClient client; // Composed, not pointer!

public:
    MQTTClient(const char* b, uint16_t p) : port(p) {
        strncpy(broker, b, sizeof(broker));
    }
    // No destructor needed if no dynamic allocation
};

// Usage in main.cpp:
MQTTClient mqttClient("broker.com", 1883); // Stack!
```

**Benefits:**

- ✅ No malloc/free
- ✅ No fragmentation
- ✅ Automatic cleanup
- ✅ Fast allocation

---

### 2. **Singletons (When Needed)**

```cpp
// ✅ GOOD - Static instance, no dynamic allocation
class Logger {
private:
    static Logger instance; // Static!

    Logger() {} // Private constructor

public:
    static Logger& getInstance() {
        return instance;
    }

    void log(const char* msg) {
        Serial.println(msg);
    }
};

// Define in .cpp
Logger Logger::instance;
```

**Benefits:**

- ✅ No heap allocation
- ✅ Global access without global variable
- ✅ Lazy initialization not needed

---

### 3. **Fixed-Size Arrays instead of std::vector**

```cpp
// ❌ BAD - Dynamic allocation, fragmentation
std::vector<IBootstrapStage*> stages;

// ✅ GOOD - Fixed size, stack allocation
template<typename T, size_t N>
class FixedArray {
private:
    T items[N];
    size_t count;

public:
    FixedArray() : count(0) {}

    bool add(const T& item) {
        if (count >= N) return false;
        items[count++] = item;
        return true;
    }

    T& operator[](size_t i) { return items[i]; }
    size_t size() const { return count; }
};

// Usage:
FixedArray<IBootstrapStage*, 5> stages;
stages.add(&hardwareInitStage);
```

**Memory saved:** ~20-40 bytes per container + no fragmentation

---

### 4. **Simple Ring Buffer (No Templates if Size Known)**

```cpp
// ✅ GOOD - Fixed size, optimized for ESP8266
class UARTRingBuffer {
private:
    static constexpr size_t CAPACITY = 512;
    uint8_t buffer[CAPACITY];
    size_t head;
    size_t tail;

public:
    UARTRingBuffer() : head(0), tail(0) {}

    bool push(uint8_t byte) {
        size_t next = (head + 1) % CAPACITY;
        if (next == tail) return false; // Full
        buffer[head] = byte;
        head = next;
        return true;
    }

    bool pop(uint8_t& byte) {
        if (head == tail) return false; // Empty
        byte = buffer[tail];
        tail = (tail + 1) % CAPACITY;
        return true;
    }

    size_t available() const {
        return (head >= tail) ? (head - tail) : (CAPACITY - tail + head);
    }
};
```

**Memory:** Exactly 512 bytes + ~8 bytes metadata

---

### 5. **Avoid String, Use char[] with Helper Functions**

```cpp
// ❌ BAD - String creates temporaries
String buildTopic(String station, String device) {
    return "ocpp/" + station + "/" + device + "/heartbeat";
}

// ✅ GOOD - Static buffer, no allocation
class TopicBuilder {
public:
    static void buildHeartbeat(char* out, size_t size,
                               const char* station, const char* device) {
        snprintf(out, size, "ocpp/%s/%s/heartbeat", station, device);
    }

    static void buildCommand(char* out, size_t size,
                            const char* station, const char* device) {
        snprintf(out, size, "ocpp/%s/%s/cmd/+", station, device);
    }
};

// Usage:
char topic[128];
TopicBuilder::buildHeartbeat(topic, sizeof(topic), "station001", "device001");
mqttClient.publish(topic, payload);
```

**Memory saved:** ~50-100 bytes per String operation

---

### 6. **Interfaces with Minimal Virtual Functions**

```cpp
// ✅ GOOD - Only 1-2 virtual functions per interface
class IConnectable {
public:
    virtual bool connect() = 0;
    virtual bool isConnected() const = 0;
    // Stop here! Don't add 10 methods
};

// Implementation
class WiFiConnector : public IConnectable {
    // Only implements what's needed
};
```

**Overhead:** 4 bytes per object (vtable pointer) - acceptable!

---

### 7. **Composition Over Inheritance (Favor Has-A)**

```cpp
// ❌ AVOID - Deep inheritance
class BaseManager {};
class NetworkManager : public BaseManager {};
class WiFiNetworkManager : public NetworkManager {};

// ✅ BETTER - Composition
class WiFiManager {
    // Self-contained
};

class NetworkManager {
private:
    WiFiManager wifi;  // Has-a relationship
    MQTTClient mqtt;

public:
    void handle() {
        wifi.handle();
        mqtt.handle();
    }
};
```

---

## ❌ AVOID These on ESP8266

### 1. STL Containers

```cpp
// ❌ NO!
#include <vector>
#include <map>
#include <list>
#include <set>
std::string // Use char[] instead
```

### 2. Heavy Exception Handling

```cpp
// ❌ Avoid - Adds ~2KB code size
try {
    throw std::runtime_error("Error");
} catch (...) {}

// ✅ Use error codes instead
enum class ErrorCode { SUCCESS, TIMEOUT, INVALID };
```

### 3. Dynamic Polymorphism Overuse

```cpp
// ❌ AVOID - Too many vtables
class IState { virtual void handle() = 0; };
class State1 : public IState {};
class State2 : public IState {};
// ... 10 more states

// ✅ BETTER - Simple state enum
enum class State { STATE1, STATE2, STATE3 };
void handleState(State s) {
    switch(s) { /* ... */ }
}
```

### 4. Multiple Inheritance

```cpp
// ❌ AVOID
class Device : public IConnectable, public IConfigurable, public ILoggable {};

// ✅ BETTER - Single interface, composition for rest
class Device : public IConnectable {
private:
    ConfigManager config;
    Logger logger;
};
```

---

## 🎯 OPTIMIZED Refactoring Strategy

### Phase 0: Config System (Lightweight)

```cpp
// Instead of fancy hierarchy, use simple struct + functions
struct DeviceConfig {
    char stationId[32];
    char deviceId[32];
    char mqttBroker[64];
    uint16_t mqttPort;
    // ... all config in one struct
};

class ConfigManager {
private:
    DeviceConfig factoryDefaults;
    DeviceConfig runtimeConfig;

public:
    ConfigManager() {
        loadFactoryDefaults();
    }

    bool load() {
        // Load from LittleFS
        File f = LittleFS.open("/config.json", "r");
        // Parse into runtimeConfig
    }

    const DeviceConfig& get() const { return runtimeConfig; }
    bool save();
};
```

**Memory:** ~300 bytes for config struct (predictable!)

---

### Phase 1: Class-based Modules (Stack Allocated)

```cpp
class MQTTClient {
private:
    WiFiClient wifiClient;
    PubSubClient client;
    char broker[64];
    uint16_t port;

    struct Message {
        char topic[64];
        char payload[256];
        uint8_t qos;
    };
    FixedArray<Message, 10> messageQueue; // Max 10 queued

public:
    MQTTClient(const char* b, uint16_t p);
    bool connect();
    bool publish(const char* topic, const char* payload);
    void handle();
};

// In main.cpp - STACK allocation
MQTTClient mqttClient("broker", 1883);
WiFiManager wifiManager;
STM32Comm stm32Comm;

void loop() {
    mqttClient.handle();
    wifiManager.handle();
    stm32Comm.handle();
}
```

**Total RAM:** ~2-3KB for all objects (vs 5-10KB with STL)

---

### Phase 2: Simple State Machine (No Virtual Functions)

```cpp
enum class ProvisioningState {
    UNPROVISIONED,
    PROVISIONING,
    PROVISIONED,
    OPERATIONAL,
    ERROR
};

class ProvisioningManager {
private:
    ProvisioningState state;
    uint8_t retryCount;
    uint32_t lastAttempt;

public:
    void handle() {
        switch(state) {
            case ProvisioningState::UNPROVISIONED:
                handleUnprovisioned();
                break;
            case ProvisioningState::PROVISIONING:
                handleProvisioning();
                break;
            // ...
        }
    }

private:
    void handleUnprovisioned();
    void handleProvisioning();
    void transitionTo(ProvisioningState newState);
};
```

**Memory:** ~20 bytes (vs ~100 bytes with State Pattern objects)

---

## 📊 Memory Budget Guidelines

### Total RAM Available: ~50KB

| Component        | Budget   | Optimized | Notes              |
| ---------------- | -------- | --------- | ------------------ |
| WiFi Stack       | ~25KB    | Fixed     | Can't optimize     |
| MQTT Buffer      | 2KB      | 1KB       | Reduce buffer_size |
| UART Buffer      | 1KB      | 512B      | Ring buffer        |
| Config Structs   | 1KB      | 500B      | Fixed size         |
| Message Queue    | 2KB      | 1KB       | Limit queue size   |
| Object Instances | 3KB      | 2KB       | Stack allocation   |
| Other/Temp       | 5KB      | 3KB       | Buffers, etc       |
| **Reserve**      | **11KB** | **17KB**  | Safety margin      |

**Goal:** Keep at least 10KB free for safety

---

## 🔍 Memory Monitoring

```cpp
class MemoryMonitor {
public:
    static void printStats() {
        uint32_t free = ESP.getFreeHeap();
        uint32_t frag = ESP.getHeapFragmentation();

        Serial.printf("Free Heap: %u bytes\n", free);
        Serial.printf("Fragmentation: %u%%\n", frag);

        if (free < 10000) {
            Serial.println("⚠️ LOW MEMORY WARNING!");
        }
        if (frag > 50) {
            Serial.println("⚠️ HIGH FRAGMENTATION!");
        }
    }
};

// Call every minute
void loop() {
    static uint32_t lastCheck = 0;
    if (millis() - lastCheck > 60000) {
        MemoryMonitor::printStats();
        lastCheck = millis();
    }
}
```

---

## ✅ REVISED Refactoring Checklist

### Per-Module Checklist (ESP8266 Optimized)

- [ ] ✅ **Stack allocated** - No `new`, prefer stack
- [ ] ✅ **Fixed-size buffers** - No dynamic arrays
- [ ] ✅ **char[] instead of String** - Avoid String class
- [ ] ✅ **Minimal virtual functions** - Max 2-3 per class
- [ ] ✅ **No STL containers** - Use FixedArray
- [ ] ✅ **Single inheritance only** - Avoid multiple
- [ ] ✅ **Composition preferred** - Has-a over is-a
- [ ] ✅ **Memory monitored** - Free heap tracked
- [ ] ✅ **Fragmentation < 50%** - After 24h test
- [ ] ✅ **Free heap > 10KB** - Safety margin

---

## 🎯 Build Flags for Optimization

```ini
[env:esp07s]
build_flags =
    -Os                              ; Optimize for size
    -DPIO_FRAMEWORK_ARDUINO_LWIP2_LOW_MEMORY  ; Low memory WiFi
    -DVTABLES_IN_FLASH              ; Put vtables in flash
    -fno-exceptions                  ; Disable exceptions
    -DMQTT_MAX_PACKET_SIZE=512      ; Reduce MQTT buffer
    -DSERIAL_RX_BUFFER_SIZE=128     ; Smaller RX buffer
    -DDEBUG_ESP_CORE=0              ; Disable debug in production
```

---

## 📚 Testing Memory Usage

### 1. Compile-time Size Check

```bash
pio run -t size
# Check .data + .bss sections
```

### 2. Runtime Heap Check

```cpp
void setup() {
    Serial.printf("Initial free heap: %u\n", ESP.getFreeHeap());
}

void loop() {
    uint32_t before = ESP.getFreeHeap();

    // Do work
    mqttClient.handle();

    uint32_t after = ESP.getFreeHeap();
    if (before != after) {
        Serial.printf("Heap changed: %d bytes\n", (int)(after - before));
    }
}
```

### 3. 24h Stability Test

```cpp
void loop() {
    static uint32_t startHeap = ESP.getFreeHeap();
    static uint32_t minHeap = startHeap;

    uint32_t current = ESP.getFreeHeap();
    if (current < minHeap) {
        minHeap = current;
        Serial.printf("New min heap: %u (lost %u)\n",
                     minHeap, startHeap - minHeap);
    }
}
```

---

## 🎯 FINAL RECOMMENDATION

### DO THIS:

1. ✅ Use classes for organization (encapsulation)
2. ✅ Stack allocate everything possible
3. ✅ Use simple enums for state machines
4. ✅ Use char[] with helper functions (no String)
5. ✅ Fixed-size arrays (no std::vector)
6. ✅ Minimal virtual functions (only where really needed)
7. ✅ Monitor memory continuously

### DON'T:

1. ❌ STL containers
2. ❌ Multiple inheritance
3. ❌ String class for frequent operations
4. ❌ Deep inheritance hierarchies
5. ❌ Exceptions
6. ❌ Heavy design patterns (State pattern with many objects)

---

**Result:** Clean OOP code that runs efficiently on ESP8266!
