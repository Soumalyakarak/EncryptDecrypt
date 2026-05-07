#include "ThreadManagement.hpp"
#include <iostream>
#include "../encryptDecrypt/Cryption.hpp"

// Constructor,create threads
ThreadManagement::ThreadManagement(){
    int numThreads = std::thread::hardware_concurrency();

    if (numThreads == 0) numThreads = 4;//fallback

    for (int i = 0; i < numThreads; i++){
        workers.emplace_back(&ThreadManagement::worker, this);
    }
}

//Destructor
ThreadManagement::~ThreadManagement(){
    {
        std::unique_lock<std::mutex> lock(queueMutex);
        stop = true;
    }

    condition.notify_all();

    for(auto &t : workers){
        if(t.joinable()) t.join();
    }
}

//Add task to queue
bool ThreadManagement::SubmitToQueue(std::unique_ptr<Task> task){
    {
        std::lock_guard<std::mutex>lock(queueMutex);
        taskQueue.push(task->toString());
    }

    condition.notify_one();
    return true;
}

// Worker thread loop
void ThreadManagement::worker(){
    while(true){
        std::string task;
        {
            std::unique_lock<std::mutex>lock(queueMutex);

            condition.wait(lock, [this](){
                return !taskQueue.empty() || stop; //if queue is empty , thread sleeps.
            });

            if(stop && taskQueue.empty()) //If stop signal given and no work left , thread exits cleanly.
                return;

            task = taskQueue.front();
            taskQueue.pop();
        }

        executeCryption(task);
    }
}

// Wait for all tasks to finish
// void ThreadManagement::executeTasks(){
//     {
//         std::unique_lock<std::mutex> lock(queueMutex);
//         stop = true;
//     }

//     condition.notify_all();

//     for (auto &t : workers) {
//         if (t.joinable())
//             t.join();
//     }
// }