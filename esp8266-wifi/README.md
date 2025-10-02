# ESP8266 WiFi Module for EVSE System

A modular ESP8266 firmware for Electric Vehicle Supply Equipment (EVSE) charging stations, providing WiFi connectivity, MQTT communication, and web-based configuration portal.

## 🚀 Features

### Core Functionality

- **WiFi Management**: Auto-connect with fallback configuration portal
- **MQTT Client**: Robust messaging with queue, reconnection, and TLS support
- **STM32 Communication**: UART protocol for master controller integration
- **Web Interface**: Modern configuration portal with real-time monitoring
- **OTA Updates**: Remote firmware updates capability
- **Persistent Configuration**: JSON-based settings with validation

### EVSE Integration

- **OCPP Protocol Support**: Compatible with backend MQTT topics
- **Heartbeat Monitoring**: Regular status reporting to cloud
- **Device Management**: Station/device ID configuration
- **Status Reporting**: Real-time system health monitoring
- **Time Synchronization**: NTP client for accurate timestamps

## 📁 Project Structure

```
esp8266-wifi/
├── src/
│   ├── main.cpp                    # Main application entry point
│   ├── config/
│   │   ├── config_manager.h/cpp    # Configuration management
│   ├── network/
│   │   ├── wifi_manager.h/cpp      # WiFi connection handling
│   ├── mqtt/
│   │   ├── mqtt_client.h/cpp       # MQTT client implementation
│   ├── communication/
│   │   ├── stm32_comm.h/cpp        # STM32 UART communication
│   └── web/
│       ├── web_server.h/cpp        # Web server and API endpoints
├── include/                        # Header files
├── data/
│   └── index.html                  # Web interface
├── platformio.ini                  # Build configuration
└── README.md                       # This file
```

## 🛠️ Build Requirements

### Dependencies

- **PlatformIO Core** (latest version)
- **Python 3.7+**
- **ESP8266 Arduino Core** (automatically installed)

### Libraries Used

- ArduinoJson (6.21.2+) - JSON parsing and generation
- PubSubClient (2.8+) - MQTT client
- WiFiManager (2.0.16+) - WiFi configuration portal
- NTPClient (3.2.1+) - Network time synchronization
- ESPAsyncWebServer (1.2.3+) - Async web server

## 🔧 Building and Flashing

### Quick Start

```bash
# Build and flash firmware + web interface
./tools/flash_esp8266.sh all

# Build only (no flashing)
./tools/flash_esp8266.sh build

# Test build without hardware
./tools/flash_esp8266.sh test
```

### Manual Build

```bash
cd esp8266-wifi/
pio run                    # Build firmware
pio run --target upload    # Flash firmware
pio run --target uploadfs  # Flash web interface
```

### Available Commands

- `build` - Build firmware only
- `flash` - Build and flash firmware
- `web` - Build and flash web interface
- `all` - Build and flash everything
- `monitor` - Start serial monitor
- `clean` - Clean build files
- `ota` - OTA firmware update
- `test` - Test build without flashing

## 📡 Configuration

### Default Settings

```json
{
  "station_id": "station001",
  "device_id": "device001",
  "wifi": {
    "ssid": "",
    "password": "",
    "auto_connect": true
  },
  "mqtt": {
    "broker": "localhost",
    "port": 1883,
    "username": "",
    "password": "",
    "tls_enabled": false
  },
  "system": {
    "ota_enabled": true,
    "debug_enabled": true,
    "heartbeat_interval": 30000
  }
}
```

### Configuration Methods

#### 1. Web Interface

1. Connect to `EVSE-Config` WiFi network
2. Navigate to `http://192.168.4.1`
3. Configure settings through web portal

#### 2. Runtime Configuration

- Access web interface at device IP address
- Use REST API endpoints for programmatic configuration

#### 3. File-based Configuration

- Upload `config.json` to device filesystem
- Modify configuration through serial interface

## 🌐 Web Interface

### Features

- **Real-time Status**: WiFi, MQTT, STM32 communication status
- **WiFi Scanner**: Scan and select available networks
- **MQTT Testing**: Test broker connectivity
- **System Management**: Restart, reset, OTA updates
- **Log Viewer**: Real-time system logs

### API Endpoints

- `GET /api/status` - System status
- `GET /api/config` - Current configuration
- `POST /api/config` - Update configuration
- `GET /api/wifi/scan` - Scan WiFi networks
- `POST /api/mqtt/test` - Test MQTT connection
- `POST /api/system/restart` - Restart device
- `POST /api/system/reset` - Reset to defaults

## 📊 MQTT Topics

### Outgoing (Device → Cloud)

```
ocpp/{station_id}/{device_id}/heartbeat
ocpp/{station_id}/{device_id}/status/{connector_id}/status_notification
ocpp/{station_id}/{device_id}/meter/{connector_id}/meter_values
```

### Incoming (Cloud → Device)

```
ocpp/{station_id}/{device_id}/cmd/remote_start
ocpp/{station_id}/{device_id}/cmd/remote_stop
ocpp/{station_id}/{device_id}/cmd/reset
```

## 🔌 Hardware Integration

### STM32 Communication

- **Protocol**: Custom UART packet protocol
- **Baud Rate**: 115200
- **Pins**: GPIO1 (TX), GPIO3 (RX)
- **Features**: Checksum validation, sequence numbers, retry logic

### Supported Commands

- `CMD_MQTT_PUBLISH` - Publish MQTT message
- `CMD_GET_TIME` - Request current time
- `CMD_WIFI_STATUS` - Request WiFi status
- `RSP_MQTT_ACK` - MQTT publish acknowledgment
- `RSP_TIME_DATA` - Time response
- `RSP_WIFI_STATUS` - WiFi status response

## 🐛 Debugging

### Serial Monitor

```bash
pio device monitor
# or
./tools/flash_esp8266.sh monitor
```

### Debug Output

Enable debug output by setting `debug_enabled: true` in configuration.

### Common Issues

1. **WiFi Connection Failed**: Check SSID/password, signal strength
2. **MQTT Connection Failed**: Verify broker address, credentials
3. **Web Interface Not Loading**: Ensure filesystem was flashed
4. **STM32 Communication Failed**: Check UART connections, baud rate

## 🔄 OTA Updates

### Web-based OTA

1. Access web interface
2. Navigate to System tab
3. Upload new firmware file

### Command-line OTA

```bash
./tools/flash_esp8266.sh ota
# Enter device IP and OTA password
```

### Programmatic OTA

```bash
curl -X POST http://device-ip/api/ota \
  -H "Content-Type: application/octet-stream" \
  --data-binary @firmware.bin
```

## 📈 Performance

### Memory Usage

- **Flash**: ~400KB (firmware) + ~100KB (filesystem)
- **RAM**: ~25KB (runtime)
- **Free Heap**: ~50KB available

### Network Performance

- **WiFi**: 802.11 b/g/n support
- **MQTT**: 1KB message buffer, QoS 0/1 support
- **HTTP**: Async web server, 4 concurrent connections

## 🔒 Security

### Features

- **WPA2/WPA3**: WiFi encryption support
- **MQTT TLS**: Encrypted MQTT communication
- **Web Authentication**: Basic auth for web interface
- **OTA Security**: Password-protected updates

### Best Practices

1. Change default passwords
2. Enable MQTT TLS in production
3. Use strong WiFi passwords
4. Regular firmware updates
5. Monitor system logs

## 🧪 Testing

### Unit Tests

```bash
pio test
```

### Integration Tests

```bash
# Test WiFi connectivity
./tools/flash_esp8266.sh test

# Test MQTT connection
# Use web interface MQTT test feature
```

### Hardware-in-Loop Tests

1. Flash firmware to device
2. Connect to test WiFi network
3. Configure MQTT broker
4. Verify STM32 communication
5. Test web interface functionality

## 📚 Documentation

- [Build Guide](../docs/build_guide.md) - Detailed build instructions
- [UART Protocol](../docs/uart_protocol.md) - STM32 communication protocol
- [Testing Guide](../docs/testing_guide.md) - Comprehensive testing procedures
- [API Reference](../docs/api_reference.md) - Web API documentation

## 🤝 Contributing

1. Fork the repository
2. Create feature branch (`git checkout -b feature/amazing-feature`)
3. Commit changes (`git commit -m 'Add amazing feature'`)
4. Push to branch (`git push origin feature/amazing-feature`)
5. Open Pull Request

## 📄 License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## 🆘 Support

For support and questions:

- Create an issue in the repository
- Check existing documentation
- Review debug logs
- Test with minimal configuration

---

**Version**: 1.0.0  
**Last Updated**: August 2025  
**Compatibility**: ESP8266, Arduino Framework, PlatformIO
