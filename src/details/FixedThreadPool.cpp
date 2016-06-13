#include "BSignals/details/FixedThreadPool.h"
#include <iostream>
#include <algorithm>

using std::cout;
using std::endl;

using std::mutex;
using std::lock_guard;
using std::atomic;
using std::vector;
using BSignals::details::SafeQueue;
using BSignals::details::FixedThreadPool;
using BSignals::details::TaskManager;

std::mutex FixedThreadPool::tpLock;
bool FixedThreadPool::isStarted = false;

TaskManager FixedThreadPool::threadPooledFunctions;
//moodycamel::BlockingConcurrentQueue<std::function<void()>> FixedThreadPool::threadPooledFunctions {};
std::vector<std::thread> FixedThreadPool::queueMonitors;

FixedThreadPool::_init FixedThreadPool::_initializer;

FixedThreadPool::_init::~_init() {
    std::lock_guard<mutex> lock(tpLock);
    isStarted = false;
    //threadPooledFunctions.stop();
    for (auto &t : queueMonitors){
        t.join();
    }
}

void FixedThreadPool::startup() {
    std::lock_guard<mutex> lock(tpLock);
    if (!isStarted){
        isStarted = true;
        for (unsigned int i=0; i<nThreads; ++i){
            queueMonitors.emplace_back(queueListener);
        }
    }
}

void FixedThreadPool::queueListener() {
    std::function<void()> func;
//    while (!threadPooledFunctions.isStopped()){
//        auto func = threadPooledFunctions.dequeue();
//        if (threadPooledFunctions.isStopped()) break;
//        func();
//    }
    while (isStarted){
        threadPooledFunctions.runSome();
    }
}

