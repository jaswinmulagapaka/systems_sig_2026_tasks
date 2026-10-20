# WEC Systems & Security SIG 2026

## Concurrency & Systems Programming in C

**M. Jaswin** · C11 · POSIX Threads · Atomics · SHA-256 · ARM64

A collection of C programs developed for the WEC Systems & Security SIG 2026
recruitment tasks. The project explores parallel array reduction, atomic
synchronization, dynamic work allocation, cache alignment, and multithreaded
SHA-256 candidate search.

It includes three approaches to parallel array summation, recursive and
iterative search implementations, and a native ARM64 execution experiment on
an Android device powered by the MediaTek Dimensity 8450.

The focus is on understanding the trade-offs between concurrency strategies
through implementation, testing, and benchmarking.

## What I Explored

| Area | Approach |
|---|---|
| Parallel Reduction | Shared atomic total vs. per-thread result slots |
| Work Distribution | Static ranges vs. dynamic atomic chunk allocation |
| Synchronization | C11 atomics and cooperative termination |
| Cache Alignment | `alignas(64)` and per-thread result storage |
| SHA-256 Search | Static and dynamic search-space partitioning |
| Candidate Generation | Recursive traversal vs. nested loops |
| Digest Comparison | Raw binary digest comparison using `memcmp()` |
| Cross-Architecture Execution | Ubuntu x86-64 and Android ARM64 |

## Repository Structure

```text
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
```

Each source file contains its own `main()` function and is compiled
independently.

## 1. Task 1 — Parallel Array Reduction

The first task investigates three ways to sum an array of 100 million integers
using four worker threads. The implementations differ in how work is assigned
and how partial results are collected.

### Common Workload

All three implementations are intended to process the same sequence of
integers, from 1 to 100,000,000.

- **Array size:** 100,000,000 elements
- **Element type:** `uint64_t`
- **Memory footprint:** Approximately 800 MB (decimal)
- **Worker threads:** 4
- **Expected sum:** `5000000050000000`

The expected result follows from:

```text
S = N(N + 1) / 2

For N = 100,000,000:
S = 5000000050000000
```

The source files should be checked to confirm that they all use this input
sequence and process every element exactly once.

### Timing Methodology

The programs use `clock_gettime(CLOCK_MONOTONIC)` to measure elapsed
wall-clock time.

The timer starts immediately before thread creation and stops after the worker
threads have joined and the final result has been collected.

The measured interval includes:

- Thread creation
- Worker execution
- Thread joining
- Final result aggregation

Array allocation and initialization are performed before timing begins.

### `task1_atomic.c` — Shared Atomic Accumulator

Each worker receives a fixed, contiguous range of 25 million elements and
calculates its partial sum locally.

After processing its range, each worker performs one `atomic_fetch_add()`
operation to update the shared total.

The main idea is to keep atomic operations out of the element-processing loop.
Instead of performing an atomic update for every element, the program performs
only four final updates.

#### Trade-offs

- Reduces shared atomic updates to four.
- Keeps the main accumulation loop local to each worker.
- All workers update the same atomic object at completion, potentially
  causing contention if their updates overlap.
- Uses a shared total that must be initialized correctly before the workers
  start.

C11 atomics do not automatically guarantee a lock-free implementation on
every architecture; that depends on the atomic type and target implementation.

### `task1_lockfree.c` — Per-Thread Result Slots

This implementation uses static partitioning but changes how partial sums are
collected.

Each worker calculates its local sum and writes it to a dedicated result slot.
After all workers finish, the main thread sums the four partial results.

This removes the shared atomic accumulator from the reduction path.

#### Cache Alignment

The result structure uses `alignas(64)` to request 64-byte alignment.

The motivation is to explore whether separating worker result slots in memory
can reduce false sharing. If multiple cores repeatedly modify data on the same
cache line, cache-coherency traffic can become a source of overhead.

However, 64-byte alignment alone does not prove that false sharing has been
eliminated. The actual layout and target cache-line size matter, and the
performance impact needs to be measured.

#### Trade-offs

- Avoids a shared atomic accumulator during result collection.
- Keeps each worker's partial sum local until the final write.
- Requires a final reduction after joining the workers.
- Introduces explicit result storage for each worker.

The filename is retained from the original implementation; whether an
implementation is genuinely lock-free depends on the operations it uses and
the target platform.

### `task1_dynamic_lockfree.c` — Dynamic Chunk Assignment

Instead of assigning fixed ranges, this implementation uses a shared atomic
cursor to distribute chunks of 10,000 elements.

Workers claim work through an operation equivalent to:

```c
atomic_fetch_add(&current_index, CHUNK_SIZE);
```

Each worker processes its claimed chunk, adds the values to a local sum, and
requests another chunk until the array has been covered. The final chunk must
be bounded by the array length.

This is dynamic chunk allocation using an atomic work cursor, not a
work-stealing queue. Workers claim work from a shared counter rather than
stealing tasks from other workers' private queues.

#### Trade-offs

- Distributes work incrementally.
- Requires an atomic read-modify-write operation for each chunk claim.
- Keeps element accumulation local to each worker.
- Adds coordination overhead compared with fixed range assignment.
- Does not inherently improve load balancing for uniform-cost integer
  addition.

Dynamic allocation is useful when work items have different execution costs.
In this array reduction, each element requires approximately the same
operation, so static partitioning already provides a straightforward way to
divide the workload evenly.

### Implementation Comparison

| Feature | `task1_atomic.c` | `task1_lockfree.c` | `task1_dynamic_lockfree.c` |
|---|---|---|---|
| Work distribution | Static | Static | Dynamic |
| Worker count | 4 | 4 | 4 |
<<<<<<< HEAD
| Intended input | Integers 1–100M | Integers 1–100M | Integers 1–100M |
| Element type | `uint64_t` | `uint64_t` | `uint64_t` |
| Local accumulation | Yes | Yes | Yes |
| Shared atomic total | Yes | No | No |
| Atomic work cursor | No | No | Yes |
| Result collection | Atomic updates | Per-thread slots | Per-thread slots |
=======
| Array pointer element type (current source) | `uint64_t` | `uint64_t` | `uint64_t` |
| Requested allocation | ~800 MB | ~800 MB | ~800 MB |
| Result collection | Shared atomic total | Per-thread slots | Per-thread slots |
>>>>>>> de23b7bc0bfc82f5761f78c45794a880a5406bbc
| Expected sum | `5000000050000000` | `5000000050000000` | `5000000050000000` |

### Architectural Overview

```text
STATIC PARTITIONING
        |
        v
Assign fixed ranges to workers
        |
        v
Calculate local partial sums
        |
        +----------------------------+
        |                            |
        v                            v
Shared atomic total            Per-thread result slots
        |                            |
        +-------------+--------------+
                      |
                      v
                Join workers
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
Calculate local partial sums
        |
        v
Per-thread result slots
        |
        v
Join workers and aggregate
```

## Benchmark Results — Linux

The following timings were recorded on the Linux development host. The
reported compiler flags were:

```text
-O3 -std=gnu17 -Wall -Wextra -Wpedantic -pthread
```

Five runs were reported for each implementation.

| Implementation | Run 1 (s) | Run 2 (s) | Run 3 (s) | Run 4 (s) | Run 5 (s) | Mean (s) |
|---|---:|---:|---:|---:|---:|---:|
| `task1_atomic.c` | 0.02306 | 0.02686 | 0.02304 | 0.02066 | 0.02694 | 0.02411 |
| `task1_dynamic_lockfree.c` | 0.01242 | 0.01218 | 0.01241 | 0.01275 | 0.01290 | 0.01253 |
| `task1_lockfree.c` | 0.01313 | 0.01330 | 0.01272 | 0.01328 | 0.01307 | 0.01310 |

These are the reported measurements, not independently reproduced results.
The actual source code and output should be checked before treating the runs
as verified comparisons of identical workloads.

## Benchmark Results — Android / Termux

One run per implementation was reported on an Android device powered by the
MediaTek Dimensity 8450.

| Implementation | Runs | Reported Time (s) |
|---|---:|---:|
| `task1_atomic.c` | 1 | 0.02097 |
| `task1_dynamic_lockfree.c` | 1 | 0.13520 |
| `task1_lockfree.c` | 1 | 0.01179 |

The Android compiler flags, compiler version, device configuration, and actual
output logs were not recorded alongside these timings.

### Interpreting the Results

The Linux timings show that the dynamic implementation had the lowest
reported mean among these runs, followed closely by the per-thread slot
implementation. On Android, the per-thread slot implementation had the
lowest reported single-run time.

These observations are specific to the recorded experiments. They do not
establish a universal ranking because the Android data consists of single
runs, and the workload and build settings must be verified before making a
direct comparison.

For a more reliable comparison, all three implementations should be rebuilt
with identical input data and compiler settings, their results verified, and
the measurements repeated under consistent conditions.

## 2. Bonus — ARM64 Execution on Android

I also explored running the array reduction program natively on an Android
device powered by the MediaTek Dimensity 8450, using Termux.

The goal was to move beyond the usual desktop Linux environment and
investigate whether the C implementation could be compiled and executed in an
ARM64 Android userspace.

### Native Compilation with Termux

The Android build uses Clang rather than the GCC toolchain used on the Linux
development host.

Install Clang inside Termux:

```sh
pkg update
pkg install clang
```

From the repository root, compile and run the program:

```sh
clang -O3 -std=c11 -Wall -Wextra \
  task1_parallel_computing/task1_lockfree.c \
  -o sum_arm64 -pthread

./sum_arm64
```

This command assumes the source is compatible with the C11 threading support
available in the installed environment.

### Native Compilation vs. Cross-Compilation

There are two common ways to build C programs for Android:

- **Native compilation:** Compile directly on the Android device using
  Termux and its installed Clang toolchain.
- **Cross-compilation:** Compile on a host machine using the Android NDK
  toolchain, then transfer the binary to a compatible Android environment.

An NDK ARM64 compiler commonly follows a naming pattern similar to:

```text
aarch64-linux-android<API-level>-clang
```

The actual compiler path depends on the installed NDK version and
configuration.

### Bionic libc and Threading

Android uses Bionic libc, whereas Ubuntu normally uses glibc. Differences
between the environments can affect language-library availability, threading
APIs, and linking requirements.

- **POSIX threads:** Android provides pthread functionality through Bionic.
- **C11 threads:** Availability of `<threads.h>`, `thrd_create()`, and
  `thrd_join()` depends on the compiler and target environment.
- **CPU scheduling:** The operating system controls how worker threads are
  scheduled across the available CPU cores.

The experiment does not include CPU-affinity measurements, hardware
performance counters, or memory-bandwidth profiling. Therefore, the results
should be treated as an exploratory portability exercise rather than evidence
of a specific CPU or memory bottleneck.

## 3. Task 2 — Multithreaded SHA-256 Candidate Search

The second task explores candidate generation and SHA-256 digest matching
using OpenSSL's cryptographic library.

The repository contains five implementations:

| Source File | Focus |
|---|---|
| `password1_static.c` | Static search-space partitioning |
| `password1_dynamic.c` | Dynamic work allocation |
| `password2_recursive.c` | Recursive candidate generation |
| `password2_forloops.c` | Iterative candidate generation using nested loops |
| `pass2(assumed 8 letters is password).c` | Search based on an assumed eight-character password |

### Raw Digest Comparison

A SHA-256 digest contains 32 bytes. A hexadecimal representation contains 64
characters.

Instead of formatting each generated digest as a hexadecimal string before
comparing it, the implementations use `memcmp()` to compare raw digest bytes.

The intended approach is to decode the target hexadecimal digest once before
the search begins and compare each generated digest directly against the
decoded target.

This avoids repeated string formatting in the candidate-processing loop.
However, the repository does not contain a controlled benchmark comparing raw
binary comparison against hexadecimal formatting, so no measured throughput
improvement is claimed.

### Static vs. Dynamic Search Partitioning

Static partitioning assigns predetermined portions of the search space to
workers. Dynamic partitioning allocates work as workers request it.

Static partitioning can reduce coordination overhead, while dynamic
allocation can be useful when task execution costs vary. The relative
performance depends on candidate-generation cost, chunk size, synchronization,
and the search-space structure.

### Recursive vs. Iterative Candidate Generation

The recursive implementation constructs candidates by traversing successive
positions in the search space. The nested-loop implementation enumerates
candidates using explicit loops.

Both approaches can explore a candidate space, but their suitability depends
on the candidate length, character set, and implementation complexity.

### Eight-Character Search Experiment

The eight-character mixed-case search explores the 52-letter ASCII alphabet.

The total number of candidates is:

```text
52^8 = 53,459,728,531,456
```

password 1 is threadz 
password2 i ran the code for 15hrs but did not get any output though my 8 cores were running the program did not crash 
           the cores were not fixed as i dint uses any thread pining 
           so linux CFS was changing my threads(software) to run on differnt cores 


### Measurement Status

The Task 2 programs do not currently provide a consistent set of
elapsed-time measurements or verified throughput comparisons. The repository
therefore focuses on implementation strategies and candidate-generation
approaches rather than claiming a measured performance ranking.

## 4. Build and Execution

### Prerequisites

On Ubuntu or Debian:

```sh
sudo apt update
sudo apt install build-essential libssl-dev
```

### Task 1 — Parallel Array Sum

Run these commands from the repository root.

#### Shared atomic accumulator

```sh
gcc -std=gnu17 -O3 -Wall -Wextra -Wpedantic -pthread \
  task1_parallel_computing/task1_atomic.c \
  -o /tmp/task1_atomic && /tmp/task1_atomic
```

#### Per-thread result slots

```sh
gcc -std=gnu17 -O3 -Wall -Wextra -Wpedantic -pthread \
  task1_parallel_computing/task1_lockfree.c \
  -o /tmp/task1_lockfree && /tmp/task1_lockfree
```

#### Dynamic chunk assignment

```sh
gcc -std=gnu17 -O3 -Wall -Wextra -Wpedantic -pthread \
  task1_parallel_computing/task1_dynamic_lockfree.c \
  -o /tmp/task1_dynamic_lockfree && /tmp/task1_dynamic_lockfree
```

### Task 2 — SHA-256 Search

These commands assume the source files use OpenSSL's SHA-256 interfaces and
the corresponding headers.

#### Password 1 — Static partitioning

```sh
gcc -std=gnu17 -O3 -Wall -Wextra -Wpedantic -pthread \
  task2_parallel_computing/password1_static.c \
  -o /tmp/password1_static -lcrypto
```

#### Password 1 — Dynamic allocation

```sh
gcc -std=gnu17 -O3 -Wall -Wextra -Wpedantic -pthread \
  task2_parallel_computing/password1_dynamic.c \
  -o /tmp/password1_dynamic -lcrypto
```

#### Password 2 — Recursive generation

```sh
gcc -std=gnu17 -O3 -Wall -Wextra -Wpedantic -pthread \
  task2_parallel_computing/password2_recursive.c \
  -o /tmp/password2_recursive -lcrypto
```

#### Password 2 — Nested loops

```sh
gcc -std=gnu17 -O3 -Wall -Wextra -Wpedantic -pthread \
  task2_parallel_computing/password2_forloops.c \
  -o /tmp/password2_forloops -lcrypto
```

#### Password 2 — Case-variant search

```sh
gcc -std=gnu17 -O3 -Wall -Wextra -Wpedantic \
  'task2_parallel_computing/pass2(assumed 8  letters is password).c' \
  -o /tmp/pass2_fast -lcrypto
```

Compile each source file independently because each contains its own `main()`
function. If a build fails, check the source includes, threading API, and
OpenSSL interface used by that program.

## 5. Development Environment

### Linux Development Host

- **Laptop:** Lenovo Yoga Slim 7i
- **Processor:** Intel Core Ultra 9 185H
- **Architecture:** x86-64
- **Memory:** 32 GB LPDDR5X
- **Operating system:** Ubuntu 24.04 LTS
- **Compiler:** GCC 13.3.0
- **Cryptographic library:** OpenSSL 3.0

### Android Mobile Target

- **Processor:** MediaTek Dimensity 8450
- **Architecture:** ARM64 (`aarch64`)
- **Environment:** Android / Termux
- **Compiler:** Clang
- **C library:** Bionic

## 6. Future Improvements

The next steps for these experiments are:
- use thread pining 
- Repeat the benchmarks and report the median and variability alongside the
  mean.
- Record compiler flags, device details, and actual program output for
  reproducibility.
- Measure cache behaviour rather than inferring it from alignment alone.
- Add consistent elapsed-time and search-progress reporting to Task 2.
- Compare candidate-generation strategies using the same character set,
  candidate ordering, and stopping conditions.

## Conclusion

This project gave me an opportunity to explore concurrency and systems
programming through practical C implementations. The array reduction
programs examine atomic coordination, local accumulation, and dynamic work
allocation, while the SHA-256 exercises explore parallel candidate generation
and search-space partitioning.

