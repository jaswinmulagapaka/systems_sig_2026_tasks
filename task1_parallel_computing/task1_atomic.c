
// step 1 call the library
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <threads.h>
#include <stdatomic.h>
#include <time.h>

#define NUM_THREADS 1

_Atomic uint64_t total_sum = 0;

// struct ThreadData {
//   uint64_t *array;
// uint64_t start_index;
// uint64_t end_index;
// };

// step 2 struct
typedef struct
{
    uint64_t *array;
    uint64_t start;
    uint64_t end;
} ThreadData;

// step 3 the engine

int sum_array_chunk(void *arg)
{
    ThreadData *data;
    // casting
    data = (ThreadData *)arg;
    uint64_t local_sum = 0;

    for (uint64_t i = data->start; i < data->end; i++)
    {
        local_sum += data->array[i];
    }
    atomic_fetch_add(&total_sum, local_sum);
    return 0;
}

int main()
{
    uint64_t ARRAY_SIZE = 100000000;
    uint64_t *massive_array = (uint64_t *)malloc(ARRAY_SIZE * sizeof(uint64_t));
    if (massive_array == NULL)
    {
        printf("critical error out of ram  \n ");
        return 1;
    }
    printf("generating numbers ................\n");
    for (uint64_t i = 0; i < ARRAY_SIZE; i++)
    {
        massive_array[i] = 1 + i;
    }
    
    thrd_t threads[NUM_THREADS];
    ThreadData thread_data[NUM_THREADS];
    uint64_t chunk_size = ARRAY_SIZE / NUM_THREADS;    

    struct timespec start_time, end_time;
    // Start the stopwatch
    clock_gettime(CLOCK_MONOTONIC, &start_time);    

    printf("spawning %d threads to to calculate sum\n", NUM_THREADS);
    for (int i = 0; i < NUM_THREADS; i++)
    {
        thread_data[i].array = massive_array;
        thread_data[i].start = i * chunk_size;
        // thread_data[i].end = (i + 1) * chunk_size;
      if (i== NUM_THREADS - 1)
        {
            thread_data[i].end = ARRAY_SIZE; // Last thread takes the remainder
        }
        else
        {
            thread_data[i].end = (i + 1) * chunk_size;
        }   

        thrd_create(&threads[i], sum_array_chunk, &thread_data[i]);
    }

    printf("waiting for threads to finish \n");
    for (int i = 0; i < NUM_THREADS; i++)
    {
        int status = thrd_join(threads[i], NULL);
        if (status != thrd_success)
        {
            printf("error joining threads %d\n", i);
        }
    }
    // Stop the stopwatch the exact moment the last thread joins!
    clock_gettime(CLOCK_MONOTONIC, &end_time);

    // Calculate the difference in seconds
    double time_taken = (end_time.tv_sec - start_time.tv_sec) +
                        (end_time.tv_nsec - start_time.tv_nsec) / 1000000000.0;

    printf("Time taken: %.5f seconds\n", time_taken);

    printf("all threads finished \n");
    uint64_t final_sum = atomic_load(&total_sum);

    printf("final sum = %lu \n", (unsigned long )final_sum);
    free(massive_array);
    return 0;
}
