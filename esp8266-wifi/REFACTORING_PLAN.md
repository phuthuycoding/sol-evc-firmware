# ESP8266 Firmware Refactoring Plan

**Ngày tạo:** 2025-10-02
**Version:** 1.0.0
**Tác giả:** Code Review Analysis

---

## 📋 Tổng Quan

Document này mô tả chi tiết kế hoạch refactoring cho firmware ESP8266 EVSE WiFi Module. Mục tiêu chính:

- **Fix critical bugs** (memory leaks, security issues)
- **Apply OOP principles** để code dễ maintain và extend
- **Unified configuration system** cho bootstrap management
- **Improve architecture** với proper error handling và state management

---

## 🎯 Core Refactoring Principles

### **OOP Design Guidelines**

#### 1. **Encapsulation**

✅ **DO:**

- Wrap global state vào classes
- Private members với public interface
- Hide implementation details

❌ **DON'T:**

- Expose global variables (`g_mqtt_client`, `g_config`, etc.)
- Direct access to internal state
- Public data members

**Example:**

```cpp
// ❌ Current (C-style)
static mqtt_config_t g_mqtt_config;
static PubSubClient* g_mqtt_client = nullptr;
bool mqtt_client_init(const mqtt_config_t* config);

// ✅ Refactored (OOP)
class MQTTClient {
private:
    mqtt_config_t config;
    PubSubClient client;
public:
    MQTTClient(const mqtt_config_t& cfg);
    ErrorCode connect();
    ErrorCode publish(const String& topic, const String& payload);
};
```

---

#### 2. **Single Responsibility Principle (SRP)**

Mỗi class chỉ làm 1 việc duy nhất.

❌ **Current Issues:**

- `main.cpp` làm quá nhiều việc (orchestration + initialization + business logic)
- `device_provisioning.cpp` vừa manage state vừa handle HTTP vừa save config

✅ **Solution:**

```cpp
// Separate concerns
class ProvisioningStateMachine;  // Only state transitions
class ProvisioningHTTPClient;    // Only HTTP communication
class ProvisioningConfigManager; // Only config storage
class ProvisioningOrchestrator;  // Coordinates above classes
```

---

#### 3. **Dependency Injection**

Classes không tự tạo dependencies, inject từ ngoài vào.

❌ **Current:**

```cpp
void mqtt_client_init() {
    g_mqtt_client = new PubSubClient(g_wifi_client); // Tight coupling!
}
```

✅ **Refactored:**

```cpp
class MQTTClient {
public:
    MQTTClient(WiFiClient& wifiClient, const Config& config);
    // Dependencies injected via constructor
};

// In main:
WiFiClient wifiClient;
Config config = configManager.load();
MQTTClient mqttClient(wifiClient, config);
```

**Benefits:**

- Easy to test (mock dependencies)
- Loose coupling
- Clear dependencies

---

#### 4. **Interface Segregation**

Tách interfaces nhỏ, specific cho từng use case.

✅ **Example:**

```cpp
// Instead of one big interface:
class INetworkManager {
    virtual bool connectWiFi() = 0;
    virtual bool connectMQTT() = 0;
    virtual bool sendHTTP() = 0;
};

// Split into focused interfaces:
class IWiFiConnector {
    virtual ErrorCode connect(const WiFiConfig& cfg) = 0;
    virtual bool isConnected() const = 0;
};

class IMQTTPublisher {
    virtual ErrorCode publish(const Message& msg) = 0;
};

class IHTTPClient {
    virtual Response get(const String& url) = 0;
};
```

---

#### 5. **Polymorphism & Strategy Pattern**

Use interfaces cho interchangeable behaviors.

✅ **Example - Retry Strategies:**

```cpp
class IRetryPolicy {
public:
    virtual uint32_t nextRetryDelay(uint8_t attemptCount) = 0;
};

class ExponentialBackoff : public IRetryPolicy {
    uint32_t nextRetryDelay(uint8_t attemptCount) override {
        return min(1000 * (1 << attemptCount), 60000);
    }
};

class FixedDelay : public IRetryPolicy {
    uint32_t nextRetryDelay(uint8_t attemptCount) override {
        return 5000;
    }
};

// Usage:
class MQTTClient {
    IRetryPolicy* retryPolicy;
public:
    void setRetryPolicy(IRetryPolicy* policy) {
        retryPolicy = policy;
    }
};
```

---

#### 6. **RAII (Resource Acquisition Is Initialization)**

Resources (memory, files, connections) managed by object lifetime.

❌ **Current:**

```cpp
g_mqtt_client = new PubSubClient(...); // Never deleted - MEMORY LEAK!
```

✅ **Refactored:**

```cpp
class MQTTClient {
private:
    PubSubClient client; // Stack allocation
public:
    MQTTClient() { /* init */ }
    ~MQTTClient() { /* cleanup automatic */ }
};

// Or use smart pointers:
std::unique_ptr<PubSubClient> client;
```

---

## 🔴 Critical Issues Found

### 1. **Hardcoded Credentials & Security**

- **Location:** `main.cpp:289, 231-232`, `mqtt_client.cpp:86`, `device_provisioning.cpp:37-38`
- **OOP Violation:** Magic strings, no configuration abstraction
- **Impact:** 🔥 Critical

### 2. **Memory Leaks**

- **Location:** `mqtt_client.cpp:42`
- **OOP Violation:** Manual `new` without `delete`, no RAII
- **Impact:** 🔥 Critical

### 3. **Buffer Overflow Risk**

- **Location:** `stm32_comm.cpp:97-110`
- **OOP Violation:** Raw buffers, no encapsulation
- **Impact:** 🔥 Critical

### 4. **Fragmented Bootstrap Configuration**

- **Location:** `config_manager.cpp`, `device_provisioning.cpp`
- **OOP Violation:** Duplicate config systems, no single source of truth
- **Impact:** 🔥 Critical

### 5. **Global State Everywhere**

- **Location:** Every `.cpp` file has `static` globals
- **OOP Violation:** No encapsulation, tight coupling
- **Impact:** 🔥 Critical
- **Examples:**
  ```cpp
  static mqtt_config_t g_mqtt_config;           // mqtt_client.cpp
  static stm32_comm_status_t g_comm_status;     // stm32_comm.cpp
  static provisioning_state_t g_provisioning_state; // device_provisioning.cpp
  ```

### 6. **Code Duplication**

- **Location:** `stm32_comm.cpp` (process vs handle)
- **OOP Violation:** DRY principle violated
- **Impact:** ⚠️ High

### 7. **Poor State Management**

- **Location:** `main.cpp:134-157`, `device_provisioning.cpp:302-347`
- **OOP Violation:** State machine logic mixed với side effects
- **Impact:** ⚠️ High

---

## 🎯 Refactoring Roadmap

### **Phase 0: Bootstrap & Configuration Foundation** (Priority P0 - 2-3 days)

#### ✅ Task 0.1: Create Unified Config System (OOP)

- [ ] Design `ConfigManager` class hierarchy:

  ```cpp
  class IConfig {
      virtual String get(const String& key) const = 0;
      virtual void set(const String& key, const String& value) = 0;
  };

  class FactoryConfig : public IConfig;      // Read-only defaults
  class BootstrapConfig : public IConfig;    // Upload from admin
  class RuntimeConfig : public IConfig;      // User modifications
  class CloudConfig : public IConfig;        // Provisioning server

  class ConfigManager {
      ConfigManager(IConfig* factory, IConfig* bootstrap,
                   IConfig* runtime, IConfig* cloud);
      String get(const String& key) const; // Merge với priority
      void save();
  };
  ```

- [ ] Implement config validation (Strategy pattern)
- [ ] Implement config persistence (Repository pattern)
- [ ] Add config versioning & migration

**New files:**

- `include/config/i_config.h` - Interface
- `include/config/config_manager.h` - Main manager
- `src/config/factory_config.cpp`
- `src/config/bootstrap_config.cpp`
- `src/config/runtime_config.cpp`
- `src/config/cloud_config.cpp`

**OOP Principles Applied:**

- ✅ Interface Segregation (IConfig)
- ✅ Dependency Injection (inject configs vào manager)
- ✅ Strategy Pattern (config priority/merge)
- ✅ Repository Pattern (persistence)

---

#### ✅ Task 0.2: Bootstrap Sequence Manager (OOP)

- [ ] Create `BootstrapManager` orchestrator:

  ```cpp
  class IBootstrapStage {
      virtual ErrorCode execute() = 0;
      virtual String getName() const = 0;
  };

  class HardwareInitStage : public IBootstrapStage;
  class ConfigLoadStage : public IBootstrapStage;
  class NetworkInitStage : public IBootstrapStage;
  class ProvisioningStage : public IBootstrapStage;

  class BootstrapManager {
      std::vector<IBootstrapStage*> stages;
  public:
      void addStage(IBootstrapStage* stage);
      ErrorCode execute();
      void onStageComplete(std::function<void(String)> callback);
  };
  ```

- [ ] Implement recovery mechanism
- [ ] Add checkpoint logging

**OOP Principles Applied:**

- ✅ Strategy Pattern (pluggable stages)
- ✅ Template Method Pattern (execute sequence)
- ✅ Observer Pattern (stage callbacks)

---

#### ✅ Task 0.3: Factory Config Template & Upload

- [ ] Create JSON schema validator class
- [ ] Implement web upload endpoint (Controller pattern)
- [ ] Create Serial config command handler

**Files:**

- `data/factory_defaults.json`
- `include/web/config_upload_controller.h`
- `tools/upload_bootstrap_config.py`

---

### **Phase 1: Critical Fixes** (Priority P0 - 2-3 days)

#### ✅ Task 1.1: Eliminate Global State

- [ ] Refactor tất cả modules thành classes
- [ ] Move global state thành private members
- [ ] Use dependency injection

**Refactoring Plan:**

**MQTT Module:**

```cpp
// ❌ Before (C-style)
static mqtt_config_t g_mqtt_config;
static PubSubClient* g_mqtt_client = nullptr;
bool mqtt_client_init(const mqtt_config_t* config);

// ✅ After (OOP)
class MQTTClient {
private:
    mqtt_config_t config;
    PubSubClient client;
    std::queue<Message> messageQueue;

public:
    MQTTClient(const mqtt_config_t& cfg);
    ~MQTTClient();

    ErrorCode connect();
    ErrorCode disconnect();
    ErrorCode publish(const String& topic, const String& payload, uint8_t qos = 0);
    ErrorCode subscribe(const String& topic, MessageCallback callback);
    void handle();

    bool isConnected() const;
    MQTTStatus getStatus() const;
};
```

**WiFi Module:**

```cpp
class WiFiManager {
private:
    WiFiConfig config;
    WiFiStatus status;

public:
    WiFiManager(const WiFiConfig& cfg);
    ErrorCode connect();
    ErrorCode startConfigPortal();
    void handle();
    bool isConnected() const;
};
```

**STM32 Communication:**

```cpp
class STM32Communicator {
private:
    RingBuffer<uint8_t> rxBuffer;
    CommStatus status;
    PacketParser parser;

public:
    STM32Communicator(size_t bufferSize = 512);
    ErrorCode sendPacket(const UARTPacket& packet);
    void handle();
    void setPacketCallback(PacketCallback callback);
};
```

**Provisioning:**

```cpp
class ProvisioningOrchestrator {
private:
    ProvisioningStateMachine stateMachine;
    ProvisioningHTTPClient httpClient;
    ConfigManager& configManager;

public:
    ProvisioningOrchestrator(ConfigManager& cfgMgr);
    ErrorCode start();
    void handle();
    bool isComplete() const;
    ProvisioningState getState() const;
};
```

**Files to modify:**

- ALL `.cpp` files in `src/` - Convert to classes
- ALL `.h` files in `include/` - Define class interfaces

**OOP Principles Applied:**

- ✅ Encapsulation (private state)
- ✅ RAII (constructor/destructor)
- ✅ Dependency Injection
- ✅ Single Responsibility

---

#### ✅ Task 1.2: Fix MQTT Memory Leak (RAII)

- [ ] Remove `new PubSubClient()` - use stack allocation
- [ ] Implement proper destructor
- [ ] Add move semantics if needed

```cpp
class MQTTClient {
private:
    WiFiClient wifiClient;
    PubSubClient client; // Stack allocated!

public:
    MQTTClient(const MQTTConfig& cfg)
        : client(wifiClient) {
        client.setServer(cfg.broker, cfg.port);
    }

    ~MQTTClient() {
        // Automatic cleanup
        if (client.connected()) {
            client.disconnect();
        }
    }

    // Prevent copying (use move if needed)
    MQTTClient(const MQTTClient&) = delete;
    MQTTClient& operator=(const MQTTClient&) = delete;
};
```

---

#### ✅ Task 1.3: Refactor UART Buffer (Encapsulation)

- [ ] Create `RingBuffer<T>` template class
- [ ] Create `UARTPacketParser` class
- [ ] Merge duplicate functions

```cpp
template<typename T>
class RingBuffer {
private:
    T* buffer;
    size_t capacity;
    size_t readPos;
    size_t writePos;
    size_t count;

public:
    RingBuffer(size_t size);
    ~RingBuffer();

    bool push(const T& item);
    bool pop(T& item);
    bool peek(T& item) const;
    size_t available() const;
    bool isFull() const;
    bool isEmpty() const;
    void clear();
};

class UARTPacketParser {
private:
    RingBuffer<uint8_t>& rxBuffer;

public:
    UARTPacketParser(RingBuffer<uint8_t>& buffer);
    bool tryParse(UARTPacket& packet);
};
```

**OOP Principles:**

- ✅ Encapsulation (buffer logic hidden)
- ✅ Template for reusability
- ✅ RAII (automatic cleanup)

---

### **Phase 2: Architecture Improvements** (Priority P1 - 3-5 days)

#### ✅ Task 2.1: Error Handling System (OOP)

- [ ] Create error code hierarchy
- [ ] Implement error reporter with callbacks

```cpp
enum class ErrorCategory {
    SUCCESS = 0,
    NETWORK = 100,
    MQTT = 200,
    PROVISIONING = 300,
    UART = 400,
    CONFIG = 500
};

class Error {
private:
    ErrorCategory category;
    int code;
    String message;

public:
    Error(ErrorCategory cat, int c, const String& msg);
    String toString() const;
    bool isSuccess() const;
    operator bool() const { return !isSuccess(); }
};

class IErrorHandler {
public:
    virtual void onError(const Error& error) = 0;
};

class ErrorReporter {
private:
    std::vector<IErrorHandler*> handlers;

public:
    void addHandler(IErrorHandler* handler);
    void report(const Error& error);
};
```

**Usage:**

```cpp
Error result = mqttClient.connect();
if (result) {
    errorReporter.report(result);
    // Handle error
}
```

---

#### ✅ Task 2.2: State Machine Pattern

- [ ] Extract state machine thành class
- [ ] Implement State Pattern

```cpp
class IProvisioningState {
public:
    virtual void onEnter() = 0;
    virtual void onExit() = 0;
    virtual void handle(ProvisioningContext& ctx) = 0;
    virtual String getName() const = 0;
};

class UnprovisionedState : public IProvisioningState;
class ProvisioningState : public IProvisioningState;
class ProvisionedState : public IProvisioningState;
class OperationalState : public IProvisioningState;
class ErrorState : public IProvisioningState;

class ProvisioningStateMachine {
private:
    IProvisioningState* currentState;
    std::map<String, IProvisioningState*> states;

public:
    void transitionTo(const String& stateName);
    void handle(ProvisioningContext& ctx);
    String getCurrentStateName() const;
};
```

**OOP Principles:**

- ✅ State Pattern
- ✅ Open/Closed Principle (easy to add states)

---

#### ✅ Task 2.3: Ring Buffer Implementation

See Task 1.3 above (combined)

---

### **Phase 3: Advanced Features** (Priority P2 - 5-7 days)

#### ✅ Task 3.1: Manager Classes Architecture

- [ ] Create application architecture

```cpp
class DeviceManager {
private:
    ConfigManager configManager;
    NetworkManager networkManager;
    CommunicationManager commManager;
    BootstrapManager bootstrapManager;

public:
    DeviceManager();
    ErrorCode initialize();
    void run(); // Main loop
};

class NetworkManager {
private:
    WiFiManager wifiManager;
    MQTTClient mqttClient;

public:
    NetworkManager(const NetworkConfig& cfg);
    void handle();
};

class CommunicationManager {
private:
    STM32Communicator stm32Comm;
    WebServer webServer;

public:
    CommunicationManager(const CommConfig& cfg);
    void handle();
};
```

**main.cpp becomes:**

```cpp
DeviceManager deviceManager;

void setup() {
    Serial.begin(115200);
    deviceManager.initialize();
}

void loop() {
    deviceManager.run();
}
```

**OOP Principles:**

- ✅ Facade Pattern (DeviceManager)
- ✅ Composition over Inheritance
- ✅ Separation of Concerns

---

#### ✅ Task 3.2: Retry Strategies (Strategy Pattern)

See OOP principle example above (section 5)

```cpp
class ConnectionManager {
private:
    IRetryPolicy* retryPolicy;
    IConnectable* connector;

public:
    ConnectionManager(IConnectable* conn, IRetryPolicy* policy);
    ErrorCode connect();
};
```

---

#### ✅ Task 3.3: Logging System (Singleton + Observer)

```cpp
enum class LogLevel { DEBUG, INFO, WARN, ERROR };

class ILogOutput {
public:
    virtual void write(LogLevel level, const String& msg) = 0;
};

class SerialLogOutput : public ILogOutput;
class MQTTLogOutput : public ILogOutput;
class FileLogOutput : public ILogOutput;

class Logger {
private:
    static Logger* instance;
    std::vector<ILogOutput*> outputs;
    LogLevel minLevel;

    Logger();

public:
    static Logger& getInstance();
    void addOutput(ILogOutput* output);
    void log(LogLevel level, const String& msg);

    // Convenience methods
    void debug(const String& msg) { log(LogLevel::DEBUG, msg); }
    void info(const String& msg) { log(LogLevel::INFO, msg); }
    void warn(const String& msg) { log(LogLevel::WARN, msg); }
    void error(const String& msg) { log(LogLevel::ERROR, msg); }
};

// Usage:
Logger::getInstance().info("Device started");
```

**OOP Principles:**

- ✅ Singleton Pattern
- ✅ Observer Pattern (multiple outputs)
- ✅ Strategy Pattern (different outputs)

---

## 📊 OOP Refactoring Checklist

### Per-Module Checklist

Cho mỗi module, check:

- [ ] ✅ **No global variables** - Tất cả state trong classes
- [ ] ✅ **Private data members** - Encapsulation đúng
- [ ] ✅ **Public interface minimal** - Only expose what's needed
- [ ] ✅ **RAII compliant** - Constructor/destructor manage resources
- [ ] ✅ **Dependency injection** - Dependencies passed in, not created
- [ ] ✅ **Single Responsibility** - Class làm 1 việc
- [ ] ✅ **Testable** - Có thể test isolated
- [ ] ✅ **No magic numbers** - Constants có name rõ ràng
- [ ] ✅ **Error handling** - Return Error objects, not bool
- [ ] ✅ **Documentation** - Class purpose, public methods documented

---

## 📝 Code Style Guidelines

### Naming Conventions

```cpp
class ClassName;                    // PascalCase
interface IInterfaceName;           // I prefix + PascalCase
void methodName();                  // camelCase
int variableName;                   // camelCase
const int CONSTANT_NAME = 10;       // UPPER_SNAKE_CASE
private: int _privateMember;        // _prefix for private (optional)
```

### File Organization

```
include/
  module_name/
    i_interface.h          // Interface
    class_name.h           // Class declaration

src/
  module_name/
    class_name.cpp         // Implementation
```

### Header Guards

```cpp
#ifndef MODULE_CLASS_NAME_H
#define MODULE_CLASS_NAME_H

// ... code ...

#endif // MODULE_CLASS_NAME_H
```

---

## 🧪 Testing Strategy

### Unit Tests (với OOP)

```cpp
// Mock dependencies for testing
class MockWiFiClient : public IWiFiClient {
    // Fake implementation
};

// Test
TEST(MQTTClientTest, ConnectSuccess) {
    MockWiFiClient mockWifi;
    MQTTConfig config = getTestConfig();
    MQTTClient client(mockWifi, config);

    Error result = client.connect();
    ASSERT_FALSE(result);
    ASSERT_TRUE(client.isConnected());
}
```

---

## 📚 References

- **Design Patterns:** Gang of Four (GoF) patterns
- **Clean Code:** Robert C. Martin
- **Effective C++:** Scott Meyers
- **ESP8266 Arduino Core:** https://arduino-esp8266.readthedocs.io/

---

## ✅ Definition of Done

Một task được coi là **Done** khi:

- ✅ Code follows OOP principles (checklist above)
- ✅ No global variables (except singletons if needed)
- ✅ All classes documented (purpose, usage)
- ✅ Unit tests pass
- ✅ Integration tests pass
- ✅ No memory leaks (tested 24h)
- ✅ Code review approved
- ✅ Checklist trong plan này checked

---

**Last Updated:** 2025-10-02
**Next Review:** After Phase 0 completion
