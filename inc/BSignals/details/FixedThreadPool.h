/* 
 * File:   FixedThreadPool.h
 * Author: Barath Kannan
 *
 * Created on 13 June 2016, 12:18 AM
 */

#ifndef FIXEDTHREADPOOL_H
#define FIXEDTHREADPOOL_H

#include <atomic>
#include <vector>
#include <functional>
#include <thread>
#include <list>
#include <mutex>
#include <chrono>
#include <condition_variable>
#include "BSignals/details/SafeQueue.hpp"
#include "BSignals/details/blockingconcurrentqueue.h"
#include "BSignals/details/TaskManager.h"

namespace BSignals{ namespace details{

class FixedThreadPool {
public:
    
//    template <typename... Args>
//    static void run(const std::function<void(Args...)> &task, const Args &... p){
//        threadPooledFunctions.enqueue([task, p...](){task(p...);});
//    }
    
    static void run(const std::function<void()> &task){
        //threadPooledFunctions.enqueue(task);
        threadPooledFunctions.addTask(task);
    }
    
    //only invoke start up if a thread pooled slot has been connected
    static void startup();
    
private:
    static void queueListener();
    static class _init {
    public:
        ~_init(); //destructor
    } _initializer;
    
    static const uint32_t nThreads{8};
    static std::mutex tpLock;
    static bool isStarted;
    static TaskManager threadPooledFunctions;
    //static moodycamel::BlockingConcurrentQueue<std::function<void()>> threadPooledFunctions;
    static std::vector<std::thread> queueMonitors;
};
}}

#endif /* FIXEDTHREADPOOL_H */

