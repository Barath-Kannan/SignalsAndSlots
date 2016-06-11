/* 
 * File:   ThreadPool.h
 * Author: Barath Kannan
 *
 * Created on 18 May 2016, 11:37 PM
 */

#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <atomic>
#include <vector>
#include <functional>
#include <thread>
#include <list>
#include <mutex>
#include <chrono>
#include <condition_variable>
#include "BSignals/details/MPMCQueue.hpp"

namespace BSignals{ namespace details{

class ThreadPool {
public:
    template <typename... Args>
    static void run(const std::function<void(Args...)> &task, Args... p){
        threadPooledFunctions.enqueue([=](){task(p...);});
        realTasks++;
    }
    
private:
    static void queueListener();
    static void poolMonitor();
    
    static class _init {
    public:
        _init();
        ~_init(); //destructor
    } _initializer;

    static std::function<void()> bfc;
    static const uint16_t maxPoolThreads;
    static const uint16_t basePoolThreads;
    static BSignals::details::MPMCQueue<std::function<void()>> threadPooledFunctions;
    static std::thread poolMonitorThread;
    static std::atomic<uint32_t> nThreads;
    static std::atomic<uint32_t> realTasks;
};
}}
#endif /* THREADPOOL_H */

