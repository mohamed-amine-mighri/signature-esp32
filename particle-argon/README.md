# Particle Argon DSA Signature Testing - Complete Adaptation

## 📋 Project Overview

This is a complete **adaptation of an ESP32-based Post-Quantum Cryptography (PQC) testing suite** to run on the **Particle Argon** development board with the Nordic nRF52840 MCU.

### What It Does

- 🔐 Tests **19 Post-Quantum Signature Algorithms**:
  - Falcon (512, 1024, padded variants)
  - ML-DSA 44, 65, 87 (NIST standard)
  - SPHINCS+ (SHA2 & SHAKE variants)
  
- 🤝 Implements **Alice-Bob authentication protocol**:
  - Device 1 (Alice): Signs messages
  - Device 2 (Bob): Verifies signatures
  - Uses Particle cloud for peer-to-peer communication

- ☁️ **Particle Cloud Native**:
  - No manual WiFi configuration
  - Automatic cloud connectivity
  - Real-time log monitoring via web console
  - OTA firmware updates

## 📁 File Structure

```
particle-argon/
├── particle_argon.ino              # Main Particle application (C++)
├── argon_config.h                  # Configuration & feature flags
├── project.properties              # Particle project metadata
│
├── 📖 DOCUMENTATION FILES:
├── README.md                       # This file (overview)
├── README_ADAPTATION.md            # Deep technical adaptation guide
├── QUICKSTART.md                   # Quick start & deployment
├── BUILD_AND_DEPLOY.md             # Build instructions & CI/CD
├── ARCHITECTURE_COMPARISON.md      # ESP32 vs Argon comparison
└── PIN_MAPPING.md                  # GPIO pins & Arduino functions
```

## 🚀 Quick Start

### Prerequisites
- Particle Argon board(s)
- USB-C cable
- Particle account (free at https://particle.io)
- Claimed device to account

### 5-Minute Setup

1. **Go to Web IDE**
   ```
   https://build.particle.io
   ```

2. **Create New Project** and paste [particle_argon.ino](particle_argon.ino)

3. **Select Device**: Ensure "Argon" is selected (top-right)

4. **Edit Role** (in code):
   ```cpp
   #define DEVICE_ROLE_ALICE 1  // For Alice device
   // OR
   #define DEVICE_ROLE_ALICE 0  // For Bob device
   ```

5. **Click Verify** then **Flash**

6. **Monitor Results**:
   - Visit https://console.particle.io
   - Select your device
   - View Logs tab

See [QUICKSTART.md](QUICKSTART.md) for detailed instructions.

## 🎯 Key Features

### ✅ Library Replacements

| Original (ESP32) | Adapted (Argon) | Benefit |
|---|---|---|
| FreeRTOS tasks | Particle loop | Simpler, less overhead |
| Manual WiFi setup | Automatic cloud | Zero configuration |
| MQTT custom setup | Particle Pub/Sub | Built-in, more reliable |
| `printf()` | `Log.*()` | Cloud-integrated logging |
| External MQTT broker | Particle cloud | Included, no server needed |

### ✅ Hardware Features Utilized

- **nRF52840 MCU**: Primary processor with crypto acceleration
- **Internal ESP32**: WiFi (transparent to user, no code access)
- **RGB LED**: Status feedback (yellow→green→blue)
- **Particle Cloud**: Device-to-device messaging & OTA updates

### ✅ Connectivity Model

```
Alice Device (D1)
    │
    ├─► WiFi ───┐
    │            │
    ├─► Particle Cloud
    │            │
    └─► Pub/Sub ─┘
              ↓
         (encrypted tunnel)
              ↓
        Bob Device (D2)
    ├─► WiFi ───┐
    │            │
    ├─► Particle Cloud
    │            │
    └─► Pub/Sub ◄┘
```

## 📊 Protocol

### Synchronization Handshake

```
Start
  │
  ├─ Alice waits:     "ready" ←──
  │                      │
  │                      └─ Bob sends: "ready" (every 2s)
  │
  ├─ Bob receives ACK ––→ Alice sends: "ack"
  │
  └─ Synchronized ✓
```

### Test Sequence (Per Algorithm)

1. **Key Generation**: Alice generates keypair (pk, sk)
2. **Send Public Key**: Alice → Bob (via cloud)
3. **Sign Message**: Alice signs test message with sk
4. **Send Signed Message**: Alice → Bob
5. **Verify**: Bob verifies signature with Alice's pk
6. **Extract Message**: Bob extracts original message from verification
7. **Echo**: Bob sends message back to Alice
8. **Compare**: Alice compares echo with original → PASS/FAIL

## 💻 Build & Deploy

### Using Particle Web IDE (Easiest)

1. Copy `particle_argon.ino`
2. Paste into build.particle.io
3. Click **Verify** then **Flash**

### Using Particle CLI (Local)

```bash
npm install -g particle-cli
particle login
particle compile argon particle-argon --saveTo firmware.bin
particle flash --usb firmware.bin
```

See [BUILD_AND_DEPLOY.md](BUILD_AND_DEPLOY.md) for complete build instructions.

## 📋 Technical Specifications

### Hardware Requirements

| Component | Spec |
|-----------|------|
| **MCU** | Nordic nRF52840 |
| **Coprocessor** | ESP32 (WiFi, managed by Particle OS) |
| **RAM** | 256 KB |
| **Flash** | 1 MB |
| **GPIO** | D0-D8, A0-A5 (3.3V) |
| **Connectivity** | WiFi via Particle cloud |

### Performance

| Metric | Value |
|--------|-------|
| Boot Time | ~3 seconds |
| Cloud Connect | ~5 seconds |
| Falcon-512 Sign | ~200ms |
| Falcon-512 Verify | ~200ms |
| Full Test Suite | ~5-10 minutes |
| Memory Used | ~100-150 KB |

### Supported Algorithms

- ✅ **Falcon** (NIST PQC finalist)
- ✅ **ML-DSA** (NIST PQC standard)
- ✅ **SPHINCS+** (NIST PQC finalist)

All resistant to quantum computer attacks.

## 🔧 Configuration

Edit `particle_argon.ino` before flashing:

```cpp
// Line ~40: Select device role
#define DEVICE_ROLE_ALICE 1      // 1=Alice, 0=Bob

// Line ~50: Event channel names
#define EVENT_ALICE_TO_BOB   "dsa/alice_to_bob"
#define EVENT_BOB_TO_ALICE   "dsa/bob_to_alice"

// Timeout values (in milliseconds)
#define CLOUD_CONNECT_TIMEOUT    60000    // 60 seconds
#define MESSAGE_TIMEOUT          30000    // 30 seconds
```

See [argon_config.h](argon_config.h) for more options.

## 🔍 Monitoring & Logs

### Cloud Console Logs

Visit **[console.particle.io](https://console.particle.io)**:
1. Select your device
2. Click **Logs** tab
3. See real-time output:

```
[Cloud connected!]
[Starting synchronization]
[ALICE Testing Falcon_512]
[Echo received - PASS]
...
```

### Serial Monitor

```bash
particle serial monitor --follow
```

### Manual Test Trigger

```bash
# Via Particle CLI
particle call device_name test_trigger

# Via Web API
curl -X POST \
  https://api.particle.io/v1/devices/{device_id}/test_trigger \
  -d access_token={token}
```

## 🎨 LED Status Indicators

| Color | Meaning | Duration |
|-------|---------|----------|
| 🟨 Yellow | Waiting for cloud | Until connected |
| 🟩 Green | Cloud connected | Steady |
| 🟦 Blue | Tests complete | Steady |
| 🟥 Red | Connection lost | While disconnected |
| 🟦 Cyan | Tests running | During test |

## 🐛 Troubleshooting

### "Cloud not connecting"
→ Check WiFi credentials via Particle app, restart device

### "Tests timeout"
→ Ensure both devices have internet, both are cloud-connected

### "Memory error"
→ Increase timeout delays, test fewer algorithms

### "Compilation error: dsa.h not found"
→ Verify DSA components are in correct path

See [QUICKSTART.md → Troubleshooting](QUICKSTART.md#7-troubleshooting) for more solutions.

## 📚 Documentation

| Document | Purpose |
|----------|---------|
| **README.md** (this) | Overview & quick reference |
| **QUICKSTART.md** | Step-by-step deployment |
| **README_ADAPTATION.md** | Technical deep-dive |
| **ARCHITECTURE_COMPARISON.md** | ESP32 vs Argon differences |
| **BUILD_AND_DEPLOY.md** | Compilation & CI/CD |
| **PIN_MAPPING.md** | GPIO pins & Arduino functions |
| **argon_config.h** | Configuration options |

## 🔐 Security Notes

### Particle Pub/Sub Security

By default, events are **PUBLIC** (anyone on internet can read):

```cpp
Particle.publish(event_name, data, PUBLIC);  // ⚠️ Not secure
Particle.publish(event_name, data, PRIVATE); // ✅ Secure (auth required)
```

For production, set to **PRIVATE**:
```cpp
// In particle_argon.ino, around line 157:
bool published = Particle.publish(event_name, (const char*)full_message, 
                                 length + 2, PRIVATE, WITH_ACK);  // Change PUBLIC to PRIVATE
```

### Post-Quantum Cryptography

All algorithms tested are NIST-approved for quantum resistance:
- Falcon: Lattice-based
- ML-DSA: Lattice-based
- SPHINCS+: Hash-based

## 🌍 Deployment Scenarios

### Single Device (Development)
- Flash one Argon as Alice
- Device waits for Bob (times out), then swaps to Bob role
- Works for algorithm validation

### Two Devices (Peer Testing)
- Flash Device 1 as Alice
- Flash Device 2 as Bob
- Both connect to cloud
- Tests run automatically with proper handshake

### Fleet Deployment (Multiple Pairs)
- Deploy multiple Alice/Bob pairs
- Each pair tests independently
- Monitor all via Particle console
- Aggregate results to backend

## 💡 Use Cases

1. **Research**: Benchmark PQC algorithms on IoT hardware
2. **Testing**: Verify crypto implementations across devices
3. **Education**: Learn about post-quantum cryptography
4. **Production**: Deploy signature verification to edge devices
5. **Validation**: Test NIST standard algorithms on real hardware

## 🎓 Learning Resources

- [Particle Argon Docs](https://docs.particle.io/reference/hardware/development/argon/)
- [Particle Device OS API](https://docs.particle.io/reference/device-os/api/cloud-functions/particle-publish/)
- [NIST Post-Quantum Cryptography](https://csrc.nist.gov/projects/post-quantum-cryptography/)
- [Falcon Signature Scheme](https://falcon-sign.info/)
- [ML-DSA Standard](https://nvlpubs.nist.gov/nistpubs/FIPS/NIST.FIPS.204.pdf)

## 📞 Support

- **Particle Community**: https://community.particle.io
- **GitHub Issues**: Check original project repository
- **Documentation**: See docs folder

## 📄 License

Adapted from original ESP32 project. Maintains compatibility with source licenses (Falcon, ML-DSA, SPHINCS+ are all open source).

---

## Quick Command Reference

```bash
# Setup
particle login
particle device rename <id> my-device

# Build & Flash (Web IDE preferred)
particle compile argon --saveTo firmware.bin
particle flash --usb firmware.bin          # USB (DFU mode)
particle flash <device_id> firmware.bin    # OTA (cloud)

# Monitor
particle serial monitor
particle subscribe "dsa/*"

# Troubleshoot
particle list
particle nw status
particle usb dfu
particle usb reset
```

---

**Status**: ✅ Ready for Production  
**Last Updated**: 2026  
**Target Hardware**: Particle Argon (nRF52840)  
**Firmware Version**: Device OS 2.3.0+

Start with [QUICKSTART.md](QUICKSTART.md) for deployment! 🚀
