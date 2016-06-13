
/* 
 * File:   TaskBatch.h
 * Author: Barath Kannan
 *
 * Created on 13 June 2016, 9:58 PM
 */

#ifndef TASKBATCH_H
#define TASKBATCH_H

#include <functional>
#include <atomic>
#include <mutex>

namespace BSignals{ namespace details{

template <uint32_t N>
class TaskBatch{
public:
    bool addTask(const std::function<void()> &task){
        std::shared_lock<std::shared_timed_mutex> lock(flushLock);
        uint32_t index = nElems.fetch_add(1);
        if (index >= tasks.size())
            return false;
        tasks[index] = task;
        return true;
    }
    
    bool isFull(){
        return (nElems == tasks.size());
    }
    
    std::function<void()> package(){
        std::unique_lock<std::shared_timed_mutex> lock(flushLock);
        auto taskCopy = tasks;
        auto ret = [=](){
            for (uint32_t i=0; i<nElems; ++i){
                taskCopy[i]();
            }
        };
        nElems = 0;
        return ret;
    }
    
    void execute(){
        std::unique_lock<std::shared_timed_mutex> lock(flushLock);
        for (uint32_t i=0; i<nElems; ++i){
            tasks[i]();
        }
        nElems = 0;
    }
    
    void clear(){
        nElems = 0;
    }
    
private:
    std::shared_timed_mutex flushLock;
    std::array<std::function<void()>, N> tasks;
    std::atomic<uint32_t> nElems{0};
};
}}
#endif /* TASKBATCH_H */

