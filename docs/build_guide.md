# EVSE Dual Firmware Build Guide

## Prerequisites

### Required Tools

- **PlatformIO Core** - `pip install platformio`
- **STLink Tools** - For STM32F103 flashing
- **Node.js & npm** - For ESP8266 web interface build
- **Git** - Version control

### Hardware Requirements

- STM32F103C8T6 development board
- ESP8266 module (ESP-12E/F)
- ST-Link V2 programmer
- USB-to-Serial adapter
- Breadboard and jumper wires

## Project Structure

```
firmware/
├── stm32-master/          # STM32F103 Master Controller
├── esp8266-wifi/          # ESP8266 WiFi Module
├── shared/                # Common protocol headers
├── tools/                 # Build and flash scripts
└── docs/                  # Documentation
```

## Quick Start

### 1. Build Both Firmware

```bash
cd /path/to/firmware
./tools/dual_build.sh build
```

### 2. Flash Both Firmware

```bash
./tools/dual_build.sh flash
```

### 3. Monitor Serial Output

```bash
# STM32F103 monitor
./tools/flash_stm32.sh monitor

# ESP8266 monitor
./tools/flash_esp8266.sh monitor
```

## Individual Firmware Build

### STM32F103 Master Controller

#### Build Commands

```bash
cd stm32-master

# Build firmware
pio run

# Flash via ST-Link
pio run --target upload

# Serial monitor
pio device monitor --baud 115200
```

#### Memory Usage

- **Flash**: 64KB (STM32F103C8T6)
- **RAM**: 20KB
- **Stack per task**: 1KB-2KB
- **Heap**: Dynamic allocation

#### Pin Configuration

```c
// Relay Control: PA0-PA9 (10 channels)
// CS5460A CS: PB0-PB9 (10 channels)
// ESP8266 UART: PA9(TX), PA10(RX)
// RS485: PB10(TX), PB11(RX), PA11(DE)
// Status LED: PC13
```

### ESP8266 WiFi Module

#### Build Commands

```bash
cd esp8266-wifi

# Build web interface + firmware
pio run

# Flash via USB
pio run --target upload

# Flash filesystem
pio run --target uploadfs

# OTA update
pio run --target upload --upload-port evse-device.local
```

#### Memory Usage

- **Flash**: 4MB (ESP-12E)
- **Program**: ~300KB
- **Filesystem**: ~1MB (web interface)
- **Heap**: ~50KB available

#### Pin Configuration

```c
// STM32 UART: GPIO1(TX), GPIO3(RX)
// Status LED: GPIO2
// Reset: GPIO0 (boot mode)
```

## Communication Protocol

### UART Configuration

- **Baud Rate**: 115200
- **Data Bits**: 8
- **Stop Bits**: 1
- **Parity**: None
- **Flow Control**: None

### Packet Format

```c
typedef struct {
    uint8_t start_byte;    // 0xAA
    uint8_t cmd_type;      // Command type
    uint16_t length;       // Payload length
    uint8_t sequence;      // Sequence number
    uint8_t payload[512];  // Data
    uint8_t checksum;      // XOR checksum
    uint8_t end_byte;      // 0x55
} uart_packet_t;
```

## Development Workflow

### 1. Code Changes

```bash
# Edit source files
vim stm32-master/src/main.c
vim esp8266-wifi/src/main.cpp

# Build and test
./tools/dual_build.sh build
```

### 2. Testing

```bash
# Flash to hardware
./tools/dual_build.sh flash

# Monitor both devices
# Terminal 1:
./tools/flash_stm32.sh monitor

# Terminal 2:
./tools/flash_esp8266.sh monitor
```

### 3. Debugging

#### STM32F103 Debugging

```bash
# GDB debugging via ST-Link
pio debug

# Or use external debugger
arm-none-eabi-gdb .pio/build/stm32f103c8/firmware.elf
```

#### ESP8266 Debugging

```bash
# Serial debugging
pio device monitor --filter esp8266_exception_decoder

# Web interface debugging
curl http://evse-device.local/api/status
```

## Troubleshooting

### Build Issues

#### STM32F103 Build Fails

```bash
# Clean and rebuild
./tools/flash_stm32.sh clean
./tools/flash_stm32.sh build

# Check dependencies
pio platform show ststm32
```

#### ESP8266 Build Fails

```bash
# Clean and rebuild
./tools/flash_esp8266.sh clean
./tools/flash_esp8266.sh build

# Update platform
pio platform update espressif8266
```

### Flash Issues

#### STM32F103 Flash Fails

```bash
# Check ST-Link connection
st-info --probe

# Try different upload method
st-flash write .pio/build/stm32f103c8/firmware.bin 0x8000000

# Check jumper settings (BOOT0 = 0)
```

#### ESP8266 Flash Fails

```bash
# Check USB connection
pio device list

# Try different baud rate
pio run --target upload --upload-speed 115200

# Hold FLASH button during upload
```

### Runtime Issues

#### No UART Communication

1. Check baud rate settings (115200)
2. Verify TX/RX pin connections
3. Check ground connection
4. Monitor with oscilloscope

#### WiFi Connection Issues

1. Check WiFi credentials
2. Use WiFi Manager config portal
3. Check signal strength
4. Try ethernet fallback

#### MQTT Connection Issues

1. Verify broker settings
2. Check network connectivity
3. Validate credentials
4. Monitor MQTT logs

## Performance Optimization

### STM32F103 Optimization

```c
// Compiler flags
-Os -flto -fdata-sections -ffunction-sections

// Memory optimization
#define configTOTAL_HEAP_SIZE (8192)  // 8KB heap
#define configMINIMAL_STACK_SIZE (128) // 128 words
```

### ESP8266 Optimization

```cpp
// WiFi power management
WiFi.setSleepMode(WIFI_LIGHT_SLEEP);

// Memory management
ESP.wdtFeed();  // Feed watchdog
system_soft_wdt_feed();  // Software watchdog
```

## Production Deployment

### 1. Version Management

```bash
# Update version in shared/device_config.h
#define FIRMWARE_VERSION "1.0.1"

# Tag release
git tag v1.0.1
```

### 2. Build Release

```bash
# Clean build
./tools/dual_build.sh clean

# Release build
./tools/dual_build.sh build

# Verify binaries
./tools/dual_build.sh status
```

### 3. OTA Deployment

```bash
# Upload to OTA server
./tools/flash_esp8266.sh ota

# Batch update multiple devices
./tools/batch_ota.py --version 1.0.1 --devices devices.txt
```
