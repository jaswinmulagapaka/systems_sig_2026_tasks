# WEC Systems & Security SIG 2026

**M. Jaswin** · C11 · POSIX Threads · Atomics · SHA-256 · ARM64


## What I Explored

| Area | Approach |
|---|---|
| Parallel Reduction | Shared atomic total vs. per-thread result slots |
| Work Distribution | Static ranges vs. dynamic  allocation |
| Synchronization | C11 atomics and cooperative termination |
| Cache Alignment | `alignas(64)` and array slot  |
| SHA-256 Search | Static and dynamic versions of code  |
| permutations  | recursions vs for loops  |
| comparisions  | memcmp vs strcmp and sprintf |
| Cross-compiling | Ubuntu x86-64 and Android ARM64 |


each of my file has its own main()

## 1. Task 1 — Parallel Array Reduction
sum of 100 million elements 


### Common Workload
800MB array same input in all the cases 1 to 100Million 

### Timing 
clock starts right before threads are created and stops right afer thrd_join 

### `task1_atomic.c` — Shared Atomic Accumulator
i used atomic_fetch_add once in every summation that is thread after each thread has done its summation to prevent contention 
as cpu might take more time in managing than doing the actual math 

hence only 4 atomic updates no need to worry about  100Million lock contentions 


not all atomic updates use lock neither are all lock free it depends on cpu architecture u can actually find this out by using atomic_is_lock_free


### `task1_lockfree.c` — uses a array to store local sum 

i used an array to be precise a structure using alignment to make it 64 byte  so that it fits in cpu cache line so that i can prevent false sharing 

#### Cache Alignment

T `alignas(64)`  is a request to give 64 bytes memory to that variable 
 actually false sharing is a concequency of cache coherancy protocol mesi it which though they are updating diff variables if they lie in the same 
 cache line(think of it as a car which moves across the l1 l2 l3 ram to bring stuff) 
 so if 2 diff variables are in the same cache line mesi invalidates the variable in other threads 


### `task1_dynamic_lockfree.c` — Dynamic Chunk Assignment
i came to this approach after i got to know about the dynamic version of password 1 
to be honest this is not much usefull here as addition is not such a big task to have variable workloads across threads
the idea is that to make every thread do something insted of still idle after comleting their work 
more simply strong people help weaker people 
it doesnt mean that strong thread steals work from weak threads but it takes the next chunk of work avaliable 
i delibrately dint prevent false sharing here 
tbh its not a  big deal as only 4 atomic operations at most if all happen to complete their work simulltaneously 

##  testcase Results — Linux

i just tried to make a table to show which works fast 

intel core ultra 9 185H

Five runs were reported for each implementation.

| Implementation | Run 1 (s) | Run 2 (s) | Run 3 (s) | Run 4 (s) | Run 5 (s) | Mean (s) |
|---|---:|---:|---:|---:|---:|---:|
| `task1_atomic.c` | 0.02306 | 0.02686 | 0.02304 | 0.02066 | 0.02694 | 0.02411 |
| `task1_dynamic_lockfree.c` | 0.01242 | 0.01218 | 0.01241 | 0.01275 | 0.01290 | 0.01253 |
| `task1_lockfree.c` | 0.01313 | 0.01330 | 0.01272 | 0.01328 | 0.01307 | 0.01310 |

dynamic happend to be the fastest tbg idk y that happend i thought lock_free is going to be fastest as workload almost similar 

## Benchmark Results — Android / Termux
mediatek dimensity 8450 

| Implementation | Runs | Reported Time (s) |
|---|---:|---:|
| `task1_atomic.c` | 1 | 0.02097 |
| `task1_dynamic_lockfree.c` | 1 | 0.13520 |
| `task1_lockfree.c` | 1 | 0.01179 |
here lock free was the fastest 

## 2. Bonus — task cross compiling 
 i used termux native compilation which uses clang under the hood but command alias is gcc which points to clang and bionic libc 
 
 i dint use android ndk as its difficult i tried learning but i dint understand it 
 but i know that it uses bionic libc and makes them .so file (shared object ) so that gradle a google tool can pack them into .apk files and move to play store 


## 3. Task 2 — Multithreaded SHA-256 Candidate Search
 
cracking sha 256 

| Source File | Focus |
|---|---|
| `password1_static.c` | Static search-space partitioning |
| `password1_dynamic.c` | Dynamic work allocation |
| `password2_recursive.c` | Recursive candidate generation |
| `password2_forloops.c` | Iterative candidate generation using nested loops |
| `pass2(assumed 8 letters is password).c` | Search based on an assumed eight-character password |

### SHA_256 
its a mathematical operation 
what is does is it take a string and makes packages of 64 bytes if string is not 64 bytes it uses padding and if its more than 64 bytes it make more than 1 package 
and runs 64 mathematical operations or simply grinds them and pour them finally into 32 bytes package which cant be reverse engineered only way is brute force 



### Static vs. Dynamic Search Partitioning

 static is like assiging  fixed amount of work to each thread i did this after 2 versions of 100million sum task 
 later i got to know about dynamic work load my gemini(which helped me a lot )  so i implemented dynamic work load by using a global counter and assinging starting letter to each thread 
 

### Recursive vs. Iterative Candidate Generation
first i implemented iteration which resulted in 52^8 candidates to check 
then i asked gemini can i use backtracing reccursions technique which i learned in hacker rank c problem while i was learning c 
that thing is really hard it took so many hours to do understand the logic 
  
password 1 took around 5 mins to give answer as threadz  i dint use clock here as i thought it might take hours 
password 2 i ran over night around 13.5 hrs but it dint gave any answer so  i stopped in between the laptop became really hot during that time 
then i for fun assumed the password2 might be the word password all 56 combinations and ran for fun but its not the answer 



       
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


## Conclusion
i learned a  lot about cpu hardware cpu architectures and all as i am an ece student i am intrested in the intersection of software and hardware so i tried this task 
please ignore my spelling mistakes

