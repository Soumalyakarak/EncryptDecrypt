#pragma once
#include <chrono>
#include <iostream>
#include <iomanip>
#include <string>
#include <atomic>
#include <thread>
#include <mutex>
#include <fstream>

class BenchmarkLogger2 {
private:
    std::string operation_name;
    std::chrono::steady_clock::time_point start_time;
    std::thread::id main_thread_id;

    // Atomic counters (shared across threads)
    static std::atomic<size_t> files_processed;
    static std::atomic<size_t> files_successful;
    static std::atomic<size_t> files_failed;
    static std::atomic<size_t> total_bytes;
    static std::atomic<int> crypto_operations_completed;
    
    // Mutex for thread-safe output
    static std::mutex output_mutex;

public:
    BenchmarkLogger2(const std::string& operation = "Multithreaded Crypto Operations")
        : operation_name(operation), main_thread_id(std::this_thread::get_id()) {
        
        start_time = std::chrono::steady_clock::now();
        
        // Reset counters
        files_processed.store(0);
        files_successful.store(0);
        files_failed.store(0);
        total_bytes.store(0);
        crypto_operations_completed.store(0);
        
        std::lock_guard<std::mutex> lock(output_mutex);
        std::cout << "\n=== ENCRYPTDECRYPT BENCHMARK START (THREADS) ===" << std::endl;
        std::cout << "Operation: " << operation_name << std::endl;
        std::cout << "Main Thread ID: " << main_thread_id << std::endl;
        std::cout << "CPU Cores: " << std::thread::hardware_concurrency() << std::endl;
        std::cout << "Clock: steady_clock (nanosecond precision)" << std::endl;
        std::cout << "========================================" << std::endl;
    }

    ~BenchmarkLogger2() {
        // Only main thread prints final report
        if (std::this_thread::get_id() == main_thread_id) {
            print_final_crypto_report();
        }
    }

    std::thread::id getMainThreadID() const { return main_thread_id; }

    static void record_file_operation(const std::string& filepath, bool success) {
        files_processed.fetch_add(1);

        if (success) {
            files_successful.fetch_add(1);
        } else {
            files_failed.fetch_add(1);
            std::lock_guard<std::mutex> lock(output_mutex);
            std::cout << "[TID:" << std::this_thread::get_id() << "] FAILED: " << filepath << std::endl;
        }

        // Progress every 50 files
        size_t count = files_processed.load();
        if (count > 0 && count % 50 == 0) {
            std::lock_guard<std::mutex> lock(output_mutex);
            std::cout << "[PROGRESS] " << count << " files submitted for processing..." << std::endl;
        }
    }

    static void record_crypto_completion(const std::string& filepath, bool encrypt_mode) {
        int completed = crypto_operations_completed.fetch_add(1) + 1;
        std::thread::id tid = std::this_thread::get_id();

        // Try to get file size safely (file should be closed and complete now)
        try {
            std::ifstream file(filepath, std::ios::binary | std::ios::ate);
            if (file.is_open()) {
                std::streamsize file_size = file.tellg();
                file.close();
                
                if (file_size > 0 && file_size < 1000000000) { // Sanity check: < 1GB
                    total_bytes.fetch_add(static_cast<size_t>(file_size));
                }
            }
        } catch (...) {
            // Silently continue if file size can't be determined
        }

        // Progress every 25 crypto operations
        if (completed % 25 == 0) {
            std::lock_guard<std::mutex> lock(output_mutex);
            std::cout << "[TID:" << tid << "] Completed " << completed 
                      << " crypto operations (" << (encrypt_mode ? "ENCRYPT" : "DECRYPT") << ")" << std::endl;
        }
    }

    template<typename Func>
    static auto time_crypto_operation(const std::string& filepath, bool encrypt_mode, Func&& crypto_func)
        -> decltype(crypto_func()) {
        
        std::thread::id tid = std::this_thread::get_id();
        auto start = std::chrono::steady_clock::now();
        auto result = crypto_func();
        auto end = std::chrono::steady_clock::now();
        auto duration_us = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

        record_crypto_completion(filepath, encrypt_mode);

        if (duration_us.count() > 1000) {
            std::lock_guard<std::mutex> lock(output_mutex);
            std::cout << "[TID:" << tid << "] "
                      << (encrypt_mode ? "ENCRYPT" : "DECRYPT") << " "
                      << filepath << ": " << duration_us.count() << "μs" << std::endl;
        }

        return result;
    }

    static void log(const std::string& message) {
        std::lock_guard<std::mutex> lock(output_mutex);
        std::cout << "[TID:" << std::this_thread::get_id() << "] " << message << std::endl;
    }

private:
    void print_final_crypto_report() {
        auto end_time = std::chrono::steady_clock::now();
        auto total_duration_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time);
        double duration_sec = total_duration_ns.count() / 1e9;

        // Load all values before printing to avoid race conditions
        size_t total_files = files_processed.load();
        size_t successful = files_successful.load();
        size_t failed = files_failed.load();
        size_t bytes = total_bytes.load();
        int crypto_completed = crypto_operations_completed.load();

        std::lock_guard<std::mutex> lock(output_mutex);
        
        std::cout << "\n";
        std::cout << "================================" << std::endl;
        std::cout << "   ENCRYPTDECRYPT BENCHMARK RESULTS" << std::endl;
        std::cout << "================================" << std::endl;
        std::cout << "Operation: " << operation_name << std::endl;
        std::cout << "Total Duration: " << std::fixed << std::setprecision(6) << duration_sec << " seconds" << std::endl;
        std::cout << "Duration (ns): " << total_duration_ns.count() << " nanoseconds" << std::endl;

        std::cout << "\nFILE PROCESSING:" << std::endl;
        std::cout << "Files Submitted: " << total_files << std::endl;
        std::cout << "Files Successful: " << successful << " (✓)" << std::endl;
        std::cout << "Files Failed: " << failed << " (✗)" << std::endl;
        std::cout << "Crypto Operations Completed: " << crypto_completed << std::endl;

        if (total_files > 0) {
            double success_rate = (double(successful) / total_files) * 100.0;
            std::cout << "Success Rate: " << std::fixed << std::setprecision(1) << success_rate << "%" << std::endl;
        } else {
            std::cout << "Success Rate: N/A" << std::endl;
        }

        std::cout << "\nDATA PROCESSING:" << std::endl;
        if (bytes > 0) {
            double mb = bytes / (1024.0 * 1024.0);
            std::cout << "Total Data: " << std::fixed << std::setprecision(2) << mb << " MB" << std::endl;
        } else {
            std::cout << "Total Data: N/A (file size tracking failed)" << std::endl;
        }

        if (duration_sec > 0.000001) { // Avoid division by very small numbers
            std::cout << "\nPERFORMANCE METRICS:" << std::endl;
            std::cout << "Files/second: " << std::fixed << std::setprecision(2) << (total_files / duration_sec) << std::endl;
            
            if (crypto_completed > 0) {
                std::cout << "Crypto ops/second: " << std::fixed << std::setprecision(2) << (crypto_completed / duration_sec) << std::endl;
            }
            
            if (bytes > 0) {
                double mb_per_sec = (bytes / (1024.0 * 1024.0)) / duration_sec;
                std::cout << "MB/second: " << std::fixed << std::setprecision(2) << mb_per_sec << std::endl;
            }
        }

        std::cout << "\nMULTITHREAD INFO:" << std::endl;
        std::cout << "CPU Cores Available: " << std::thread::hardware_concurrency() << std::endl;
        std::cout << "Main Thread ID: " << main_thread_id << std::endl;

        if (duration_sec > 0.000001 && crypto_completed > 0) {
            unsigned int cores = std::thread::hardware_concurrency();
            if (cores > 0) {
                double ops_per_core = (crypto_completed / duration_sec) / cores;
                std::cout << "Operations per core: " << std::fixed << std::setprecision(2) << ops_per_core << "/sec" << std::endl;
            }
        }

        std::cout << "================================" << std::endl;
    }
};

// Static member declarations only (definitions are in BenchmarkLogger2.cpp)