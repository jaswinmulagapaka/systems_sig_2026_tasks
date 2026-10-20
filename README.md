# WEC Systems & Security SIG 2026

## Concurrency & Systems Programming in C

**M. Jaswin** · C11 · POSIX Threads · Atomics · SHA-256 · ARM64

A collection of C programs developed for the WEC Systems & Security SIG 2026
recruitment tasks. The project explores parallel array reduction,
synchronization, dynamic work allocation, cache alignment, and multithreaded
SHA-256 candidate search.

It includes three approaches to parallel array summation, recursive and
iterative search implementations, and a native ARM64 execution experiment on
an Android device powered by the MediaTek Dimensity 8450. The focus is on
understanding concurrency trade-offs through implementation, testing, and
benchmarking.

## What I Explored

| Area | Approach |
|---|---|
| Parallel reduction | Shared atomic total vs. per-thread result slots |
| Work distribution | Static ranges vs. dynamic atomic chunk allocation |
| Synchronization | C11 atomics and cooperative termination |
| Cache alignment | `alignas(64)` and per-thread result storage |
| SHA-256 search | Static and dynamic search-space partitioning |
| Candidate generation | Recursive traversal vs. nested loops |
| Digest comparison | Raw binary comparison using `memcmp()` |
| Cross-architecture execution | Ubuntu x86-64 and Android ARM64 |

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

The first task compares three ways to sum 100 million consecutive integers
using four worker threads. Each program initializes the values from 1 through
100,000,000 and is intended to produce the same sum. The implementations vary
in how they distribute work and collect partial results.

### Common Workload

The expected mathematical result is:

```text
N = 100,000,000
sum = N(N + 1) / 2 = 5,000,000,050,000,000
```

The sum is an expected result, not independent confirmation that every
benchmark run produced it.

### Timing Methodology

The programs use `clock_gettime(CLOCK_MONOTONIC)`. Timing starts immediately
before thread creation and stops after joining the workers and collecting the
final result. Thread creation, worker execution, joining, and final aggregation
are included; array allocation and initialization are outside the timed
interval.

### `task1_atomic.c` — Shared Atomic Accumulator

Four workers receive fixed, contiguous ranges and sum their elements locally.
Each worker then performs one `atomic_fetch_add()` on a shared total.

#### Trade-offs

- Performs only four shared atomic updates, rather than one per element.
- Keeps the main accumulation loop local to each worker.
- All workers update the same atomic object at completion, which may cause
  contention.
- C11 atomics are not guaranteed to be lock-free on every target.

### `task1_lockfree.c` — Per-Thread Result Slots

Workers process fixed ranges and write their local sums to separate result
slots. The main thread adds the slots after joining the workers, so no shared
atomic accumulator is used for result collection.

The result member uses `alignas(64)` to explore cache layout. Alignment alone
does not prove false sharing is eliminated or performance improved; cache-line
size and layout depend on the target.

#### Trade-offs

- Avoids a shared atomic accumulator during result collection.
- Keeps partial sums local until each worker's final write.
- Requires a final aggregation step after all workers finish.
- Alignment may affect cache behavior, but its effect must be measured.
- The filename is retained from the original implementation; the design is
  not automatically guaranteed to be lock-free on every platform.

### `task1_dynamic_lockfree.c` — Dynamic Chunk Assignment

Workers claim 10,000-element chunks using a shared atomic cursor, accumulate
locally, and store their final sums in per-thread slots. The final chunk is
bounded by the array length. This is dynamic work allocation, not a
work-stealing queue.

Dynamic allocation adds an atomic operation per chunk. It can help with
uneven-cost work, but integer addition has uniform cost, so this design does
not inherently improve load balancing over static ranges.

#### Trade-offs

- Assigns work incrementally and handles the final partial chunk.
- Uses an atomic read-modify-write operation for each chunk claim.
- Keeps element accumulation local, then writes one result per worker.
- Adds coordination overhead compared with fixed range assignment.
- Is dynamic scheduling, not work stealing.

### Implementation Comparison

| Feature | `task1_atomic.c` | `task1_lockfree.c` | `task1_dynamic_lockfree.c` |
|---|---|---|---|
| Work distribution | Static ranges | Static ranges | Dynamic chunks (10,000) |
| Worker count | 4 | 4 | 4 |
| Array pointer element type (current source) | `uint64_t` | `uint32_t` | `uint32_t` |
| Requested allocation | ~800 MB | ~800 MB | ~800 MB |
| Result collection | Shared atomic total | Per-thread slots | Per-thread slots |
| Expected sum | `5000000050000000` | `5000000050000000` | `5000000050000000` |

The current lock-free and dynamic source files still declare their array
pointers as `uint32_t *`, although their allocation expressions use
`sizeof(uint64_t)`. Their allocations therefore request about 800 MB, but the
array accesses use 32-bit elements. The atomic implementation uses `uint64_t`
elements. All values fit in 32 bits, but the pointer types should be changed
to `uint64_t *` if the intention is for all three implementations to use
64-bit elements.

### Architectural Overview

```text
STATIC PARTITIONING
Assign fixed ranges -> local sums -> shared atomic total
                                or per-thread result slots
                 -> join workers -> final result

DYNAMIC PARTITIONING
Atomic work cursor -> claim 10,000-element chunks -> local sums
                   -> per-thread result slots -> join and aggregate
```

## Benchmark Results

### Linux Host

Five runs per implementation were reported using GCC with
`-O3 -std=gnu17 -Wall -Wextra -Wpedantic -pthread`. The terminal prompt
identified the host as a Lenovo Yoga Slim 7 14IMH9.

| Implementation | Run 1 (s) | Run 2 (s) | Run 3 (s) | Run 4 (s) | Run 5 (s) | Mean (s) |
|---|---:|---:|---:|---:|---:|---:|
| `task1_atomic.c` | 0.02306 | 0.02686 | 0.02304 | 0.02066 | 0.02694 | 0.02411 |
| `task1_dynamic_lockfree.c` | 0.01242 | 0.01218 | 0.01241 | 0.01275 | 0.01290 | 0.01253 |
| `task1_lockfree.c` | 0.01313 | 0.01330 | 0.01272 | 0.01328 | 0.01307 | 0.01310 |

### Android / Termux

One timing per implementation was reported on an Android device powered by a
MediaTek Dimensity 8450.

| Implementation | Runs | Reported time (s) |
|---|---:|---:|
| `task1_atomic.c` | 1 | 0.02097 |
| `task1_dynamic_lockfree.c` | 1 | 0.13520 |
| `task1_lockfree.c` | 1 | 0.01179 |

The Termux compiler version, flags, device configuration, and output logs were
not recorded with these timings.

### Interpreting the Results

These are reported observations, not a controlled performance study. The Linux
results are from one host, and the Termux data contains single runs. The source
files should use the same element type and allocation size before their
timings are treated as a direct comparison. Verify the outputs and repeat
runs under consistent build and system conditions before drawing broader
conclusions.

## 2. Bonus — ARM64 Execution on Android

The project includes a native execution experiment on an Android device powered
by the MediaTek Dimensity 8450, using Termux. This explores portability between
Ubuntu x86-64 and an ARM64 Android userspace.

### Native Compilation with Termux

Install Clang in Termux:

```sh
pkg update
pkg install clang
```

From the repository root, compile and run the lock-free implementation:

```sh
clang -O3 -std=c11 -Wall -Wextra \
  task1_parallel_computing/task1_lockfree.c \
  -o sum_arm64 -pthread

./sum_arm64
```

Check that the installed compiler and C library support the threading APIs
used by the source.

### Native Compilation vs. Cross-Compilation

Native compilation runs Clang directly on the Android device. Alternatively,
the Android NDK can compile on a host before the executable is transferred to
a compatible Android environment. An ARM64 NDK compiler commonly follows this
pattern:

```text
aarch64-linux-android<API-level>-clang
```

The exact path depends on the installed NDK version and configuration.

### Bionic libc and Threading

Android uses Bionic libc, while Ubuntu typically uses glibc. Threading API
availability and linking requirements depend on the toolchain and target
environment. The experiment does not measure CPU affinity, cache traffic, or
memory bandwidth, so it does not establish a hardware bottleneck.

## 3. Task 2 — SHA-256 Candidate Search

The second task explores candidate generation and search-space partitioning
using SHA-256 digest matching. The repository contains five standalone
programs:

| Source file | Focus |
|---|---|
| `password1_static.c` | Static search-space partitioning |
| `password1_dynamic.c` | Dynamic work allocation |
| `password2_recursive.c` | Recursive candidate generation |
| `password2_forloops.c` | Iterative generation using nested loops |
| `pass2(assumed 8  letters is password).c` | Search with an assumed eight-character length |

### Digest Comparison

The implementations compare raw digest bytes using `memcmp()` rather than
formatting each digest as hexadecimal first. The repository has no controlled
formatting baseline, so no measured throughput improvement is claimed.

### Search and Candidate-Generation Approaches

Static partitioning assigns predetermined search-space regions; dynamic
partitioning allocates work as workers request it. Recursive generation
traverses candidate positions, while nested loops enumerate candidates with an
explicit structure. Performance and suitability depend on the candidate
length, character set, chunk size, and synchronization overhead.

The Task 2 programs do not currently provide consistent elapsed-time
measurements, so the repository documents approaches rather than a measured
performance ranking.

## 4. Build and Execution

### Prerequisites

On Ubuntu or Debian, install the standard development tools:

```sh
sudo apt update
sudo apt install build-essential libssl-dev
```

### Task 1 — Parallel Array Sum

Run these commands from the repository root. Compile each source separately
because every program defines its own `main()` function.

```sh
gcc -std=gnu17 -O3 -Wall -Wextra -Wpedantic -pthread \
  task1_parallel_computing/task1_atomic.c -o /tmp/task1_atomic
/tmp/task1_atomic

gcc -std=gnu17 -O3 -Wall -Wextra -Wpedantic -pthread \
  task1_parallel_computing/task1_lockfree.c -o /tmp/task1_lockfree
/tmp/task1_lockfree

gcc -std=gnu17 -O3 -Wall -Wextra -Wpedantic -pthread \
  task1_parallel_computing/task1_dynamic_lockfree.c -o /tmp/task1_dynamic_lockfree
/tmp/task1_dynamic_lockfree
```

### Task 2 — SHA-256 Search

For sources that use OpenSSL's SHA-256 interface, link with `-lcrypto`:

```sh
gcc -O3 -std=gnu17 -Wall -Wextra -pthread \
  path/to/source.c -o /tmp/task2_program -lcrypto
```

Replace the source path with the program to build; the required headers and
libraries depend on the source.

## 5. Development Environment

| Component | Environment |
|---|---|
| Laptop | Lenovo Yoga Slim 7i |
| Processor | Intel Core Ultra 9 185H |
| Architecture | x86-64 |
| Memory | 32 GB LPDDR5X |
| Operating system | Ubuntu 24.04 LTS |
| Desktop compiler | GCC 13.3.0 |
| Cryptographic library | OpenSSL 3.0 |
| Android processor | MediaTek Dimensity 8450 |
| Android architecture | ARM64 (`aarch64`) |
| Android environment | Termux |
| Android compiler | Clang |
| Android C library | Bionic |

## 6. Future Improvements

- Verify and record each program's final sum.
- Make the lock-free and dynamic array pointer types `uint64_t *` to match their
  allocation expressions and the common 64-bit workload.
- Repeat benchmarks and report the median and variability as well as the mean.
- Record compiler versions, flags, device details, and actual output.
- Measure cache effects instead of inferring them from alignment.
- Add consistent elapsed-time and search-progress reporting to Task 2.
- Compare search strategies with the same character set and stopping conditions.

## Conclusion

This project explores concurrency and systems programming through practical C
implementations. The array-reduction programs examine atomic coordination,
local accumulation, and dynamic work allocation; the SHA-256 exercises explore
candidate generation and search-space partitioning.

The benchmark results and Android experiment provide a starting point for
further investigation. Concurrency strategies involve trade-offs, and
understanding them requires careful implementation and reproducible
measurements.
