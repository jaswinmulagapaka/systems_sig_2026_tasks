#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <threads.h>
#include <time.h>
#include <stdalign.h>

#define NUM_THREADS 4
#define ARRAY_SIZE 100000000 

uint32_t *massive_array;

// STRUCT WITH ALIGNMENT PADDING: Forces each thread's answer onto its own 
// separate 64-byte cache line, completely eliminating false sharing.
struct ThreadResult {
    alignas(64) uint64_t sum;
};
struct ThreadResult final_answers[NUM_THREADS]; 

struct ThreadData {
    int thread_id;
    size_t start_idx;
    size_t end_idx;
};

int worker_thread(void* arg) {
    struct ThreadData* data = (struct ThreadData*)arg;
    uint64_t local_sum = 0; 

    for (size_t i = data->start_idx; i < data->end_idx; i++) {
        local_sum += massive_array[i];
    }

    // Save local sum to this thread's private cache-aligned slot
    final_answers[data->thread_id].sum = local_sum;

    return 0;
}

int main() {
    massive_array = (uint32_t*)malloc(ARRAY_SIZE * sizeof(uint32_t));
    if (!massive_array) {
        printf("Memory allocation failed!\n");
        return 1;
    }
    
    for (size_t i = 0; i < ARRAY_SIZE; i++) {
        massive_array[i] = 50; 
    }

    thrd_t threads[NUM_THREADS];
    struct ThreadData thread_info[NUM_THREADS];
    size_t chunk_size = ARRAY_SIZE / NUM_THREADS;

    for (int i = 0; i < NUM_THREADS; i++) {
        thread_info[i].thread_id = i;
        thread_info[i].start_idx = i * chunk_size;
        thread_info[i].end_idx = (i + 1) * chunk_size;
    }

    struct timespec start_time, end_time;

    // 1. START TIMER: Right before spawning threads
    clock_gettime(CLOCK_MONOTONIC, &start_time);  

    for (int i = 0; i < NUM_THREADS; i++) {
        thrd_create(&threads[i], worker_thread, &thread_info[i]);
    }

    for (int i = 0; i < NUM_THREADS; i++) {
        thrd_join(threads[i], NULL); 
    }

    // 2. STOP TIMER: The exact moment all threads have joined
    clock_gettime(CLOCK_MONOTONIC, &end_time);

    // Aggregate the grand total in main()
    uint64_t total_sum = 0;
    for (int i = 0; i < NUM_THREADS; i++) {
        total_sum += final_answers[i].sum;
    }

    // Calculate time taken in seconds
    double time_taken = (end_time.tv_sec - start_time.tv_sec) +
                        (end_time.tv_nsec - start_time.tv_nsec) / 1000000000.0;

    printf("Time taken: %.5f seconds\n", time_taken);
    printf("Final Total Sum: %lu\n", total_sum);

    free(massive_array);
    return 0;
}