# ESP32 → Particle Argon: Architecture & Library Adaptation Guide

## High-Level Architecture Comparison

### ESP32 Architecture (Original)

```
┌─────────────────────────────────────┐
│         Main Application            │
│  (app_main with FreeRTOS tasks)     │
├─────────────────────────────────────┤
│  FreeRTOS Scheduler                 │  Manages:
│  - Task 1: receive_task             │   • Task scheduling
│  - Task 2: test_task                │   • Queues
│  - System tasks                     │   • Timers
├─────────────────────────────────────┤
│  ESP32 Hardware Drivers             │
│  - WiFi: esp_wifi                   │
│  - MQTT: esp_mqtt_client            │
│  - NVS: esp_nvs_flash               │
├─────────────────────────────────────┤
│  Hardware (ESP32 SoC)               │
│  - WiFi peripheral                  │
│  - UART, GPIO, ADC, SPI/I2C         │
└─────────────────────────────────────┘
```

### Particle Argon Architecture (Adapted)

```
┌─────────────────────────────────────┐
│         Main Application            │
│  (setup() + loop())                 │
├─────────────────────────────────────┤
│  Particle Device OS                 │
│  - Cloud connectivity               │  Automatically manages:
│  - OTA updates                      │   • WiFi connection
│  - Pub/Sub messaging                │   • Cloud sync
│  - System threading                 │   • Power management
├─────────────────────────────────────┤
│  nRF52840 Drivers (primary MCU)    │
│  - BLE, GPIO, ADC, I2C, SPI, UART  │
├─────────────────────────────────────┤
│  Internal NCP Bridge                │
│  - ESP32 (WiFi only, managed)      │  The internal ESP32 handles
│  - No user code access              │  WiFi - transparent to app
├─────────────────────────────────────┤
│  Hardware Layers                    │
│  - nRF52840 MCU                     │
│  - Internal ESP32 NCP               │
│  - WiFi antenna & peripherals       │
└─────────────────────────────────────┘
```

## Library Mapping: ESP32 → Particle Argon

### WiFi & Networking

| ESP32 (IDF) | Particle Argon | Purpose |
|---|---|---|
| `#include <esp_wifi.h>` | `#include "WiFi.h"` (or just use cloud) | WiFi control |
| `esp_wifi_init(&cfg)` | Automatic (Particle OS handles) | Init WiFi |
| `esp_wifi_connect()` | `Particle.connect()` or automatic | Connect to WiFi |
| `esp_wifi_set_config()` | Config via Particle app (one-time) | WiFi SSID/password |
| `esp_event_handler_register()` | `Particle.onConnect()` / `Particle.onDisconnect()` | Cloud events |

### Communication Protocol

| Original (MQTT) | Adapted (Particle Pub/Sub) | Behavior |
|---|---|---|
| `esp_mqtt_client_publish(client, topic, data, len, ...)` | `Particle.publish(event, data, len, PUBLIC)` | Send message |
| `esp_mqtt_client_subscribe(client, topic, qos)` | `Particle.subscribe(event, callback)` | Receive message |
| MQTT broker (external server) | Particle cloud (included) | Message hub |
| Manual QoS/retries | Automatic with ACK | Reliability |

### Threading & Task Management

| ESP32 (FreeRTOS) | Particle Argon | Behavior |
|---|---|---|
| `xTaskCreatePinnedToCore(func, name, stack, ...)` | `loop()` function | Main execution |
| `xQueueCreate()` / `xQueueReceive()` | Simple variables + flag | Message passing |
| `xQueueSend(queue, item, timeout)` | Direct assignment | Send data |
| `vTaskDelay()` | `delay()` | Sleep/wait |
| `portTICK_PERIOD_MS` | `millis()` | Timing |
| Core 0 / Core 1 | Single thread (system manages background) | Multi-core vs single |

### Logging & Debug

| ESP32 | Particle Argon | Output |
|---|---|---|
| `printf()` | `Serial.println()` or `Log.info()` | Serial/USB |
| `printf()` in FreeRTOS | Particle console + Serial | Cloud logs |
| No cloud logging | `Particle.publish()` for events | Cloud integration |

### Memory & Peripherals

| Category | ESP32 | Argon |
|---|---|---|
| RAM | 520 KB (SRAM) | 256 KB (RAM) |
| Flash | 4 MB (external) | 1 MB (internal) |
| GPIO Pins | 34 (3.3V) | 16 (D0-D8, A0-A5) |
| ADC channels | 12 (12-bit) | 6 (12-bit) |
| SPI/I2C | Multiple instances | 1 each |
| UART | Multiple | 1 + Particle serial |

### System Features

| Feature | ESP32 | Argon | Notes |
|---|---|---|---|
| WiFi | Manual setup | Automatic | Argon always connected in normal mode |
| Cloud connectivity | Via MQTT | Via Particle cloud | Always on unless disabled |
| OTA updates | Via MQTT | Automatic | Argon can be updated remotely |
| Security | User-managed | Certificate built-in | Particle cloud handles security |
| Power management | Manual | System-managed | Argon OS handles efficiently |
| BLE | Available | Native (nRF52840) | Better on Argon |

## Code Pattern Conversions

### Pattern 1: Event Handlers

**ESP32 (FreeRTOS callback)**
```c
void mqtt_event_handler(void *handler_args, esp_event_base_t base, 
                       int32_t event_id, void *event_data) {
    esp_mqtt_event_handle_t event = event_data;
    
    if (event_id == MQTT_EVENT_DATA) {
        // Handle data
    }
}
```

**Argon (Particle callback)**
```cpp
void on_peer_message(const char* event, const char* data) {
    // Handle data
}

void setup() {
    Particle.subscribe("event_name", on_peer_message);
}
```

### Pattern 2: Task Creation

**ESP32 (FreeRTOS)**
```c
void task_function(void *param) {
    for(;;) {
        // Do work
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

void app_main() {
    xTaskCreatePinnedToCore(task_function, "name", 2048, NULL, 5, NULL, 0);
}
```

**Argon (Particle)**
```cpp
unsigned long last_run = 0;
const unsigned long INTERVAL = 1000;

void loop() {
    if (millis() - last_run >= INTERVAL) {
        // Do work
        last_run = millis();
    }
}
```

### Pattern 3: Queue-Based Communication

**ESP32 (FreeRTOS)**
```c
QueueHandle_t my_queue = xQueueCreate(10, sizeof(message_t));

// Send
xQueueSend(my_queue, &msg, pdMS_TO_TICKS(100));

// Receive
if (xQueueReceive(my_queue, &msg, portMAX_DELAY) == pdTRUE) {
    // Process msg
}
```

**Argon (Simple flag/variable)**
```cpp
struct {
    uint8_t* content;
    size_t size;
    bool received;
} last_message = {NULL, 0, false};

void on_message(const char* event, const char* data) {
    last_message.received = true;
}

void loop() {
    if (last_message.received) {
        // Process message
        last_message.received = false;
    }
}
```

### Pattern 4: Cloud Connectivity Check

**ESP32 (Manual)**
```c
if (esp_mqtt_client_is_connected(client)) {
    // Send data
}
```

**Argon (Simple check)**
```cpp
if (Particle.connected()) {
    Particle.publish("event", data);
}
```

## Memory Usage Optimization

### ESP32 Project
- **FreeRTOS tasks**: ~3 KB each (overhead)
- **MQTT client**: ~10 KB (buffers)
- **WiFi driver**: ~50 KB (allocated)
- **NVS flash**: ~4 KB partition
- **Available for app**: ~400 KB

### Argon Project
- **Particle OS**: ~100 KB (system)
- **Cloud connection**: ~20 KB (handled by OS)
- **User application**: ~100 KB available
- **Total RAM**: 256 KB (more constrained)

### Optimization Strategies

```cpp
// Instead of allocating per-task stacks:
// Use Particle's thread pool (no explicit stacks)

// Instead of large MQTT buffers:
// Particle automatically manages buffers

// Reduce message size:
uint8_t* full_message = (uint8_t*)malloc(length + 2);  // Temporary
// Instead of permanent MQTT buffers

// Use stack for small data:
uint8_t temp_buffer[256];  // On stack, auto-freed
```

## Porting Checklist

- [x] Remove FreeRTOS includes
- [x] Replace `app_main()` with `setup()` + `loop()`
- [x] Remove manual WiFi setup code
- [x] Replace MQTT with `Particle.publish()`/`subscribe()`
- [x] Replace `printf()` with `Log` or `Serial`
- [x] Replace task creation with loop timing
- [x] Replace queues with simple variables
- [x] Replace `esp_` functions with Particle equivalents
- [x] Update GPIO references to Particle pins (D0-D8, A0-A5)
- [x] Test on Argon hardware

## Performance Comparison

| Metric | ESP32 | Argon | Notes |
|---|---|---|---|
| Boot time | ~2 seconds | ~3 seconds | Argon includes cloud handshake |
| Cloud publish latency | N/A (manual MQTT) | ~100-500ms | Via Particle infrastructure |
| Crypto operation (Falcon) | ~200ms | ~250ms | nRF52840 slightly slower |
| Memory usage | ~100 KB (app) | ~80 KB (app) | Argon more constrained |
| Power usage (active) | 100 mA | 80 mA | Argon more efficient |
| Power usage (idle) | 50 mA (WiFi on) | 1 mA (sleep) | Argon supports deeper sleep |

## Debugging Differences

### ESP32 Debugging
- Serial monitor: `idf.py monitor`
- Log output: UART0 @ 115200 baud
- External debugging: Requires FTDI cable

### Argon Debugging
- Cloud logs: Particle console (web UI)
- Serial monitor: `particle serial monitor`
- No external hardware needed
- Real-time log streaming

## Certification & Compliance

| Standard | ESP32 | Argon |
|---|---|---|
| **WiFi** | Manual by user | Pre-certified (Particle) |
| **Security** | User-implemented | Particle managed + keys |
| **FCC (US)** | Depends on implementation | Pre-approved |
| **CE (EU)** | Requires additional work | Pre-certified |
| **RoHS** | Compliant | Compliant |

---

**Summary**: ESP32 provides more control but requires more setup. Particle Argon abstracts away connectivity and security, making it faster to deploy with less code. The tradeoff is less direct hardware control and slightly tighter memory constraints.
