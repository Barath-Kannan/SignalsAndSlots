#include "BSignals/details/WheeledThreadPool.h"
#include <iostream>
#include <algorithm>

using std::cout;
using std::endl;

using std::mutex;
using std::lock_guard;
using std::atomic;
using std::vector;
using BSignals::details::Wheel;
using BSignals::details::SafeQueue;
using BSignals::details::MPMCQueue;
using BSignals::details::WheeledThreadPool;
using BSignals::details::ProducerConsumerQueue;
using BSignals::details::SafeQueueVariant;

std::mutex WheeledThreadPool::tpLock;
bool WheeledThreadPool::isStarted = false;
Wheel<moodycamel::BlockingConcurrentQueue<std::function<void()>>, BSignals::details::WheeledThreadPool::nThreads> WheeledThreadPool::threadPooledFunctions {};
std::vector<std::thread> WheeledThreadPool::queueMonitors;

WheeledThreadPool::_init WheeledThreadPool::_initializer;

WheeledThreadPool::_init::~_init() {
    std::lock_guard<mutex> lock(tpLock);
    isStarted = false;
    for (uint32_t i=0; i<nThreads; i++){
        threadPooledFunctions.getSpoke().enqueue(nullptr);
    }
    for (auto &t : queueMonitors){
        t.join();
    }
}

void WheeledThreadPool::startup() {
    std::lock_guard<mutex> lock(tpLock);
    if (!isStarted){
        isStarted = true;
        for (unsigned int i=0; i<nThreads; ++i){
            queueMonitors.emplace_back(queueListener, i);
        }
        
    }
}

void WheeledThreadPool::queueListener(uint32_t index) {
    auto &spoke = threadPooledFunctions.getSpoke(index);
    //while (!spoke.isStopped()){
    std::function<void()> func;
    while (isStarted){
        spoke.wait_dequeue(func);
        if (isStarted) func();    
    }
}

