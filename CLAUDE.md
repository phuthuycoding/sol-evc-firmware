# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

This is a **dual-microcontroller firmware** project for an Electric Vehicle Supply Equipment (EVSE) charging station with 10 independent charging connectors. The system uses:

- **STM32F103C8T6**: Main controller handling OCPP 1.6J protocol, relay control, energy metering, and safety monitoring
- **ESP8266 (ESP-07S)**: WiFi/network connectivity module handling MQTT communication, web configuration, and OTA updates

The two MCUs communicate via UART using a custom packet-based protocol.

## Build Commands

### Build Both Firmware

```bash
# Build STM32 and ESP8266 firmware
./tools/dual_build.sh build

# Build STM32 only
cd stm32-master && pio run

# Build ESP8266 only
cd esp8266-wifi && pio run
```

### Flash Firmware

```bash
# Flash both firmware
./tools/dual_build.sh flash

# Flash STM32 via ST-Link
cd stm32-master && pio run --target upload

# Flash ESP8266 via USB
cd esp8266-wifi && pio run --target upload

# Flash ESP8266 via OTA
cd esp8266-wifi && pio run --target upload --upload-port evse-device.local
```

### Testing

```bash
# Run STM32 unit tests
cd stm32-master && pio test

# Run ESP8266 unit tests
cd esp8266-wifi && pio test

# Monitor serial output
# STM32: pio device monitor --baud 115200
# ESP8266: pio device monitor --filter esp8266_exception_decoder
```

### Cleaning

```bash
# Clean all builds
./tools/dual_build.sh clean

# Clean individual projects
cd stm32-master && pio run --target clean
cd esp8266-wifi && pio run --target clean
```

## Architecture

### Directory Structure

```
firmware/
├── stm32-master/       # STM32F103 main controller
│   ├── src/core/       # Core application logic
│   └── platformio.ini
├── esp8266-wifi/       # ESP8266 WiFi module
│   ├── src/
│   │   ├── main.cpp           # Entry point (~80 lines, stack-allocated objects)
│   │   ├── core/              # Device manager (orchestrator)
│   │   ├── drivers/           # WiFi, MQTT, UART drivers (thin wrappers)
│   │   ├── handlers/          # Business logic (optional, if refactored)
│   │   ├── config/            # Configuration management
│   │   ├── network/           # WiFi management
│   │   ├── mqtt/              # MQTT client
│   │   ├── communication/     # STM32 UART interface
│   │   └── utils/             # Logger, ring buffer, helpers
│   └── platformio.ini
├── shared/             # Shared protocol definitions
│   ├── uart_protocol.h # UART communication protocol
│   ├── ocpp_messages.h # OCPP message structures
│   └── device_config.h # Common device configuration
└── tools/              # Build and flash scripts
```

### Communication Protocol

The STM32 and ESP8266 communicate via **UART at 115200 baud** using a custom packet-based protocol:

**Packet Structure:**
```c
start_byte (0xAA) | cmd_type | length (2B) | sequence | payload (0-512B) | checksum | end_byte (0x55)
```

**Key Commands:**
- `CMD_MQTT_PUBLISH (0x01)`: STM32 → ESP8266 MQTT publish
- `CMD_GET_TIME (0x02)`: STM32 → ESP8266 time request
- `RSP_MQTT_RECEIVED (0x85)`: ESP8266 → STM32 incoming MQTT message

See `docs/uart_protocol.md` for full protocol specification.

### STM32F103 Responsibilities

- **OCPP 1.6J Client**: Implements OCPP protocol logic
- **Relay Control**: Manages 10 SSR channels (30A each, PA0-PA9)
- **Energy Metering**: Reads 10 CS5460A chips via SPI (PB0-PB9 CS pins)
- **Safety Monitoring**: Overcurrent, overvoltage, temperature protection
- **Transaction Management**: Charging session tracking and persistence
- **RS485 Communication**: Optional slave device support

**Framework**: STM32Cube HAL + FreeRTOS

### ESP8266 Responsibilities

- **WiFi Management**: Auto-reconnect, WiFi Manager config portal
- **MQTT Client**: Publish/subscribe with TLS support
- **Network Services**: NTP time sync, Ethernet fallback
- **Web Interface**: Configuration portal (LittleFS filesystem)
- **OTA Updates**: Over-the-air firmware updates for both MCUs
- **UART Bridge**: Bidirectional communication with STM32

**Framework**: Arduino Core for ESP8266

**Architecture**: Simplified 3-layer (memory-optimized for ESP8266)
```
Application Layer (handlers/) - Business logic, stateless handlers
       ↓
Drivers Layer (drivers/) - Thin wrappers around platform libraries
       ↓
Platform Layer - ESP8266WiFi, PubSubClient, LittleFS
```

## Key Configuration

### Hardware Limits

- **10 connectors** with independent relay and meter channels
- **Max current per channel**: 30A (SSR rating)
- **Max voltage**: 240V AC
- **Max power per channel**: 7.2kW
- **Overcurrent threshold**: 35A (triggers safety shutdown)

### Memory Constraints

**STM32F103C8T6:**
- Flash: 64KB
- RAM: 20KB
- FreeRTOS heap: 8KB
- Stack per task: 1-2KB

**ESP8266:**
- Flash: 4MB (ESP-07S)
- Program: ~300KB
- Filesystem: ~1MB (web interface)
- Available heap: ~50KB

### Timing

- Heartbeat interval: 30s
- Meter reading: 1s
- Status check: 100ms
- Safety check: 50ms (critical)
- UART timeout: 1s with 3 retries

## Development Guidelines

### Working with UART Protocol

When modifying UART communication:
1. Update protocol definitions in `shared/uart_protocol.h`
2. Keep STM32 and ESP8266 implementations synchronized
3. Test packet validation and checksum calculation
4. Verify timeout/retry logic for reliability
5. Use sequence numbers to prevent duplicate processing

### Working with OCPP

The STM32 implements OCPP 1.6J messaging via MQTT transport:
- MQTT topics follow pattern: `ocpp/{station_id}/{device_id}/{type}/{connector}/{message}`
- JSON payloads defined in `shared/ocpp_messages.h`
- ESP8266 handles MQTT transport, STM32 handles OCPP logic

### Safety-Critical Code

Code affecting relay control and safety monitoring requires extra care:
- **Never disable safety checks** (overcurrent, overvoltage, temperature)
- Safety checks run every 50ms on STM32
- Emergency stop must immediately disable all relays
- Safety faults must persist until manual reset
- Test all safety scenarios before deployment

### Memory Management

**STM32:**
- Use FreeRTOS heap (`pvPortMalloc`/`vPortFree`)
- Monitor stack high water marks (`uxTaskGetStackHighWaterMark`)
- Avoid dynamic allocation in interrupt context
- Compiler flags: `-Os -flto -fdata-sections -ffunction-sections`

**ESP8266 (Critical - Only ~50KB heap available!):**
- **Prefer stack allocation** over heap (`new`/`malloc`)
- Use **fixed-size arrays** instead of dynamic containers (std::vector, String concatenation)
- **Minimize polymorphism** - virtual functions add overhead (vtables in RAM)
- Feed watchdog regularly (`ESP.wdtFeed()`)
- Use `PROGMEM` for constants to save RAM
- Monitor heap: `ESP.getFreeHeap()`, `ESP.getHeapFragmentation()`
- Build flags: `-DPIO_FRAMEWORK_ARDUINO_LWIP2_LOW_MEMORY -DVTABLES_IN_FLASH`
- **Avoid Clean Architecture patterns** - interfaces/polymorphism too memory-heavy for ESP8266
- Current architecture: **DeviceManager + Drivers** (minimal OOP, stack-based)

### Version Management

Firmware version is defined in `shared/device_config.h`:
```c
#define FIRMWARE_VERSION "1.0.0"
```

Update this when releasing new versions. Both STM32 and ESP8266 share the same version number.

## Common Tasks

### Adding a New UART Command

1. Add command type to `shared/uart_protocol.h`
2. Implement handler in `stm32-master/src/core/esp8266_comm.c`
3. Implement sender in `esp8266-wifi/src/communication/stm32_comm.cpp`
4. Update protocol documentation in `docs/uart_protocol.md`
5. Add test cases for packet validation

### Adding a New OCPP Message

1. Define message structure in `shared/ocpp_messages.h`
2. Implement message builder on STM32 side
3. Add MQTT topic routing on ESP8266 side
4. Test message flow end-to-end with MQTT broker

### Modifying Relay or Metering Logic

1. Changes typically in STM32 only (ESP8266 not involved)
2. Update safety thresholds in `shared/device_config.h` if needed
3. Test with hardware-in-loop (HIL) setup
4. Verify safety monitoring still functions correctly
5. Check memory usage after changes (tight RAM constraints)

### Adding New ESP8266 Feature (Memory-Conscious)

1. **Check heap first**: Ensure `ESP.getFreeHeap() > 20KB` after adding feature
2. **Prefer stack allocation**: Avoid `new`, use stack objects or fixed arrays
3. **Add to DeviceManager**: New drivers go in `drivers/`, logic in `core/device_manager.cpp`
4. **Test memory**: Run 24h test, monitor `ESP.getHeapFragmentation()`
5. **Example pattern**:
   ```cpp
   // ✅ Good - stack allocated
   class MyDriver {
       uint8_t buffer[256];  // Fixed size
   public:
       MyDriver() {}  // No new/malloc
   };

   // In main.cpp or device_manager.cpp
   MyDriver driver;  // Stack allocated

   // ❌ Bad - heap allocated
   MyDriver* driver = new MyDriver();  // Avoid this!
   ```

## Debugging

### Serial Monitoring

Monitor both MCUs simultaneously:
```bash
# Terminal 1 (STM32)
cd stm32-master && pio device monitor --baud 115200

# Terminal 2 (ESP8266)
cd esp8266-wifi && pio device monitor --filter esp8266_exception_decoder
```

### UART Protocol Debugging

Both firmwares have debug logging for UART packets. Enable with debug flags in respective platformio.ini files.

### STM32 Debugging

```bash
# GDB debugging via ST-Link
cd stm32-master && pio debug
```

Set breakpoint at `main` with `debug_init_break = tbreak main` in platformio.ini.

### ESP8266 Debugging

Serial debugging with exception decoder is primary method. For web interface debugging:
```bash
curl http://evse-device.local/api/status
```

## Important Notes

- The two firmwares are **independent**: changes to one don't require rebuilding the other unless protocol changes
- UART protocol changes require **both firmwares** to be updated and tested together
- Safety-critical code in STM32 should be tested with physical hardware (HIL testing)
- **ESP8266 memory is CRITICAL (~50KB heap)**:
  - Current architecture uses **stack allocation + minimal OOP** to save memory
  - Avoid Clean Architecture / Use-Case patterns (too memory-heavy)
  - No virtual functions unless absolutely necessary
  - Fixed-size buffers, not dynamic allocation
  - Monitor heap fragmentation regularly
- STM32 has very tight RAM (20KB) - monitor task stack usage carefully
- OTA updates must handle both firmwares - test fail-safe recovery mechanisms

## ESP8266 Architecture Notes

**Current Pattern**: `DeviceManager` (Facade) + Drivers
- `main.cpp`: Entry point, stack-allocated objects, minimal code (~80 lines)
- `core/device_manager.cpp`: Orchestrates all components, manages lifecycle
- `drivers/`: WiFi, MQTT, UART thin wrappers (WiFiManager, MQTTClient, STM32Communicator)
- `config/`: Unified configuration system
- `utils/`: Logger, ring buffer, helpers

**Why Not Clean Architecture?**
- Interfaces/polymorphism = vtables in RAM (expensive!)
- Use-case objects = heap allocation (fragmentation risk)
- Deep layer separation = function call overhead
- **For ESP8266**: Simpler is better. Direct calls, stack allocation, minimal abstraction.
