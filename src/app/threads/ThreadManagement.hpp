#ifndef THREAD_MANAGEMENT_HPP
#define THREAD_MANAGEMENT_HPP

#include "Task.hpp"
#include <queue>
#include <memory>
#include <atomic>
#include <semaphore.h>
#include <mutex>

class ThreadManagement
{
     sem_t* itemsSemaphore;
     sem_t* emptySlotsSemaphore;
public:
     ThreadManagement();
     ~ThreadManagement();
     bool SubmitToQueue(std::unique_ptr<Task> task);
     void executeTasks();

private:
     struct SharedMemory
     {
          std::atomic<int> size;
          char tasks[1000][256];
          int front;
          int rear;

          void printSharedMemory()
          {
               std::cout << size << std::endl;
               std::cout << front << std::endl;
               std::cout << rear << std::endl;
          }
     };
     SharedMemory *sharedMem;
     int shmFd;
     const char *SHM_NAME = "/my_queue";
     std::mutex queueLock;
};

#endif