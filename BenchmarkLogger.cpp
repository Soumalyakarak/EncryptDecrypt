#include "BenchmarkLogger.hpp"

// Define static members here (only once in the whole program)
std::atomic<size_t> BenchmarkLogger::files_processed{0};
std::atomic<size_t> BenchmarkLogger::files_successful{0};
std::atomic<size_t> BenchmarkLogger::files_failed{0};
std::atomic<size_t> BenchmarkLogger::total_bytes{0};
std::atomic<int> BenchmarkLogger::crypto_operations_completed{0};
