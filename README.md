# Your Parallel Encryption and Decryption

## Overview

This project demonstrates the implementation of encryption and decryption mechanisms using parallel processing techniques in C++. By leveraging both multiprocessing and multithreading, the project aims to enhance the efficiency and performance of cryptographic operations.

## Branches

The repository contains two distinct parallel processing approach:

### 1. `MultiProcessing`

**Description:** This approach showcases the use of parallel multiprocessing by creating child processes to handle encryption and decryption tasks. It utilizes the `fork()` system call to spawn child processes, enabling concurrent execution of tasks.

**Key Features:**

- **Process Management:** Implements process creation and management using `fork()`.
- **Task Queue:** Manages encryption and decryption tasks using a queue structure.
- **Task Execution:** Child processes execute tasks independently, allowing parallel processing.

### 2. `Multithreading`

**Description:** This approach focuses on multithreading combined with shared memory to perform encryption and decryption. It employs POSIX threads (`pthread`) and utilizes shared memory segments for efficient inter-thread communication.

**Key Features:**

- **Multithreading:** Implements concurrent execution using POSIX threads.
- **Shared Memory:** Utilizes shared memory for communication between threads.
- **Semaphores:** Employs semaphores to manage synchronization and ensure data consistency.

## System Architecture

The following diagram illustrates the high-level workflow used in
the parallel file encryption/decryption system:

> Tasks are passed to child workers using RAII-safe ownership transfer (`std::unique_ptr`)
> and processed in parallel using shared memory and semaphores.

<p align="center">
  <img src="images/Parallel_Cryption_architecture.png" alt="Parallel Encryption Architecture Diagram" width="90%">
</p>


## Benchmark Results

All timing and benchmarking is done using C++'s `<chrono>` library for high-precision measurement.

Here is a screenshot of the benchmark output:

![Benchmark Result](images/MultiThreading_encrypt(1).png)
![Benchmark Result](images/MultiThreading_decrypt(1).png)

For MultiThreading results:

- [MultiProcessing Encrypt](images/MultiProcessing_encrypt(1).png)
- [MultiProcessing Decrypt](images/MultiProcessing_decrypt(1).png)

## Getting Started

To explore the implementation:

   ```bash
   git clone <repo-url>
   cd EncryptDecrypt
   # Now make a virtual env and activate
   python -m venv /myvenv
   source myvenv/bin/activate
   python3 makeDirs.py
   make all
   # In case of MultiProcessing
   ./encrypty_decrypt
   # In case of MultiThreading
   ./encrypty_decrypt_mt
   # type directory name which is created from makeDirs.py
   test
   encrypt # after giving directory name, give encrypt or decrypt to tell what to do
   ```

## Future Scope

Support Multiple Encryption Algorithms-
Allow users to choose from different encryption standards (e.g., AES, ChaCha20, RSA) to balance performance and security requirements.

GPU Acceleration-
Offload heavy cryptographic operations to GPUs using CUDA or OpenCL to dramatically speed up processing of large datasets.

## 🪪 License
This project is licensed under the [MIT License](./LICENSE) © 2025 Soumalya Karak.

⭐ **If you found this project helpful, give it a star!**
