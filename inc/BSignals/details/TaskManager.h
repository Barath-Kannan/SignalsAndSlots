
/* 
 * File:   TaskManager.h
 * Author: Barath Kannan
 *
 * Created on 13 June 2016, 10:10 PM
 */

#ifndef TASKMANAGER_H
#define TASKMANAGER_H

#include <queue>
#include <shared_mutex>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <chrono>

#include "BSignals/details/TaskBatch.h"
#include "BSignals/details/SafeQueue.hpp"
#include "TaskBatch.h"

#include <iostream>
using std::cout;
using std::endl;

namespace BSignals{ namespace details{

class TaskManager{
public:
    void addTask(const std::function<void()> &task){
        while (!currentBatch.addTask(task)){
            std::lock_guard<std::mutex> lock(taskBatchLock);
            if (currentBatch.isFull()){
                taskBatches.enqueue(currentBatch.package());
                currentBatch.clear();
            }
        }
    }
    
    void runSome(){
        std::pair<std::function<void()>, bool> dq;
        for (;;){
            dq = taskBatches.wait_for_dequeue(std::chrono::microseconds(1));
            if (dq.second){
                dq.first();
                return;
            }
            //cout << "Executing current batch" << endl;
            currentBatch.package()();
        }
    }
    
private:
    static const uint32_t taskBatchSize{1024};
    std::atomic<uint32_t> runners{0};
    std::mutex taskBatchLock;
    std::condition_variable taskBatchCV;
    TaskBatch<taskBatchSize> currentBatch;
    SafeQueue<std::function<void()>> taskBatches;
};
    
}}

#endif /* TASKMANAGER_H */

