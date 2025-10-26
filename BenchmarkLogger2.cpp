#include "BenchmarkLogger2.hpp"

// Define static members (shared across all threads)
std::atomic<size_t> BenchmarkLogger2::files_processed{0};
std::atomic<size_t> BenchmarkLogger2::files_successful{0};
std::atomic<size_t> BenchmarkLogger2::files_failed{0};
std::atomic<size_t> BenchmarkLogger2::total_bytes{0};
std::atomic<int> BenchmarkLogger2::crypto_operations_completed{0};
std::mutex BenchmarkLogger2::output_mutex;
