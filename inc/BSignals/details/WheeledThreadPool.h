/* 
 * File:   WheeledThreadPool.h
 * Author: Barath Kannan
 *
 * Created on 13 June 2016, 3:36 PM
 */

#include <vector>
#include <atomic>
#include <functional>
#include <thread>
#include <mutex>
#include <queue>
#include "BSignals/details/Wheel.hpp"
#include "BSignals/details/MPSCQueue.hpp"

#ifndef BSIGNALS_WHEELEDTHREADPOOL_H
#define BSIGNALS_WHEELEDTHREADPOOL_H

namespace BSignals{ namespace details{

class WheeledThreadPool {
public:
    
    template <typename... Args>
    static void run(const std::function<void(Args...)> &task, const Args &... p) noexcept{
        run([task, p...](){task(p...);});
    }
    
    static void run(const std::function<void()> task) noexcept;
    
    //only invoke start up if a thread pooled slot has been connected
    static void startup();
    
    static std::chrono::duration<double> getMaxWait();
private:
    static void queueListener(uint32_t index);
    static class _init {
    public:
        _init(); 
        ~_init(); 
    } _initializer;
    
    static const uint32_t nThreads{32};
    static std::chrono::duration<double> maxWait;
    static std::mutex tpLock;
    static bool isStarted;
    static Wheel<MPSCQueue<std::function<void()>, 256>, BSignals::details::WheeledThreadPool::nThreads> threadPooledFunctions;
    static std::vector<std::thread> queueMonitors;
};
}}

#endif /* BSIGNALS_WHEELEDTHREADPOOL_H */