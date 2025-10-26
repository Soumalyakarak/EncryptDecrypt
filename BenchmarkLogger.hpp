#pragma once
#include <chrono>
#include <iostream>
#include <iomanip>
#include <string>
#include <atomic>
#include <unistd.h>
#include <filesystem>

class BenchmarkLogger {
private:
    std::string operation_name;
    std::chrono::steady_clock::time_point start_time;
    pid_t main_process_id;
    
    // Atomic counters (work across processes)
    static std::atomic<size_t> files_processed;
    static std::atomic<size_t> files_successful;
    static std::atomic<size_t> files_failed;
    static std::atomic<size_t> total_bytes;
    static std::atomic<int> crypto_operations_completed;

public:
    BenchmarkLogger(const std::string& operation = "Multiprocess Crypto Operations") 
        : operation_name(operation), main_process_id(getpid()) {
        
        start_time = std::chrono::steady_clock::now();
        
        // Reset counters
        files_processed.store(0);
        files_successful.store(0);
        files_failed.store(0);
        total_bytes.store(0);
        crypto_operations_completed.store(0);
        
        std::cout << "\n=== ENCRYPTDECRYPT BENCHMARK START ===" << std::endl;
        std::cout << "Operation: " << operation_name << std::endl;
        std::cout << "Main PID: " << main_process_id << std::endl;
        std::cout << "CPU Cores: " << sysconf(_SC_NPROCESSORS_ONLN) << std::endl;
        std::cout << "Clock: steady_clock (nanosecond precision)" << std::endl;
        std::cout << "===============================" << std::endl;
    }

    ~BenchmarkLogger() {
        // Only main process prints final report
        if (getpid() == main_process_id) {
            print_final_crypto_report();
        }
    }

    pid_t getMainPID() const { return main_process_id; }
    
    // Call this from your main.cpp when file operation starts
    static void record_file_operation(const std::string& filepath, bool success) {
        files_processed.fetch_add(1);
        
        if (success) {
            files_successful.fetch_add(1);
            
            try {
                if (std::filesystem::exists(filepath)) {
                    size_t file_size = std::filesystem::file_size(filepath);
                    total_bytes.fetch_add(file_size);
                }
            } catch (...) {
                // Continue if file size unavailable
            }
        } else {
            files_failed.fetch_add(1);
            std::cout << "[PID:" << getpid() << "] FAILED: " << filepath << std::endl;
        }
        
        // Progress every 50 files
        size_t count = files_processed.load();
        if (count > 0 && count % 50 == 0) {
            std::cout << "[PROGRESS] " << count << " files submitted for processing..." << std::endl;
        }
    }

    // Call this from executeCryption() when crypto operation completes
    static void record_crypto_completion(const std::string& filepath, bool encrypt_mode) {
        int completed = crypto_operations_completed.fetch_add(1) + 1;
        pid_t current_pid = getpid();
        
        // Progress every 25 crypto operations
        if (completed % 25 == 0) {
            std::cout << "[PID:" << current_pid << "] Completed " << completed 
                      << " crypto operations (" << (encrypt_mode ? "ENCRYPT" : "DECRYPT") << ")" << std::endl;
        }
    }

    // Time the entire crypto operation
    template<typename Func>
    static auto time_crypto_operation(const std::string& filepath, bool encrypt_mode, Func&& crypto_func) 
        -> decltype(crypto_func()) {
        
        pid_t current_pid = getpid();
        auto start = std::chrono::steady_clock::now();
        
        // Execute the crypto function
        auto result = crypto_func();
        
        auto end = std::chrono::steady_clock::now();
        auto duration_us = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        
        // Record completion
        record_crypto_completion(filepath, encrypt_mode);
        
        // Log timing for slow operations (> 1ms)
        if (duration_us.count() > 1000) {
            std::cout << "[PID:" << current_pid << "] " 
                      << (encrypt_mode ? "ENCRYPT" : "DECRYPT") << " " 
                      << std::filesystem::path(filepath).filename().string()
                      << ": " << duration_us.count() << "μs" << std::endl;
        }
        
        return result;
    }

    static void log(const std::string& message) {
        std::cout << "[PID:" << getpid() << "] " << message << std::endl;
    }

private:
    void print_final_crypto_report() {
        auto end_time = std::chrono::steady_clock::now();
        auto total_duration_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time);
        double duration_sec = total_duration_ns.count() / 1e9;
        
        size_t total_files = files_processed.load();
        size_t successful = files_successful.load();
        size_t failed = files_failed.load();
        size_t bytes = total_bytes.load();
        int crypto_completed = crypto_operations_completed.load();
        
        std::cout << "\n";
        std::cout << "================================" << std::endl;
        std::cout << "  ENCRYPTDECRYPT BENCHMARK RESULTS" << std::endl;
        std::cout << "================================" << std::endl;
        std::cout << "Operation: " << operation_name << std::endl;
        std::cout << "Total Duration: " << std::fixed << std::setprecision(6) << duration_sec << " seconds" << std::endl;
        std::cout << "Duration (ns): " << total_duration_ns.count() << " nanoseconds" << std::endl;
        
        std::cout << "\nFILE PROCESSING:" << std::endl;
        std::cout << "Files Submitted: " << total_files << std::endl;
        std::cout << "Files Successful: " << successful << " (✓)" << std::endl;
        std::cout << "Files Failed: " << failed << " (✗)" << std::endl;
        std::cout << "Crypto Operations Completed: " << crypto_completed << std::endl;
        
        double success_rate = total_files > 0 ? (double(successful) / total_files) * 100.0 : 0.0;
        std::cout << "Success Rate: " << std::fixed << std::setprecision(1) << success_rate << "%" << std::endl;
        
        std::cout << "\nDATA PROCESSING:" << std::endl;
        std::cout << "Total Data: " << std::fixed << std::setprecision(2) << (bytes / (1024.0 * 1024.0)) << " MB" << std::endl;
        
        if (duration_sec > 0) {
            std::cout << "\nPERFORMANCE METRICS:" << std::endl;
            std::cout << "Files/second: " << std::fixed << std::setprecision(2) << (total_files / duration_sec) << std::endl;
            std::cout << "Crypto ops/second: " << std::fixed << std::setprecision(2) << (crypto_completed / duration_sec) << std::endl;
            std::cout << "MB/second: " << std::fixed << std::setprecision(2) << ((bytes / (1024.0 * 1024.0)) / duration_sec) << std::endl;
        }
        
        std::cout << "\nMULTIPROCESS INFO:" << std::endl;
        std::cout << "CPU Cores Available: " << sysconf(_SC_NPROCESSORS_ONLN) << std::endl;
        std::cout << "Main Process PID: " << main_process_id << std::endl;
        
        // Estimate process efficiency
        if (duration_sec > 0 && crypto_completed > 0) {
            double ops_per_core = (crypto_completed / duration_sec) / sysconf(_SC_NPROCESSORS_ONLN);
            std::cout << "Operations per core: " << std::fixed << std::setprecision(2) << ops_per_core << "/sec" << std::endl;
        }
        
        std::cout << "================================" << std::endl;
    }
};