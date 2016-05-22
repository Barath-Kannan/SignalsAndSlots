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
#include "BSignals/details/MPMCQueue.hpp"
#include "BSignals/details/SafeQueue.hpp"

class ThreadPool {
public:
    template <typename... Args>
    static void run(std::function<void(Args...)> task, Args... p){
        threadPooledFunctions.enqueue([=](){task(p...);});
    }
    
private:
    static void queueListener();
    
    static class _init {
    public:
        _init();
        ~_init(); //destructor
    } _initializer;
    
    static const uint16_t maxPoolThreads;
    static const uint16_t basePoolThreads;
    static MPMCQueue<std::function<void()>> threadPooledFunctions;
    //static SafeQueue<std::function<void()>> threadPooledFunctions;
    static std::list<std::thread> pool;
};

#endif /* THREADPOOL_H */

