# DSA Signature Testing on Particle Argon - Quick Start Guide

## 1. Hardware Setup

### Required
- **Particle Argon** × 1-2 (one per role: Alice & Bob)
- **USB-C Cable** (for power & programming)
- **WiFi Network** (with internet access)
- **Computer** (for flashing firmware)

### Optional
- LiPo Battery (for wireless testing)
- USB Power Bank (for field testing)

## 2. Device Preparation

### Step 1: Claim Device to Your Particle Account

1. Go to [Particle Console](https://console.particle.io)
2. Click **Devices** → **Add Device**
3. Power on Argon (via USB-C)
4. LED will blink **blue** (listening mode)
5. Follow on-screen prompts to connect to WiFi
6. Device will claim automatically once connected

### Step 2: Set WiFi Credentials

The easiest way:
- Use **Particle Mobile App** → Add Device
- Select WiFi network → Enter password
- Device connects and claims to your account

Alternative (Particle CLI):
```bash
particle login
particle serial monitor  # Put device in listening mode (hold MODE 3 sec)
# Follow prompts in serial monitor
```

## 3. Firmware Upload

### Method A: Using Particle Web IDE (Easiest)

1. Go to [build.particle.io](https://build.particle.io)
2. **Create New Project**
3. Copy content from `particle_argon.ino`
4. Ensure **device target is set to "Argon"**
5. Click **Verify** (check for errors)
6. Click **Flash**
7. Device LED will **blink magenta** (flashing) then **cyan** when done

### Method B: Using Particle CLI

```bash
# Install Particle CLI (if not installed)
npm install -g particle-cli

# Login to your account
particle login

# List your devices
particle list
# Output: argon_name (xxxxx...) [online]

# Flash the application
particle flash <device_id> particle_argon.ino

# Or if file is in current directory:
particle flash <device_id> .
```

### Method C: Using VS Code

1. Install **"Particle for VS Code"** extension
2. Open `particle-argon` folder
3. Command Palette (Ctrl+Shift+P): **Particle: Flash**
4. Select your device
5. Wait for completion

## 4. Configuration Before Deployment

### Edit `particle_argon.ino` Device Role

**For Alice Device:**
```cpp
#define DEVICE_ROLE_ALICE 1  // Line ~40
```

**For Bob Device:**
```cpp
#define DEVICE_ROLE_ALICE 0  // Change to 0
```

Then re-flash each device with its respective role.

### Optional: Adjust Timeouts

If you have slow network or devices far from WiFi:

```cpp
#define CLOUD_CONNECT_TIMEOUT    120000  // 2 minutes
#define MESSAGE_TIMEOUT          60000   // 1 minute
```

## 5. Running the Tests

### Single Device Test

If testing with only one device (not both roles):
- Device will start as Alice
- Wait for "ready" will timeout (no Bob)
- Then device swaps role and tests as Bob
- Works for algorithm verification (just slower initialization)

### Two Device Test (Recommended)

1. **Flash Device 1** with `DEVICE_ROLE_ALICE = 1`
2. **Flash Device 2** with `DEVICE_ROLE_ALICE = 0`
3. **Power both** devices (wait for cloud connect - steady green LED)
4. **Tests start automatically** once both are cloud-connected
5. Monitor via:
   - **Device → Logs** in [Particle Console](https://console.particle.io)
   - **Serial Monitor**: `particle serial monitor`

### Monitoring Test Progress

#### Via Particle Console

1. Go to [console.particle.io](https://console.particle.io)
2. Select your Argon device
3. Click **Logs** tab
4. See real-time test output:
   ```
   [DSA ALICE] Starting synchronization
   [DSA ALICE] Synchronized - commencing tests
   [DSA ALICE] FALCON_512 - PASS
   [DSA BOB] Signature verified - echo sent
   ...
   ```

#### Via Serial Monitor

```bash
particle serial monitor --follow
```

Output:
```
Particle Argon DSA Signature Test
Waiting for cloud connection...
Cloud connected!
ALICE: Starting synchronization
BOB: Synchronized
ALICE: Testing Falcon_512
...
```

## 6. Expected Output

### Successful Test Sequence (Alice Side)

```
Cloud connected!
===== Starting test round 1: ALICE =====
Starting synchronization (ALICE)
ALICE: Received READY, sending ACK
Synchronization complete
ALICE: Testing Falcon_512
ALICE: Echo received - PASS
ALICE: Testing ML_DSA_44
ALICE: Echo received - PASS
...
===== All tests completed =====
RGB LED: SOLID BLUE (tests complete)
```

### Test Result Indicators

| LED Color | Meaning |
|-----------|---------|
| **Yellow (pulsing)** | Waiting for cloud connection |
| **Green (solid)** | Connected to Particle cloud |
| **Red (blink)** | Connection lost |
| **Cyan** | Tests running |
| **Blue (solid)** | Tests completed successfully |

## 7. Troubleshooting

### Device won't connect to cloud

**Symptoms**: LED stays **yellow/blinking blue**, never turns solid green

**Solutions**:
1. Check WiFi password is correct (re-claim device)
2. Ensure WiFi has internet access
3. Restart device: Unplug USB, wait 5 seconds, plug back in
4. Factory reset:
   ```bash
   particle usb dfu
   particle flash --usb tinker
   particle usb reset
   ```

### Tests don't start

**Symptoms**: Cloud connected, but no test logs

**Solutions**:
1. Both devices must be cloud-connected
2. Check device roles (Alice and Bob must be different)
3. Manually trigger via:
   ```bash
   particle call <device_id> test_trigger
   ```

### Memory issues or crashes

**Symptoms**: Logs stop, device reboots

**Solutions**:
1. Reduce number of algorithms in `algorithms[]` array
2. Allocate memory more carefully (see comments in code)
3. Increase heap size (advanced - modify Device OS)

### Messages not receiving

**Symptoms**: Tests timeout waiting for messages

**Solutions**:
1. Verify both devices have internet & cloud connected
2. Check event names match (Alice sends to `dsa/alice_to_bob`, Bob listens)
3. Increase message timeout:
   ```cpp
   #define MESSAGE_TIMEOUT 60000  // 60 seconds instead of 30
   ```

## 8. Testing All Algorithms

The code tests 19 PQC algorithms:
- **Falcon**: 512, 1024, padded variants
- **ML-DSA**: 44, 65, 87 (NIST standard)
- **SPHINCS+**: SHA2/SHAKE variants, 128/192/256-bit

**Total test time**: ~5-10 minutes per full rotation (depending on device load)

## 9. Advanced: Remote Test Trigger

You can trigger tests remotely via Particle API:

```bash
# Direct API call
curl -X POST \
  https://api.particle.io/v1/devices/{DEVICE_ID}/test_trigger \
  -d access_token={ACCESS_TOKEN}

# Using Particle CLI
particle call <device_name_or_id> test_trigger

# In Particle Console: Devices → Functions → test_trigger → Call
```

## 10. Next Steps

### For Development
- Modify `test_message` to use custom data
- Add additional algorithms
- Implement BLE broadcast of test results
- Add battery monitoring during tests

### For Deployment
- Secure events: Change `PUBLIC` → `PRIVATE`
- Add HMAC authentication between devices
- Integrate with cloud backend (Webhooks)
- Set up low-power mode for edge devices

### For IoT Integration
- Publish test results to server via Particle Webhook
- Monitor multiple devices from dashboard
- Set up alerts for failed tests
- Record performance metrics

## 11. Clean Up / Removing Code

To restore device to original state:

```bash
# Flash Tinker firmware (default Particle app)
particle flash <device_id> --factory

# Or upload standard Particle examples
particle flash <device_id> --known
```

## 12. Support & Resources

- **Particle Docs**: https://docs.particle.io
- **Community Forum**: https://community.particle.io
- **GitHub**: https://github.com/particle-iot
- **Device Troubleshooting**: https://support.particle.io

---

**Tips**:
- Keep USB cable plugged in during initial testing
- Check Particle console regularly to monitor device health
- Use `particle serial monitor` for detailed debugging
- Test with single device first, then add peer device

**Common Commands**:
```bash
particle login                    # Login to account
particle list                     # List claimed devices
particle device-name <id> <name>  # Rename device
particle serial monitor           # Monitor serial output
particle call <id> test_trigger   # Manually trigger tests
particle flash <id> <file>       # Flash firmware
particle usb dfu                 # Enter DFU mode
particle usb reset               # Reset device
```

Happy Testing! 🚀
