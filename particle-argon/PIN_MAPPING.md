# Pin Mapping & Arduino-Compatible Functions - Particle Argon

## Argon GPIO Pin Layout

```
Particle Argon
┌─────────────────────────────┐
│   ┌───────────────────────┐   │
│   │     nRF52840 MCU      │   │
│   │                       │   │
│   │   ESP32 NCP (WiFi)    │   │
│   └───────────────────────┘   │
├─────────────────────────────┤
│ D0  D1  D2  D3  D4  TX  RX  │  (Digital / Serial)
│ D5  D6  D7  D8  GND  3.3V  │  (Digital / Power)
│ A0  A1  A2  A3  A4  A5      │  (Analog / 12-bit ADC)
│ RST BAT GND VUSB            │  (Power / Reset)
└─────────────────────────────┘
```

## Pin Categories

### Digital I/O (D0-D8)

3.3V logic level, max 16mA per pin

```cpp
#define BUTTON_PIN     D0    // Digital input
#define SENSOR_PIN     D1    // Digital input
#define CONTROL_PIN    D2    // Digital output

// Usage
pinMode(BUTTON_PIN, INPUT_PULLUP);
int state = digitalRead(BUTTON_PIN);
digitalWrite(CONTROL_PIN, HIGH);
```

### Analog Input (A0-A5)

12-bit ADC (0-4095), 0-3.3V input

```cpp
#define VOLTAGE_PIN    A0    // ADC input
#define SENSOR_PIN     A1    // ADC input

// Usage
pinMode(VOLTAGE_PIN, INPUT);
int reading = analogRead(VOLTAGE_PIN);   // 0-4095
float voltage = reading * (3.3 / 4096);  // Convert to volts
```

### Analog Output (PWM)

Pins D0, D1, D2, D3, D4, D6, D7, A4, A5 support PWM

```cpp
#define LED_PWM_PIN    D0    // PWM output
#define FAN_CTRL_PIN   D1    // PWM speed control

// Usage
pinMode(LED_PWM_PIN, OUTPUT);
analogWrite(LED_PWM_PIN, 128);  // 0-255 (0-100%)
```

### Serial / UART

- **TX**: Serial TX to computer
- **RX**: Serial RX from computer
- Used by `Serial.println()` and `Serial.print()`

```cpp
void setup() {
    Serial.begin(115200);
    Serial.println("Hello from Argon!");
}
```

### Power Pins

- **3.3V**: 3.3V output (max 500mA total for all peripherals)
- **GND**: Ground (connect external GND to MCU GND)
- **VUSB**: USB input voltage (when plugged in)
- **BAT**: Battery input (LiPo connector)

### Special Pins

- **RST**: Reset button (resets MCU)
- **MODE**: Mode button (entering listening mode, DFU mode)
- **RGB LED**: Built-in tricolor LED (controlled via `RGB` object)

## Pin Availability

Some pins have multiple functions. Check datasheet for conflicts:

| Pin | Alternative | Notes |
|-----|------------|-------|
| D0 | PWM | Can be PWM or digital |
| D1 | PWM | Can be PWM or digital |
| D2 | PWM | Can be PWM or digital |
| D3 | PWM | Can be PWM or digital |
| D4 | PWM | Can be PWM or digital |
| D6 | PWM | Can be PWM or digital |
| D7 | PWM | Can be PWM or digital |
| A0-A5 | Analog | Can be used as digital with digitalRead/Write |

## Example: GPIO Usage in Particle Code

```cpp
// Configuration
#define BUTTON_PIN     D0       // Input
#define LED_PIN        D8       // Output
#define SENSOR_PIN     A0       // Analog input
#define FAN_PIN        D1       // PWM output

void setup() {
    Serial.begin(115200);
    
    // Configure pins
    pinMode(BUTTON_PIN, INPUT_PULLUP);
    pinMode(LED_PIN, OUTPUT);
    pinMode(SENSOR_PIN, INPUT);
    pinMode(FAN_PIN, OUTPUT);
    
    digitalWrite(LED_PIN, LOW);  // Turn off LED
}

void loop() {
    // Read button
    int button = digitalRead(BUTTON_PIN);
    if (button == LOW) {  // Pressed (pullup = inverted)
        digitalWrite(LED_PIN, HIGH);
        Log.info("Button pressed");
    } else {
        digitalWrite(LED_PIN, LOW);
    }
    
    // Read sensor
    int sensor_val = analogRead(SENSOR_PIN);
    float percentage = (sensor_val / 4096.0) * 100;
    Log.info("Sensor: %d (~%d%%)", sensor_val, (int)percentage);
    
    // PWM control
    int pwm_value = map(sensor_val, 0, 4096, 0, 255);
    analogWrite(FAN_PIN, pwm_value);  // PWM 0-255
    
    delay(100);
}
```

## I2C/SPI Interface (Advanced)

Particle Argon supports I2C and SPI for sensor/peripheral communication:

### I2C Pins
- **SDA**: D4
- **SCL**: D5

```cpp
// Example: Reading from I2C sensor
#include <Wire.h>

#define SENSOR_ADDR 0x68

void setup() {
    Wire.begin();
}

void loop() {
    Wire.beginTransmission(SENSOR_ADDR);
    Wire.write(0x0F);  // Register to read
    Wire.endTransmission();
    
    Wire.requestFrom(SENSOR_ADDR, 1);
    byte data = Wire.read();
    
    Log.info("I2C Data: %02X", data);
    delay(100);
}
```

### SPI Pins
- **MOSI**: D0
- **MISO**: D1
- **SCK**: D2
- **SS**: D3 (configurable)

```cpp
#include <SPI.h>

#define CS_PIN D3

void setup() {
    SPI.begin();
    pinMode(CS_PIN, OUTPUT);
    digitalWrite(CS_PIN, HIGH);
}

void loop() {
    digitalWrite(CS_PIN, LOW);
    byte data = SPI.transfer(0x00);
    digitalWrite(CS_PIN, HIGH);
    
    Log.info("SPI Data: %02X", data);
    delay(100);
}
```

## RGB LED (Built-in)

Tricolor LED on Argon - no pins needed

```cpp
void setup() {
    // RGB LED is automatic, no setup needed
}

void loop() {
    RGB.color(255, 0, 0);      // Red
    delay(1000);
    
    RGB.color(0, 255, 0);      // Green
    delay(1000);
    
    RGB.color(0, 0, 255);      // Blue
    delay(1000);
    
    RGB.color(255, 255, 0);    // Yellow
    delay(1000);
}
```

## Pin Notes for DSA Project

### Current Implementation
The DSA project doesn't require external GPIO in the Particle version because:
- All communication is via Particle cloud (no UART/MQTT hardware)
- Status is shown via built-in RGB LED
- No external sensors/actuators needed

### If You Want to Add GPIO

Example: External status light or button

```cpp
#define STATUS_LED D0
#define TRIGGER_BUTTON D1

void setup() {
    pinMode(STATUS_LED, OUTPUT);
    pinMode(TRIGGER_BUTTON, INPUT_PULLUP);
}

void loop() {
    // In test loop, set LED
    if (test_in_progress) {
        digitalWrite(STATUS_LED, HIGH);
    } else {
        digitalWrite(STATUS_LED, LOW);
    }
    
    // Check button for manual trigger
    if (digitalRead(TRIGGER_BUTTON) == LOW) {
        Particle.publish("button_pressed", "Manual test trigger", PUBLIC);
    }
}
```

## Power Considerations

### Current Draw (Typical)
- **WiFi on, cloud connected**: 80-100 mA
- **WiFi off, sleeping**: 50 μA
- **Per GPIO**: < 1 mA
- **Total peripheral max**: 500 mA (3.3V rail)

### Voltage Levels
- All GPIO: **3.3V logic** (NOT 5V tolerant!)
- ADC range: 0-3.3V
- Do NOT connect 5V devices directly

If connecting 5V devices, use level shifter:
```
5V Device → Level Shifter → Argon (3.3V)
```

## Useful Arduino-Compatible Functions

```cpp
// Digital
pinMode(pin, INPUT|OUTPUT|INPUT_PULLUP);
digitalWrite(pin, HIGH|LOW);
digitalRead(pin);

// Analog
pinMode(pin, INPUT);
analogRead(pin);           // 0-4095
analogWrite(pin, value);   // 0-255 (PWM)

// Serial
Serial.begin(115200);
Serial.print("text");
Serial.println("text");
Serial.printf("Value: %d\n", value);

// Timing
delay(milliseconds);
delayMicroseconds(microseconds);
millis();
micros();

// PWM
analogWrite(pin, 0-255);   // 50% duty = 127
analogWriteResolution(bits);
analogWriteFrequency(pin, frequency);

// Interrupts
attachInterrupt(pin, handler, RISING|FALLING|CHANGE);
detachInterrupt(pin);
```

## Debugging GPIO

```cpp
void debug_pin_state() {
    Log.info("D0=%d D1=%d D2=%d D3=%d", 
             digitalRead(D0), digitalRead(D1), 
             digitalRead(D2), digitalRead(D3));
    
    Log.info("A0=%d A1=%d A2=%d A3=%d", 
             analogRead(A0), analogRead(A1), 
             analogRead(A2), analogRead(A3));
    
    Log.info("Free Memory: %d bytes", System.freeMemory());
}
```

---

**Reference**: [Particle Argon Pinout](https://docs.particle.io/reference/hardware/development/argon/#pinout)
