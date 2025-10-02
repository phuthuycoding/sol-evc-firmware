# EVSE Firmware Testing Guide

## Testing Strategy

### 1. Unit Testing

- Individual component testing
- Mock hardware interfaces
- Protocol validation
- Memory leak detection

### 2. Integration Testing

- STM32 ↔ ESP8266 communication
- MQTT message flow
- Hardware abstraction layer
- Error handling scenarios

### 3. Hardware-in-Loop Testing

- Real hardware validation
- Performance benchmarking
- Safety system testing
- Long-term reliability

### 4. System Testing

- End-to-end OCPP compliance
- Backend integration
- Load testing
- Failover scenarios

## Test Environment Setup

### Hardware Requirements

```
Test Bench Components:
├── STM32F103C8T6 development board
├── ESP8266 NodeMCU/Wemos D1 Mini
├── ST-Link V2 programmer
├── USB-to-Serial adapters (2x)
├── Logic analyzer (optional)
├── Oscilloscope (optional)
├── Power supply (3.3V/5V)
└── Breadboard + jumper wires
```

### Software Requirements

```bash
# Install test dependencies
pip install platformio pytest pyserial

# Install MQTT broker for testing
sudo apt-get install mosquitto mosquitto-clients

# Install logic analyzer software
sudo apt-get install pulseview sigrok-cli
```

## Unit Tests

### STM32F103 Unit Tests

#### UART Protocol Tests

```c
// test_uart_protocol.c
#include "unity.h"
#include "uart_protocol.h"

void test_packet_checksum(void) {
    uart_packet_t packet;
    uart_init_packet(&packet, CMD_MQTT_PUBLISH, 1);
    strcpy((char*)packet.payload, "test");
    packet.length = 4;

    uint8_t checksum = uart_calculate_checksum(&packet);
    packet.checksum = checksum;

    TEST_ASSERT_TRUE(uart_validate_packet(&packet));
}

void test_packet_validation(void) {
    uart_packet_t packet = {0};

    // Invalid start byte
    packet.start_byte = 0xFF;
    TEST_ASSERT_FALSE(uart_validate_packet(&packet));

    // Valid packet
    uart_init_packet(&packet, CMD_GET_TIME, 1);
    packet.checksum = uart_calculate_checksum(&packet);
    TEST_ASSERT_TRUE(uart_validate_packet(&packet));
}

void test_sequence_numbers(void) {
    static uint8_t sequence = 0;

    for (int i = 0; i < 300; i++) {
        uint8_t seq = get_next_sequence();
        TEST_ASSERT_EQUAL(sequence, seq);
        sequence = (sequence + 1) % 256;
    }
}
```

#### Meter Service Tests

```c
// test_meter_service.c
void test_cs5460a_reading(void) {
    meter_sample_t sample;

    // Mock SPI responses
    mock_spi_response(CS5460A_ENERGY_REG, 0x12345678);
    mock_spi_response(CS5460A_POWER_REG, 0x1000);

    bool result = meter_service_read_channel(0, &sample);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL(0x12345678, sample.energy_wh);
    TEST_ASSERT_EQUAL(0x1000, sample.power_w);
}

void test_meter_calibration(void) {
    // Test calibration constants
    meter_calibration_t cal = {
        .voltage_gain = 1.0f,
        .current_gain = 1.0f,
        .power_offset = 0.0f
    };

    meter_service_set_calibration(0, &cal);

    meter_sample_t raw = {.voltage_v = 240, .current_a = 10};
    meter_sample_t calibrated;

    meter_service_apply_calibration(&raw, &cal, &calibrated);

    TEST_ASSERT_EQUAL(240, calibrated.voltage_v);
    TEST_ASSERT_EQUAL(10, calibrated.current_a);
    TEST_ASSERT_EQUAL(2400, calibrated.power_w);
}
```

### ESP8266 Unit Tests

#### MQTT Client Tests

```cpp
// test_mqtt_client.cpp
#include <gtest/gtest.h>
#include "mqtt_client.h"

class MQTTClientTest : public ::testing::Test {
protected:
    void SetUp() override {
        mqtt_client_init();
    }

    void TearDown() override {
        mqtt_client_deinit();
    }
};

TEST_F(MQTTClientTest, TopicGeneration) {
    char topic[128];

    mqtt_build_topic(topic, "station001", "device001", "heartbeat");
    EXPECT_STREQ("ocpp/station001/device001/event/0/heartbeat", topic);

    mqtt_build_topic(topic, "station001", "device001", "status_notification", 1);
    EXPECT_STREQ("ocpp/station001/device001/status/1/status_notification", topic);
}

TEST_F(MQTTClientTest, MessageQueue) {
    mqtt_message_t msg;
    strcpy(msg.topic, "test/topic");
    strcpy(msg.payload, "{\"test\":true}");
    msg.qos = 1;

    EXPECT_TRUE(mqtt_queue_message(&msg));

    mqtt_message_t received;
    EXPECT_TRUE(mqtt_dequeue_message(&received));
    EXPECT_STREQ("test/topic", received.topic);
    EXPECT_STREQ("{\"test\":true}", received.payload);
}
```

#### WiFi Manager Tests

```cpp
TEST_F(WiFiTest, ConnectionRetry) {
    wifi_config_t config;
    strcpy(config.ssid, "TestNetwork");
    strcpy(config.password, "TestPassword");

    // Mock WiFi failure
    mock_wifi_connect_result(false);

    bool result = wifi_connect_with_retry(&config, 3);
    EXPECT_FALSE(result);

    // Mock WiFi success on retry
    mock_wifi_connect_result(true);
    result = wifi_connect_with_retry(&config, 3);
    EXPECT_TRUE(result);
}
```

## Integration Tests

### UART Communication Test

```python
# test_uart_integration.py
import serial
import struct
import time

class UARTProtocolTest:
    def __init__(self, stm32_port, esp8266_port):
        self.stm32 = serial.Serial(stm32_port, 115200, timeout=1)
        self.esp8266 = serial.Serial(esp8266_port, 115200, timeout=1)

    def test_mqtt_publish_flow(self):
        # Send MQTT publish command from STM32
        packet = self.build_packet(0x01, b'{"topic":"test","data":"hello"}')
        self.stm32.write(packet)

        # Verify ESP8266 receives and processes
        response = self.esp8266.read(100)
        self.assertTrue(len(response) > 0)

        # Check for ACK response
        ack = self.stm32.read(10)
        self.assertEqual(ack[1], 0x81)  # RSP_MQTT_ACK

    def build_packet(self, cmd_type, payload):
        start = 0xAA
        length = len(payload)
        sequence = 1
        checksum = cmd_type ^ (length & 0xFF) ^ (length >> 8) ^ sequence
        for b in payload:
            checksum ^= b
        end = 0x55

        return struct.pack('<BBHB', start, cmd_type, length, sequence) + payload + struct.pack('<BB', checksum, end)
```

### MQTT Integration Test

```python
# test_mqtt_integration.py
import paho.mqtt.client as mqtt
import json
import time

class MQTTIntegrationTest:
    def __init__(self):
        self.client = mqtt.Client()
        self.received_messages = []
        self.client.on_message = self.on_message
        self.client.connect("localhost", 1883, 60)
        self.client.loop_start()

    def on_message(self, client, userdata, msg):
        self.received_messages.append({
            'topic': msg.topic,
            'payload': msg.payload.decode()
        })

    def test_heartbeat_flow(self):
        # Subscribe to heartbeat topic
        self.client.subscribe("ocpp/+/+/event/0/heartbeat")

        # Trigger heartbeat from device
        self.trigger_heartbeat()

        # Wait for message
        time.sleep(2)

        # Verify heartbeat received
        heartbeat_msgs = [msg for msg in self.received_messages
                         if 'heartbeat' in msg['topic']]

        assert len(heartbeat_msgs) > 0

        # Validate payload
        payload = json.loads(heartbeat_msgs[0]['payload'])
        assert 'msgId' in payload
        assert 'ts' in payload
```

## Hardware-in-Loop Tests

### Relay Control Test

```c
// test_relay_control.c
void test_relay_switching(void) {
    // Test all relay channels
    for (uint8_t channel = 0; channel < MAX_RELAY_CHANNELS; channel++) {
        // Turn on relay
        relay_control_set_state(channel, true);
        HAL_Delay(100);

        // Verify relay state (read back GPIO)
        bool state = relay_control_get_state(channel);
        TEST_ASSERT_TRUE(state);

        // Turn off relay
        relay_control_set_state(channel, false);
        HAL_Delay(100);

        state = relay_control_get_state(channel);
        TEST_ASSERT_FALSE(state);
    }
}

void test_relay_current_monitoring(void) {
    // Enable relay and load
    relay_control_set_state(0, true);
    HAL_Delay(1000);

    // Read current
    meter_sample_t sample;
    meter_service_read_channel(0, &sample);

    // Verify current is within expected range
    TEST_ASSERT_GREATER_THAN(0, sample.current_a);
    TEST_ASSERT_LESS_THAN(35, sample.current_a);  // Below overcurrent limit
}
```

### Safety System Test

```c
// test_safety_system.c
void test_overcurrent_protection(void) {
    // Simulate overcurrent condition
    mock_meter_reading(0, 40);  // 40A > 35A limit

    // Run safety check
    safety_monitor_check_all();

    // Verify relay is turned off
    bool relay_state = relay_control_get_state(0);
    TEST_ASSERT_FALSE(relay_state);

    // Verify fault status
    connector_status_t status = ocpp_client_get_connector_status(0);
    TEST_ASSERT_EQUAL(CONNECTOR_FAULTED, status);
}

void test_emergency_stop(void) {
    // Trigger emergency stop
    safety_monitor_emergency_stop();

    // Verify all relays are off
    for (uint8_t i = 0; i < MAX_RELAY_CHANNELS; i++) {
        bool state = relay_control_get_state(i);
        TEST_ASSERT_FALSE(state);
    }

    // Verify system is in fault state
    device_state_t state = device_manager_get_state();
    TEST_ASSERT_EQUAL(DEVICE_STATE_FAULTED, state);
}
```

## Performance Tests

### Memory Usage Test

```c
// test_memory_usage.c
void test_heap_usage(void) {
    size_t initial_heap = xPortGetFreeHeapSize();

    // Allocate and free memory multiple times
    for (int i = 0; i < 100; i++) {
        void* ptr = pvPortMalloc(1024);
        TEST_ASSERT_NOT_NULL(ptr);
        vPortFree(ptr);
    }

    size_t final_heap = xPortGetFreeHeapSize();

    // Check for memory leaks
    TEST_ASSERT_EQUAL(initial_heap, final_heap);
}

void test_stack_usage(void) {
    // Check stack high water mark for each task
    UBaseType_t heartbeat_stack = uxTaskGetStackHighWaterMark(heartbeat_task_handle);
    UBaseType_t meter_stack = uxTaskGetStackHighWaterMark(meter_task_handle);
    UBaseType_t safety_stack = uxTaskGetStackHighWaterMark(safety_task_handle);

    // Verify sufficient stack remaining (>25%)
    TEST_ASSERT_GREATER_THAN(64, heartbeat_stack);  // 256 words * 0.25
    TEST_ASSERT_GREATER_THAN(128, meter_stack);     // 512 words * 0.25
    TEST_ASSERT_GREATER_THAN(64, safety_stack);     // 256 words * 0.25
}
```

### Timing Test

```c
// test_timing.c
void test_task_timing(void) {
    uint32_t start_time = HAL_GetTick();

    // Run for 10 seconds
    while (HAL_GetTick() - start_time < 10000) {
        vTaskDelay(pdMS_TO_TICKS(100));
    }

    // Check task execution counts
    uint32_t heartbeat_count = get_heartbeat_count();
    uint32_t meter_count = get_meter_reading_count();
    uint32_t status_count = get_status_check_count();

    // Verify expected execution rates
    TEST_ASSERT_INT_WITHIN(1, 0, heartbeat_count);      // 0 heartbeats in 10s
    TEST_ASSERT_INT_WITHIN(2, 10, meter_count);         // ~10 meter readings
    TEST_ASSERT_INT_WITHIN(10, 100, status_count);      // ~100 status checks
}
```

## Automated Testing

### Test Runner Script

```bash
#!/bin/bash
# run_tests.sh

echo "=== EVSE Firmware Test Suite ==="

# Unit tests
echo "Running STM32 unit tests..."
cd stm32-master
pio test

echo "Running ESP8266 unit tests..."
cd ../esp8266-wifi
pio test

# Integration tests
echo "Running integration tests..."
cd ../tests
python -m pytest integration/ -v

# Hardware tests (if hardware connected)
if [ "$1" == "--hardware" ]; then
    echo "Running hardware-in-loop tests..."
    python -m pytest hardware/ -v
fi

echo "Test suite complete!"
```

### Continuous Integration

```yaml
# .github/workflows/test.yml
name: Firmware Tests

on: [push, pull_request]

jobs:
  test:
    runs-on: ubuntu-latest

    steps:
      - uses: actions/checkout@v2

      - name: Setup PlatformIO
        run: |
          pip install platformio
          pio platform install ststm32
          pio platform install espressif8266

      - name: Run STM32 tests
        run: |
          cd stm32-master
          pio test

      - name: Run ESP8266 tests
        run: |
          cd esp8266-wifi
          pio test

      - name: Run integration tests
        run: |
          pip install pytest pyserial paho-mqtt
          cd tests
          python -m pytest integration/ -v
```

## Test Reports

### Coverage Report

```bash
# Generate coverage report
pio test --with-coverage

# View coverage
genhtml coverage.info -o coverage_html
firefox coverage_html/index.html
```

### Performance Report

```python
# performance_report.py
import json
import matplotlib.pyplot as plt

def generate_performance_report():
    # Load test results
    with open('test_results.json', 'r') as f:
        results = json.load(f)

    # Plot memory usage over time
    plt.figure(figsize=(12, 8))

    plt.subplot(2, 2, 1)
    plt.plot(results['heap_usage'])
    plt.title('Heap Usage Over Time')
    plt.ylabel('Bytes')

    plt.subplot(2, 2, 2)
    plt.plot(results['response_times'])
    plt.title('MQTT Response Times')
    plt.ylabel('Milliseconds')

    plt.subplot(2, 2, 3)
    plt.bar(results['task_names'], results['cpu_usage'])
    plt.title('CPU Usage by Task')
    plt.ylabel('Percentage')

    plt.subplot(2, 2, 4)
    plt.plot(results['power_consumption'])
    plt.title('Power Consumption')
    plt.ylabel('Watts')

    plt.tight_layout()
    plt.savefig('performance_report.png')
    print("Performance report generated: performance_report.png")
```

## Debugging Tools

### Logic Analyzer Setup

```
Channel Assignment:
├── CH0: STM32 TX (PA9)
├── CH1: STM32 RX (PA10)
├── CH2: ESP8266 TX (GPIO1)
├── CH3: ESP8266 RX (GPIO3)
├── CH4: Relay Control (PA0)
├── CH5: CS5460A CS (PB0)
├── CH6: SPI CLK
└── CH7: SPI MOSI
```

### Serial Monitor

```python
# serial_monitor.py
import serial
import time
import threading

class DualSerialMonitor:
    def __init__(self, stm32_port, esp8266_port):
        self.stm32 = serial.Serial(stm32_port, 115200)
        self.esp8266 = serial.Serial(esp8266_port, 115200)

    def start_monitoring(self):
        threading.Thread(target=self.monitor_stm32, daemon=True).start()
        threading.Thread(target=self.monitor_esp8266, daemon=True).start()

    def monitor_stm32(self):
        while True:
            if self.stm32.in_waiting:
                data = self.stm32.readline().decode().strip()
                print(f"[STM32] {time.strftime('%H:%M:%S')} {data}")

    def monitor_esp8266(self):
        while True:
            if self.esp8266.in_waiting:
                data = self.esp8266.readline().decode().strip()
                print(f"[ESP8266] {time.strftime('%H:%M:%S')} {data}")
```

This comprehensive testing guide ensures reliable firmware development and deployment for the EVSE dual firmware system.
