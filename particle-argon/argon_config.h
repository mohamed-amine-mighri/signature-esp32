// Particle Argon - DSA Configuration Header
// Platform-specific definitions for Argon (nRF52840)

#ifndef PARTICLE_ARGON_CONFIG_H
#define PARTICLE_ARGON_CONFIG_H

#include "application.h"

// ============================================================================
// DEVICE CONFIGURATION
// ============================================================================

// Role selection: 1 = ALICE, 0 = BOB
#define DEVICE_ROLE_ALICE 1

// ============================================================================
// PARTICLE EVENT CHANNELS
// ============================================================================

// Event names for peer-to-peer communication via Particle cloud
#define EVENT_ALICE_TO_BOB   "dsa/alice_to_bob"
#define EVENT_BOB_TO_ALICE   "dsa/bob_to_alice"

// Event visibility
#define EVENT_VISIBILITY     PUBLIC    // Use PRIVATE for secure deployments

// ============================================================================
// TIMING CONFIGURATION (milliseconds)
// ============================================================================

#define CLOUD_CONNECT_TIMEOUT    60000   // 60 seconds to connect to cloud
#define SYNC_TIMEOUT             60000   // 60 seconds for device synchronization
#define MESSAGE_TIMEOUT          30000   // 30 seconds to receive message
#define TEST_RETRY_DELAY         2000    // 2 seconds between "ready" broadcasts
#define INTER_TEST_DELAY         100     // 100ms between algorithm tests

// ============================================================================
// MEMORY MANAGEMENT
// ============================================================================

// Check free memory periodically during tests
#define ENABLE_MEMORY_LOGGING    1
#define MEMORY_CHECK_INTERVAL    5000    // Check every 5 seconds

// ============================================================================
// LOGGING CONFIGURATION
// ============================================================================

#define LOG_LEVEL_                LOG_LEVEL_INFO   // DEBUG, INFO, WARN, ERROR
#define ENABLE_SERIAL_DEBUG      1                // Serial output mirror

// Trace important events
#define TRACE_MESSAGE_SEND       1
#define TRACE_MESSAGE_RECV       1
#define TRACE_EVENTS             1

// ============================================================================
// RGB LED FEEDBACK COLORS
// ============================================================================

// Color definitions (R, G, B)
#define LED_WAITING      {255, 255, 0}   // Yellow: Waiting for cloud
#define LED_CONNECTED    {0, 255, 0}     // Green:  Connected to cloud
#define LED_ERROR        {255, 0, 0}     // Red:    Error/Disconnected
#define LED_TESTING      {0, 255, 255}   // Cyan:   Running tests
#define LED_COMPLETE     {0, 0, 255}     // Blue:   Tests complete
#define LED_FAILED       {255, 128, 0}   // Orange: Test failed

// ============================================================================
// FEATURE FLAGS
// ============================================================================

// Enable manual test trigger via Particle function
#define ENABLE_REMOTE_TRIGGER    1

// Enable OTA firmware updates
#define ENABLE_OTA_UPDATES       1

// Reduced algorithm set for faster testing (comment to test all)
// #define TEST_SUBSET_ONLY 1  // Test only first 5 algorithms

// ============================================================================
// ARGON-SPECIFIC FEATURES
// ============================================================================

// Use Argon's RGB LED for status feedback
#define USE_RGB_LED              1

// Enable BLE (if needed for future features)
#define ENABLE_BLE               0

// Enable low-power modes
#define ENABLE_SLEEP_MODE        0

// Use internal RTC for accurate timing
#define USE_SYSTEM_RTC           1

// ============================================================================
// DEBUG MACROS
// ============================================================================

#if ENABLE_SERIAL_DEBUG
    #define DEBUG_PRINT(fmt, ...) Serial.printlf(fmt, ##__VA_ARGS__)
#else
    #define DEBUG_PRINT(fmt, ...) 
#endif

#define SET_LED(r, g, b) do { \
    if (USE_RGB_LED) { RGB.color(r, g, b); } \
} while(0)

// ============================================================================
// ARGON FEATURE AVAILABILITY
// ============================================================================

// Check current features
#define ARGON_HAS_NRF52840        1      // Primary MCU
#define ARGON_HAS_WIFI            1      // Via internal ESP32 NCP
#define ARGON_HAS_BLE             1      // Via nRF52840
#define ARGON_HAS_RGB_LED         1      // Built-in
#define ARGON_HAS_BATTERY_CHARGE  1      // LiPo charging circuit

#endif // PARTICLE_ARGON_CONFIG_H
