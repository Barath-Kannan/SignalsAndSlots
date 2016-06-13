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
using BSignals::details::mpsc_queue_t;
using BSignals::details::SafeQueue;
using BSignals::details::WheeledThreadPool;

std::mutex WheeledThreadPool::tpLock;
bool WheeledThreadPool::isStarted = false;
Wheel<mpsc_queue_t<std::function<void()>>, BSignals::details::WheeledThreadPool::nThreads> WheeledThreadPool::threadPooledFunctions {};
std::vector<std::thread> WheeledThreadPool::queueMonitors;

WheeledThreadPool::_init WheeledThreadPool::_initializer;

WheeledThreadPool::_init::~_init() {
    std::lock_guard<mutex> lock(tpLock);
    isStarted = false;
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
    std::function<void()> func;
    std::chrono::duration<double> waitTime = std::chrono::nanoseconds(1);
    while (isStarted){
        if (spoke.dequeue(func)){
            func();
            waitTime = std::chrono::nanoseconds(1);
        }
        else{
            std::this_thread::sleep_for(waitTime);
            waitTime*=2;
        }
        if (waitTime > std::chrono::milliseconds(1)){
            cout << "Blocking dequeue" << endl;
            spoke.blocking_dequeue(func);
            func();
            waitTime = std::chrono::nanoseconds(1);
        }
    }
}