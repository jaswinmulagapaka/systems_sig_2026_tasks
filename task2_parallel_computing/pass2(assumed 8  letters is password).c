// password2_recursive.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <threads.h>
#include <stdatomic.h>
#include <openssl/sha.h>

#define NUM_THREADS 8


const char *target_hex = "4a8d2dcdcc90b1ee98bc16f7097163854f879654e44ec4ffc71e54afcf5d0c13";
unsigned char target_hash[SHA256_DIGEST_LENGTH];

_Atomic uint64_t global_ticket = 0; // 0 to 51 (A-Z and a-z)
_Atomic bool password_found = false;


void hex_to_bytes(const char *hex_string, unsigned char *byte_array) {
    for (int i = 0; i < 32; i++) {
        sscanf(hex_string + 2 * i, "%2hhx", &byte_array[i]);
    }
}

void generate(int pos, int upper_count, int lower_count, char *current_str) {
    if (atomic_load(&password_found)) return;

    if (pos == 8) {
        current_str[8] = '\0'; 
        unsigned char computed_hash[SHA256_DIGEST_LENGTH];
        SHA256((unsigned char*)current_str, 8, computed_hash);
        
        if (memcmp(computed_hash, target_hash, SHA256_DIGEST_LENGTH) == 0) {
            printf("\n[!!!] RECURSION SUCCESS: Hash 2 cracked!\n");
            printf("[!!!] The password is: %s\n\n", current_str);
            atomic_store(&password_found, true); 
        }
        return;
    }

    // took 3 hrs to figure out this logic, but it works now though i have already delt with in a hacker rank problem, but this is a different one, so i had to figure out the logic again tbh i took help of gemini pro a lot 
    if (upper_count < 3) {
        for (char c = 'A'; c <= 'Z'; c++) {
            current_str[pos] = c;
            generate(pos + 1, upper_count + 1, lower_count, current_str);
            if (atomic_load(&password_found)) return; 
        }
    }

    if (lower_count < 5) {
        for (char c = 'a'; c <= 'z'; c++) {
            current_str[pos] = c;
            generate(pos + 1, upper_count, lower_count + 1, current_str);
            if (atomic_load(&password_found)) return; 
        }
    }
}

int brute_force_thread(void *arg) {
    char thread_buffer[9]; 

    while (!atomic_load(&password_found)) {
        uint64_t ticket = atomic_fetch_add(&global_ticket, 1);
        if (ticket > 51) break; 

        if (ticket < 26) {
            thread_buffer[0] = 'A' + ticket;
            generate(1, 1, 0, thread_buffer); 
        } else {
            thread_buffer[0] = 'a' + (ticket - 26);
            generate(1, 0, 1, thread_buffer); 
        }
    }
    return 0;
}

int main() {
    hex_to_bytes(target_hex, target_hash);

    printf("[*] Spawning %d recursive threads for Hash 2...\n", NUM_THREADS);
    thrd_t threads[NUM_THREADS];
    
    for (int i = 0; i < NUM_THREADS; i++) {
        thrd_create(&threads[i], brute_force_thread, NULL);
    }

    for (int i = 0; i < NUM_THREADS; i++) {
        thrd_join(threads[i], NULL);
    }

    if (!atomic_load(&password_found)) printf("[-] Hash 2 not found.\n");
    return 0;
}
