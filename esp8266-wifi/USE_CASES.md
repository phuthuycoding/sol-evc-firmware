# Use Cases Documentation

**ESP8266 WiFi Module for SolEVC Charging Point Controller**
**Version:** 3.0.0
**Status:** ✅ All use cases fully implemented

---

## Overview

Document này mô tả đầy đủ tất cả use cases được implement trong ESP8266 firmware.

---

## Table of Contents

1. [STM32 → ESP8266 Commands](#1-stm32--esp8266-commands)
2. [Cloud → ESP8266 → STM32](#2-cloud--esp8266--stm32)
3. [ESP8266 → Cloud (MQTT)](#3-esp8266--cloud-mqtt)
4. [Internal Use Cases](#4-internal-use-cases)

---

## 1. STM32 → ESP8266 Commands

### UC-01: CMD_MQTT_PUBLISH (0x01)

**Description:** STM32 yêu cầu ESP8266 publish một OCPP message lên cloud qua MQTT

**Flow:**
```
STM32 ──[UART]──> ESP8266
    CMD_MQTT_PUBLISH
    Payload: {
        "topic": "ocpp/station/device/status/1/notification",
        "data": "{json_payload}"
    }

ESP8266 Actions:
    1. Parse JSON từ UART packet
    2. Extract topic và data
    3. Publish to MQTT broker
    4. Send ACK to STM32

STM32 <──[UART]── ESP8266
    RSP_MQTT_ACK (STATUS_SUCCESS / STATUS_ERROR)
```

**Handler:** `STM32CommandHandler::handleMqttPublish()`

**Code:** `/Users/mindx/Work/Personal/sol-evc-firmware/esp8266-wifi/src/handlers/stm32_command_handler.cpp:53`

**Example:**
```cpp
// STM32 code
uart_packet_t packet;
uart_init_packet(&packet, CMD_MQTT_PUBLISH, seq++);

StaticJsonDocument<256> doc;
doc["topic"] = "ocpp/STATION-01/DEVICE-001/status/1/notification";
doc["data"] = "{\"status\":\"Available\"}";

serializeJson(doc, (char*)packet.payload);
packet.length = strlen((char*)packet.payload);

stm32_send_packet(&packet);
```

---

### UC-02: CMD_GET_TIME (0x02)

**Description:** STM32 request thời gian hiện tại từ ESP8266 (NTP-synced)

**Flow:**
```
STM32 ──[UART]──> ESP8266
    CMD_GET_TIME
    Payload: empty

ESP8266 Actions:
    1. Get current time from NTPTimeDriver
    2. Build time_data_payload_t
    3. Send response to STM32

STM32 <──[UART]── ESP8266
    RSP_TIME_DATA
    Payload: time_data_payload_t {
        unix_timestamp: uint32_t
        timezone_offset: int16_t (minutes)
        ntp_synced: uint8_t (0/1)
    }
```

**Handler:** `STM32CommandHandler::handleGetTime()`

**Code:** `/Users/mindx/Work/Personal/sol-evc-firmware/esp8266-wifi/src/handlers/stm32_command_handler.cpp:91`

**Features:**
- ✅ NTP synchronized time
- ✅ Timezone offset support
- ✅ Sync status indicator
- ✅ Fallback to millis() if not synced

**Example:**
```cpp
// STM32 code
uart_packet_t packet;
uart_init_packet(&packet, CMD_GET_TIME, seq++);
packet.length = 0;
stm32_send_packet(&packet);

// Wait for response
uart_packet_t response;
if (uart_receive(&response, 1000)) {
    time_data_payload_t* time = (time_data_payload_t*)response.payload;
    printf("Time: %u, Synced: %d\n", time->unix_timestamp, time->ntp_synced);
}
```

---

### UC-03: CMD_WIFI_STATUS (0x03)

**Description:** STM32 request WiFi connection status từ ESP8266

**Flow:**
```
STM32 ──[UART]──> ESP8266
    CMD_WIFI_STATUS
    Payload: empty

ESP8266 Actions:
    1. Get WiFi status
    2. Build wifi_status_payload_t
    3. Send response

STM32 <──[UART]── ESP8266
    RSP_WIFI_STATUS
    Payload: wifi_status_payload_t {
        wifi_connected: uint8_t (0/1)
        mqtt_connected: uint8_t (0/1)
        rssi: int8_t (dBm)
        uptime: uint32_t (seconds)
        ip_address: uint8_t[4]
    }
```

**Handler:** `STM32CommandHandler::handleWiFiStatus()`

**Code:** `/Users/mindx/Work/Personal/sol-evc-firmware/esp8266-wifi/src/handlers/stm32_command_handler.cpp:114`

**Use Case:**
- STM32 hiển thị WiFi status trên LCD
- Monitor connection quality (RSSI)
- Track uptime
- Show IP address

---

### UC-04: CMD_CONFIG_UPDATE (0x04)

**Description:** STM32 yêu cầu ESP8266 update runtime configuration

**Flow:**
```
STM32 ──[UART]──> ESP8266
    CMD_CONFIG_UPDATE
    Payload: JSON config string

ESP8266 Actions:
    1. Parse JSON config
    2. Validate parameters
    3. Update config in memory
    4. Save to config.json (LittleFS)
    5. Apply changes (if needed)
    6. Send ACK

STM32 <──[UART]── ESP8266
    RSP_CONFIG_ACK (STATUS_SUCCESS / STATUS_INVALID)
```

**Handler:** `ConfigUpdateHandler::handleFromSTM32()`

**Code:** `/Users/mindx/Work/Personal/sol-evc-firmware/esp8266-wifi/src/handlers/config_update_handler.cpp`

**Supported Config Updates:**
- MQTT broker address/port
- Heartbeat interval
- Log level
- Device ID / Station ID

**Example:**
```cpp
// STM32 code
uart_packet_t packet;
uart_init_packet(&packet, CMD_CONFIG_UPDATE, seq++);

const char* configJson = "{\"mqtt\":{\"broker\":\"new-broker.com\"}}";
strcpy((char*)packet.payload, configJson);
packet.length = strlen(configJson);

stm32_send_packet(&packet);
```

---

### UC-05: CMD_OTA_REQUEST (0x05)

**Description:** STM32 yêu cầu ESP8266 thực hiện OTA firmware update

**Flow:**
```
STM32 ──[UART]──> ESP8266
    CMD_OTA_REQUEST
    Payload: Firmware URL (string)

ESP8266 Actions:
    1. Validate URL
    2. Download firmware from HTTP
    3. Verify size and checksum
    4. Flash new firmware
    5. Send status (if not rebooting)
    6. Reboot (if success)

STM32 <──[UART]── ESP8266
    RSP_OTA_STATUS
    Payload: {
        status: uint8_t (OTAResult enum)
        message: char[64]
    }
```

**Handler:** `OTAHandler::handleFromSTM32()`

**Code:** `/Users/mindx/Work/Personal/sol-evc-firmware/esp8266-wifi/src/handlers/ota_handler.cpp:90`

**OTA Process:**
1. Check free sketch space (need > 100KB)
2. Download firmware via ESP8266HTTPClient
3. Verify downloaded file
4. Flash to OTA partition
5. Reboot automatically

**Status Codes:**
- `SUCCESS` (0) - Update successful, rebooting
- `FAILED_HTTP` - HTTP download failed
- `FAILED_NO_SPACE` - Insufficient flash space
- `FAILED_FLASH` - Flash write error
- `FAILED_VERIFY` - Checksum mismatch
- `FAILED_INVALID_URL` - Invalid URL format

**Example:**
```cpp
// STM32 code
uart_packet_t packet;
uart_init_packet(&packet, CMD_OTA_REQUEST, seq++);

const char* url = "http://example.com/firmware.bin";
strcpy((char*)packet.payload, url);
packet.length = strlen(url);

stm32_send_packet(&packet);

// Wait for status (or device will reboot if successful)
```

---

### UC-06: CMD_PUBLISH_METER_VALUES (0x06)

**Description:** STM32 gửi meter values để ESP8266 publish lên MQTT

**Flow:**
```
STM32 ──[UART]──> ESP8266
    CMD_PUBLISH_METER_VALUES
    Payload: meter_values_t {
        msg_id: char[32]
        connector_id: uint8_t
        transaction_id: uint32_t
        sample: {
            energy_wh: uint32_t
            voltage_mv: uint32_t
            current_ma: uint32_t
            power_w: uint32_t
            temperature_c: int16_t
            timestamp: uint32_t
        }
    }

ESP8266 Actions:
    1. Parse meter_values_t from packet
    2. Build OCPP MeterValues JSON
    3. Publish to MQTT topic
    4. Send ACK to STM32

STM32 <──[UART]── ESP8266
    ACK (STATUS_SUCCESS / STATUS_ERROR)
```

**Handler:** `STM32CommandHandler::handlePublishMeterValues()` → `OCPPMessageHandler::publishMeterValues()`

**Code:** `/Users/mindx/Work/Personal/sol-evc-firmware/esp8266-wifi/src/handlers/stm32_command_handler.cpp:168`

**MQTT Topic Format:**
```
ocpp/{stationId}/{deviceId}/meter/{connector}/meter_values
```

**MQTT Payload Format:**
```json
{
  "msgId": "METER-001",
  "timestamp": 1234567890,
  "connectorId": 1,
  "transactionId": 100,
  "sample": {
    "energy_wh": 5000,
    "voltage_mv": 230000,
    "current_ma": 16000,
    "power_w": 3680,
    "temperature_c": 25,
    "timestamp": 1234567890
  }
}
```

**Example:**
```cpp
// STM32 code
uart_packet_t packet;
uart_init_packet(&packet, CMD_PUBLISH_METER_VALUES, seq++);

meter_values_t meter;
strcpy(meter.msg_id, "METER-001");
meter.connector_id = 1;
meter.transaction_id = 100;
meter.sample.energy_wh = 5000;
meter.sample.voltage_mv = 230000;
meter.sample.current_ma = 16000;
meter.sample.power_w = 3680;
meter.sample.temperature_c = 25;
meter.sample.timestamp = get_current_time();

memcpy(packet.payload, &meter, sizeof(meter_values_t));
packet.length = sizeof(meter_values_t);

stm32_send_packet(&packet);
```

---

## 2. Cloud → ESP8266 → STM32

### UC-07: RSP_MQTT_RECEIVED (0x85)

**Description:** Cloud gửi remote command qua MQTT, ESP8266 forward to STM32

**Flow:**
```
Cloud ──[MQTT]──> ESP8266
    Topic: ocpp/{station}/{device}/cmd/{command}
    Payload: JSON command

ESP8266 Actions:
    1. MQTT callback triggered
    2. Filter by device ID
    3. Parse command
    4. Build UART packet
    5. Forward to STM32

STM32 <──[UART]── ESP8266
    RSP_MQTT_RECEIVED
    Payload: JSON command string
```

**Handler:** `MQTTIncomingHandler::execute()`

**Code:** `/Users/mindx/Work/Personal/sol-evc-firmware/esp8266-wifi/src/handlers/mqtt_incoming_handler.cpp`

**Supported Commands:**
- `RemoteStartTransaction`
- `RemoteStopTransaction`
- `ChangeConfiguration`
- `Reset`
- `UnlockConnector`

**Example Flow:**
```
1. Cloud publishes:
   Topic: ocpp/STATION-01/DEVICE-001/cmd/RemoteStartTransaction
   Payload: {"connectorId": 1, "idTag": "USER-123"}

2. ESP8266 receives via MQTT callback

3. ESP8266 forwards to STM32:
   UART Packet:
   - cmd_type: RSP_MQTT_RECEIVED
   - payload: {"connectorId": 1, "idTag": "USER-123"}

4. STM32 processes command and starts charging
```

---

## 3. ESP8266 → Cloud (MQTT)

### UC-08: Boot Notification

**Description:** ESP8266 gửi boot notification khi khởi động

**Trigger:** Mỗi lần device boot và kết nối MQTT thành công

**Handler:** `OCPPMessageHandler::publishBootNotification()`

**Code:** `/Users/mindx/Work/Personal/sol-evc-firmware/esp8266-wifi/src/handlers/ocpp_message_handler.cpp:153`

**Topic:**
```
ocpp/{stationId}/{deviceId}/event/0/boot_notification
```

**Payload:**
```json
{
  "msgId": "BOOT-001",
  "timestamp": 1234567890,
  "firmwareVersion": "1.0.0",
  "stationId": "STATION-01",
  "bootReason": 1,
  "chargePointSerialNumber": "SN-12345"
}
```

**Boot Reasons:**
- 0: Unknown
- 1: PowerUp
- 2: Reset
- 3: Watchdog
- 4: SoftwareUpdate

---

### UC-09: Heartbeat

**Description:** ESP8266 gửi heartbeat định kỳ để báo hiệu device còn sống

**Trigger:** Mỗi 30 giây (configurable via `system.heartbeatInterval`)

**Handler:** `HeartbeatHandler::execute()`

**Code:** `/Users/mindx/Work/Personal/sol-evc-firmware/esp8266-wifi/src/handlers/heartbeat_handler.cpp`

**Topic:**
```
ocpp/{stationId}/{deviceId}/heartbeat
```

**Payload:**
```json
{
  "msgId": "1234567890",
  "uptime": 3600,
  "rssi": -45,
  "freeHeap": 42000,
  "heapFrag": 15
}
```

**Fields:**
- `msgId`: Timestamp (millis)
- `uptime`: Seconds since boot
- `rssi`: WiFi signal strength (dBm)
- `freeHeap`: Free heap memory (bytes)
- `heapFrag`: Heap fragmentation (%)

---

### UC-10: Status Notification

**Description:** Publish connector status changes

**Trigger:** STM32 gửi status update qua CMD_MQTT_PUBLISH

**Handler:** `OCPPMessageHandler::publishStatusNotification()`

**Topic:**
```
ocpp/{stationId}/{deviceId}/status/{connectorId}/status_notification
```

**Payload:**
```json
{
  "msgId": "STATUS-001",
  "timestamp": 1234567890,
  "connectorId": 1,
  "status": 1,
  "errorCode": 0,
  "info": "Available",
  "vendorId": "SOLEVC"
}
```

**Status Codes:**
- 0: Unavailable
- 1: Available
- 2: Preparing
- 3: Charging
- 4: Finishing
- 5: Faulted

---

### UC-11: Meter Values

**Description:** Publish periodic energy meter readings

**Trigger:** STM32 gửi qua CMD_PUBLISH_METER_VALUES

**Handler:** `OCPPMessageHandler::publishMeterValues()`

**Topic:**
```
ocpp/{stationId}/{deviceId}/meter/{connectorId}/meter_values
```

**Payload:** (See UC-06)

---

### UC-12: Start Transaction

**Description:** Publish transaction start event

**Handler:** `OCPPMessageHandler::publishStartTransaction()`

**Topic:**
```
ocpp/{stationId}/{deviceId}/transaction/start
```

**Payload:**
```json
{
  "msgId": "TX-START-001",
  "timestamp": 1234567890,
  "connectorId": 1,
  "idTag": "USER-123",
  "meterStart": 1000,
  "reservationId": 0
}
```

---

### UC-13: Stop Transaction

**Description:** Publish transaction stop event

**Handler:** `OCPPMessageHandler::publishStopTransaction()`

**Topic:**
```
ocpp/{stationId}/{deviceId}/transaction/stop
```

**Payload:**
```json
{
  "msgId": "TX-STOP-001",
  "timestamp": 1234567890,
  "transactionId": 100,
  "meterStop": 6000,
  "reason": 1,
  "idTag": "USER-123"
}
```

**Stop Reasons:**
- 0: EmergencyStop
- 1: Local
- 2: Remote
- 3: PowerLoss
- 4: Other

---

## 4. Internal Use Cases

### UC-14: WiFi Provisioning

**Description:** Auto-provisioning qua Captive Portal khi chưa có WiFi config

**Flow:** Xem [WIFI_PROVISIONING.md](./WIFI_PROVISIONING.md)

**Handler:** `WiFiManager::startConfigPortal()`

---

### UC-15: NTP Time Sync

**Description:** Tự động đồng bộ thời gian từ NTP server

**Trigger:**
- Mỗi lần kết nối WiFi
- Mỗi 1 giờ (auto re-sync)

**Handler:** `NTPTimeDriver::update()`

**Code:** `/Users/mindx/Work/Personal/sol-evc-firmware/esp8266-wifi/src/drivers/time/ntp_time.cpp`

**Server:** `pool.ntp.org` (configurable)

---

### UC-16: Auto-Reconnect

**WiFi Auto-Reconnect:**
- Handler: `WiFiManager::handle()`
- Interval: 30 seconds
- Max retries: Unlimited

**MQTT Auto-Reconnect:**
- Handler: `MQTTClient::handle()`
- Interval: Exponential backoff
- Max retries: Unlimited

---

## Summary

| Category | Use Cases | Status |
|----------|-----------|--------|
| STM32 → ESP8266 | 6 commands | ✅ 100% |
| Cloud → STM32 | 1 forward | ✅ 100% |
| ESP8266 → Cloud | 6 messages | ✅ 100% |
| Internal | 3 features | ✅ 100% |
| **Total** | **16 use cases** | **✅ 100%** |

---

**Version:** 3.0.0
**Last Updated:** 2025-10-02
**Status:** ✅ All use cases fully implemented - NO TODOs!
