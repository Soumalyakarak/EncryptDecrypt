#include "Cryption.hpp"
#include "../processes/Task.hpp"
#include "../FileHandling/ReadEnv.cpp"
#include <ctime>
#include <iomanip>

#ifdef MULTITHREAD
#include "BenchmarkLogger2.hpp"
#define BENCHMARK BenchmarkLogger2
#else
#include "BenchmarkLogger.hpp"
#define BENCHMARK BenchmarkLogger
#endif

int executeCryption(const std::string &taskData)
{
    try
    {
        Task task = Task::fromString(taskData);
        ReadEnv env;

        std::string envkey = env.getenv();
        int key = std::stoi(envkey);

        if (task.action == Action::ENCRYPT)
        {
            char ch;
            while (task.f_stream.get(ch))
            {
                ch = (ch + key) % 256;
                task.f_stream.seekp(-1, std::ios::cur);
                task.f_stream.put(ch);
            }
            task.f_stream.close();
            // ADD THIS LINE - Record crypto completion
            BENCHMARK::record_crypto_completion(task.filePath, true);
        }
        else
        {
            char ch;
            while (task.f_stream.get(ch))
            {
                ch = (ch - key + 256) % 256;
                task.f_stream.seekp(-1, std::ios::cur);
                task.f_stream.put(ch);
            }
            task.f_stream.close();

            // ADD THIS LINE - Record crypto completion
            BENCHMARK::record_crypto_completion(task.filePath, false);
        }
        // std::time_t t = std::time(nullptr);
        // std::tm* now = std::localtime(&t);
        // std::cout << "Exiting the encryption/decryption at: " << std::put_time(now, "%Y-%m-%d %H:%M:%S") << std::endl;
    }
    catch (const std::exception &e)
    {
        std::cerr << "[CRYPTO ERROR] File: " << taskData
                  << ", reason: " << e.what() << std::endl;
        BENCHMARK::record_file_operation(taskData, false);
        return -1; // indicate failure
    }

    return 0;
}