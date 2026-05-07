#include "BenchmarkLogger2.hpp"
#include <iostream>
#include <filesystem>
#include "./src/app/threads/ThreadManagement.hpp"
#include "./src/app/threads/Task.hpp"

namespace fs = std::filesystem;

int main(int argc, char *argv[]) {
    std::string directory;
    std::string action;

    std::cout << "Enter the directory: ";
    std::getline(std::cin, directory);

    std::cout << "Enter the action(encrypt/decrypt): ";
    std::getline(std::cin, action);

    BenchmarkLogger2 benchmark("Multithreaded " + action + "ion");

    try {
        if (fs::exists(directory) && fs::is_directory(directory)){

            ThreadManagement threadManagement;

            Action taskAction = (action == "encrypt") ? Action::ENCRYPT : Action::DECRYPT;

            for (const auto &entry : fs::recursive_directory_iterator(directory)) {

                if (!entry.is_regular_file()) continue;

                std::string filePath = entry.path().string();

                 // -----------Pre-filter before opening file--------------
                    bool isEnc = filePath.size() >= 4 &&
                    filePath.substr(filePath.size() - 4) == ".enc";
                    if (action == "encrypt" && isEnc)  continue; // skip .enc on encrypt
                    if (action == "decrypt" && !isEnc) continue; // skip non-.enc on decrypt
                 // --------------------------------------------------------

                // Just create task using filePath
                auto task = std::make_unique<Task>(taskAction, filePath);

                threadManagement.SubmitToQueue(std::move(task));

                BenchmarkLogger2::record_file_operation(filePath, true);
            }

            BenchmarkLogger2::log("About to execute tasks...");

            // Wait for all threads to finish
            // threadManagement.executeTasks();

            BenchmarkLogger2::log("Tasks execution completed");
        } else {
            std::cout << "Invalid directory Path!" << std::endl;
        }
    }catch(const fs::filesystem_error &e){
        std::cerr << "Filesystem error: " << e.what() << std::endl;
    }

    if(std::this_thread::get_id() == benchmark.getMainThreadID()){
        auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        std::cout << "Exiting the encryption/decryption at: "
                  << std::put_time(std::localtime(&now), "%Y-%m-%d %H:%M:%S") << std::endl;
    }

    return 0;
}