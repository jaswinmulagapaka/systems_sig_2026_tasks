# WEC Systems & Security SIG 2026 | Concurrency & Systems Programming in C

**M. Jaswin** · C11 / POSIX Threads · Atomics · Cache Coherency · Cryptographic Brute-Forcing

This repository contains my solutions and architectural explorations for the **WEC Systems & Security SIG (2026)** recruitment tasks. It consists of eight standalone C programs investigating a core systems engineering question: *when multiple threads attack the same workload, how should they partition the search space, synchronize shared state, and aggregate results without bottlenecking the CPU?*

The **Task 1 (Array Reduction)** programs compare shared atomic accumulation, cache-aligned per-thread result slots, and dynamic atomic chunk assignment. The dynamic version uses a shared atomic cursor; it is not a work-stealing queue. The repository also documents a reported ARM64 Termux exploration.

The **Task 2 (SHA-256 Cracker)** programs explore static versus dynamic prefix partitioning and recursive versus iterative candidate generation. They compare raw SHA-256 digests with `memcmp`; the repository has no string-formatting baseline or measured throughput comparison.

---

## What This Repository Explores

| Area | Implementation Focus |
| :--- | :--- |
| **Work Distribution** | Static index partitioning vs. dynamic atomic work tickets (`atomic_fetch_add`) |
| **Synchronization** | C11 atomics, memory ordering, and cooperative early-termination flags (`atomic_load`) |
| **Result Aggregation** | Shared atomic accumulator vs. thread-local reduction and post-join summation |
| **Cache Layout** | 64-byte alignment (`alignas(64)`) for result slots in one program; cache effects are not benchmarked |
| **Search Strategies** | Fixed-depth nested loops vs. recursive state-space traversal |
| **Cross-Architecture (Bonus)** | x86_64 (Ubuntu/glibc) vs. ARM64 (Android Termux/Bionic `libc`) compilation and execution |

---

## Repository Map

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

Each source file defines its own `main()` entry point and compiles independently.

---

## 1. Task 1: Parallel Array Reduction (100M Elements)

The first three programs compute the sum of 100,000,000 integers across 4 worker threads, evolving the memory access and synchronization patterns at each step.

In all three implementations, `clock_gettime(CLOCK_MONOTONIC)` starts immediately before thread creation and stops after all workers have joined and the final result has been collected. Allocation and array initialization are outside the timed interval. The timed interval includes thread creation, worker execution, joins, and result aggregation.

### `task1_atomic.c` — Local Accumulation with Shared Atomic Total
* **Design:** Each of the 4 C11 worker threads receives a fixed, contiguous 25M-element slice of a `uint64_t` array (~800 MB). Each worker accumulates its slice locally and performs a single `atomic_fetch_add()` on the global total after processing its range.
* **Trade-off:** The program performs four shared atomic read-modify-write updates, rather than one per array element. These updates may contend for the same atomic object's cache line if they overlap; this is not mutex lock contention, and no performance result is claimed.

### `task1_lockfree.c` — Cache-Aligned Per-Thread Slots
* **Design:** Eliminates atomic instructions from the reduction path entirely. Each worker writes its final local sum into a dedicated slot in a global results array (`final_answers[thread_id]`), and `main()` performs a final sequential sweep after `thrd_join()`.
* **Cache layout:** The result member uses `alignas(64)`. This gives the member 64-byte alignment, but the program does not query the target's cache-line size or measure cache traffic. Treat it as an alignment experiment, not a demonstrated false-sharing or performance result.

### `task1_dynamic_lockfree.c` — Dynamic Chunk Assignment
* **Design:** Replaces static range slicing with a shared atomic work cursor. Workers repeatedly claim 10,000-element chunks via `atomic_fetch_add(&global_index, CHUNK_SIZE)`, sum the chunk locally, and repeat until all 10,000 elements-per-chunk claims are exhausted. Four additional claims terminate the workers after the cursor passes the array end.
* **Trade-off Analysis:** Each worker claims chunks dynamically. With uniform-cost integer addition, the source does not demonstrate a load-balancing advantage over static partitioning. The cursor adds atomic read-modify-write coordination; no measured cost or speedup is claimed.

### Architectural Comparison

```text
STATIC PARTITIONING                      DYNAMIC PARTITIONING
Assign fixed 25M index ranges            Shared atomic chunk cursor (10k elements)
             |                                        |
             v                                        v
   Worker-local register sum               Workers claim chunks dynamically
             |                                        |
             v                                        v
Shared atomic add OR aligned(64) slots     Worker-local totals -> result slots
             |                                        |
             v                                        v
   Join workers & finish                    Join workers & aggregate in main()
```

| Program | Element Type | Heap Footprint | Expected Mathematical Sum |
| :--- | :--- | :--- | :--- |
| `task1_atomic.c` | `uint64_t` (1 to 100M) | ~800 MB | `5,000,000,050,000,000` |
| `task1_lockfree.c` | `uint32_t` (all `50`s) | ~400 MB | `5,000,000,050,000` |
| `task1_dynamic_lockfree.c` | `uint32_t` (all `1`s) | ~400 MB | `5,000,000,050,000` |

### Benchmark Status

#### Linux (host)

Five runs per implementation, compiled with GCC using
`-O3 -std=gnu17 -Wall -Wextra -Wpedantic -pthread`. The terminal prompt
identifies the machine as a Lenovo Yoga Slim 7 14IMH9. Times are in seconds.

| Implementation | Run 1 | Run 2 | Run 3 | Run 4 | Run 5 | Mean | Final sum |
| :--- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `task1_atomic.c` | 0.02306 | 0.02686 | 0.02304 | 0.02066 | 0.02694 | **0.02411** | `5000000050000000` |
| `task1_dynamic_lockfree.c` | 0.01242 | 0.01218 | 0.01241 | 0.01275 | 0.01290 | **0.01253** | `5000000050000000` |
| `task1_lockfree.c` | 0.01313 | 0.01330 | 0.01272 | 0.01328 | 0.01307 | **0.01310** | `5000000050000000` |

#### Android / Termux

One reported run per implementation; times are in seconds. Compiler, flags,
device details, and final-sum output were not provided.

| Implementation | Runs | Time (seconds) |
| :--- | ---: | ---: |
| `task1_atomic.c` | 1 | 0.02097 |
| `task1_dynamic_lockfree.c` | 1 | 0.1352 |
| `task1_lockfree.c` | 1 | 0.01179 |

The Linux timings are from one host, and the Termux timings are single-run
reports; neither should be generalized. The implementations have different
data and work-distribution details, so compare their timings in that context.

---

## 2. Bonus Task: ARM64 Execution on Android via Termux

The author reports running `task1_lockfree.c` on an Android smartphone (ARM64 / `aarch64`) using **Termux**. See the benchmark table above for the reported timings; device details and reproducible output have not yet been added.

### Native Compilation (Termux) vs. Cross-Compilation (NDK)
There are two ways to build C binaries for Android:

1. **Cross-Compilation:** Compiling on an x86_64 Linux host using the Android NDK toolchain (`aarch64-linux-android-clang`) and pushing the binary via `adb push` to `/data/local/tmp`.
2. **Native Compilation:** Running an ARM64 build of `clang` directly inside the Termux environment on the phone's CPU.

### The Bionic `libc` & Threading Quirks
Android and Ubuntu use different C libraries and may have different compiler/runtime support. Verify details against the specific Android device and Termux toolchain:

* **Bionic `libc` Integration:** Unlike desktop Linux—where POSIX threads historically lived in a separate library requiring `-lpthread`—Android uses Google's **Bionic `libc`**, which bakes `pthread` directly into `libc.so`.
* **C11 `<threads.h>` vs. `<pthread.h>`:** Older revisions of Bionic lacked full C11 `<threads.h>` (`thrd_create` / `thrd_join`) wrappers, making POSIX `<pthread.h>` the more portable standard across mobile toolchains, though modern Termux `clang` environments provide full support.
* **Heterogeneous Core Scheduling:** The Android scheduler and SoC may have heterogeneous cores. The repository has no measurements showing worker placement or identifying memory bandwidth as a bottleneck.

### Termux Build & Execution Commands (Reported; Not Reproduced Here)

```bash
# Inside Termux (aarch64-linux-android)
clang -O3 -std=c11 task1_lockfree.c -o sum_arm64 -lpthread
./sum_arm64
```

---

## 3. Task 2: SHA-256 Candidate-Generation Exercises

The repository contains five Task 2 programs: `password1_static.c`,
`password1_dynamic.c`, `password2_recursive.c`, `password2_forloops.c`, and
`pass2(assumed 8  letters is password).c`. This README leaves out their
implementation details and build/run commands. These sources do not print
elapsed search times; timing instrumentation is limited to the three array
reduction programs.

---

## 4. Build & Execution Guide

### Prerequisites (Ubuntu 24.04 LTS / Debian)
Ensure `gcc` and POSIX pthread support are available:

```bash
sudo apt update
sudo apt install build-essential
```

### Compiling Task 1 (Parallel Array Sum)

```bash
# 1. Shared Atomic Accumulator
gcc -std=gnu17 -O3 -Wall -Wextra -pthread \
  task1_parallel_computing/task1_atomic.c -o /tmp/task1_atomic && /tmp/task1_atomic

# 2. Cache-Aligned Per-Thread Slots
gcc -std=gnu17 -O3 -Wall -Wextra -pthread \
  task1_parallel_computing/task1_lockfree.c -o /tmp/task1_lockfree && /tmp/task1_lockfree

# 3. Dynamic Chunk Queue
gcc -std=gnu17 -O3 -Wall -Wextra -pthread \
  task1_parallel_computing/task1_dynamic_lockfree.c -o /tmp/task1_dynamic_lockfree && /tmp/task1_dynamic_lockfree
```

---

## 5. Development Environment

* **Primary Host:** Lenovo Yoga Slim 7i · Intel Core Ultra 9 185H (x86_64) · 32 GB LPDDR5X · Ubuntu 24.04 LTS (`gcc 13.3.0`, OpenSSL 3.0)
* **Mobile Target (Bonus Task):** Android   (ARM64 / `aarch64`) · Termux (`clang`, Bionic `libc`)
