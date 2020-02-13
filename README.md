# Thread-free-malloc-and-free
This repository is created for ECE650 project 1

- Constructed two versions of thread-safe malloc() and free(), supporting merging the adjacent free regions and garbage collection.

- Implemented first version with lock-based synchronization to prevent race conditions, implemented the second version with synchronization primitives and Thread-Local Storage.
