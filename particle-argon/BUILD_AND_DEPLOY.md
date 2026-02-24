# Building & Deploying on Particle Argon

## Build Methods

### 1. Particle Web IDE (Easiest - No Installation)

**Pros**: Works anywhere, no setup, automatic compilation  
**Cons**: Limited offline capability

**Steps**:
1. Go to https://build.particle.io
2. Create new project
3. Paste content from `particle_argon.ino`
4. Select device: **Argon** (top-right dropdown)
5. Click **Verify** button (check syntax)
6. Click **Flash** button
7. Watch for "Flash successful!" message

### 2. Particle CLI (Local Build)

**Pros**: Local development, version control, CI/CD integration  
**Cons**: Requires Node.js installation

#### Installation

```bash
# Install Node.js if not already installed
# Download from https://nodejs.org/

# Install Particle CLI globally
npm install -g particle-cli

# Verify installation
particle --version
```

#### Build Process

```bash
# Navigate to project directory
cd c:\Users\Amine_Mighri\Desktop\ML-KEM-IOT$\signature-esp32\particle-argon

# Create Particle project (if not exists)
particle project create --name dsa-argon

# Copy your .ino file to project
copy particle_argon.ino project/

# Compile locally (generates .bin file)
particle compile argon --saveTo firmware.bin

# Flash to device (must be connected via USB)
particle flash --usb firmware.bin

# Or flash over cloud (if claimed to account)
particle login
particle flash <device_id> firmware.bin

# Monitor output
particle serial monitor
```

#### Full Workflow with CLI

```bash
# Login once (saves credentials)
particle login

# List devices
particle list

# Build binary locally
particle compile argon . --saveTo dsa-argon.bin

# Flash USB (in DFU mode or regular mode)
particle flash --usb dsa-argon.bin

# Or flash OTA (cloud)
particle flash <device_name_or_id> dsa-argon.bin

# Watch logs in real-time
particle serial monitor --follow

# Restart device
particle usb reset
```

### 3. VS Code with Particle Extension

**Pros**: Full IDE with debugging, all in VS Code  
**Cons**: Additional extension installation

#### Setup

1. Install VS Code: https://code.visualstudio.com/
2. Install extension: **Particle for VS Code** (publisher: Particle)
   - Open extensions: Ctrl+Shift+X
   - Search "Particle"
   - Click Install
3. Command Palette (Ctrl+Shift+P): "Particle: Cloud Login"
4. Select your Argon device
5. Verify setup: "Particle: Configure Project"

#### Building & Flashing

```
Open Command Palette (Ctrl+Shift+P)

Particle: Compile Project
Particle: Flash Project
Particle: Flash Using Device OS
Particle: Flash Device Firmware Over USB
Particle: Serial Monitor
```

## Continuous Integration / Automated Building

### GitHub Actions (Automated Builds on Push)

Create `.github/workflows/build.yml`:

```yaml
name: Build Particle Firmware

on: [push, pull_request]

jobs:
  build:
    runs-on: ubuntu-latest
    
    steps:
    - uses: actions/checkout@v2
    
    - name: Setup Node.js
      uses: actions/setup-node@v2
      with:
        node-version: '16'
    
    - name: Install Particle CLI
      run: npm install -g particle-cli
    
    - name: Compile for Argon
      run: |
        cd particle-argon
        particle compile argon . --saveTo firmware.bin
    
    - name: Upload Firmware Artifact
      uses: actions/upload-artifact@v2
      with:
        name: argon-firmware
        path: particle-argon/firmware.bin
```

Then push to GitHub - firmware builds automatically!

## Troubleshooting Builds

### Build Fails: "Cannot find compiler"

**Solution**: Use Particle web IDE or ensure CLI installed correctly

```bash
npm install -g particle-cli
particle --version
```

### Build Fails: "Cannot find dsa.h"

**Issue**: DSA component headers not in include path

**Solution**: In `particle_argon.ino`, update include paths:

```cpp
// If DSA headers are in components/DSA/src/:
extern "C" {
    #include "../components/DSA/src/dsa.h"
}
```

### "Device not found" When Flashing USB

**Solution**: Put device in DFU mode

```bash
# Hold MODE button for 3 seconds until LED blinks yellow
particle usb dfu

# Then flash
particle flash --usb firmware.bin

# Reset device
particle usb reset
```

### Flash Over Cloud Fails

**Solution**: Ensure device is online & claimed

```bash
# List devices and status
particle list

# Check if connected
particle nw status <device_id>

# Reclaim device (if needed)
particle device add <device_id>
```

## Binary Size Management

### Check Binary Size

```bash
particle compile argon . --saveTo firmware.bin
# Output: Binary size: XXXXX bytes

# View without actually flashing
particle compile argon .
# Shows size without --saveTo
```

### Reduce Binary Size

```cpp
// Remove unused code by commenting out algorithms:

// Only test subset:
enum DSA_ALGO algorithms[] = {
    FALCON_512,
    ML_DSA_44,
    // Comment out to save space:
    // FALCON_1024,
    // SPHINCS_SHA2_256F,
};
size_t num_algorithms = sizeof(algorithms) / sizeof(algorithms[0]);
```

### Maximum Size

- **Argon firmware limit**: 1 MB (Device OS handles partitioning)
- **Application code**: ~300-500 KB typical for this project

## Automated Testing / CI

### Local Testing Before Commit

Create test script `test-build.sh`:

```bash
#!/bin/bash
set -e

echo "🔨 Building firmware..."
particle compile argon particle-argon --saveTo test-firmware.bin

if [ -f test-firmware.bin ]; then
    SIZE=$(stat -f%z test-firmware.bin 2>/dev/null || stat -c%s test-firmware.bin)
    echo "✅ Build successful! Size: $SIZE bytes"
    exit 0
else
    echo "❌ Build failed!"
    exit 1
fi
```

Run before pushing:
```bash
bash test-build.sh
```

## Deployment Workflow

```
Development
    ↓
Local Testing: particle compile
    ↓
Commit to Git
    ↓
GitHub Actions: Automated build (optional)
    ↓
Flash to Device: particle flash --usb firmware.bin
    ↓
Verify: particle serial monitor
    ↓
Production Deploy: particle flash <device_id> firmware.bin (OTA)
```

## Production Deployment

### For Single Device

```bash
# One-time setup
particle login
particle device rename <device_id> dsa-alice-1

# Deploy
particle flash dsa-alice-1 firmware.bin
```

### For Multiple Devices

```bash
# Create fleet config (devices.json):
{
  "devices": [
    "device_id_1",
    "device_id_2",
    "device_id_3"
  ]
}

# Deploy to all:
for device in $(cat devices.json | jq -r '.devices[]'); do
    echo "Flashing $device..."
    particle flash "$device" firmware.bin
    sleep 5
done
```

Or using Particle CLI batch:

```bash
particle flash dsa-alice-1 firmware.bin &
particle flash dsa-bob-1 firmware.bin &
wait
echo "All deployments complete!"
```

## Rollback Procedure

If new firmware has issues:

```bash
# Flash previous known-good binary
particle flash <device_id> previous-firmware.bin

# Or restore factory Particle firmware
particle flash <device_id> --factory
```

## Monitoring After Deployment

```bash
# Stream live logs
particle serial monitor --follow

# Check device status
particle list

# View events published
particle subscribe "dsa/*"

# Check cloud connectivity
particle nw status <device_id>
```

## Scripts for Automation

### Deploy Script

```bash
#!/bin/bash
# deploy.sh - Deploy DSA firmware to Argon devices

set -e

DEVICE_ROLE=${1:-alice}  # alice or bob
BUILD_DIR="particle-argon"
FIRMWARE="$BUILD_DIR/firmware.bin"

echo "=== Building DSA Firmware for Particle Argon ==="

# Build
particle compile argon "$BUILD_DIR" --saveTo "$FIRMWARE"

if [ ! -f "$FIRMWARE" ]; then
    echo "❌ Build failed!"
    exit 1
fi

SIZE=$(stat -f%z "$FIRMWARE" 2>/dev/null || stat -c%s "$FIRMWARE")
echo "✅ Build successful! ($SIZE bytes)"

# Flash USB
echo "Flashing firmware to device..."
particle flash --usb "$FIRMWARE"

echo "✅ Flash complete! Monitoring output..."
sleep 2

# Monitor
particle serial monitor
```

Usage:
```bash
bash deploy.sh alice
```

---

**Reference**:
- Particle CLI Docs: https://docs.particle.io/reference/cli/
- GitHub Actions: https://github.com/features/actions
- Particle Build API: https://docs.particle.io/reference/cloud-apis/build/
