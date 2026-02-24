# Particle Argon DSA Signature Testing - Adaptation Guide

## Overview

This project adapts an ESP32-based DSA (Digital Signature Algorithm) cryptographic testing suite to run on the **Particle Argon** development board, which uses the Nordic nRF52840 SoC as its primary MCU.

## Hardware

### Particle Argon Specifications

- **Primary MCU**: Nordic nRF52840
- **Coprocessor**: ESP32 (Wi-Fi/NCP, internally managed)
- **OS**: Particle Device OS (C++ Wiring framework)
- **Connectivity**: Automatic cloud connection via Particle's infrastructure
- **Battery**: LiPo supported (on-board charging)
- **GPIO Pins**: D0-D8, A0-A5
- **Built-in RGB LED**: Use `RGB` object
- **BLE**: Native support via nRF52840

### Key Differences from ESP32

| Feature | ESP32 | Argon (nRF52840) |
|---------|-------|------------------|
| **WiFi Management** | Manual setup via `esp_wifi` | Automatic via Particle cloud |
| **Communication** | MQTT (custom setup) | Particle Pub/Sub (built-in) |
| **RTOS** | FreeRTOS (explicit) | Particle OS (transparent) |
| **Device OS** | ESP-IDF | Particle Device OS |
| **Power Management** | Manual | Particle system managed |

## Code Adaptations

### 1. **Connectivity Model**

**ESP32 (Original)**:
```c
// Manual WiFi setup
esp_wifi_init(&cfg);
esp_wifi_start();
esp_wifi_connect();
```

**Argon (Adapted)**:
```cpp
// Automatic cloud connection - Particle handles WiFi internally
// Device connects to Particle cloud automatically
// No manual WiFi configuration needed
```

**Key Points**:
- Particle Argon connects to the Particle cloud automatically
- No need to manage WiFi SSID/password (configured via Particle app)
- Cloud connectivity is always-on unless explicitly disabled

### 2. **Message Transport**

**ESP32 (Original)**:
```c
// MQTT with explicit topic management
esp_mqtt_client_publish(client, SENDING_TOPIC, data, length, 0, 0);
```

**Argon (Adapted)**:
```cpp
// Particle Pub/Sub - native event system
Particle.publish(event_name, (const char*)data, length, PUBLIC);
// Subscription for receiving
Particle.subscribe(EVENT_NAME, on_peer_message);
```

**Protocol**:
- **Alice** publishes to: `dsa/alice_to_bob`
- **Bob** publishes to: `dsa/bob_to_alice`
- Each device subscribes to the other's channel
- Both can be realized on two Argon devices OR one Argon paired with another platform

### 3. **Task Management**

**ESP32 (Original)**:
```c
// FreeRTOS tasks
xTaskCreatePinnedToCore(&receive_task, "receive_task", 2048, NULL, 5, NULL, 1);
xTaskCreatePinnedToCore(&task_test_all_dsa, "test_task", 130000, NULL, 1, NULL, 0);
```

**Argon (Adapted)**:
```cpp
// Single loop() function - Particle OS handles threading
void loop() {
    // Main application logic
    // Non-blocking delays to keep system responsive
}
```

**Key Points**:
- Argon's `loop()` runs in the application thread
- System services (cloud, BLE) run in background
- Use `delay()` for non-blocking operation (yields to system)
- No explicit task creation needed for simple scenarios

### 4. **Logging**

**ESP32 (Original)**:
```c
printf("Message\n");
```

**Argon (Adapted)**:
```cpp
Log.info("Message");
Log.warn("Warning");
Log.error("Error");
```

**Advantages**:
- Automatic timestamping
- Log levels (DEBUG, INFO, WARN, ERROR)
- Viewable in Particle console
- Serial output also available via `Serial.println()`

### 5. **LED Feedback**

**ESP32**: External LED via GPIO

**Argon**: Built-in RGB LED
```cpp
RGB.color(255, 0, 0);    // Red
RGB.color(0, 255, 0);    // Green
RGB.color(0, 0, 255);    // Blue
RGB.color(255, 255, 0);  // Yellow (waiting)
```

## File Structure

```
particle-argon/
├── particle_argon.ino          # Main Particle application (setup/loop)
├── project.properties          # Particle project metadata
├── README_ADAPTATION.md        # This file
└── [DSA components inherit from ESP32 build]
```

## Compilation for Particle Argon

### Prerequisites

1. **Particle CLI** or Web IDE (https://build.particle.io)
2. **Device Mode**: Argon set to normal (not DFU) mode
3. **Credentials**: Particle account and device claimed to account

### Option A: Using Particle Web IDE

1. Navigate to [build.particle.io](https://build.particle.io)
2. Create new project
3. Copy content from `particle_argon.ino`
4. Ensure target device is "Argon"
5. Click "Flash" or "Verify"

### Option B: Using Particle CLI

```bash
# Install CLI
npm install -g particle-cli

# Login
particle login

# Select device
particle list

# Flash application
particle flash <device_id> particle_argon.ino

# Monitor logs
particle serial monitor
```

### Option C: Using VS Code + Particle Extension

1. Install "Particle for VS Code" extension
2. Open project folder
3. Command palette: "Particle: Flash"

## Protocol & Synchronization

### Alice-Bob Handshake

Both devices implement the same protocol with role-aware behavior:

```
ALICE                              BOB
  |                                 |
  |--- wait for "ready" ---------->|
  |                           send "ready" → ✓
  |                           (retry every 2s)
  |<-- "ready" received ------------|
  send "ack" ──────────────────────>|
  |                                 |
  |                           "ack" received
  |                           (stop retrying)
  └─────── Synchronized ──────────>┘
```

### Test Sequence (per algorithm)

1. **Alice**: Generate key pair → Send public key
2. **Bob**: Receive and store public key
3. **Alice**: Sign test message → Send signed message
4. **Bob**: Verify signature → Extract message → Echo message
5. **Alice**: Compare echo with original → Pass/Fail

## GPIO Pin Mapping (IF Needed)

Particle Argon provides standard GPIO:

| Pin | Function | Notes |
|-----|----------|-------|
| D0-D8 | Digital I/O | 3.3V logic |
| A0-A5 | Analog Input | 12-bit ADC |
| TX/RX | Serial (built-in) | Particle console |

**Example**: Reading a button
```cpp
#define BUTTON_PIN D0

pinMode(BUTTON_PIN, INPUT_PULLUP);
int button_state = digitalRead(BUTTON_PIN);
```

## Connectivity Features

### Particle Device OS Features Available

- **OTA Updates**: Firmware updates via Particle cloud
- **Particle Functions**: Remote control via `Particle.function()`
- **Particle Variables**: Expose data via `Particle.variable()`
- **Particle Publish/Subscribe**: Device-to-cloud-to-device messaging
- **BLE**: Via nRF52840 native BLE
- **Sleep Modes**: `System.sleep()` for low power

### Example: Remote Test Trigger

```cpp
int trigger_tests(String cmd) {
    if (!test_in_progress) {
        test_complete = false;
        return 1;
    }
    return 0;
}

STARTUP(
    Particle.function("test_trigger", trigger_tests);
)
```

Trigger via:
- Web: https://api.particle.io/v1/devices/{DEVICE_ID}/test_trigger
- CLI: `particle call <device_id> test_trigger`
- Phone app: Particle Mobile App

## Performance Considerations

### Memory

- **Argon RAM**: 256 KB (vs ESP32: 520 KB)
- **DSA operations**: Allocate/free immediately (see `crypto_sign_message()`)
- **Monitor**: Use `System.freeMemory()` in debug

### Timing

- **Crypto operations**: May take several seconds per algorithm
- **Cloud publish**: ~100-500ms latency
- **Timeouts**: Set to 30-60 seconds for robustness

### Power

- Standard use: ~80mA (with WiFi/cloud)
- Low power sleep: ~50µA (with `System.sleep()`)

## Debugging

### Enable Debug Logging

```cpp
LogLevel(LOG_LEVEL_DEBUG);  // Verbose logs
Serial.begin(115200);        // Serial monitor
```

### View Logs

**Via Particle Console**:
- https://console.particle.io → Select Device → Logs

**Via Serial**:
```bash
particle serial monitor
```

### Common Issues

| Issue | Solution |
|-------|----------|
| **Cloud not connecting** | Check WiFi (Argon acts as station). Ensure credentials set. Restart device. |
| **Memory errors** | Reduce DSA key sizes or test fewer algorithms. Free memory after each test. |
| **Message timeout** | Increase timeout in sync function. Check peer device is running. |
| **Compile error: `dsa.h` not found** | Ensure DSA components are linked properly in build system. |

## System Requirements

- **Particle Argon** × 1-2 (depending on test configuration)
- **Power**: USB-C (development) or LiPo battery
- **Cloud Access**: Particle account with active device
- **Network**: WiFi with internet access

## Security Considerations

### Particle Pub/Sub Security

- Events are by default **PUBLIC** on Particle cloud
- For sensitive data: Use Particle's **private events** (authenticated subscribers only)
- Recommendation: Set to **PRIVATE** for production

```cpp
Particle.publish(event_name, data, length, PRIVATE);  // Requires auth
```

### Post-Quantum Cryptography

This implementation tests:
- **Falcon**: NIST PQC finalist (signature)
- **ML-DSA**: NIST PQC standard (signature)
- **SPHINCS+**: NIST PQC finalist (hash-based signature)

All are resistant to quantum computer attacks.

## Next Steps

1. **Flash device**: Use Particle CLI or Web IDE
2. **Monitor logs**: Via Particle console or `particle serial monitor`
3. **Verify communication**: Both Alice and Bob should show synchronized status
4. **Review test results**: Logs show "PASS/FAIL" for each algorithm
5. **Integrate into IoT system**: Use Particle webhooks for cloud integration

## References

- [Particle Argon Documentation](https://docs.particle.io/reference/hardware/development/argon/)
- [Particle Device OS API](https://docs.particle.io/reference/device-os/api/cloud-functions/particle-publish/)
- [NIST Post-Quantum Cryptography](https://csrc.nist.gov/projects/post-quantum-cryptography/)
- [Project Source (ESP32)](../../)

---

**Author**: Adapted for Particle Argon (nRF52840)  
**Date**: 2026  
**Status**: Ready for deployment on Particle Argon boards
