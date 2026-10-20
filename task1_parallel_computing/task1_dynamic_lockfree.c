
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <threads.h>
#include <stdatomic.h>

#define ARRAY_SIZE 100000000
#define CHUNK_SIZE 10000
#define NUM_THREADS 4


_Atomic uint64_t global_index = 0;


uint64_t *massive_array;
uint64_t thread_results[NUM_THREADS] = {0};


typedef struct
{
    int thread_id;
} ThreadData;

int sum_thread(void *arg)
{
    ThreadData *my_data = (ThreadData *)arg;

    uint64_t thread_total_sum = 0;

    while (1)
    {
        uint64_t start = atomic_fetch_add(&global_index, CHUNK_SIZE);

        if (start >= ARRAY_SIZE)
            break;

        uint64_t end = start + CHUNK_SIZE;
        if (end > ARRAY_SIZE)
            end = ARRAY_SIZE;

        
        uint64_t local_chunk_sum = 0;
        for (uint64_t i = start; i < end; i++)
        {
            local_chunk_sum += massive_array[i];
        }

        thread_total_sum += local_chunk_sum;
    }

 

    // i delibrately used false sharing here to show the performance difference between this and the lockfree version/*  */
    thread_results[my_data->thread_id] = thread_total_sum;

    return 0;
}

int main()
{
  
    massive_array = (uint64_t *)malloc(ARRAY_SIZE * sizeof(uint64_t));

    if (massive_array == NULL)
        return 1;

    for (uint64_t i = 0; i < ARRAY_SIZE; i++)
        massive_array[i] = 1+i;

    thrd_t threads[NUM_THREADS];
    ThreadData args[NUM_THREADS];

    printf("[*] Spawning %d dynamic hybrid threads...\n", NUM_THREADS);
    fflush(stdout);

    struct timespec start_time, end_time;
    // Start the stopwatch
    if (clock_gettime(CLOCK_MONOTONIC, &start_time) != 0)
    {
        perror("clock_gettime");
        free(massive_array);
        return 1;
    }

    for (int i = 0; i < NUM_THREADS; i++)
    {
        args[i].thread_id = i;
        thrd_create(&threads[i], sum_thread, &args[i]);
    }

    for (int i = 0; i < NUM_THREADS; i++)
    {
        thrd_join(threads[i], NULL);
    }
    uint64_t final_global_sum = 0;
    for (int i = 0; i < NUM_THREADS; i++)
    {
        final_global_sum += thread_results[i];
    }

    if (clock_gettime(CLOCK_MONOTONIC, &end_time) != 0)
    {
        perror("clock_gettime");
        free(massive_array);
        return 1;
    }

    // Calculate the difference in seconds
    double time_taken = (end_time.tv_sec - start_time.tv_sec) +
                        (end_time.tv_nsec - start_time.tv_nsec) / 1000000000.0;

    printf("Time taken: %.5f seconds\n", time_taken);
    fflush(stdout);

    printf("[*] Final Global Sum: %lu\n", (unsigned long)final_global_sum);

    free(massive_array);
    return 0;
}