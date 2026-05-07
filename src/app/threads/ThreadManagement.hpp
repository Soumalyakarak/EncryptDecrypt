#ifndef THREAD_MANAGEMENT_HPP
#define THREAD_MANAGEMENT_HPP

#include "Task.hpp"

#include <queue>
#include <memory>
#include <mutex>
#include <condition_variable>
#include <vector>
#include <thread>
#include <string>

class ThreadManagement
{
public:
    ThreadManagement();              
    ~ThreadManagement(); 

    bool SubmitToQueue(std::unique_ptr<Task> task); 

private:
    // Worker function
    void worker();

    // Task queue
    std::queue<std::string> taskQueue;

    // Synchronization
    std::mutex queueMutex;
    std::condition_variable condition;

    // Thread pool
    std::vector<std::thread> workers;

    // Stop signal
    bool stop = false;
};

#endif