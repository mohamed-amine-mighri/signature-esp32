#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "dsa.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "transport.h"

#define ROLE_ALICE_1_BOB_0 1
#if (ROLE_ALICE_1_BOB_0)
    #define ROLE_ALICE
#else
    #define ROLE_BOB
#endif

static const char* message = "Test message for DSA";
static const char* ready_message = "ready";
static const char* ack_message = "ack";
static const char* failed_message = "failed";

enum ROLE {
    ALICE,
    BOB
};

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


int crypto_sign_message(
    uint8_t* sm, size_t* smlen,
    const uint8_t* m, size_t mlen, const uint8_t* sk, 
    enum DSA_ALGO algo) {
        uint8_t* signature = malloc(get_signature_length(algo));
        size_t signature_len = 0;
        if (!signature) return -1;

        uint8_t hdr[2] = {(uint16_t)mlen >> 8, (uint16_t)mlen & 0xFF };
        dsa_signature(algo, signature, &signature_len, m, mlen, sk);

        memcpy(sm, hdr, 2);
        memcpy(sm + 2, m, mlen);
        memcpy(sm + 2 + mlen, signature, signature_len);
        free(signature);
        *smlen = 2 + mlen + signature_len;
        return 0;
    }

int crypto_open_message(
    uint8_t* m, size_t* mlen,
    const uint8_t* sm, size_t smlen, const uint8_t* pk,
    enum DSA_ALGO algo) {
        *mlen = ((size_t)sm[0] << 8) | sm[1];

        uint8_t* signature = malloc(get_signature_length(algo));
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

void synchronize(enum ROLE role) {
    
    bool isMessageReceived = false;
    message_struct_t messageReceived;
    switch(role){
        case ALICE:
                // IF ALICE wait for ready then send ack
                printf("Starting as Alice\n");
                while(!isMessageReceived){
                    if(xQueueReceive(receive_queue, &messageReceived, portMAX_DELAY) == pdTRUE){
                        if(messageReceived.size == strlen(ready_message) && memcmp(ready_message, messageReceived.content, messageReceived.size) == 0) {
                            printf("READY received\n");
                            int send = send_message((const uint8_t*)ack_message, strlen(ack_message));
                            printf("Ack send : %d\n", send);
                            vTaskDelay(pdMS_TO_TICKS(1000));
                            isMessageReceived = true;
                        }
                        free(messageReceived.content);
                    }
                }
                return;
            break;

        case BOB:
                // IF BOB send ready until got ack
                printf("Starting as Bob\n");
                while(!isMessageReceived){
                    if(xQueueReceive(receive_queue, &messageReceived, pdMS_TO_TICKS(3000)) == pdTRUE) {
                        if(messageReceived.size == strlen(ack_message) && memcmp(ack_message, messageReceived.content, messageReceived.size) == 0) {
                            printf("Ack received\n");
                            isMessageReceived = true;
                        } else {
                            printf("Got %s\n", messageReceived.content);
                        }
                        free(messageReceived.content);
                    }
                    
                    if(!isMessageReceived) {
                        send_message((uint8_t*)ready_message, strlen(ready_message));
                        vTaskDelay(pdMS_TO_TICKS(2000));
                    }
                }
                return;
            break;
        default:
            printf("Wrong role\n");
            return;
            break;
    }
}

bool test_dsa_Alice(enum DSA_ALGO algo){
    uint8_t *pk = NULL, *sk = NULL;
    size_t pk_len = 0, sk_len = 0, sig_len = 0;

    alloc_space_for_dsa(algo, &pk, &sk, &pk_len, &sk_len, &sig_len);
    if(!pk || !sk || pk_len == 0 || sk_len == 0 || sig_len == 0){
        printf("Failed to allocate space for keys\n");
        free_space_for_dsa(pk, sk);
        return false;
    }

    // Generate key
    if(dsa_keygen(algo, pk, sk) != 0) {
        printf("Failed to generate keypair\n");
        free_space_for_dsa(pk, sk);
        return false;
    }

    // Send pk
    int sent = send_message(pk, pk_len);
    //printf("Pk sent %d\n", sent);

    // Sign message
    uint8_t* signed_message = malloc(strlen(message) + sig_len);
    if(!signed_message) {
        printf("Failed to allocate memory for signed_message\n");
        free_space_for_dsa(pk, sk);
        free(signed_message);
        return false;
    }

    size_t actual_sm_size = strlen(message) + sig_len;
    if(crypto_sign_message(signed_message, &actual_sm_size, (const uint8_t*)message, strlen(message), sk, algo) != 0) {
        printf("Failed to sign message\n");
        free_space_for_dsa(pk, sk);
        free(signed_message);
        return false;
    }

    // Send signed message
    sent = send_message(signed_message, actual_sm_size);
    //printf("sm : %d\n", sent);

    // Wait for message
    bool isMessageReceived = false;
    message_struct_t messageReceived;
    while(!isMessageReceived){
        if(xQueueReceive(receive_queue, &messageReceived, pdMS_TO_TICKS(1000)) == pdTRUE){
            isMessageReceived = true;
        }
    }

    // Verify message
    if(strlen(message) != messageReceived.size || memcmp(message, messageReceived.content, messageReceived.size) != 0) {
        printf("sm :");
        for(int i = 0; i < actual_sm_size; i++) {
            printf("%02x", signed_message[i]);
        }
        printf("\n");

        free_space_for_dsa(pk, sk);
        free(signed_message);
        free(messageReceived.content);
        return false;
    }

    free_space_for_dsa(pk, sk);
    free(signed_message);
    free(messageReceived.content);
    return true;
}

void test_dsa_Bob(enum DSA_ALGO algo){
    uint8_t *pk = NULL;
    size_t pk_len = get_public_key_length(algo), sm_actual_len = 0;
    
    // Wait for pk
    printf("Waiting for public key\n");
    bool isMessageReceived = false;
    message_struct_t messageReceived;
    while(!isMessageReceived){
        if(xQueueReceive(receive_queue, &messageReceived, pdMS_TO_TICKS(1000)) == pdTRUE){
            if(messageReceived.size == pk_len) {
                printf("Received pk\n");
                pk = messageReceived.content;
                isMessageReceived = true;
            } else {
                printf("Received wrong size : %zu, expecting : %zu\n", messageReceived.size, pk_len);
                free(messageReceived.content);
            }
        }
    }

    uint8_t* signed_message;

    // Wait for signed message
    printf("Waiting for signed message\n");
    isMessageReceived = false;
    while(!isMessageReceived){
        if(xQueueReceive(receive_queue, &messageReceived, pdMS_TO_TICKS(1000)) == pdTRUE){
            signed_message = messageReceived.content;
            sm_actual_len = messageReceived.size;
            isMessageReceived = true;
            printf("Received sm of size %zu\n", sm_actual_len);
        }
    }

    // Decrypt message
    printf("Decrypting message\n");
    uint8_t* message_to_send = malloc(strlen(message) + 20);
    if(!message_to_send) {
        send_message((uint8_t*)failed_message, strlen(failed_message));
        printf("Unable to allocate space\n");
        free(pk);
        free(signed_message);
        free(message_to_send);
        return;
    }
    size_t message_len = 0;
    if(crypto_open_message(message_to_send, &message_len, signed_message, sm_actual_len, pk, algo) != 0) {
        send_message((uint8_t*)failed_message, strlen(failed_message));
        printf("Sm :");
        for(int i = 0; i < sm_actual_len; i++) {
            printf("%02x", signed_message[i]);
        }
        printf("\n");
        free(pk);
        free(signed_message);
        free(message_to_send);
        printf("failed to decrypt message\n");
        return;
    }

    // Send message
    printf("Sending message\n");
    send_message(message_to_send, message_len);

    free(pk);
    free(signed_message);
    free(message_to_send);

    return;
}

void test_dsa_alice_bob(enum ROLE role) {
    synchronize(role);

    switch (role) {
    case ALICE:
        for(int i = 0; i < num_algorithms; i++) {
            printf("Beginning algorithm %s.\n", getAlgoName(algorithms[i]));
            if(test_dsa_Alice(algorithms[i])) {
                printf("DSA algorithm %s passed the test.\n", getAlgoName(algorithms[i]));
            } else {
                printf("DSA algorithm %s failed the test.\n", getAlgoName(algorithms[i]));
            }
        }
        break;
    
    case BOB:
        for(int i = 0; i < num_algorithms; i++) {
            printf("Beginning algorithm %s.\n", getAlgoName(algorithms[i]));
            test_dsa_Bob(algorithms[i]);
        }
        break;
    
    default:
        printf("Wrong algorithm \n");
        break;
    }
    printf("All algorithms done \n");
}

void test_dsa_all_alice_bob() {


    enum ROLE role;
    #if defined(ROLE_ALICE)
        role = ALICE;
    #elif defined(ROLE_BOB)
        role = BOB;
    #endif
    
    test_dsa_alice_bob(role);

    // change les roles
    #if defined(ROLE_ALICE)
        role = BOB;
    #elif defined(ROLE_BOB)
        role = ALICE;
    #endif

    test_dsa_alice_bob(role);
}


void task_test_all_dsa(void *pvParameter)
{
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    for(;;){
        test_dsa_all_alice_bob();
        vTaskDelete(NULL);
    }
}

void app_main(void)
{    
    setup_transport();
    while (!initialized) {
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    
    if(xTaskCreatePinnedToCore(&receive_task, "receive_task", 2048, NULL, 5, NULL, 1) != pdPASS) {
        printf("Couldn't create receive task\n");
    }
    if(xTaskCreatePinnedToCore(&task_test_all_dsa, "task_test_all_dsa", 130000, NULL, 1, NULL, 0) != pdPASS) {
        printf("Couldn't create task\n");
    }
}