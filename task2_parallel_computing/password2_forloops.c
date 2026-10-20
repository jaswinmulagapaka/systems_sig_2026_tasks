#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <stdatomic.h>
#include <openssl/sha.h>


const char *target_hex = "4a8d2dcdcc90b1ee98bc16f7097163854f879654e44ec4ffc71e54afcf5d0c13";
unsigned char target_bytes[SHA256_DIGEST_LENGTH];

const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
const int charset_len = 52;


atomic_int global_i0 = 0;       
atomic_int found_flag = 0;      

void hex_to_bytes(const char *hex, unsigned char *bytes) {
    for (int i = 0; i < 32; i++) {
        sscanf(hex + 2 * i, "%2hhx", &bytes[i]);
    }
}


void* worker(void* arg) {
    int thread_id = *(int*)arg;
    
    char attempt[9];
    attempt[8] = '\0'; 
    unsigned char generated_hash[SHA256_DIGEST_LENGTH];

   
    while (!atomic_load(&found_flag)) {
        
       
        int staring_index = atomic_fetch_add(&global_i0, 1);
        
        
        if (staring_index >= charset_len) {
            break;
        }

        attempt[0] = charset[staring_index];
        printf("Thread %d taking chunk: passwords starting with '%c'\n", thread_id, attempt[0]);

       
        for (int i1 = 0; i1 < charset_len; i1++) {
            
            
            if (atomic_load(&found_flag)) return NULL;
            
            attempt[1] = charset[i1];
            for (int i2 = 0; i2 < charset_len; i2++) {
                attempt[2] = charset[i2];
                for (int i3 = 0; i3 < charset_len; i3++) {
                    attempt[3] = charset[i3];
                    for (int i4 = 0; i4 < charset_len; i4++) {
                        attempt[4] = charset[i4];
                        for (int i5 = 0; i5 < charset_len; i5++) {
                            attempt[5] = charset[i5];
                            for (int i6 = 0; i6 < charset_len; i6++) {
                                attempt[6] = charset[i6];
                                
                                
                                for (int i7 = 0; i7 < charset_len; i7++) {
                                    attempt[7] = charset[i7];
                                    
                                   
                                    SHA256((unsigned char*)attempt, 8, generated_hash);
                                    if (memcmp(target_bytes, generated_hash, SHA256_DIGEST_LENGTH) == 0) {
                                        printf("\n>>> SUCCESS! Thread %d found the password: %s <<<\n", thread_id, attempt);
                                        
                                        
                                        atomic_store(&found_flag, 1);
                                        return NULL;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    return NULL;
}

int main() {
   
    hex_to_bytes(target_hex, target_bytes);

    int num_threads = 8;
    pthread_t threads[8];
    int thread_ids[8];

    printf("Starting 8-thread SHA-256 brute force...\n\n");

  
    for (int i = 0; i < num_threads; i++) {
        thread_ids[i] = i;
        pthread_create(&threads[i], NULL, worker, &thread_ids[i]);
    }

   
    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }

    if (!atomic_load(&found_flag)) {
        printf("\nNo match found in all 53.4 trillion combinations.\n");
    }

    return 0;
}