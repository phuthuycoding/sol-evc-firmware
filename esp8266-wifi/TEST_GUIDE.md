# ESP8266 Firmware Testing Guide

## Overview
This project uses **PlatformIO Unity** testing framework with a layered testing approach.

## Test Structure

```
test/
├── test_handlers/          # Business logic tests (stateless)
│   ├── test_heartbeat_handler.cpp
│   ├── test_ocpp_message_handler.cpp
│   └── test_stm32_command_handler.cpp
├── test_drivers/           # Driver layer tests
│   ├── test_ntp_time.cpp
│   ├── test_mqtt_client.cpp
│   └── test_stm32_comm.cpp
├── test_protocol/          # Protocol tests
│   └── test_uart_protocol.cpp
└── test_mocks/             # Mock objects for testing
    ├── mock_mqtt_client.h
    ├── mock_wifi_manager.h
    └── mock_stm32_comm.h
```

## Test Environments

### 1. Native Testing (on PC)
Fast unit tests that run on your development machine:

```bash
# Run all native tests
pio test -e native

# Run specific test
pio test -e native -f test_uart_protocol

# Run with verbose output
pio test -e native -v
```

**Advantages:**
- ⚡ Very fast (no upload time)
- 🔁 Quick iteration during development
- 💻 Can debug with GDB

**Limitations:**
- No hardware-specific features (WiFi, UART, etc.)
- Best for handlers and pure logic

### 2. Embedded Testing (on ESP8266)
Tests that run on actual hardware:

```bash
# Run tests on ESP8266
pio test -e test_esp

# Upload and monitor specific test
pio test -e test_esp -f test_ntp_time
```

**Advantages:**
- ✅ Real hardware behavior
- 🌐 Can test WiFi, NTP, UART
- 📊 Accurate memory usage

**Use for:**
- Driver tests (WiFi, MQTT, NTP)
- Integration tests
- Hardware-specific features

## Writing Tests

### Handler Tests (Stateless Logic)

```cpp
#include <unity.h>
#include "handlers/heartbeat_handler.h"
#include "test_mocks/mock_mqtt_client.h"

void test_heartbeat_publishes(void) {
    // Arrange
    MockMQTTClient mqtt;
    MockWiFiManager wifi;
    DeviceConfig config = {.deviceId = "TEST-001"};

    // Act
    bool result = HeartbeatHandler::execute(mqtt, wifi, config, 0);

    // Assert
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_TRUE(mqtt.wasPublishCalled());
}

void process(void) {
    UNITY_BEGIN();
    RUN_TEST(test_heartbeat_publishes);
    UNITY_END();
}
```

### Driver Tests (Hardware Features)

```cpp
#include <unity.h>
#include "drivers/time/ntp_time.h"

void test_ntp_init(void) {
    // Arrange
    NTPTimeDriver ntp;

    // Act
    ntp.init("pool.ntp.org", 420);  // UTC+7

    // Assert
    TEST_ASSERT_EQUAL(420, ntp.getTimezoneOffset());
}
```

### Protocol Tests (UART)

```cpp
#include <unity.h>
#include "../../shared/uart_protocol.h"

void test_uart_checksum(void) {
    // Arrange
    uart_packet_t packet;
    uart_init_packet(&packet, CMD_GET_TIME, 1);

    // Act
    uint8_t checksum = uart_calculate_checksum(&packet);

    // Assert
    TEST_ASSERT_EQUAL_UINT8(expected, checksum);
}
```

## Mock Objects

### MockMQTTClient
```cpp
MockMQTTClient mqtt;
mqtt.setConnected(true);
mqtt.publish("topic", "payload", 1);

TEST_ASSERT_TRUE(mqtt.wasPublishCalled());
TEST_ASSERT_EQUAL_STRING("topic", mqtt.getLastTopic());
```

### MockWiFiManager
```cpp
MockWiFiManager wifi;
wifi.setConnected(true);
wifi.setRSSI(-45);

TEST_ASSERT_TRUE(wifi.isConnected());
TEST_ASSERT_EQUAL(-45, wifi.getRSSI());
```

### MockSTM32Communicator
```cpp
MockSTM32Communicator stm32;
stm32.sendAck(5, STATUS_SUCCESS);

TEST_ASSERT_TRUE(stm32.wasAckSent());
TEST_ASSERT_EQUAL(5, stm32.getLastSequence());
```

## Test Development Workflow

### TDD Approach (Recommended)
```bash
# 1. Write test first (RED)
vim test/test_handlers/test_new_handler.cpp

# 2. Run test (should fail)
pio test -e native -f test_new_handler

# 3. Implement handler (GREEN)
vim src/handlers/new_handler.cpp

# 4. Run test (should pass)
pio test -e native -f test_new_handler

# 5. Refactor if needed
# 6. Test on hardware
pio test -e test_esp -f test_new_handler
```

### Test-After Approach
```bash
# 1. Implement feature
vim src/handlers/feature.cpp

# 2. Write tests
vim test/test_handlers/test_feature.cpp

# 3. Run tests locally
pio test -e native

# 4. Verify on hardware
pio test -e test_esp
```

## CI/CD Integration

Add to `.github/workflows/test.yml`:
```yaml
name: Run Tests
on: [push, pull_request]
jobs:
  test:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v2
      - uses: actions/setup-python@v2
      - run: pip install platformio
      - run: cd esp8266-wifi && pio test -e native
```

## Best Practices

### ✅ DO:
- Test business logic (handlers) with native tests
- Use mocks for external dependencies
- Test one thing per test function
- Use descriptive test names: `test_<what>_<condition>_<expected>`
- Keep tests independent (setUp/tearDown)
- Test edge cases and error conditions

### ❌ DON'T:
- Don't test Arduino libraries (assume they work)
- Don't write tests for trivial getters/setters
- Don't use delays in native tests
- Don't depend on test execution order
- Don't test implementation details

## Coverage Goals

| Layer | Coverage Target | Test Type |
|-------|----------------|-----------|
| Handlers | 90%+ | Native |
| Drivers | 70%+ | Embedded |
| Protocol | 100% | Native |

## Common Unity Assertions

```cpp
// Boolean
TEST_ASSERT_TRUE(condition);
TEST_ASSERT_FALSE(condition);

// Equality
TEST_ASSERT_EQUAL(expected, actual);
TEST_ASSERT_EQUAL_STRING("expected", actual);
TEST_ASSERT_EQUAL_UINT8(expected, actual);

// Null checks
TEST_ASSERT_NULL(pointer);
TEST_ASSERT_NOT_NULL(pointer);

// Numeric comparisons
TEST_ASSERT_GREATER_THAN(threshold, actual);
TEST_ASSERT_LESS_THAN(threshold, actual);
TEST_ASSERT_WITHIN(delta, expected, actual);

// Memory
TEST_ASSERT_EQUAL_MEMORY(expected, actual, len);
```

## Debugging Failed Tests

```bash
# Verbose output
pio test -e native -v

# Filter specific test
pio test -e native -f test_name

# Native debugging with GDB
pio test -e native --program-arg --debug
```

## Memory Constraints

ESP8266 has limited RAM (~50KB). Keep tests:
- **Focused**: Test one handler at a time
- **Lightweight**: Use stack allocation
- **Mock-heavy**: Don't instantiate real WiFi/MQTT in tests

## Example Test Run

```bash
$ pio test -e native

Testing...
test/test_protocol/test_uart_protocol.cpp:15:test_uart_init_packet:PASS
test/test_protocol/test_uart_protocol.cpp:16:test_uart_checksum:PASS
test/test_handlers/test_heartbeat_handler.cpp:20:test_heartbeat_publishes:PASS

-----------------------
3 Tests 0 Failures 0 Ignored
OK
```

## Next Steps

1. Run existing tests: `pio test -e native`
2. Add tests for new handlers before implementing
3. Use embedded tests for WiFi/NTP integration
4. Set up CI/CD for automated testing

## Resources

- [PlatformIO Testing](https://docs.platformio.org/en/latest/advanced/unit-testing/)
- [Unity Framework](https://github.com/ThrowTheSwitch/Unity)
- [TDD for Embedded C](https://pragprog.com/titles/jgade/test-driven-development-for-embedded-c/)
