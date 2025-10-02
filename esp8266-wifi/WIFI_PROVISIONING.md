# WiFi Provisioning Guide

## Overview

ESP8266 WiFi Module hỗ trợ **WiFi Provisioning** qua **Captive Portal** khi chưa được cấu hình hoặc không kết nối được.

---

## First Boot Flow (Chưa Provisioning)

### 1. Khởi động lần đầu

```
[Main] ESP8266 SolEVC Charging Point Controller WiFi Module v3.0
[Main] Chip ID: 0x12345678
[Main] Initializing device manager...

[Config] Loading configuration from /config.json
[Config] ⚠️  WiFi SSID empty - not provisioned
[Config] Device: DEVICE-001, Station: STATION-01

[STM32] UART initialized @ 115200 baud

[WiFi] Initialized
[WiFi] ⚠️  Not configured - starting config portal
[WiFi] Starting AP: SolEVC Charging Point Controller-12345678
```

### 2. Access Point được tạo

**SSID:** `SolEVC Charging Point Controller-{ChipID}` (ví dụ: `SolEVC Charging Point Controller-12345678`)
**Password:** Không có (open network)
**IP Gateway:** `192.168.4.1`

### 3. User kết nối vào AP

```
📱 Smartphone/Laptop
  └─> Scan WiFi
      └─> Connect to "SolEVC Charging Point Controller-12345678"
          └─> Captive Portal tự động mở
              (hoặc truy cập http://192.168.4.1)
```

### 4. Web Config Portal

```html
╔══════════════════════════════════════╗
║   WiFi Configuration Portal          ║
╠══════════════════════════════════════╣
║                                      ║
║  Select your WiFi network:           ║
║  ○ HomeWiFi          [-75 dBm]      ║
║  ○ OfficeNetwork     [-60 dBm]      ║
║  ○ Cafe_Free         [-80 dBm]      ║
║                                      ║
║  Password: [_____________]           ║
║                                      ║
║  [Save] [Exit]                       ║
║                                      ║
╚══════════════════════════════════════╝
```

### 5. Sau khi Save

```
[WiFi] Configuration saved
[WiFi] Connecting to: HomeWiFi
[WiFi] ................
[WiFi] ✓ Connected!
[WiFi] IP: 192.168.1.100

[MQTT] Connecting to broker: mqtt.example.com:1883
[MQTT] ✓ Connected successfully

[NTP] Synchronizing time...
[NTP] ✓ Time synced: 2025-10-02 14:30:00

[Main] ✓ All components initialized
```

---

## Provisioning Flow Diagram

```
┌─────────────────────────────────────────────────────┐
│              Device Boot                            │
└──────────────────┬──────────────────────────────────┘
                   │
                   ▼
         ┌─────────────────────┐
         │ Load config.json    │
         └──────────┬──────────┘
                    │
                    ▼
         ┌──────────────────────┐
         │ Check SSID in config │
         └──────────┬───────────┘
                    │
        ┌───────────┴────────────┐
        │                        │
   SSID empty?            SSID exists?
        │                        │
        ▼                        ▼
┌───────────────┐       ┌────────────────┐
│ Start AP Mode │       │ Try Connect    │
│ SolEVC Charging Point Controller-XXXXXX   │       │ to saved WiFi  │
└───────┬───────┘       └────────┬───────┘
        │                        │
        │                   ┌────┴─────┐
        │                   │          │
        │              Connected?   Failed?
        │                   │          │
        │                   ▼          ▼
        │           ┌──────────┐  ┌──────────┐
        │           │ Success  │  │ Start AP │
        │           │ Continue │  │ Mode     │
        │           └──────────┘  └────┬─────┘
        │                              │
        └──────────────┬───────────────┘
                       │
                       ▼
          ┌─────────────────────────┐
          │ Show Config Portal      │
          │ http://192.168.4.1      │
          └─────────────┬───────────┘
                        │
                   User configures
                        │
                        ▼
          ┌─────────────────────────┐
          │ Save credentials        │
          │ to config.json          │
          └─────────────┬───────────┘
                        │
                        ▼
          ┌─────────────────────────┐
          │ Restart WiFi            │
          │ Connect with new SSID   │
          └─────────────┬───────────┘
                        │
                        ▼
               ┌────────────────┐
               │ Success        │
               │ → MQTT Connect │
               │ → NTP Sync     │
               └────────────────┘
```

---

## Configuration Storage

### Config File Location

Path: `/config.json` (LittleFS filesystem)

### Config File Format

```json
{
  "deviceId": "DEVICE-001",
  "stationId": "STATION-01",
  "wifi": {
    "ssid": "",                    ← Empty on first boot
    "password": "",
    "autoConnect": true,
    "configPortalTimeout": 180
  },
  "mqtt": {
    "broker": "mqtt.example.com",
    "port": 1883,
    "username": "",
    "password": "",
    "tlsEnabled": false,
    "keepAlive": 60
  },
  "system": {
    "heartbeatInterval": 30000,
    "logLevel": 2
  }
}
```

### After Provisioning

```json
{
  "wifi": {
    "ssid": "HomeWiFi",            ← Filled by user
    "password": "MyPassword123",   ← Filled by user
    "autoConnect": true,
    "configPortalTimeout": 180
  }
}
```

---

## Device Manager Logic

### Code Flow (device_manager.cpp)

```cpp
bool DeviceManager::initializeNetwork() {
    // 1. Initialize WiFi driver
    wifiManager = new WiFiManager(config);
    wifiManager->init();

    // 2. Check if provisioned
    if (strlen(config.wifi.ssid) == 0) {
        // NOT PROVISIONED
        LOG_WARN("WiFi", "Not configured - starting config portal");
        wifiManager->startConfigPortal();

        // After config, try to connect
        WiFiError err = wifiManager->connect();
        if (err != WiFiError::SUCCESS) {
            LOG_ERROR("WiFi", "Connection failed after provisioning");
            return false;
        }
    } else {
        // ALREADY PROVISIONED
        LOG_INFO("WiFi", "Connecting to saved network: %s", config.wifi.ssid);
        WiFiError err = wifiManager->connect();

        if (err != WiFiError::SUCCESS) {
            // Connection failed - maybe password changed
            LOG_WARN("WiFi", "Connection failed, starting config portal");
            wifiManager->startConfigPortal();
            return false;
        }
    }

    // 3. Continue with MQTT and NTP
    // ...
}
```

---

## WiFi Manager Implementation

### startConfigPortal() Logic

```cpp
WiFiError WiFiManager::startConfigPortal() {
    // Build AP name: SolEVC Charging Point Controller-{ChipID}
    char apName[32];
    sprintf(apName, "SolEVC Charging Point Controller-%08X", ESP.getChipId());

    LOG_INFO("WiFi", "Starting AP: %s", apName);
    LOG_INFO("WiFi", "Connect to AP and open http://192.168.4.1");

    // Start WiFiManager captive portal
    if (wifiManagerLib.startConfigPortal(apName)) {
        // User configured and connected
        status.apMode = false;
        updateStatus();
        return WiFiError::SUCCESS;
    }

    // Timeout (default 180 seconds)
    return WiFiError::TIMEOUT;
}
```

**Timeout:** 180 giây (3 phút)
- Nếu user không config trong 3 phút → timeout
- Device sẽ reboot hoặc retry

---

## User Experience

### Scenario 1: First Time Setup (New Device)

1. **Power on device**
   ```
   LED: Blinking (searching for WiFi)
   Serial: "Not configured - starting AP"
   ```

2. **Phone notification**
   ```
   📱 "New WiFi network detected: SolEVC Charging Point Controller-12345678"
   ```

3. **Connect to AP**
   ```
   📱 Tap "Connect"
   📱 Captive portal opens automatically
   ```

4. **Select WiFi + Enter password**
   ```
   📱 Select "HomeWiFi"
   📱 Enter password: ••••••••
   📱 Tap "Save"
   ```

5. **Device connects**
   ```
   LED: Solid (connected)
   Serial: "Connected to HomeWiFi"
   Serial: "IP: 192.168.1.100"
   ```

### Scenario 2: WiFi Password Changed

1. **Device can't connect**
   ```
   LED: Blinking fast (failed to connect)
   Serial: "Connection failed after 20 attempts"
   Serial: "Starting config portal"
   ```

2. **AP appears again**
   ```
   📱 "SolEVC Charging Point Controller-12345678" appears
   ```

3. **Reconfigure**
   ```
   📱 Connect → Enter new password → Save
   ```

### Scenario 3: Moving to New Location

1. **Manual reset** (via button or command)
   ```
   Hold reset button for 5 seconds
   → Clears WiFi credentials
   → Starts AP mode
   ```

2. **Or via Serial command**
   ```
   > reset_wifi
   [WiFi] Credentials cleared
   [WiFi] Rebooting...
   ```

---

## Advanced Configuration

### Config Portal Customization

```cpp
// In wifi_manager.cpp
wifiManagerLib.setConfigPortalTimeout(180);  // 3 minutes
wifiManagerLib.setAPCallback([](WiFiManager* mgr) {
    LOG_INFO("WiFi", "Config portal started");
    LOG_INFO("WiFi", "SSID: %s", mgr->getConfigPortalSSID().c_str());
});
```

### Custom AP Name

```cpp
// Build from deviceId instead of ChipID
char apName[32];
sprintf(apName, "SolEVC Charging Point Controller-%s", config.deviceId);
```

### Static IP Configuration

```json
{
  "wifi": {
    "ssid": "HomeWiFi",
    "password": "password",
    "staticIp": "192.168.1.100",
    "gateway": "192.168.1.1",
    "subnet": "255.255.255.0",
    "dns": "8.8.8.8"
  }
}
```

---

## Troubleshooting

### Problem 1: AP doesn't appear

**Symptoms:**
- Device boots
- Serial says "Starting AP"
- But no SSID visible on phone

**Solutions:**
1. Check 2.4GHz WiFi on phone (ESP8266 doesn't support 5GHz)
2. Restart phone WiFi
3. Check ESP8266 antenna connection
4. Try manual URL: `http://192.168.4.1`

### Problem 2: Captive portal doesn't open

**Symptoms:**
- Connected to AP
- No popup appears

**Solutions:**
1. Manually open browser: `http://192.168.4.1`
2. Disable cellular data on phone
3. Try different browser
4. Check phone captive portal settings

### Problem 3: Config saves but won't connect

**Symptoms:**
- User enters credentials
- "Saved" message appears
- Device still can't connect

**Solutions:**
1. Check WiFi password (case-sensitive!)
2. Check WiFi network is 2.4GHz
3. Check WiFi router MAC filtering
4. Check DHCP enabled on router
5. Serial debug: Enable WiFi debug logs

### Problem 4: Timeout in config portal

**Symptoms:**
- Portal opens
- After 3 minutes, device reboots
- AP disappears

**Solutions:**
1. Configure faster (within 3 minutes)
2. Increase timeout in code:
   ```cpp
   wifiManagerLib.setConfigPortalTimeout(300); // 5 minutes
   ```

---

## Production Recommendations

### 1. Factory Reset Button

Add physical button for WiFi reset:

```cpp
// In main.cpp setup()
pinMode(RESET_BUTTON_PIN, INPUT_PULLUP);

// In main.cpp loop()
if (digitalRead(RESET_BUTTON_PIN) == LOW) {
    static uint32_t pressTime = 0;
    if (pressTime == 0) pressTime = millis();

    if (millis() - pressTime > 5000) {
        // Held for 5 seconds
        LOG_INFO("Reset", "Factory reset triggered");
        wifiManager->clearCredentials();
        ESP.restart();
    }
} else {
    pressTime = 0;
}
```

### 2. LED Indicators

```cpp
// WiFi Status LEDs
void updateLED() {
    if (wifiManager->isConnected()) {
        digitalWrite(LED_PIN, HIGH);  // Solid = Connected
    } else if (wifiManager->isAPMode()) {
        blinkLED(500);                // Slow blink = AP mode
    } else {
        blinkLED(100);                // Fast blink = Connecting
    }
}
```

### 3. Web Portal Branding

Customize config portal HTML:

```cpp
wifiManagerLib.setTitle("SolEVC Charging Point Controller Charger Setup");
wifiManagerLib.setWelcomeMessage("Configure your WiFi");
```

### 4. Pre-provisioning (Factory)

Flash devices with pre-configured WiFi at factory:

```json
{
  "wifi": {
    "ssid": "FACTORY-NETWORK",
    "password": "factory123"
  }
}
```

End users can reconfigure via portal.

---

## Testing Checklist

- [ ] First boot without config → AP starts
- [ ] Connect to AP → Portal opens
- [ ] Configure WiFi → Saves and connects
- [ ] Power cycle → Reconnects automatically
- [ ] Wrong password → Portal reopens
- [ ] Timeout test → Device handles gracefully
- [ ] Multiple devices → Unique AP names
- [ ] WiFi changes → Reconfiguration works
- [ ] Signal strength → Connects at -80 dBm
- [ ] DHCP vs Static IP → Both modes work

---

## Summary

✅ **Auto-provisioning:** Không cần cable/serial để config WiFi
✅ **User-friendly:** Captive portal tự động mở trên phone
✅ **Robust:** Xử lý mọi trường hợp lỗi
✅ **Production-ready:** LED status, factory reset, branding

**Flow tóm tắt:**
1. **Chưa config** → Phát AP `SolEVC Charging Point Controller-XXXXXX`
2. **User connect** → Portal mở tự động
3. **Chọn WiFi + password** → Lưu vào config.json
4. **Reboot** → Kết nối WiFi → MQTT → NTP → Sẵn sàng!
