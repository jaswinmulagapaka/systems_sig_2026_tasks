WEC Systems & Security SIG 2026
Concurrency & Systems Programming in C

Author: M. Jaswin
Technologies: C11, POSIX Threads, C11 Atomics, SHA-256, Linux, Android Termux

This repository contains my solutions and architectural explorations for the WEC Systems & Security SIG 2026 recruitment tasks. It explores concurrency, work distribution, synchronization, cache alignment, parallel reduction, and cryptographic candidate generation in C.

The central question explored throughout the repository is:

When multiple threads process the same workload, how should work be partitioned, shared state synchronized, and results aggregated without introducing unnecessary coordination overhead?

The repository contains two task groups:

Task 1 — Parallel Array Reduction: Compares shared atomic accumulation, per-thread result slots, and dynamic work allocation using an atomic cursor.
Task 2 — SHA-256 Candidate Generation: Explores static and dynamic search-space partitioning, recursive and iterative candidate generation, and raw digest comparison.

Each source file is a standalone C program with its own main() function and is intended to be compiled independently.

Table of Contents
Repository Structure
Task 1: Parallel Array Reduction
Architectural Comparison
Benchmark Results
Bonus: ARM64 Execution on Android
Task 2: SHA-256 Candidate Generation
Build and Execution Guide
Development Environment
Limitations and Future Improvements
Repository Structure
.
├── task1_parallel_computing/
│   ├── task1_atomic.c
│   ├── task1_lockfree.c
│   └── task1_dynamic_lockfree.c
├── task2_parallel_computing/
│   ├── password1_static.c
│   ├── password1_dynamic.c
│   ├── password2_recursive.c
│   ├── password2_forloops.c
│   └── pass2(assumed 8  letters is password).c
├── .vscode/
│   ├── c_cpp_properties.json
│   └── tasks.json
└── README.md

Task 1: Parallel Array Reduction

Task 1 investigates three approaches to parallel array summation using four worker threads:

Local accumulation followed by a shared atomic update.
Local accumulation followed by writing to per-thread result slots.
Dynamic chunk assignment using a shared atomic work cursor.
Common Workload

All three implementations are intended to process the same input array containing 100,000,000 consecutive integers from 1 to 100,000,000.

Element type: uint64_t
Array size: 100,000,000 elements
Memory footprint: Approximately 800 MB (decimal), excluding other process memory
Worker threads: 4
Expected sum: 5000000050000000

The mathematical sum is:

𝑆
=
𝑁
(
𝑁
+
1
)
2

For 
𝑁
=
100,000,000
:

5000000050000000


The intended workload is identical across all three implementations. Their differences lie in work distribution, synchronization, and result aggregation.

Timing Methodology

Each Task 1 program uses clock_gettime(CLOCK_MONOTONIC) to measure elapsed wall-clock time.

The timed interval starts immediately before thread creation and ends after all workers have joined and the final result has been collected.

Included in the timed interval:

Thread creation
Worker execution
Thread joining
Final result aggregation

Excluded from the timed interval:

Array allocation
Array initialization

This measures the complete parallel execution phase rather than only the time spent processing array elements.

1. task1_atomic.c — Shared Atomic Accumulator

Design

Divides the array into four contiguous ranges of 25 million elements.
Each worker calculates its partial sum locally.
Each worker performs one atomic_fetch_add() operation to add its partial sum to a shared atomic total.

Synchronization

The workers perform their main accumulation locally, avoiding atomic updates for individual array elements. Only the final partial sums are added to the shared total.

Trade-offs

Performs four shared atomic read-modify-write operations.
Avoids an atomic operation for every element.
All workers update the same atomic object, which may introduce contention if the updates overlap.
Atomic operations are not equivalent to mutex locking.

Using C11 atomics does not automatically guarantee lock-free implementation on every platform; that depends on the atomic type and target implementation.

2. task1_lockfree.c — Per-Thread Result Slots

Design

Uses the same input array as task1_atomic.c.
Assigns each worker a fixed, non-overlapping range.
Each worker calculates a local sum and stores it in a dedicated result slot.
The main thread aggregates the partial sums after joining all workers.

Synchronization

The workers do not need a shared atomic accumulator. Each worker writes to a distinct result slot, and thrd_join() or the equivalent thread-joining operation establishes the synchronization needed before the main thread reads the results.

Cache alignment

The implementation uses alignas(64) for a result slot.

This explores 64-byte alignment as a potential cache-layout consideration. The target cache-line size is not measured, and the implementation does not independently demonstrate that false sharing has been eliminated or that performance has improved.

Trade-offs

Eliminates shared atomic updates from the final reduction path.
Keeps the primary accumulation local to each worker.
Requires a final aggregation step after the workers finish.
Cache alignment may influence memory layout, but its impact must be measured rather than assumed.
3. task1_dynamic_lockfree.c — Dynamic Chunk Assignment

Design

Uses the same input array as the other Task 1 programs.
Divides the workload into chunks of 10,000 elements.
Uses a shared atomic cursor to assign chunks dynamically.
Each worker accumulates its assigned elements locally and stores its final result in a per-thread slot.

A worker claims a chunk using an operation equivalent to:

atomic_fetch_add(&global_index, CHUNK_SIZE);


The returned index identifies the beginning of the claimed chunk. Workers process valid indices, handle the array boundary, and continue requesting chunks until all elements have been covered.

This implementation uses an atomic work cursor, not a work-stealing queue. Workers do not steal tasks from private queues belonging to other workers.

Trade-offs

Allocates work incrementally.
Introduces atomic coordination for chunk allocation.
Performs more shared atomic operations than the static-partitioning designs.
Requires correct handling of the final partial chunk.
Does not inherently improve load balancing for a uniform-cost operation such as integer addition.

Dynamic scheduling can be useful when task execution times vary, but this implementation does not establish a load-balancing advantage or guarantee better performance than static partitioning.

Expected Results
Implementation	Work Distribution	Result Aggregation	Input Data	Expected Sum
task1_atomic.c	Static ranges	Shared atomic total	Integers 1–100,000,000	5000000050000000
task1_lockfree.c	Static ranges	Per-thread result slots	Integers 1–100,000,000	5000000050000000
task1_dynamic_lockfree.c	Dynamic chunks of 10,000	Per-thread result slots	Integers 1–100,000,000	5000000050000000

All three programs should produce the same mathematical result when they process the same input correctly and every element is counted exactly once.

Architectural Comparison
STATIC PARTITIONING
        |
        v
Assign fixed ranges to workers
        |
        v
Each worker calculates a local sum
        |
        +-----------------------------+
        |                             |
        v                             v
Shared atomic total             Per-thread result slots
        |                             |
        +--------------+--------------+
                       |
                       v
              Join all workers
                       |
                       v
                Final result


DYNAMIC PARTITIONING
        |
        v
Shared atomic work cursor
        |
        v
Workers claim 10,000-element chunks
        |
        v
Each worker calculates a local sum
        |
        v
Per-thread result slots
        |
        v
Join workers and aggregate results

Feature	task1_atomic.c	task1_lockfree.c	task1_dynamic_lockfree.c
Work allocation	Static	Static	Dynamic
Input type	uint64_t	uint64_t	uint64_t
Input values	1 to 100 million	1 to 100 million	1 to 100 million
Array size	100 million	100 million	100 million
Worker count	4	4	4
Local accumulation	Yes	Yes	Yes
Shared atomic accumulator	Yes	No	No
Atomic work cursor	No	No	Yes
Result collection	Atomic updates	Per-thread slots	Per-thread slots
Expected sum	5000000050000000	5000000050000000	5000000050000000
Benchmark Results
Linux Host

The following measurements were reported from the development host using GCC with these flags:

-O3 -std=gnu17 -Wall -Wextra -Wpedantic -pthread


The terminal prompt identified the machine as a Lenovo Yoga Slim 7 14IMH9.

Each implementation was reportedly run five times.

Implementation	Run 1 (s)	Run 2 (s)	Run 3 (s)	Run 4 (s)	Run 5 (s)	Mean (s)	Expected Sum
task1_atomic.c	0.02306	0.02686	0.02304	0.02066	0.02694	0.02411	5000000050000000
task1_dynamic_lockfree.c	0.01242	0.01218	0.01241	0.01275	0.01290	0.01253	5000000050000000
task1_lockfree.c	0.01313	0.01330	0.01272	0.01328	0.01307	0.01310	5000000050000000

The expected sums in the table are mathematical expectations, not a claim that every recorded run was independently verified.

Android / Termux

The following timings were reported from an Android device using a MediaTek Dimensity 8450 processor and Termux.

Implementation	Runs	Reported Time (s)	Expected Sum
task1_atomic.c	1	0.02097	5000000050000000
task1_dynamic_lockfree.c	1	0.13520	5000000050000000
task1_lockfree.c	1	0.01179	5000000050000000

The Android timings are single-run reports. The device model, compiler version, compiler flags, and actual output logs were not supplied with the measurements.

Interpreting the Results

These measurements are preliminary observations, not a controlled performance study.

The Linux measurements come from one host.
The Android measurements contain only one reported run per implementation.
The timing interval includes thread creation, worker execution, joining, and result aggregation.
No CPU-affinity, cache-traffic, hardware-counter, or memory-bandwidth measurements were supplied.
Equivalent input data and compiler settings must be confirmed before making direct performance comparisons.

The timings do not establish a general performance ranking across processors or architectures. For a rigorous comparison, all three programs should be rebuilt with the same workload and compiler settings, their outputs verified, and the measurements repeated.

Bonus: ARM64 Execution on Android

The author reports exploring task1_lockfree.c on an Android device powered by the MediaTek Dimensity 8450, using Termux.

This exercise investigates portability between an x86-64 Linux development environment and an ARM64 Android userspace environment.

Native Compilation with Termux

Native compilation runs the compiler directly on the Android device.

Install the compiler from Termux:

pkg update
pkg install clang


From the repository root, compile and run the program:

clang -O3 -std=c11 -Wall -Wextra \
  task1_parallel_computing/task1_lockfree.c \
  -o sum_arm64 -pthread

./sum_arm64


These commands assume the source uses the C11 threading APIs or otherwise supports the selected compiler and flags. Confirm that the actual source code and installed toolchain are compatible.

Cross-Compilation with the Android NDK

An alternative is to compile on an x86-64 Linux host using the Android NDK toolchain, then transfer the resulting executable to a compatible Android environment.

The ARM64 compiler executable commonly follows a naming pattern similar to:

aarch64-linux-android<API-level>-clang


The exact path and API-level suffix depend on the installed NDK version and configuration. Deployment also depends on Android permissions and the target execution environment.

Bionic libc and Threading Portability

Android uses Bionic libc, whereas Ubuntu typically uses glibc. Differences between these environments can affect compiler behavior, language-library availability, and linking requirements.

POSIX threads: Bionic provides pthread functionality through the system C library. Exact linking requirements depend on the toolchain.
C11 threads: Availability of <threads.h>, thrd_create(), and thrd_join() depends on the Android environment and compiler/library configuration. Verify support on the actual device.
Heterogeneous CPU scheduling: Android devices may use heterogeneous CPU cores. Scheduling and core placement can affect performance, but this repository does not measure worker placement or establish a memory-bandwidth bottleneck.

The reported Android timings are exploratory and should not be generalized to all ARM64 devices.

Task 2: SHA-256 Candidate Generation

Task 2 explores candidate generation and search-space partitioning for SHA-256 digest matching.

The repository contains five standalone programs:

Source File	Focus
password1_static.c	Static search-space partitioning
password1_dynamic.c	Dynamic search-space partitioning
password2_recursive.c	Recursive candidate generation
password2_forloops.c	Iterative candidate generation using nested loops
pass2(assumed 8 letters is password).c	Candidate search under an assumed eight-character length
Digest Comparison

The implementations compare raw SHA-256 digest bytes using memcmp() rather than converting each digest to a hexadecimal string before comparison.

This avoids hexadecimal formatting in the comparison path. However, the repository does not include a string-formatting baseline or a measured throughput comparison, so no quantitative performance improvement is claimed.

Static vs. Dynamic Search Partitioning

Static partitioning assigns predetermined portions of the search space to workers. Dynamic partitioning distributes work as workers request it.

Their relative performance depends on candidate-generation costs, chunk sizes, synchronization overhead, and how evenly the workload is distributed.

Recursive vs. Iterative Candidate Generation

Recursive generation expresses candidate construction as a traversal through successive positions in the search space. Iterative generation uses nested loops to enumerate candidates with a fixed structure.

The appropriate approach depends on candidate length, character set, implementation complexity, and resource constraints.

Measurement Status

The Task 2 programs do not currently report elapsed search times. The repository therefore documents implementation approaches rather than a measured performance comparison between the candidate-generation strategies.

Build and Execution Guide
Prerequisites

On Ubuntu 24.04 LTS or Debian-based Linux distributions, install GCC and the standard development tools:

sudo apt update
sudo apt install build-essential

Compile and Run Task 1

Run these commands from the repository root.

1. Shared atomic accumulator

gcc -std=gnu17 -O3 -Wall -Wextra -Wpedantic -pthread \
  task1_parallel_computing/task1_atomic.c \
  -o /tmp/task1_atomic

/tmp/task1_atomic


2. Per-thread result slots

gcc -std=gnu17 -O3 -Wall -Wextra -Wpedantic -pthread \
  task1_parallel_computing/task1_lockfree.c \
  -o /tmp/task1_lockfree

/tmp/task1_lockfree


3. Dynamic chunk assignment

gcc -std=gnu17 -O3 -Wall -Wextra -Wpedantic -pthread \
  task1_parallel_computing/task1_dynamic_lockfree.c \
  -o /tmp/task1_dynamic_lockfree

/tmp/task1_dynamic_lockfree


Compile each source file separately because each defines its own main() function.

Task 2 Build Requirements

The exact compilation commands depend on the headers and cryptographic library used by each source file.

If a program uses OpenSSL's SHA-256 interface, install the OpenSSL development package:

sudo apt install libssl-dev


A typical compilation command for a program that uses OpenSSL is:

gcc -O3 -std=gnu17 -Wall -Wextra -pthread \
  path/to/source.c -o /tmp/task2_program -lcrypto


Replace path/to/source.c with the appropriate source path. This command applies only if the program uses OpenSSL and its source is compatible with the installed library.

Development Environment
Primary Development Host
Laptop: Lenovo Yoga Slim 7i
Processor: Intel Core Ultra 9 185H
Architecture: x86-64
Memory: 32 GB LPDDR5X
Operating System: Ubuntu 24.04 LTS
Compiler: GCC 13.3.0
Cryptographic Library: OpenSSL 3.0
Mobile Target
Processor: MediaTek Dimensity 8450
Platform: Android
Architecture: ARM64 (aarch64)
Environment: Termux
Compiler: Clang
C Library: Android Bionic in the native Android environment
Limitations and Future Improvements

The repository focuses on implementation and architectural exploration. The following improvements would strengthen the experiments and make the results more reproducible.

Validate output correctness: Capture the actual final sum from each implementation.
Standardize benchmarks: Use identical input data, compiler flags, and timing boundaries.
Increase sample sizes: Perform more repetitions and report the mean, median, and variability.
Record system details: Include compiler versions, device details, build commands, and relevant system configuration.
Investigate cache effects: Use profiling tools or hardware performance counters rather than inferring cache behavior from alignment alone.
Measure Task 2 execution: Add elapsed-time reporting and document the search-space size and stopping conditions.
Improve portability documentation: Record the Android device details, Termux package versions, compiler flags, and actual program output.
Conclusion

These projects explore practical systems programming concepts through standalone C implementations. Task 1 examines static and dynamic work distribution, atomic coordination, local accumulation, and result aggregation using a common array workload. Task 2 explores candidate-generation strategies and parallel search-space partitioning for SHA-256 digest matching.

The emphasis is on understanding implementation trade-offs, validating correctness, exploring cross-architecture portability, and distinguishing measured observations from assumptions about performance.


