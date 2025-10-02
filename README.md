# EVSE Dual Firmware Project

## Architecture Overview

This project contains **two separate firmware** for the EVSE charging station:

1. **STM32F103 Master Firmware** - Main controller handling OCPP logic, relay control, and metering
2. **ESP8266 WiFi Firmware** - Network connectivity, MQTT client, and OTA updates

## Project Structure

```
firmware/
├── stm32-master/          # STM32F103 Master Controller Firmware
│   ├── src/
│   │   ├── core/          # Main application logic
│   │   ├── drivers/       # Hardware drivers (SPI, UART, GPIO)
│   │   ├── services/      # Business logic services
│   │   └── application/   # OCPP client implementation
│   ├── lib/               # Third-party libraries
│   ├── platformio.ini     # PlatformIO configuration
│   └── CMakeLists.txt     # CMake build system
│
├── esp8266-wifi/          # ESP8266 WiFi Module Firmware
│   ├── src/
│   │   ├── core/          # Main WiFi/MQTT logic
│   │   ├── network/       # WiFi and Ethernet management
│   │   ├── mqtt/          # MQTT client implementation
│   │   └── communication/ # STM32 UART interface
│   ├── data/              # Web interface files
│   ├── platformio.ini     # PlatformIO configuration
│   └── package.json       # Node.js dependencies for web build
│
├── shared/                # Shared protocol definitions
│   ├── uart_protocol.h    # UART communication protocol
│   ├── ocpp_messages.h    # OCPP message structures
│   └── device_config.h    # Common device configuration
│
├── tools/                 # Development and build tools
│   ├── flash_stm32.sh     # STM32 flashing script
│   ├── flash_esp8266.sh   # ESP8266 flashing script
│   └── dual_ota.py        # Dual firmware OTA update tool
│
└── docs/                  # Documentation
    ├── uart_protocol.md   # UART communication specification
    ├── build_guide.md     # Build and flash instructions
    └── testing_guide.md   # Testing procedures
```

## Communication Protocol

The two MCUs communicate via **UART** using a custom protocol:

- **Baud Rate**: 115200
- **Packet Structure**: Start(0xAA) + CMD + Length + Sequence + Payload + Checksum + End(0x55)
- **Flow Control**: ACK/NACK with sequence numbers
- **Error Handling**: Timeout and retry mechanism

## Key Features

### STM32F103 Master

- OCPP 1.6J client implementation
- 10-channel relay control (SSR 30A)
- 10-channel energy metering (CS5460A)
- RS485 slave device communication
- Transaction management and safety monitoring
- Persistent storage for offline operation

### ESP8266 WiFi Module

- WiFi connection management with auto-reconnect
- MQTT client with TLS support
- Ethernet fallback via RJ45
- Web configuration portal
- OTA updates for both firmwares
- Network time synchronization (NTP)

## Development Environment

- **Platform**: PlatformIO
- **STM32 Framework**: STM32Cube HAL + FreeRTOS
- **ESP8266 Framework**: Arduino Core + ESP8266WiFi
- **Build System**: CMake (STM32) + PlatformIO (ESP8266)
- **Debugging**: ST-Link (STM32) + Serial (ESP8266)

## Getting Started

1. Install PlatformIO Core
2. Clone this repository
3. Build STM32 firmware: `cd stm32-master && pio run`
4. Build ESP8266 firmware: `cd esp8266-wifi && pio run`
5. Flash both firmwares using provided scripts

## Testing

- **Unit Tests**: Individual component testing
- **Integration Tests**: UART protocol validation
- **Hardware-in-Loop**: Full system testing with real hardware
- **OCPP Compliance**: Backend integration testing
