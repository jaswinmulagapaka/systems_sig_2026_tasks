
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <threads.h>
#include <stdatomic.h>
#include <openssl/sha.h>

#define NUM_THREADS 4

// Hash 1 from the assignment
const char *target_hex = "32adca3bde67a61815896740ee954b0b3dea15f6851f1b35079698729081e2ba";
unsigned char target_hash[SHA256_DIGEST_LENGTH];

_Atomic bool password_found = false;

// Converts the raw hex string into bytes for OpenSSL
void hex_to_bytes(const char *hex_string, unsigned char *byte_array) {
    for (int i = 0; i < 32; i++) {
        sscanf(hex_string + 2 * i, "%2hhx", &byte_array[i]);
    }
}

int static_brute_force(void *arg) {
    int thread_id = *(int*)arg;
    
    // Static Chunking: Divide 26 letters among NUM_THREADS
    int chunk_size = 26 / NUM_THREADS;
    char start_char = 'a' + (thread_id * chunk_size);
    char end_char = (thread_id == NUM_THREADS - 1) ? 'z' : start_char + chunk_size - 1;

    char guess[8];
    guess[7] = '\0';
    unsigned char computed_hash[SHA256_DIGEST_LENGTH];

    // Thread only loops through its statically assigned starting letters
    for (char c1 = start_char; c1 <= end_char; c1++) {
        if (atomic_load(&password_found)) return 0;
        guess[0] = c1;
        
        for (char c2 = 'a'; c2 <= 'z'; c2++) {
            if (atomic_load(&password_found)) return 0;
            guess[1] = c2;
            for (char c3 = 'a'; c3 <= 'z'; c3++) {
                guess[2] = c3;
                for (char c4 = 'a'; c4 <= 'z'; c4++) {
                    guess[3] = c4;
                    for (char c5 = 'a'; c5 <= 'z'; c5++) {
                        guess[4] = c5;
                        for (char c6 = 'a'; c6 <= 'z'; c6++) {
                            guess[5] = c6;
                            for (char c7 = 'a'; c7 <= 'z'; c7++) {
                                guess[6] = c7;
                                
                                SHA256((unsigned char*)guess, 7, computed_hash);
                                if (memcmp(computed_hash, target_hash, SHA256_DIGEST_LENGTH) == 0) {
                                    printf("\n[!!!] THREAD %d SUCCESS: Hash 1 cracked!\n", thread_id);
                                    printf("[!!!] The password is: %s\n\n", guess);
                                    atomic_store(&password_found, true);
                                    return 0;
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    return 0;
}

int main() {
    hex_to_bytes(target_hex, target_hash);
    
    printf("[*] Spawning %d static threads for Hash 1...\n", NUM_THREADS);
    thrd_t threads[NUM_THREADS];
    int thread_ids[NUM_THREADS];

    for (int i = 0; i < NUM_THREADS; i++) {
        thread_ids[i] = i;
        thrd_create(&threads[i], static_brute_force, &thread_ids[i]);
    }

    for (int i = 0; i < NUM_THREADS; i++) {
        thrd_join(threads[i], NULL);
    }

    if (!atomic_load(&password_found)) printf("[-] Hash 1 not found.\n");
    return 0;
}