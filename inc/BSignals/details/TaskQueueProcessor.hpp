/* 
 * File:   TaskQueueProcessor.hpp
 * Author: Barath Kannan
 * Alternative to the WheeledThreadPool
 * 1 MPMC task queue consumed by multiple threads, instead of multiple MPSC queues
 * Created on 9 July 2016, 3:05 PM
 */

#ifndef TASKQUEUEPROCESSOR_HPP
#define TASKQUEUEPROCESSOR_HPP

#include <thread>
#include <functional>
#include <vector>
#include "BSignals/details/UnboundedMPMCQueue.hpp"
#include "BSignals/details/MPMCQueue.hpp"
#include "BSignals/details/MPSCQueue.hpp"

namespace BSignals{ namespace details{

class TaskQueueProcessor{
public:
    template <typename... Args>
    static void run(const std::function<void(Args...)> &task, const Args &... p){
        run([task, p...](){task(p...);});
    }
    
    static void run(const std::function<void()> task);
    
    //only invoke start up if a thread pooled slot has been connected
    static void startup();
    
    
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
    static BSignals::details::UnboundedMPMCQueue<std::function<void()> > taskQueue;
    //static BSignals::details::MPMCQueue<std::function<void()> > taskQueue;
    //static BSignals::details::MPSCQueue<std::function<void()> > taskQueue;
    static std::vector<std::thread> queueMonitors;
};

}}
#endif /* TASKQUEUEPROCESSOR_HPP */

