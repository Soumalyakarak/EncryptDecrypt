#include "BenchmarkLogger.hpp"
#include<iostream>
#include<filesystem>
#include "./src/app/processes/ProcessManagement.hpp"
#include "./src/app/processes/Task.hpp"

namespace fs = std::filesystem;

int main(int argc, char *argv[]){
    std::string directory;
    std::string action;

    std::cout<<"Enter the directory: ";
    std::getline(std::cin, directory);

    std::cout<<"Enter the action(encrypt/decrypt): ";
    std::getline(std::cin, action);

    BenchmarkLogger benchmark("Multiprocess " + action + "ion"); 

    try
    {
        if(fs::exists(directory) && fs::is_directory(directory)){
            ProcessManagement processManagement; 

            for(const auto &entry : fs::recursive_directory_iterator(directory)){
                if(entry.is_regular_file()){
                    std::string filePath = entry.path().string();
                    IO io(filePath);
                    std::fstream f_stream = std::move(io.getFileStream());

                    if(f_stream.is_open()){
                        Action taskAction = (action == "encrypt") ? Action::ENCRYPT : Action::DECRYPT;
                        auto task = std::make_unique<Task>(std::move(f_stream), taskAction, filePath);
                        processManagement.SubmitToQueue(std::move(task));

                        BenchmarkLogger::record_file_operation(filePath, true);
                    }else{
                        std::cout<<"Unable to open the file: "<<filePath<<std::endl;

                        BenchmarkLogger::record_file_operation(filePath, false);
                    }
                }
            }
            BenchmarkLogger::log("About to execute tasks...");
            // run tasks
            processManagement.executeTasks();

            BenchmarkLogger::log("Tasks execution completed");
        }else{
            std::cout<<"Invalid directory Path!"<<std::endl;
        }
    }
    catch(const fs::filesystem_error &e)
    {
        std::cerr <<"Filesystem error: "<<e.what()<<std::endl;
    }

    if (getpid() == benchmark.getMainPID()) {
        auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        std::cout << "Exiting the encryption/decryption at: "
                  << std::put_time(std::localtime(&now), "%Y-%m-%d %H:%M:%S") << std::endl;
    }
    
    return 0;
}