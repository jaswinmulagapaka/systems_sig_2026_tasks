#include <stdio.h>
#include <string.h>
#include <openssl/sha.h>

// Helper to convert the target hex string into raw binary bytes once
void hex_to_bytes(const char *hex, unsigned char *bytes) {
    for (int i = 0; i < 32; i++) {
        sscanf(hex + 2 * i, "%2hhx", &bytes[i]);
    }
}

int main() {
    // The target hash for the task
    const char *target_hex = "4a8d2dcdcc90b1ee98bc16f7097163854f879654e44ec4ffc71e54afcf5d0c13";
    unsigned char target_bytes[SHA256_DIGEST_LENGTH];
    unsigned char generated_hash[SHA256_DIGEST_LENGTH];
    
    // Parse target once outside the loop to save CPU cycles
    hex_to_bytes(target_hex, target_bytes);
    
    char base[] = "password";

    // Loop 1: Picks the position of the 1st capital letter (indices 0 to 5)
    for (int i = 0; i < 6; i++) {
        
        // Loop 2: Picks the 2nd capital letter (must be after the 1st)
        for (int j = i + 1; j < 7; j++) {
            
            // Loop 3: Picks the 3rd capital letter (must be after the 2nd)
            for (int k = j + 1; k < 8; k++) {
                
                char attempt[9];
                strcpy(attempt, base);
                
                // ASCII math: subtract 32 to convert lowercase to uppercase
                attempt[i] -= 32; 
                attempt[j] -= 32;
                attempt[k] -= 32;
                
                // Hash the generated string
                SHA256((unsigned char*)attempt, 8, generated_hash);
                
                // Fast memory compare against the raw target bytes
                if (memcmp(target_bytes, generated_hash, SHA256_DIGEST_LENGTH) == 0) {
                    printf("\nSUCCESS! The password is: %s\n", attempt);
                    return 0; // Exit program immediately
                }
            }
        }
    }
    
    printf("No match found in the 56 combinations.\n");
    return 0;
}
