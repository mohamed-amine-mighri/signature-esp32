/*
 * Particle Argon DSA Signature Testing
 * Adapted from ESP32 MQTT-based implementation to Particle's native cloud communication
 * 
 * Author: Adapted for Particle Argon (nRF52840)
 * Features:
 *  - Uses Particle.publish() for event-based communication with peer device
 *  - Utilizes Particle's automatic cloud connectivity
 *  - Implements Alice/Bob DSA testing protocol
 */

#include "application.h"
#include <string.h>
#include <stdlib.h>

// Platform-agnostic DSA library interfaces (linked from components/DSA)
extern "C" {
    #include "dsa.h"
}

// ============================================================================
// CONSTANTS & CONFIGURATION
// ============================================================================

#define DEVICE_ROLE_ALICE 1  // Set to 0 for Bob, 1 for Alice

#if (DEVICE_ROLE_ALICE)
    #define ROLE_ALICE
#else
    #define ROLE_BOB
#endif

// Particle event names for Alice->Bob and Bob->Alice communication
#define EVENT_NAME_ALICE_TO_BOB      "dsa/alice_to_bob"
#define EVENT_NAME_BOB_TO_ALICE      "dsa/bob_to_alice"

// Protocol messages
static const char* ready_message   = "ready";
static const char* ack_message     = "ack";
static const char* failed_message  = "failed";
static const char* test_message    = "Test message for DSA";

// ============================================================================
// GLOBAL STATE VARIABLES
// ============================================================================

enum ROLE {
    ALICE,
    BOB
};

// Message reception state machine
typedef struct {
    uint8_t* content;
    size_t size;
    bool received;
    unsigned long timestamp;
} ReceivedMessage;

ReceivedMessage last_received_message = {NULL, 0, false, 0};
bool cloud_connected = false;
bool initialized = false;

// DSA Algorithm array for testing
enum DSA_ALGO algorithms[] = {
    FALCON_512,
    FALCON_1024,
    FALCON_PADDED_512,
    FALCON_PADDED_1024,
    ML_DSA_44,
    ML_DSA_65,
    ML_DSA_87,
    SPHINCS_SHA2_128F,
    SPHINCS_SHA2_128S,
    SPHINCS_SHA2_192F,
    SPHINCS_SHA2_192S,
    SPHINCS_SHA2_256F,
    SPHINCS_SHA2_256S,
    SPHINCS_SHAKE_128F,
    SPHINCS_SHAKE_128S,
    SPHINCS_SHAKE_192F,
    SPHINCS_SHAKE_192S,
    SPHINCS_SHAKE_256F,
    SPHINCS_SHAKE_256S
};
size_t num_algorithms = sizeof(algorithms) / sizeof(algorithms[0]);

// ============================================================================
// MESSAGE TRANSMISSION FUNCTIONS
// ============================================================================

/**
 * Send binary message to peer device via Particle event
 * Message format: [length_hi][length_lo][data...]
 */
int send_message(const uint8_t* data, uint16_t length) {
    // Ensure we're connected to Particle cloud
    if (!Particle.connected()) {
        Log.error("Cloud not connected, cannot send message");
        return -1;
    }

    // Create buffer with header + data
    uint8_t* full_message = (uint8_t*)malloc(length + 2);
    if (!full_message) {
        Log.error("Failed to allocate buffer for message");
        return -1;
    }

    // Add 2-byte length header
    full_message[0] = length >> 8;
    full_message[1] = length & 0xFF;
    memcpy(full_message + 2, data, length);

    // Transmit via appropriate event channel based on device role
    #if defined(ROLE_ALICE)
        const char* event_name = EVENT_NAME_ALICE_TO_BOB;
    #else
        const char* event_name = EVENT_NAME_BOB_TO_ALICE;
    #endif

    // Publish as binary event (max 1KB per event)
    bool published = Particle.publish(event_name, (const char*)full_message, 
                                     length + 2, PUBLIC, WITH_ACK);
    
    free(full_message);
    
    if (!published) {
        Log.error("Failed to publish message");
        return -1;
    }
    
    Log.info("Message sent: %d bytes", length);
    return 0;
}

/**
 * Particle cloud event handler - receives inbound messages from peer device
 */
void on_peer_message(const char* event, const char* data) {
    if (!data || strlen(data) < 2) {
        Log.error("Invalid message received");
        return;
    }

    // Parse message length from header
    const uint8_t* msg_data = (const uint8_t*)data;
    size_t msg_len = ((uint16_t)msg_data[0] << 8) | msg_data[1];
    
    // Extract actual message payload
    if (strlen(data) < msg_len + 2) {
        Log.error("Message size mismatch: expected %zu, got %zu", 
                 msg_len + 2, strlen(data));
        return;
    }

    // Clear previous message if exists
    if (last_received_message.content) {
        free(last_received_message.content);
    }

    // Allocate and copy new message
    last_received_message.content = (uint8_t*)malloc(msg_len);
    if (!last_received_message.content) {
        Log.error("Failed to allocate memory for received message");
        last_received_message.size = 0;
        last_received_message.received = false;
        return;
    }

    memcpy(last_received_message.content, msg_data + 2, msg_len);
    last_received_message.size = msg_len;
    last_received_message.received = true;
    last_received_message.timestamp = millis();
    
    Log.info("Message received: %zu bytes", msg_len);
}

/**
 * Cloud connectivity event handler
 */
void on_cloud_connected() {
    cloud_connected = true;
    RGB.color(0, 255, 0);  // Green LED
    Log.info("Connected to Particle cloud");
}

void on_cloud_disconnected() {
    cloud_connected = false;
    RGB.color(255, 0, 0);  // Red LED
    Log.warn("Disconnected from Particle cloud");
}

// ============================================================================
// SYNCHRONIZATION PROTOCOL
// ============================================================================

/**
 * Synchronize with peer device - establishes handshake
 * Alice waits for "ready" and sends "ack"
 * Bob sends "ready" until receives "ack"
 */
void synchronize(enum ROLE role) {
    Log.info("Starting synchronization (%s)", role == ALICE ? "ALICE" : "BOB");
    
    unsigned long timeout = 60000;  // 60 second timeout
    unsigned long start_time = millis();
    bool synchronized = false;

    while (!synchronized && (millis() - start_time) < timeout) {
        if (role == ALICE) {
            // Alice: wait for "ready" message, then send "ack"
            if (last_received_message.received) {
                if (last_received_message.size == strlen(ready_message) &&
                    memcmp(ready_message, last_received_message.content, 
                           last_received_message.size) == 0) {
                    Log.info("ALICE: Received READY, sending ACK");
                    send_message((const uint8_t*)ack_message, strlen(ack_message));
                    last_received_message.received = false;
                    synchronized = true;
                }
            }
        } else {
            // Bob: send "ready" until receives "ack"
            if (last_received_message.received) {
                if (last_received_message.size == strlen(ack_message) &&
                    memcmp(ack_message, last_received_message.content, 
                           last_received_message.size) == 0) {
                    Log.info("BOB: Received ACK");
                    last_received_message.received = false;
                    synchronized = true;
                }
            } else if ((millis() - start_time) % 2000 < 100) {
                // Send "ready" every 2 seconds
                send_message((const uint8_t*)ready_message, strlen(ready_message));
            }
        }
        
        delay(10);  // Prevent watchdog timeout
    }

    if (!synchronized) {
        Log.error("Synchronization timeout");
    } else {
        Log.info("Synchronization complete");
    }
}

// ============================================================================
// DSA TESTING FUNCTIONS
// ============================================================================

/**
 * Wrapper for crypto_sign with message length header
 */
int crypto_sign_message(
    uint8_t* sm, size_t* smlen,
    const uint8_t* m, size_t mlen, const uint8_t* sk, 
    enum DSA_ALGO algo) {
    
    uint8_t* signature = (uint8_t*)malloc(get_signature_length(algo));
    size_t signature_len = 0;
    if (!signature) return -1;

    uint8_t hdr[2] = {(uint16_t)mlen >> 8, (uint16_t)mlen & 0xFF};
    dsa_signature(algo, signature, &signature_len, m, mlen, sk);

    memcpy(sm, hdr, 2);
    memcpy(sm + 2, m, mlen);
    memcpy(sm + 2 + mlen, signature, signature_len);
    free(signature);
    *smlen = 2 + mlen + signature_len;
    return 0;
}

/**
 * Wrapper for crypto_open with message length header verification
 */
int crypto_open_message(
    uint8_t* m, size_t* mlen,
    const uint8_t* sm, size_t smlen, const uint8_t* pk,
    enum DSA_ALGO algo) {
    
    *mlen = ((size_t)sm[0] << 8) | sm[1];

    uint8_t* signature = (uint8_t*)malloc(get_signature_length(algo));
    size_t signature_len = smlen - *mlen - 2;
    if (!signature) return -1;

    memcpy(signature, sm + 2 + *mlen, signature_len);

    if (dsa_verify(algo, signature, signature_len, sm + 2, *mlen, pk) < 0) {
        free(signature);
        return -1;
    }

    memcpy(m, sm + 2, *mlen);
    free(signature);
    return 0;
}

/**
 * Alice role: Generate key pair, send public key, sign message, wait for echo
 */
bool test_dsa_alice(enum DSA_ALGO algo) {
    Log.info("ALICE: Testing %s", getAlgoName(algo));
    
    uint8_t *pk = NULL, *sk = NULL;
    size_t pk_len = 0, sk_len = 0, sig_len = 0;

    alloc_space_for_dsa(algo, &pk, &sk, &pk_len, &sk_len, &sig_len);
    if (!pk || !sk || pk_len == 0 || sk_len == 0 || sig_len == 0) {
        Log.error("Failed to allocate space for keys");
        return false;
    }

    // Generate key pair
    if (dsa_keygen(algo, pk, sk) != 0) {
        Log.error("Failed to generate keypair");
        free(pk);
        free(sk);
        return false;
    }

    // Send public key
    if (send_message(pk, pk_len) != 0) {
        Log.error("Failed to send public key");
        free(pk);
        free(sk);
        return false;
    }

    // Sign message
    uint8_t* signed_message = (uint8_t*)malloc(strlen(test_message) + sig_len + 10);
    if (!signed_message) {
        Log.error("Failed to allocate memory for signed_message");
        free(pk);
        free(sk);
        return false;
    }

    size_t actual_sm_size = strlen(test_message) + sig_len + 10;
    if (crypto_sign_message(signed_message, &actual_sm_size, 
                           (const uint8_t*)test_message, strlen(test_message), sk, algo) != 0) {
        Log.error("Failed to sign message");
        free(pk);
        free(sk);
        free(signed_message);
        return false;
    }

    // Send signed message
    if (send_message(signed_message, actual_sm_size) != 0) {
        Log.error("Failed to send signed message");
        free(pk);
        free(sk);
        free(signed_message);
        return false;
    }

    // Wait for echo
    unsigned long echo_timeout = 30000;
    unsigned long start = millis();
    bool got_echo = false;
    
    while ((millis() - start) < echo_timeout) {
        if (last_received_message.received) {
            if (last_received_message.size == strlen(test_message) &&
                memcmp(test_message, last_received_message.content, 
                       last_received_message.size) == 0) {
                got_echo = true;
                last_received_message.received = false;
                Log.info("ALICE: Echo received - PASS");
            }
            break;
        }
        delay(10);
    }

    free(pk);
    free(sk);
    free(signed_message);

    if (!got_echo) {
        Log.error("ALICE: Echo timeout - FAIL");
        return false;
    }
    return true;
}

/**
 * Bob role: Receive public key, receive signed message, verify and echo
 */
bool test_dsa_bob(enum DSA_ALGO algo) {
    Log.info("BOB: Testing %s", getAlgoName(algo));
    
    uint8_t *pk = NULL;
    size_t pk_len = get_public_key_length(algo);
    
    // Wait for public key
    unsigned long timeout = 30000;
    unsigned long start = millis();
    
    while ((millis() - start) < timeout) {
        if (last_received_message.received) {
            if (last_received_message.size == pk_len) {
                Log.info("BOB: Received public key");
                pk = last_received_message.content;
                last_received_message.received = false;
                break;
            } else {
                Log.warn("BOB: Received wrong size: %zu, expecting: %zu", 
                        last_received_message.size, pk_len);
                free(last_received_message.content);
                last_received_message.content = NULL;
                last_received_message.received = false;
            }
        }
        delay(10);
    }

    if (!pk) {
        Log.error("BOB: Timeout waiting for public key");
        return false;
    }

    // Wait for signed message
    uint8_t* signed_message = NULL;
    size_t sm_actual_len = 0;
    start = millis();
    
    while ((millis() - start) < timeout) {
        if (last_received_message.received) {
            signed_message = last_received_message.content;
            sm_actual_len = last_received_message.size;
            Log.info("BOB: Received signed message (%zu bytes)", sm_actual_len);
            last_received_message.received = false;
            break;
        }
        delay(10);
    }

    if (!signed_message) {
        Log.error("BOB: Timeout waiting for signed message");
        free(pk);
        return false;
    }

    // Verify and extract message
    uint8_t* message_to_echo = (uint8_t*)malloc(strlen(test_message) + 20);
    if (!message_to_echo) {
        Log.error("BOB: Failed to allocate memory");
        send_message((uint8_t*)failed_message, strlen(failed_message));
        free(pk);
        free(signed_message);
        return false;
    }

    size_t message_len = 0;
    if (crypto_open_message(message_to_echo, &message_len, 
                           signed_message, sm_actual_len, pk, algo) != 0) {
        Log.error("BOB: Failed to verify signature");
        send_message((uint8_t*)failed_message, strlen(failed_message));
        free(pk);
        free(signed_message);
        free(message_to_echo);
        return false;
    }

    Log.info("BOB: Verified - sending echo");
    send_message(message_to_echo, message_len);

    free(pk);
    free(signed_message);
    free(message_to_echo);
    return true;
}

/**
 * Run all algorithms in one role
 */
void test_dsa_alice_bob(enum ROLE role) {
    synchronize(role);

    for (size_t i = 0; i < num_algorithms; i++) {
        bool passed = false;
        
        if (role == ALICE) {
            passed = test_dsa_alice(algorithms[i]);
        } else {
            passed = test_dsa_bob(algorithms[i]);
        }
        
        Log.info("Algorithm %s: %s", 
                getAlgoName(algorithms[i]), 
                passed ? "PASS" : "FAIL");
        
        delay(100);  // Small delay between tests
    }
    
    Log.info("Role %s complete", role == ALICE ? "ALICE" : "BOB");
}

/**
 * Run all tests - Alice testing, then role swap, then Bob testing
 */
void test_dsa_all_alice_bob() {
    enum ROLE role;
    
    #if defined(ROLE_ALICE)
        role = ALICE;
    #elif defined(ROLE_BOB)
        role = BOB;
    #else
        Log.error("Device role not defined");
        return;
    #endif
    
    Log.info("===== Starting test round 1: %s =====", 
            role == ALICE ? "ALICE" : "BOB");
    test_dsa_alice_bob(role);

    Log.info("===== Swapping roles =====");
    
    // Swap role
    role = (role == ALICE) ? BOB : ALICE;
    
    Log.info("===== Starting test round 2: %s =====", 
            role == ALICE ? "ALICE" : "BOB");
    test_dsa_alice_bob(role);
    
    Log.info("===== All tests completed =====");
}

// ============================================================================
// PARTICLE SETUP & LOOP
// ============================================================================

void setup() {
    // Initialize serial logging
    Serial.begin(115200);
    Log.info("Particle Argon DSA Signature Test");
    
    // Set up cloud event handlers
    Particle.onConnect(on_cloud_connected);
    Particle.onDisconnect(on_cloud_disconnected);
    
    // Subscribe to inbound messages from peer
    #if defined(ROLE_ALICE)
        Particle.subscribe(EVENT_NAME_BOB_TO_ALICE, on_peer_message);
        Log.info("ALICE: Subscribed to peer events");
    #else
        Particle.subscribe(EVENT_NAME_ALICE_TO_BOB, on_peer_message);
        Log.info("BOB: Subscribed to peer events");
    #endif
    
    // Set device name for identification
    #if defined(ROLE_ALICE)
        Particle.setDeviceName("dsa-alice");
    #else
        Particle.setDeviceName("dsa-bob");
    #endif
    
    // Enable Particle logging
    LogLevel(LOG_LEVEL_INFO);
    
    // RGB LED: Yellow while waiting for cloud
    RGB.color(255, 255, 0);
    
    // Wait for cloud connection
    Log.info("Waiting for cloud connection...");
    unsigned long cloud_timeout = 60000;
    unsigned long start = millis();
    
    while (!Particle.connected() && (millis() - start) < cloud_timeout) {
        delay(100);
    }
    
    if (Particle.connected()) {
        Log.info("Cloud connected!");
        RGB.color(0, 255, 0);  // Green
    } else {
        Log.error("Cloud connection timeout");
        RGB.color(255, 0, 0);  // Red
    }
    
    initialized = true;
}

static unsigned long last_test_start = 0;
static bool test_in_progress = false;
static bool test_complete = false;

void loop() {
    // Ensure we stay connected
    if (!Particle.connected()) {
        Log.warn("Reconnecting to cloud...");
        Particle.connect();
        delay(1000);
    }
    
    // Start tests once initialized and connected
    if (initialized && Particle.connected() && !test_in_progress) {
        if (!test_complete) {
            Log.info("Starting DSA tests...");
            test_in_progress = true;
            last_test_start = millis();
            test_dsa_all_alice_bob();
            test_complete = true;
            test_in_progress = false;
            
            unsigned long duration = millis() - last_test_start;
            Log.info("Tests completed in %lu ms", duration);
            
            // Signal completion with RGB LED
            RGB.color(0, 0, 255);  // Blue
        }
    }
    
    delay(100);
}

// Optional: Particle function to manually trigger tests
int trigger_tests(String cmd) {
    if (!test_in_progress) {
        test_complete = false;
        Particle.publish("dsa/event", "Tests triggered manually", PUBLIC);
        return 1;
    }
    return 0;
}

STARTUP(
    System.enableFeature(FEATURE_RETAINED_MEMORY);
    Particle.function("test_trigger", trigger_tests);
)
