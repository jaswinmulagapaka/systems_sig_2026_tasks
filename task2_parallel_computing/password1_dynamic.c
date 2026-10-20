
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <threads.h>
#include <stdatomic.h>
#include <openssl/sha.h>

#define NUM_THREADS 8


const char *target_hex = "32adca3bde67a61815896740ee954b0b3dea15f6851f1b35079698729081e2ba";
unsigned char target_hash[SHA256_DIGEST_LENGTH];

_Atomic uint64_t global_ticket = 0; // 0 to 25 for 'a' to 'z'
_Atomic bool password_found = false;


void hex_to_bytes(const char *hex_string, unsigned char *byte_array) {
    for (int i = 0; i < 32; i++) {
        sscanf(hex_string + 2 * i, "%2hhx", &byte_array[i]);
    }
}

int dynamic_brute_force(void *arg) {
    char guess[8];
    guess[7] = '\0';
    unsigned char computed_hash[SHA256_DIGEST_LENGTH];

    while (!atomic_load(&password_found)) {
        uint64_t ticket = atomic_fetch_add(&global_ticket, 1);
        if (ticket > 25) break; 
        
        guess[0] = 'a' + ticket;

      
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
                                    printf("\n[!!!] SUCCESS: Password is: %s\n", guess);
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
    // Convert string to bytes before starting threads
    hex_to_bytes(target_hex, target_hash);
    
    thrd_t threads[NUM_THREADS];
    for (int i = 0; i < NUM_THREADS; i++) {
        thrd_create(&threads[i], dynamic_brute_force, NULL);
    }
    for (int i = 0; i < NUM_THREADS; i++) {
        thrd_join(threads[i], NULL);
    }
    return 0;
}